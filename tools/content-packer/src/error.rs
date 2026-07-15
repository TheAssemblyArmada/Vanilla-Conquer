use std::path::PathBuf;

use thiserror::Error;

pub type Result<T> = std::result::Result<T, Error>;

#[derive(Debug, Error)]
pub enum Error {
    #[error("I/O error: {0}")]
    Io(#[from] std::io::Error),

    #[error("ZIP error: {0}")]
    Zip(#[from] zip::result::ZipError),

    #[error("JSON error: {0}")]
    Json(#[from] serde_json::Error),

    #[error("invalid SHA-256 digest: {0}")]
    InvalidDigest(String),

    #[error("invalid MEG archive: {0}")]
    InvalidMeg(String),

    #[error("MEG archive exceeds a safety limit: {0}")]
    MegLimit(String),

    #[error("invalid MIX archive: {0}")]
    InvalidMix(String),

    #[error("MIX archive exceeds a safety limit: {0}")]
    MixLimit(String),

    #[error("invalid Westwood AUD sample: {0}")]
    InvalidAud(String),

    #[error("Westwood AUD sample exceeds a safety limit: {0}")]
    AudLimit(String),

    #[error("content conversion failed: {0}")]
    Conversion(String),

    #[error("unsafe archive path `{path}`: {reason}")]
    UnsafePath { path: String, reason: String },

    #[error("archive paths collide on a case-insensitive filesystem: `{0}`")]
    DuplicatePath(String),

    #[error("invalid package manifest: {0}")]
    InvalidManifest(String),

    #[error("package exceeds a safety limit: {0}")]
    PackageLimit(String),

    #[error("package entry `{0}` is not declared by the manifest")]
    UndeclaredEntry(String),

    #[error("manifest entry `{0}` is missing from the package")]
    MissingEntry(String),

    #[error("size mismatch for `{path}`: expected {expected} bytes, got {actual}")]
    SizeMismatch {
        path: String,
        expected: u64,
        actual: u64,
    },

    #[error("SHA-256 mismatch for `{path}`: expected {expected}, got {actual}")]
    HashMismatch {
        path: String,
        expected: String,
        actual: String,
    },

    #[error("output already exists: {0}")]
    OutputExists(PathBuf),

    #[error("unsupported filesystem entry `{path}`: {kind}")]
    UnsupportedFile { path: PathBuf, kind: &'static str },

    #[error("installation validation failed: {0}")]
    Install(String),
}

impl Error {
    pub(crate) fn unsafe_path(path: impl Into<String>, reason: impl Into<String>) -> Self {
        Self::UnsafePath {
            path: path.into(),
            reason: reason.into(),
        }
    }
}
