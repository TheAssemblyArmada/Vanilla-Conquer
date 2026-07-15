use std::collections::{BTreeMap, BTreeSet};
use std::fs::{self, File, OpenOptions};
use std::io::{Read, Write};
use std::path::{Path, PathBuf};
use std::time::{SystemTime, UNIX_EPOCH};

use serde::{Deserialize, Serialize};
use zip::write::SimpleFileOptions;
use zip::{CompressionMethod, DateTime, ZipArchive, ZipWriter};

use crate::error::{Error, Result};
use crate::hash::copy_and_hash;
use crate::manifest::{
    infer_role, ContentDescriptorV1, ManifestFileV1, ManifestV1, SourceV1, MANIFEST_PATH,
};
use crate::path::{collision_key, normalize_package_path, safe_join};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct PackageLimits {
    pub max_entries: usize,
    pub max_manifest_bytes: u64,
    pub max_file_bytes: u64,
    pub max_total_bytes: u64,
    pub max_compression_ratio: u64,
}

/// Resource profile accepted by the browser's `.cncweb` v1 importer.
///
/// Keep this in sync with `fixtures/browser-package-profile-v1.json`. Both the
/// Rust and TypeScript test suites assert their defaults against that fixture.
pub const BROWSER_PACKAGE_LIMITS_V1: PackageLimits = PackageLimits {
    max_entries: 100_000,
    max_manifest_bytes: 4 * 1024 * 1024,
    max_file_bytes: 64 * 1024 * 1024,
    max_total_bytes: 2 * 1024 * 1024 * 1024,
    max_compression_ratio: 1_000,
};

impl PackageLimits {
    /// Returns the limits for packages intended for direct browser import.
    pub const fn browser_v1() -> Self {
        BROWSER_PACKAGE_LIMITS_V1
    }
}

impl Default for PackageLimits {
    fn default() -> Self {
        Self::browser_v1()
    }
}

#[derive(Debug, Clone)]
pub struct CreateOptions {
    pub package_id: String,
    pub created_at_unix_ms: u64,
    pub source: SourceV1,
    pub content: ContentDescriptorV1,
    pub compression_level: i64,
    pub limits: PackageLimits,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct ExtractReport {
    pub manifest: ManifestV1,
    pub files_extracted: usize,
    pub bytes_extracted: u64,
}

struct StagedFile {
    source_path: PathBuf,
    manifest_file: ManifestFileV1,
}

struct PendingStagedFile {
    source_path: PathBuf,
    logical_path: String,
    size: u64,
}

struct PreparedEntry {
    index: usize,
    path: String,
    size: u64,
}

struct PreparedPackage {
    manifest: ManifestV1,
    manifest_bytes: Vec<u8>,
    entries: Vec<PreparedEntry>,
    total_bytes: u64,
}

/// Creates a deterministic, ZIP64-capable `.cncweb` package from a local
/// staging directory. Symlinks and special files are rejected, and the output
/// is atomically renamed into place only after every source hash is rechecked.
pub fn create_package(
    input_directory: impl AsRef<Path>,
    output_path: impl AsRef<Path>,
    options: CreateOptions,
) -> Result<ManifestV1> {
    let input_directory = input_directory.as_ref();
    let output_path = output_path.as_ref();
    if output_path.exists() {
        return Err(Error::OutputExists(output_path.to_path_buf()));
    }
    let metadata = fs::metadata(input_directory)?;
    if !metadata.is_dir() {
        return Err(Error::UnsupportedFile {
            path: input_directory.to_path_buf(),
            kind: "staging input is not a directory",
        });
    }

    let staged_files = collect_staged_files(input_directory, options.limits)?;
    let manifest_files = staged_files
        .iter()
        .map(|file| file.manifest_file.clone())
        .collect();
    let manifest = ManifestV1::new(
        options.package_id,
        options.created_at_unix_ms,
        options.source,
        options.content,
        manifest_files,
    );
    manifest.validate()?;

    let mut manifest_bytes = serde_json::to_vec_pretty(&manifest)?;
    manifest_bytes.push(b'\n');
    if manifest_bytes.len() as u64 > options.limits.max_manifest_bytes {
        return Err(Error::PackageLimit(format!(
            "manifest is {} bytes; limit is {}",
            manifest_bytes.len(),
            options.limits.max_manifest_bytes
        )));
    }

    let parent = output_path.parent().unwrap_or_else(|| Path::new("."));
    fs::create_dir_all(parent)?;
    let (temporary_path, temporary_file) = create_temporary_file(output_path)?;

    let write_result = (|| -> Result<()> {
        let mut zip = ZipWriter::new(temporary_file);
        let manifest_options = file_options(CompressionMethod::Deflated, options.compression_level);
        zip.start_file(MANIFEST_PATH, manifest_options)?;
        zip.write_all(&manifest_bytes)?;

        for staged in &staged_files {
            let method = compression_for(&staged.manifest_file.path);
            let file_options = file_options(method, options.compression_level);
            zip.start_file(&staged.manifest_file.path, file_options)?;
            let source = File::open(&staged.source_path)?;
            let mut limited_source = source.take(staged.manifest_file.size.saturating_add(1));
            let (actual_hash, actual_size) = copy_and_hash(&mut limited_source, &mut zip)?;
            if actual_size != staged.manifest_file.size {
                return Err(Error::SizeMismatch {
                    path: staged.manifest_file.path.clone(),
                    expected: staged.manifest_file.size,
                    actual: actual_size,
                });
            }
            if actual_hash != staged.manifest_file.sha256 {
                return Err(Error::HashMismatch {
                    path: staged.manifest_file.path.clone(),
                    expected: staged.manifest_file.sha256.to_string(),
                    actual: actual_hash.to_string(),
                });
            }
        }

        let completed = zip.finish()?;
        completed.sync_all()?;
        Ok(())
    })();

    if let Err(error) = write_result {
        let _ = fs::remove_file(&temporary_path);
        return Err(error);
    }
    // Do not publish an archive that this profile would reject on import. In
    // particular, this checks the ZIP's actual compressed sizes and catches a
    // compression ratio that could not be known from staging metadata alone.
    if let Err(error) = inspect_package(&temporary_path, options.limits) {
        let _ = fs::remove_file(&temporary_path);
        return Err(error);
    }
    if let Err(error) = fs::rename(&temporary_path, output_path) {
        let _ = fs::remove_file(&temporary_path);
        return Err(Error::Io(error));
    }

    Ok(manifest)
}

/// Reads and validates package metadata and central-directory limits without
/// decompressing all content files.
pub fn inspect_package(
    package_path: impl AsRef<Path>,
    limits: PackageLimits,
) -> Result<ManifestV1> {
    let file = File::open(package_path)?;
    let mut zip = ZipArchive::new(file)?;
    Ok(prepare_package(&mut zip, limits)?.manifest)
}

/// Streams every file through SHA-256 and verifies it against the manifest.
pub fn verify_package(package_path: impl AsRef<Path>, limits: PackageLimits) -> Result<ManifestV1> {
    let file = File::open(package_path)?;
    let mut zip = ZipArchive::new(file)?;
    let prepared = prepare_package(&mut zip, limits)?;
    let declared: BTreeMap<&str, &ManifestFileV1> = prepared
        .manifest
        .files
        .iter()
        .map(|file| (file.path.as_str(), file))
        .collect();

    for entry in &prepared.entries {
        let expected = declared[entry.path.as_str()];
        let file = zip.by_index(entry.index)?;
        let mut limited_file = file.take(expected.size.saturating_add(1));
        let (actual_hash, actual_size) = copy_and_hash(&mut limited_file, &mut std::io::sink())?;
        verify_file(expected, actual_size, actual_hash)?;
    }
    Ok(prepared.manifest)
}

/// Safely extracts into a fresh sibling directory and atomically renames it to
/// the requested output only after all hashes match. Existing output is never
/// overwritten.
pub fn extract_package(
    package_path: impl AsRef<Path>,
    output_directory: impl AsRef<Path>,
    limits: PackageLimits,
) -> Result<ExtractReport> {
    let output_directory = output_directory.as_ref();
    if output_directory.exists() {
        return Err(Error::OutputExists(output_directory.to_path_buf()));
    }
    let parent = output_directory.parent().unwrap_or_else(|| Path::new("."));
    fs::create_dir_all(parent)?;

    let file = File::open(package_path)?;
    let mut zip = ZipArchive::new(file)?;
    let prepared = prepare_package(&mut zip, limits)?;
    let declared: BTreeMap<&str, &ManifestFileV1> = prepared
        .manifest
        .files
        .iter()
        .map(|file| (file.path.as_str(), file))
        .collect();
    let temporary_directory = create_temporary_directory(output_directory)?;

    let extraction_result = (|| -> Result<()> {
        for entry in &prepared.entries {
            let expected = declared[entry.path.as_str()];
            let destination = safe_join(&temporary_directory, &entry.path)?;
            if let Some(parent) = destination.parent() {
                fs::create_dir_all(parent)?;
            }
            let mut output = OpenOptions::new()
                .write(true)
                .create_new(true)
                .open(&destination)?;
            let input = zip.by_index(entry.index)?;
            let mut limited_input = input.take(expected.size.saturating_add(1));
            let (actual_hash, actual_size) = copy_and_hash(&mut limited_input, &mut output)?;
            output.sync_all()?;
            verify_file(expected, actual_size, actual_hash)?;
        }

        let manifest_destination = safe_join(&temporary_directory, MANIFEST_PATH)?;
        let mut manifest_output = OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(manifest_destination)?;
        manifest_output.write_all(&prepared.manifest_bytes)?;
        manifest_output.sync_all()?;
        Ok(())
    })();

    if let Err(error) = extraction_result {
        let _ = fs::remove_dir_all(&temporary_directory);
        return Err(error);
    }
    if let Err(error) = fs::rename(&temporary_directory, output_directory) {
        let _ = fs::remove_dir_all(&temporary_directory);
        return Err(Error::Io(error));
    }

    Ok(ExtractReport {
        files_extracted: prepared.entries.len(),
        bytes_extracted: prepared.total_bytes,
        manifest: prepared.manifest,
    })
}

fn collect_staged_files(root: &Path, limits: PackageLimits) -> Result<Vec<StagedFile>> {
    let mut directories = vec![root.to_path_buf()];
    let mut pending_files = Vec::new();
    let mut collision_keys = BTreeSet::new();
    let mut total_bytes = 0_u64;

    while let Some(directory) = directories.pop() {
        let mut children = fs::read_dir(&directory)?.collect::<std::io::Result<Vec<_>>>()?;
        children.sort_by_key(|entry| entry.file_name());
        // Reverse because directories are processed from a stack. Final files
        // are sorted independently below, but this keeps traversal predictable.
        for entry in children.into_iter().rev() {
            let path = entry.path();
            let metadata = fs::symlink_metadata(&path)?;
            let file_type = metadata.file_type();
            if file_type.is_symlink() {
                return Err(Error::UnsupportedFile {
                    path,
                    kind: "symbolic links are forbidden",
                });
            }
            if file_type.is_dir() {
                directories.push(path);
                continue;
            }
            if !file_type.is_file() {
                return Err(Error::UnsupportedFile {
                    path,
                    kind: "only regular files are supported",
                });
            }

            let relative = path.strip_prefix(root).map_err(|_| {
                Error::unsafe_path(path.to_string_lossy(), "file escaped staging root")
            })?;
            let mut segments = Vec::new();
            for component in relative.components() {
                let segment = component.as_os_str().to_str().ok_or_else(|| {
                    Error::unsafe_path(relative.to_string_lossy(), "path is not valid Unicode")
                })?;
                segments.push(segment);
            }
            let logical_path = normalize_package_path(&segments.join("/"))?;
            if collision_key(&logical_path) == collision_key(MANIFEST_PATH) {
                return Err(Error::InvalidManifest(
                    "staging directory contains reserved manifest.json (case-insensitive)".into(),
                ));
            }
            if !collision_keys.insert(collision_key(&logical_path)) {
                return Err(Error::DuplicatePath(logical_path));
            }
            if metadata.len() > limits.max_file_bytes {
                return Err(Error::PackageLimit(format!(
                    "`{logical_path}` is {} bytes; per-file limit is {}",
                    metadata.len(),
                    limits.max_file_bytes
                )));
            }
            total_bytes = total_bytes
                .checked_add(metadata.len())
                .ok_or_else(|| Error::PackageLimit("total content size overflow".into()))?;
            if total_bytes > limits.max_total_bytes {
                return Err(Error::PackageLimit(format!(
                    "staged content exceeds {} bytes",
                    limits.max_total_bytes
                )));
            }
            if pending_files.len() >= limits.max_entries {
                return Err(Error::PackageLimit(format!(
                    "staged content exceeds {} entries",
                    limits.max_entries
                )));
            }

            pending_files.push(PendingStagedFile {
                source_path: path,
                logical_path,
                size: metadata.len(),
            });
        }
    }

    // Complete the cheap metadata inventory before hashing any content. This
    // makes an oversized browser staging tree fail immediately, including when
    // its files are sparse or reside on a slow external drive.
    pending_files.sort_by(|left, right| left.logical_path.cmp(&right.logical_path));
    let mut files = Vec::with_capacity(pending_files.len());
    for pending in pending_files {
        let source = File::open(&pending.source_path)?;
        let mut limited_source = source.take(pending.size.saturating_add(1));
        let (sha256, actual_size) = crate::hash::hash_reader(&mut limited_source)?;
        if actual_size != pending.size {
            return Err(Error::SizeMismatch {
                path: pending.logical_path,
                expected: pending.size,
                actual: actual_size,
            });
        }
        files.push(StagedFile {
            source_path: pending.source_path,
            manifest_file: ManifestFileV1 {
                role: infer_role(&pending.logical_path),
                path: pending.logical_path,
                size: actual_size,
                sha256,
            },
        });
    }

    Ok(files)
}

fn prepare_package<R: Read + std::io::Seek>(
    zip: &mut ZipArchive<R>,
    limits: PackageLimits,
) -> Result<PreparedPackage> {
    if zip.is_empty() {
        return Err(Error::InvalidManifest("package is empty".into()));
    }
    if zip.len() > limits.max_entries.saturating_add(1) {
        return Err(Error::PackageLimit(format!(
            "archive has {} entries; limit is {} plus manifest",
            zip.len(),
            limits.max_entries
        )));
    }

    let mut paths = BTreeSet::new();
    let mut entries = Vec::new();
    let mut manifest_index = None;
    let mut total_bytes = 0_u64;

    for index in 0..zip.len() {
        let file = zip.by_index(index)?;
        let raw_name = std::str::from_utf8(file.name_raw())
            .map_err(|_| Error::unsafe_path("<non-UTF-8 ZIP name>", "entry name is not UTF-8"))?;
        let path = normalize_package_path(raw_name)?;
        if file.enclosed_name().is_none() {
            return Err(Error::unsafe_path(path, "ZIP path is not enclosed"));
        }
        if file.encrypted() {
            return Err(Error::PackageLimit(format!(
                "encrypted entry `{path}` is not supported"
            )));
        }
        if !file.is_file() || !is_regular_mode(file.unix_mode()) {
            return Err(Error::UnsupportedFile {
                path: PathBuf::from(path),
                kind: "directories, symlinks, and special ZIP entries are forbidden",
            });
        }
        if !matches!(
            file.compression(),
            CompressionMethod::Stored | CompressionMethod::Deflated
        ) {
            return Err(Error::PackageLimit(format!(
                "entry `{path}` uses an unsupported compression method"
            )));
        }
        if !paths.insert(collision_key(&path)) {
            return Err(Error::DuplicatePath(path));
        }
        if file.size() > limits.max_file_bytes {
            return Err(Error::PackageLimit(format!(
                "entry `{path}` is {} bytes; per-file limit is {}",
                file.size(),
                limits.max_file_bytes
            )));
        }
        enforce_compression_ratio(&path, file.size(), file.compressed_size(), limits)?;

        if path == MANIFEST_PATH {
            if file.size() > limits.max_manifest_bytes {
                return Err(Error::PackageLimit(format!(
                    "manifest is {} bytes; limit is {}",
                    file.size(),
                    limits.max_manifest_bytes
                )));
            }
            manifest_index = Some(index);
        } else {
            total_bytes = total_bytes
                .checked_add(file.size())
                .ok_or_else(|| Error::PackageLimit("total uncompressed size overflow".into()))?;
            if total_bytes > limits.max_total_bytes {
                return Err(Error::PackageLimit(format!(
                    "total uncompressed content exceeds {} bytes",
                    limits.max_total_bytes
                )));
            }
            entries.push(PreparedEntry {
                index,
                path,
                size: file.size(),
            });
        }
    }

    let manifest_index =
        manifest_index.ok_or_else(|| Error::InvalidManifest("manifest.json is missing".into()))?;
    let manifest_file = zip.by_index(manifest_index)?;
    let manifest_reported_size = manifest_file.size();
    let manifest_capacity = usize::try_from(manifest_reported_size).map_err(|_| {
        Error::PackageLimit("manifest length does not fit in process address space".into())
    })?;
    let mut manifest_bytes = Vec::with_capacity(manifest_capacity);
    let mut limited_manifest = manifest_file.take(limits.max_manifest_bytes.saturating_add(1));
    limited_manifest.read_to_end(&mut manifest_bytes)?;
    if manifest_bytes.len() as u64 > limits.max_manifest_bytes {
        return Err(Error::PackageLimit(format!(
            "manifest exceeds {} bytes while decompressing",
            limits.max_manifest_bytes
        )));
    }
    if manifest_bytes.len() as u64 != manifest_reported_size {
        return Err(Error::SizeMismatch {
            path: MANIFEST_PATH.into(),
            expected: manifest_reported_size,
            actual: manifest_bytes.len() as u64,
        });
    }
    let manifest: ManifestV1 = serde_json::from_slice(&manifest_bytes)?;
    manifest.validate()?;

    if manifest.files.len() > limits.max_entries {
        return Err(Error::PackageLimit(format!(
            "manifest declares {} files; limit is {}",
            manifest.files.len(),
            limits.max_entries
        )));
    }
    let declared: BTreeMap<&str, &ManifestFileV1> = manifest
        .files
        .iter()
        .map(|file| (file.path.as_str(), file))
        .collect();
    for entry in &entries {
        let Some(expected) = declared.get(entry.path.as_str()) else {
            return Err(Error::UndeclaredEntry(entry.path.clone()));
        };
        if expected.size != entry.size {
            return Err(Error::SizeMismatch {
                path: entry.path.clone(),
                expected: expected.size,
                actual: entry.size,
            });
        }
    }
    let archived_paths: BTreeSet<&str> = entries.iter().map(|entry| entry.path.as_str()).collect();
    for expected in &manifest.files {
        if !archived_paths.contains(expected.path.as_str()) {
            return Err(Error::MissingEntry(expected.path.clone()));
        }
    }

    entries.sort_by(|left, right| left.path.cmp(&right.path));
    Ok(PreparedPackage {
        manifest,
        manifest_bytes,
        entries,
        total_bytes,
    })
}

fn verify_file(
    expected: &ManifestFileV1,
    actual_size: u64,
    actual_hash: crate::hash::Sha256Digest,
) -> Result<()> {
    if actual_size != expected.size {
        return Err(Error::SizeMismatch {
            path: expected.path.clone(),
            expected: expected.size,
            actual: actual_size,
        });
    }
    if actual_hash != expected.sha256 {
        return Err(Error::HashMismatch {
            path: expected.path.clone(),
            expected: expected.sha256.to_string(),
            actual: actual_hash.to_string(),
        });
    }
    Ok(())
}

fn enforce_compression_ratio(
    path: &str,
    size: u64,
    compressed_size: u64,
    limits: PackageLimits,
) -> Result<()> {
    if size == 0 {
        return Ok(());
    }
    if compressed_size == 0 {
        return Err(Error::PackageLimit(format!(
            "non-empty entry `{path}` reports zero compressed bytes"
        )));
    }
    let permitted = compressed_size.saturating_mul(limits.max_compression_ratio);
    if size > permitted {
        return Err(Error::PackageLimit(format!(
            "entry `{path}` exceeds maximum compression ratio {}:1",
            limits.max_compression_ratio
        )));
    }
    Ok(())
}

fn is_regular_mode(mode: Option<u32>) -> bool {
    let Some(mode) = mode else {
        return true;
    };
    let file_type = mode & 0o170_000;
    file_type == 0 || file_type == 0o100_000
}

fn file_options(method: CompressionMethod, compression_level: i64) -> SimpleFileOptions {
    SimpleFileOptions::default()
        .compression_method(method)
        .compression_level((method == CompressionMethod::Deflated).then_some(compression_level))
        .last_modified_time(DateTime::default())
        .unix_permissions(0o644)
        // Always emit ZIP64 size fields. This keeps the wire shape stable and
        // prevents crossing 4 GiB from failing late in a long packaging run.
        .large_file(true)
}

fn compression_for(path: &str) -> CompressionMethod {
    let extension = path
        .rsplit('.')
        .next()
        .unwrap_or_default()
        .to_ascii_lowercase();
    if matches!(
        extension.as_str(),
        "ktx2" | "dds" | "png" | "jpg" | "jpeg" | "ogg" | "mp3" | "zip" | "vqa"
    ) {
        CompressionMethod::Stored
    } else {
        CompressionMethod::Deflated
    }
}

fn create_temporary_file(destination: &Path) -> Result<(PathBuf, File)> {
    for attempt in 0..100_u32 {
        let path = temporary_sibling_path(destination, "partial", attempt);
        match OpenOptions::new().write(true).create_new(true).open(&path) {
            Ok(file) => return Ok((path, file)),
            Err(error) if error.kind() == std::io::ErrorKind::AlreadyExists => continue,
            Err(error) => return Err(Error::Io(error)),
        }
    }
    Err(Error::Io(std::io::Error::new(
        std::io::ErrorKind::AlreadyExists,
        "could not allocate a unique package temporary file",
    )))
}

fn create_temporary_directory(destination: &Path) -> Result<PathBuf> {
    for attempt in 0..100_u32 {
        let path = temporary_sibling_path(destination, "extracting", attempt);
        match fs::create_dir(&path) {
            Ok(()) => return Ok(path),
            Err(error) if error.kind() == std::io::ErrorKind::AlreadyExists => continue,
            Err(error) => return Err(Error::Io(error)),
        }
    }
    Err(Error::Io(std::io::Error::new(
        std::io::ErrorKind::AlreadyExists,
        "could not allocate a unique extraction directory",
    )))
}

fn temporary_sibling_path(destination: &Path, suffix: &str, attempt: u32) -> PathBuf {
    let parent = destination.parent().unwrap_or_else(|| Path::new("."));
    let name = destination
        .file_name()
        .and_then(|name| name.to_str())
        .unwrap_or("cncweb");
    let timestamp = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_nanos();
    parent.join(format!(
        ".{name}.{suffix}.{}.{}.{}",
        std::process::id(),
        timestamp,
        attempt
    ))
}
