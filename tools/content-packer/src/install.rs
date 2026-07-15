use std::fmt;
use std::fs;
use std::path::{Path, PathBuf};
use std::str::FromStr;

use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};

use crate::error::{Error, Result};
use crate::hash::{digest_from_hasher, hash_file, Sha256Digest};
use crate::meg::{MegArchive, MegLimits};

const SHARED_ARCHIVES: &[&str] = &[
    "CONFIG.MEG",
    "TEXTURES_COMMON_SRGB.MEG",
    "TEXTURES_SRGB.MEG",
];
const TD_ARCHIVES: &[&str] = &["TEXTURES_TD_SRGB.MEG", "MOVIES_TD.MEG"];
const RA_ARCHIVES: &[&str] = &["TEXTURES_RA_SRGB.MEG", "MOVIES_RA.MEG"];

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum InstallProfile {
    /// The five archives loaded by the official map editor at startup.
    MapEditor,
    TiberianDawn,
    RedAlert,
    Collection,
}

impl InstallProfile {
    pub fn required_archives(self) -> Vec<&'static str> {
        let mut required = SHARED_ARCHIVES.to_vec();
        match self {
            Self::MapEditor => {
                required.push("TEXTURES_RA_SRGB.MEG");
                required.push("TEXTURES_TD_SRGB.MEG");
            }
            Self::TiberianDawn => required.extend_from_slice(TD_ARCHIVES),
            Self::RedAlert => required.extend_from_slice(RA_ARCHIVES),
            Self::Collection => {
                required.extend_from_slice(TD_ARCHIVES);
                required.extend_from_slice(RA_ARCHIVES);
            }
        }
        required.sort_unstable();
        required.dedup();
        required
    }
}

impl fmt::Display for InstallProfile {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        let value = match self {
            Self::MapEditor => "map-editor",
            Self::TiberianDawn => "tiberian-dawn",
            Self::RedAlert => "red-alert",
            Self::Collection => "collection",
        };
        formatter.write_str(value)
    }
}

impl FromStr for InstallProfile {
    type Err = Error;

    fn from_str(value: &str) -> Result<Self> {
        match value {
            "map-editor" => Ok(Self::MapEditor),
            "tiberian-dawn" | "td" => Ok(Self::TiberianDawn),
            "red-alert" | "ra" => Ok(Self::RedAlert),
            "collection" => Ok(Self::Collection),
            _ => Err(Error::Install(format!(
                "unknown profile `{value}`; expected map-editor, tiberian-dawn, red-alert, or collection"
            ))),
        }
    }
}

#[derive(Debug, Clone, Copy, Default)]
pub struct ValidationOptions {
    pub compute_hashes: bool,
    pub meg_limits: MegLimits,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct ArchiveCheck {
    /// A known logical archive ID, never a user-supplied absolute filename.
    pub logical_name: String,
    pub byte_size: u64,
    pub entry_count: u32,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub sha256: Option<Sha256Digest>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum InstallIssueCode {
    DataDirectoryMissing,
    ArchiveMissing,
    ArchiveNotFile,
    ArchiveCorrupt,
    ArchiveUnreadable,
    AmbiguousName,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct InstallIssue {
    pub code: InstallIssueCode,
    pub logical_name: String,
    pub message: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct InstallReport {
    pub profile: InstallProfile,
    pub valid: bool,
    pub data_directory_found: bool,
    pub archives: Vec<ArchiveCheck>,
    pub issues: Vec<InstallIssue>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub install_fingerprint_sha256: Option<Sha256Digest>,
}

/// Structurally validates known MEG archives in a user-selected Remastered
/// Collection directory. This is a compatibility check, not an entitlement
/// check, and it never uploads or modifies the selected files.
pub fn validate_install(
    root: impl AsRef<Path>,
    profile: InstallProfile,
    options: ValidationOptions,
) -> Result<InstallReport> {
    let root = root.as_ref();
    let metadata = fs::metadata(root).map_err(|error| {
        Error::Install(format!(
            "cannot read selected installation directory: {error}"
        ))
    })?;
    if !metadata.is_dir() {
        return Err(Error::Install(
            "selected installation path is not a directory".into(),
        ));
    }

    let mut report = InstallReport {
        profile,
        valid: false,
        data_directory_found: false,
        archives: Vec::new(),
        issues: Vec::new(),
        install_fingerprint_sha256: None,
    };

    let Some(data_directory) = locate_data_directory(root)? else {
        report.issues.push(InstallIssue {
            code: InstallIssueCode::DataDirectoryMissing,
            logical_name: "DATA".into(),
            message: "No DATA directory containing CONFIG.MEG was found".into(),
        });
        return Ok(report);
    };
    report.data_directory_found = true;

    for logical_name in profile.required_archives() {
        let archive_path = match find_case_insensitive(&data_directory, logical_name) {
            Ok(Some(path)) => path,
            Ok(None) => {
                report.issues.push(InstallIssue {
                    code: InstallIssueCode::ArchiveMissing,
                    logical_name: logical_name.into(),
                    message: "Required archive is missing".into(),
                });
                continue;
            }
            Err(error) => {
                report.issues.push(InstallIssue {
                    code: InstallIssueCode::AmbiguousName,
                    logical_name: logical_name.into(),
                    message: error.to_string(),
                });
                continue;
            }
        };

        let archive_metadata = match fs::metadata(&archive_path) {
            Ok(metadata) if metadata.is_file() => metadata,
            Ok(_) => {
                report.issues.push(InstallIssue {
                    code: InstallIssueCode::ArchiveNotFile,
                    logical_name: logical_name.into(),
                    message: "Expected a regular file".into(),
                });
                continue;
            }
            Err(error) => {
                report.issues.push(InstallIssue {
                    code: InstallIssueCode::ArchiveUnreadable,
                    logical_name: logical_name.into(),
                    message: format!("Cannot read archive metadata: {error}"),
                });
                continue;
            }
        };

        let archive = match MegArchive::open_with_limits(&archive_path, options.meg_limits) {
            Ok(archive) => archive,
            Err(error) => {
                report.issues.push(InstallIssue {
                    code: InstallIssueCode::ArchiveCorrupt,
                    logical_name: logical_name.into(),
                    message: error.to_string(),
                });
                continue;
            }
        };

        let sha256 = if options.compute_hashes {
            match hash_file(&archive_path) {
                Ok((digest, _)) => Some(digest),
                Err(error) => {
                    report.issues.push(InstallIssue {
                        code: InstallIssueCode::ArchiveUnreadable,
                        logical_name: logical_name.into(),
                        message: format!("Cannot hash archive: {error}"),
                    });
                    continue;
                }
            }
        } else {
            None
        };

        report.archives.push(ArchiveCheck {
            logical_name: logical_name.into(),
            byte_size: archive_metadata.len(),
            entry_count: archive.header().file_count,
            sha256,
        });
    }

    report
        .archives
        .sort_by(|left, right| left.logical_name.cmp(&right.logical_name));
    report.valid =
        report.issues.is_empty() && report.archives.len() == profile.required_archives().len();
    if report.valid && options.compute_hashes {
        report.install_fingerprint_sha256 = Some(calculate_install_fingerprint(&report.archives)?);
    }
    Ok(report)
}

fn locate_data_directory(root: &Path) -> Result<Option<PathBuf>> {
    if find_case_insensitive(root, "CONFIG.MEG")?.is_some() {
        return Ok(Some(root.to_path_buf()));
    }
    let Some(data) = find_case_insensitive(root, "DATA")? else {
        return Ok(None);
    };
    if data.is_dir() && find_case_insensitive(&data, "CONFIG.MEG")?.is_some() {
        Ok(Some(data))
    } else {
        Ok(None)
    }
}

fn find_case_insensitive(directory: &Path, name: &str) -> Result<Option<PathBuf>> {
    let mut found = None;
    for entry in fs::read_dir(directory)? {
        let entry = entry?;
        let entry_name = entry.file_name();
        let Some(entry_name) = entry_name.to_str() else {
            continue;
        };
        if entry_name.eq_ignore_ascii_case(name) {
            if found.is_some() {
                return Err(Error::Install(format!(
                    "more than one filesystem entry matches logical name `{name}`"
                )));
            }
            found = Some(entry.path());
        }
    }
    Ok(found)
}

fn calculate_install_fingerprint(archives: &[ArchiveCheck]) -> Result<Sha256Digest> {
    let mut hasher = Sha256::new();
    hasher.update(b"CNCWEB-INSTALL-FINGERPRINT-V1\0");
    for archive in archives {
        let digest = archive.sha256.ok_or_else(|| {
            Error::Install(
                "cannot calculate installation fingerprint without archive hashes".into(),
            )
        })?;
        hasher.update((archive.logical_name.len() as u64).to_le_bytes());
        hasher.update(archive.logical_name.as_bytes());
        hasher.update(archive.byte_size.to_le_bytes());
        hasher.update(digest.as_bytes());
    }
    Ok(digest_from_hasher(hasher))
}
