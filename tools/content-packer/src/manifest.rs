use std::collections::BTreeSet;

use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};

use crate::error::{Error, Result};
use crate::hash::{digest_from_hasher, Sha256Digest};
use crate::path::{collision_key, normalize_package_path};

pub const MANIFEST_FORMAT: &str = "cncweb-content";
pub const MANIFEST_VERSION: u32 = 1;
pub const MANIFEST_PATH: &str = "manifest.json";
const JSON_SAFE_INTEGER_MAX: u64 = 9_007_199_254_740_991;

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct ManifestV1 {
    pub format: String,
    pub version: u32,
    pub package_id: String,
    pub created_at_unix_ms: u64,
    pub source: SourceV1,
    pub content: ContentDescriptorV1,
    pub content_sha256: Sha256Digest,
    pub files: Vec<ManifestFileV1>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct SourceV1 {
    pub product: SourceProduct,
    pub provider: SourceProvider,
    pub install_fingerprint_sha256: Sha256Digest,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum SourceProduct {
    CncRemasteredCollection,
    TiberianDawnFreeware,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum SourceProvider {
    Steam,
    EaApp,
    CopiedInstallation,
    EaFreeware,
    Unknown,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct ContentDescriptorV1 {
    pub games: Vec<GameId>,
    pub locales: Vec<String>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum GameId {
    TiberianDawn,
    RedAlert,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct ManifestFileV1 {
    pub path: String,
    pub size: u64,
    pub sha256: Sha256Digest,
    pub role: ContentRoleV1,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum ContentRoleV1 {
    EngineData,
    TextureAtlas,
    Audio,
    Video,
    Map,
    Configuration,
    Other,
}

impl ContentRoleV1 {
    pub const fn as_wire_name(self) -> &'static str {
        match self {
            Self::EngineData => "engine-data",
            Self::TextureAtlas => "texture-atlas",
            Self::Audio => "audio",
            Self::Video => "video",
            Self::Map => "map",
            Self::Configuration => "configuration",
            Self::Other => "other",
        }
    }
}

impl ManifestV1 {
    pub fn new(
        package_id: String,
        created_at_unix_ms: u64,
        source: SourceV1,
        content: ContentDescriptorV1,
        files: Vec<ManifestFileV1>,
    ) -> Self {
        let content_sha256 = calculate_content_digest(&files);
        Self {
            format: MANIFEST_FORMAT.to_owned(),
            version: MANIFEST_VERSION,
            package_id,
            created_at_unix_ms,
            source,
            content,
            content_sha256,
            files,
        }
    }

    /// Validates intrinsic schema invariants. Resource ceilings are applied by
    /// package inspection/extraction in addition to these checks.
    pub fn validate(&self) -> Result<()> {
        if self.format != MANIFEST_FORMAT {
            return Err(Error::InvalidManifest(format!(
                "unsupported format `{}`",
                self.format
            )));
        }
        if self.version != MANIFEST_VERSION {
            return Err(Error::InvalidManifest(format!(
                "unsupported manifest version {}",
                self.version
            )));
        }
        validate_package_id(&self.package_id)?;
        if self.created_at_unix_ms > JSON_SAFE_INTEGER_MAX {
            return Err(Error::InvalidManifest(
                "created_at_unix_ms exceeds JavaScript's exact integer range".into(),
            ));
        }

        match (self.source.product, self.source.provider) {
            (SourceProduct::CncRemasteredCollection, SourceProvider::EaFreeware)
            | (
                SourceProduct::TiberianDawnFreeware,
                SourceProvider::Steam
                | SourceProvider::EaApp
                | SourceProvider::CopiedInstallation
                | SourceProvider::Unknown,
            ) => {
                return Err(Error::InvalidManifest(
                    "source product and provider do not describe a supported provenance".into(),
                ));
            }
            _ => {}
        }

        if self.content.games.is_empty() {
            return Err(Error::InvalidManifest(
                "content.games must contain at least one game".into(),
            ));
        }
        if self.content.locales.is_empty() {
            return Err(Error::InvalidManifest(
                "content.locales must contain at least one locale".into(),
            ));
        }

        let mut games = BTreeSet::new();
        for game in &self.content.games {
            if !games.insert(*game) {
                return Err(Error::InvalidManifest("duplicate game identifier".into()));
            }
        }

        let mut locales = BTreeSet::new();
        for locale in &self.content.locales {
            validate_locale(locale)?;
            if !locales.insert(locale.to_ascii_lowercase()) {
                return Err(Error::InvalidManifest(format!(
                    "duplicate locale `{locale}`"
                )));
            }
        }

        let mut paths = BTreeSet::new();
        let mut previous: Option<&str> = None;
        for file in &self.files {
            if file.size > JSON_SAFE_INTEGER_MAX {
                return Err(Error::InvalidManifest(format!(
                    "size for `{}` exceeds JavaScript's exact integer range",
                    file.path
                )));
            }
            let path = normalize_package_path(&file.path)?;
            if collision_key(&path) == collision_key(MANIFEST_PATH) {
                return Err(Error::InvalidManifest(
                    "manifest.json is reserved (case-insensitively) and cannot appear in files"
                        .into(),
                ));
            }
            if let Some(previous) = previous {
                if previous >= path.as_str() {
                    return Err(Error::InvalidManifest(
                        "files must be strictly sorted by path".into(),
                    ));
                }
            }
            previous = Some(&file.path);

            if !paths.insert(collision_key(&path)) {
                return Err(Error::DuplicatePath(path));
            }
        }

        let actual_content_digest = calculate_content_digest(&self.files);
        if actual_content_digest != self.content_sha256 {
            return Err(Error::InvalidManifest(format!(
                "content_sha256 mismatch: expected {}, calculated {}",
                self.content_sha256, actual_content_digest
            )));
        }

        Ok(())
    }
}

pub fn calculate_content_digest(files: &[ManifestFileV1]) -> Sha256Digest {
    let mut hasher = Sha256::new();
    hasher.update(b"CNCWEB-CONTENT-MANIFEST-V1\0");
    for file in files {
        hasher.update((file.path.len() as u64).to_le_bytes());
        hasher.update(file.path.as_bytes());
        hasher.update(file.size.to_le_bytes());
        hasher.update(file.sha256.as_bytes());
        let role = file.role.as_wire_name();
        hasher.update((role.len() as u64).to_le_bytes());
        hasher.update(role.as_bytes());
    }
    digest_from_hasher(hasher)
}

pub fn infer_role(path: &str) -> ContentRoleV1 {
    let lowercase = path.to_ascii_lowercase();
    let first = lowercase.split('/').next().unwrap_or_default();
    match first {
        "engine" => ContentRoleV1::EngineData,
        "atlases" | "textures" => ContentRoleV1::TextureAtlas,
        "audio" | "music" | "speech" | "sfx" => ContentRoleV1::Audio,
        "media" | "movies" | "video" => ContentRoleV1::Video,
        "maps" | "missions" => ContentRoleV1::Map,
        "config" | "metadata" | "runtime" => ContentRoleV1::Configuration,
        _ => ContentRoleV1::Other,
    }
}

fn validate_package_id(value: &str) -> Result<()> {
    if value.is_empty() || value.len() > 128 {
        return Err(Error::InvalidManifest(
            "package_id must contain 1 to 128 characters".into(),
        ));
    }
    if !value.as_bytes()[0].is_ascii_alphanumeric()
        || !value
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_' | b'.'))
    {
        return Err(Error::InvalidManifest(
            "package_id must begin with an ASCII letter or digit and may then contain dash, underscore, and dot"
                .into(),
        ));
    }
    Ok(())
}

fn validate_locale(value: &str) -> Result<()> {
    if value.len() < 2
        || value.len() > 35
        || !value.split('-').all(|segment| {
            !segment.is_empty() && segment.bytes().all(|byte| byte.is_ascii_alphanumeric())
        })
    {
        return Err(Error::InvalidManifest(format!(
            "invalid locale identifier `{value}`"
        )));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn digest(byte: u8) -> Sha256Digest {
        Sha256Digest::from_bytes([byte; 32])
    }

    fn manifest(files: Vec<ManifestFileV1>) -> ManifestV1 {
        ManifestV1::new(
            "td-slice".into(),
            1,
            SourceV1 {
                product: SourceProduct::CncRemasteredCollection,
                provider: SourceProvider::Unknown,
                install_fingerprint_sha256: digest(7),
            },
            ContentDescriptorV1 {
                games: vec![GameId::TiberianDawn],
                locales: vec!["en-US".into()],
            },
            files,
        )
    }

    #[test]
    fn validates_a_canonical_manifest() {
        let value = manifest(vec![ManifestFileV1 {
            path: "audio/test.wav".into(),
            size: 4,
            sha256: digest(1),
            role: ContentRoleV1::Audio,
        }]);
        value.validate().unwrap();
        let json = serde_json::to_string_pretty(&value).unwrap();
        let decoded: ManifestV1 = serde_json::from_str(&json).unwrap();
        assert_eq!(decoded, value);
    }

    #[test]
    fn validates_freeware_provenance_and_rejects_crossed_pairs() {
        let mut value = manifest(Vec::new());
        value.source.product = SourceProduct::TiberianDawnFreeware;
        value.source.provider = SourceProvider::EaFreeware;
        value.validate().unwrap();

        value.source.provider = SourceProvider::Unknown;
        assert!(value.validate().is_err());

        value.source.product = SourceProduct::CncRemasteredCollection;
        value.source.provider = SourceProvider::EaFreeware;
        assert!(value.validate().is_err());
    }

    #[test]
    fn rejects_case_collisions_and_unsorted_files() {
        let files = vec![
            ManifestFileV1 {
                path: "maps/Test.ini".into(),
                size: 0,
                sha256: digest(1),
                role: ContentRoleV1::Map,
            },
            ManifestFileV1 {
                path: "maps/test.ini".into(),
                size: 0,
                sha256: digest(1),
                role: ContentRoleV1::Map,
            },
        ];
        assert!(manifest(files).validate().is_err());
    }

    #[test]
    fn rejects_non_portable_ids_and_empty_locale_subtags() {
        for package_id in [".hidden", "-leading", "_leading"] {
            let mut value = manifest(Vec::new());
            value.package_id = package_id.into();
            assert!(
                value.validate().is_err(),
                "accepted package ID {package_id}"
            );
        }

        for locale in ["en--US", "-en", "en-"] {
            let mut value = manifest(Vec::new());
            value.content.locales = vec![locale.into()];
            assert!(value.validate().is_err(), "accepted locale {locale}");
        }
    }
}
