//! Deployment descriptor generation for the same-origin classic-freeware pack.

use std::fs::OpenOptions;
use std::io::Write;
use std::path::Path;

use serde::{Deserialize, Serialize};

use crate::error::{Error, Result};
use crate::hash::{hash_file, Sha256Digest};
use crate::manifest::{ContentRoleV1, ManifestV1, SourceProduct, SourceProvider};
use crate::package::{verify_package, PackageLimits};

pub const CLASSIC_FREEWARE_BOOTSTRAP_FORMAT: &str = "cncweb-classic-freeware";
pub const CLASSIC_FREEWARE_BOOTSTRAP_VERSION: u32 = 1;

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct ClassicFreewareBootstrapV1 {
    pub format: String,
    pub version: u32,
    pub package: ClassicFreewarePackageV1,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ClassicFreewarePackageV1 {
    pub id: String,
    pub content_sha256: Sha256Digest,
    pub source: ClassicFreewareSourceV1,
    pub archive: ClassicFreewareArchiveV1,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct ClassicFreewareSourceV1 {
    pub product: SourceProduct,
    pub provider: SourceProvider,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct ClassicFreewareArchiveV1 {
    pub url: String,
    pub bytes: u64,
    pub sha256: Sha256Digest,
}

pub fn emit_classic_freeware_bootstrap(
    package_path: impl AsRef<Path>,
    output_path: impl AsRef<Path>,
    archive_url: impl Into<String>,
    limits: PackageLimits,
) -> Result<ClassicFreewareBootstrapV1> {
    let package_path = package_path.as_ref();
    let output_path = output_path.as_ref();
    if output_path.exists() {
        return Err(Error::OutputExists(output_path.to_path_buf()));
    }
    let archive_url = archive_url.into();
    validate_archive_url(&archive_url)?;
    let manifest = verify_package(package_path, limits)?;
    validate_freeware_manifest(&manifest)?;
    let (archive_sha256, archive_bytes) = hash_file(package_path)?;
    if archive_bytes == 0 || archive_bytes > limits.max_total_bytes {
        return Err(Error::PackageLimit(format!(
            "classic freeware archive is {archive_bytes} bytes; deployment limit is {}",
            limits.max_total_bytes
        )));
    }

    let descriptor = ClassicFreewareBootstrapV1 {
        format: CLASSIC_FREEWARE_BOOTSTRAP_FORMAT.into(),
        version: CLASSIC_FREEWARE_BOOTSTRAP_VERSION,
        package: ClassicFreewarePackageV1 {
            id: manifest.package_id,
            content_sha256: manifest.content_sha256,
            source: ClassicFreewareSourceV1 {
                product: manifest.source.product,
                provider: manifest.source.provider,
            },
            archive: ClassicFreewareArchiveV1 {
                url: archive_url,
                bytes: archive_bytes,
                sha256: archive_sha256,
            },
        },
    };
    if let Some(parent) = output_path.parent() {
        std::fs::create_dir_all(parent)?;
    }
    let mut bytes = serde_json::to_vec_pretty(&descriptor)?;
    bytes.push(b'\n');
    let mut output = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(output_path)?;
    output.write_all(&bytes)?;
    output.sync_all()?;
    Ok(descriptor)
}

fn validate_archive_url(value: &str) -> Result<()> {
    let Some(name) = value.strip_prefix("./") else {
        return Err(Error::InvalidManifest(
            "classic freeware archive URL must be a same-directory ./ relative URL".into(),
        ));
    };
    if name.is_empty()
        || name.len() > 255
        || name.contains('/')
        || !name.ends_with(".cncweb")
        || !name
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'.' | b'_' | b'-'))
    {
        return Err(Error::InvalidManifest(
            "classic freeware archive URL must name one portable .cncweb file".into(),
        ));
    }
    Ok(())
}

fn validate_freeware_manifest(manifest: &ManifestV1) -> Result<()> {
    if manifest.source.product != SourceProduct::TiberianDawnFreeware
        || manifest.source.provider != SourceProvider::EaFreeware
    {
        return Err(Error::InvalidManifest(
            "bootstrap package must declare the Tiberian Dawn EA freeware source".into(),
        ));
    }
    for file in &manifest.files {
        let path = file.path.to_ascii_lowercase();
        let forbidden = file.role == ContentRoleV1::Video
            || path == "engine/td/scores.mix"
            || path == "engine/td/movies.mix"
            || path.starts_with("music/")
            || path.starts_with("audio/music/")
            || path.ends_with(".aud")
            || path.ends_with(".vqa");
        if forbidden {
            return Err(Error::InvalidManifest(format!(
                "classic freeware deployment contains forbidden music or movie content: {}",
                file.path
            )));
        }
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use tempfile::tempdir;

    use super::*;
    use crate::manifest::{ContentDescriptorV1, GameId, SourceV1};
    use crate::package::{create_package, CreateOptions};

    fn package(
        root: &Path,
        product: SourceProduct,
        provider: SourceProvider,
    ) -> std::path::PathBuf {
        let staging = root.join("staging");
        std::fs::create_dir_all(staging.join("config")).unwrap();
        std::fs::write(staging.join("config/runtime.json"), b"{}\n").unwrap();
        let package = root.join("classic.cncweb");
        create_package(
            staging,
            &package,
            CreateOptions {
                package_id: "classic-freeware-gdi-v1".into(),
                created_at_unix_ms: 0,
                source: SourceV1 {
                    product,
                    provider,
                    install_fingerprint_sha256: Sha256Digest::ZERO,
                },
                content: ContentDescriptorV1 {
                    games: vec![GameId::TiberianDawn],
                    locales: vec!["en-US".into()],
                },
                compression_level: 6,
                limits: PackageLimits::browser_v1(),
            },
        )
        .unwrap();
        package
    }

    #[test]
    fn emits_the_browser_descriptor_from_a_verified_freeware_package() {
        let temp = tempdir().unwrap();
        let package = package(
            temp.path(),
            SourceProduct::TiberianDawnFreeware,
            SourceProvider::EaFreeware,
        );
        let output = temp.path().join("classic-freeware-v1.json");
        let descriptor = emit_classic_freeware_bootstrap(
            &package,
            &output,
            "./classic-freeware-gdi-v1.cncweb",
            PackageLimits::browser_v1(),
        )
        .unwrap();
        assert_eq!(descriptor.format, CLASSIC_FREEWARE_BOOTSTRAP_FORMAT);
        assert_eq!(descriptor.package.id, "classic-freeware-gdi-v1");
        assert!(descriptor.package.archive.bytes > 0);
        let decoded: ClassicFreewareBootstrapV1 =
            serde_json::from_slice(&std::fs::read(output).unwrap()).unwrap();
        assert_eq!(decoded, descriptor);
    }

    #[test]
    fn rejects_non_freeware_provenance_and_unsafe_urls() {
        let temp = tempdir().unwrap();
        let package = package(
            temp.path(),
            SourceProduct::CncRemasteredCollection,
            SourceProvider::Unknown,
        );
        assert!(emit_classic_freeware_bootstrap(
            &package,
            temp.path().join("bad.json"),
            "./classic.cncweb",
            PackageLimits::browser_v1(),
        )
        .is_err());
        assert!(emit_classic_freeware_bootstrap(
            &package,
            temp.path().join("bad-url.json"),
            "https://example.test/classic.cncweb",
            PackageLimits::browser_v1(),
        )
        .is_err());
    }
}
