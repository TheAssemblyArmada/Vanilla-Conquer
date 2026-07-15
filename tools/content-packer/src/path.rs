use std::path::{Path, PathBuf};

use crate::error::{Error, Result};

pub const MAX_ARCHIVE_PATH_BYTES: usize = 1024;
pub const MAX_ARCHIVE_SEGMENT_BYTES: usize = 255;

/// Validates a portable package path. Package paths always use `/` and are
/// deliberately stricter than platform paths so one package behaves the same
/// on Windows, macOS, Linux, and OPFS.
pub fn normalize_package_path(value: &str) -> Result<String> {
    validate_portable_path(value, false)
}

/// Converts the backslash-separated paths used by retail MEG files to the
/// package path convention, then applies the same traversal checks.
pub fn normalize_meg_path(value: &str) -> Result<String> {
    let normalized = value.replace('\\', "/");
    validate_portable_path(&normalized, true)
}

fn validate_portable_path(value: &str, from_meg: bool) -> Result<String> {
    if value.is_empty() {
        return Err(Error::unsafe_path(value, "path is empty"));
    }
    if value.len() > MAX_ARCHIVE_PATH_BYTES {
        return Err(Error::unsafe_path(value, "path is longer than 1024 bytes"));
    }
    if value.starts_with('/') {
        return Err(Error::unsafe_path(value, "absolute paths are forbidden"));
    }
    if !from_meg && value.contains('\\') {
        return Err(Error::unsafe_path(
            value,
            "package paths must use forward slashes",
        ));
    }

    for segment in value.split('/') {
        if segment.is_empty() {
            return Err(Error::unsafe_path(value, "empty path segment"));
        }
        if segment == "." || segment == ".." {
            return Err(Error::unsafe_path(value, "dot path segments are forbidden"));
        }
        if segment.len() > MAX_ARCHIVE_SEGMENT_BYTES {
            return Err(Error::unsafe_path(
                value,
                "path segment is longer than 255 bytes",
            ));
        }
        if segment.ends_with(' ') || segment.ends_with('.') {
            return Err(Error::unsafe_path(
                value,
                "segments ending in a space or dot are not portable",
            ));
        }
        if segment.chars().any(|character| {
            character.is_control() || matches!(character, '<' | '>' | ':' | '"' | '|' | '?' | '*')
        }) {
            return Err(Error::unsafe_path(
                value,
                "path contains a control or platform-reserved character",
            ));
        }
        if is_windows_device_name(segment) {
            return Err(Error::unsafe_path(
                value,
                "path uses a reserved Windows device name",
            ));
        }
    }

    Ok(value.to_owned())
}

fn is_windows_device_name(segment: &str) -> bool {
    let stem = segment
        .split('.')
        .next()
        .unwrap_or(segment)
        .to_ascii_uppercase();
    matches!(stem.as_str(), "CON" | "PRN" | "AUX" | "NUL")
        || (stem.len() == 4
            && (stem.starts_with("COM") || stem.starts_with("LPT"))
            && matches!(stem.as_bytes()[3], b'1'..=b'9'))
}

pub fn collision_key(path: &str) -> String {
    path.to_lowercase()
}

pub fn safe_join(root: &Path, portable_path: &str) -> Result<PathBuf> {
    let normalized = normalize_package_path(portable_path)?;
    let mut output = root.to_path_buf();
    for segment in normalized.split('/') {
        output.push(segment);
    }
    Ok(output)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn accepts_portable_paths() {
        assert_eq!(
            normalize_package_path("atlases/td/temperate-1.ktx2").unwrap(),
            "atlases/td/temperate-1.ktx2"
        );
        assert_eq!(
            normalize_meg_path(r"DATA\ART\UNIT.DDS").unwrap(),
            "DATA/ART/UNIT.DDS"
        );
    }

    #[test]
    fn rejects_traversal_and_platform_tricks() {
        for path in [
            "../escape",
            "/absolute",
            "C:/drive",
            r"..\escape",
            "foo//bar",
            "foo/./bar",
            "foo/NUL.txt",
            "foo/trailing. ",
            "foo/question?.dds",
        ] {
            assert!(normalize_package_path(path).is_err(), "accepted {path}");
        }
    }
}
