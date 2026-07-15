//! Mission-scoped conversion from a user-owned Remastered Collection install.
//!
//! The profiles deliberately reuse the classic MIX assets consumed by the
//! released engine. They do not download, embed, or synthesize retail content.
//! The only generated files are versioned metadata describing how the browser
//! should mount and launch the copied engine data.

use std::collections::{BTreeMap, BTreeSet};
use std::fs::{self, File, OpenOptions};
use std::io::{Read, Write};
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};

use crate::aud::AudLimits;
use crate::audio::{build_audio_index, AudioBuildOptions, AUDIO_INDEX_PATH};
use crate::error::{Error, Result};
use crate::hash::{copy_and_hash, digest_from_hasher, Sha256Digest};
use crate::manifest::{ContentDescriptorV1, GameId, SourceProduct, SourceProvider, SourceV1};
use crate::mix::{MixArchive, MixLimits};
use crate::package::{create_package, verify_package, CreateOptions, PackageLimits};

pub const RUNTIME_CATALOG_PATH: &str = "runtime/catalog-v1.json";
pub const ENGINE_ROOT: &str = "engine/td";
/// Compatibility identity for callers that still launch the first GDI mission.
pub const MISSION_ID: &str = "gdi-01-east-a";
/// Compatibility identity for callers that still launch the first GDI mission.
pub const SCENARIO_ROOT: &str = "SCG01EA";
pub const CONVERSION_REPORT_PATH: &str = "metadata/conversion-report-v1.json";

const REQUIRED_COMMON_MIXES: &[&str] = &[
    "CCLOCAL.MIX",
    "CONQUER.MIX",
    "GENERAL.MIX",
    "LOCAL.MIX",
    "SOUNDS.MIX",
    "SPEECH.MIX",
    "TRANSIT.MIX",
    "UPDATA.MIX",
    "UPDATE.MIX",
    "UPDATEC.MIX",
];

/// Scenario patches are intentionally resolved before base data. Within the
/// patch group this retains the registration order from `tiberiandawn/init.cpp`.
const MISSION_RESOLUTION_ORDER: &[&str] = &[
    "UPDATE.MIX",
    "UPDATA.MIX",
    "UPDATEC.MIX",
    "CCLOCAL.MIX",
    "CONQUER.MIX",
    "TRANSIT.MIX",
    "GENERAL.MIX",
    "SPEECH.MIX",
    "SOUNDS.MIX",
];

const EXPECTED_MAP_BYTES: u64 = 8 * 1024;
const MAX_SCENARIO_INI_BYTES: u64 = 1024 * 1024;
pub const MAX_BRIEFING_BYTES: usize = 4 * 1024;
pub const MAX_RUNTIME_CATALOG_BYTES: usize = 64 * 1024;
const MAX_CATALOG_MISSIONS: usize = 256;
const FREEWARE_BRIEFING: &str =
    "Complete the mission objectives shown by the battlefield and status panels.";

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
pub enum MissionTheater {
    Temperate,
    Desert,
    Winter,
}

impl MissionTheater {
    fn from_ini(value: &str) -> Option<Self> {
        if value.eq_ignore_ascii_case("TEMPERATE") {
            Some(Self::Temperate)
        } else if value.eq_ignore_ascii_case("DESERT") {
            Some(Self::Desert)
        } else if value.eq_ignore_ascii_case("WINTER") {
            Some(Self::Winter)
        } else {
            None
        }
    }

    const fn archive_pair(self) -> [&'static str; 2] {
        match self {
            Self::Temperate => ["TEMPERAT.MIX", "TEMPICNH.MIX"],
            Self::Desert => ["DESERT.MIX", "DESEICNH.MIX"],
            Self::Winter => ["WINTER.MIX", "WINTICNH.MIX"],
        }
    }

    const fn optional_palette(self) -> &'static str {
        match self {
            Self::Temperate => "TEMPERAT.PAL",
            Self::Desert => "DESERT.PAL",
            Self::Winter => "WINTER.PAL",
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum ConversionProfile {
    #[serde(rename = "td-gdi-01-east-a")]
    TdGdi01EastA,
    TdGdiCampaign,
    TdNodCampaign,
}

impl ConversionProfile {
    pub const fn id(self) -> &'static str {
        match self {
            Self::TdGdi01EastA => "td-gdi-01-east-a",
            Self::TdGdiCampaign => "td-gdi-campaign",
            Self::TdNodCampaign => "td-nod-campaign",
        }
    }

    const fn disc(self) -> &'static str {
        match self {
            Self::TdGdi01EastA | Self::TdGdiCampaign => "CD1",
            Self::TdNodCampaign => "CD2",
        }
    }

    const fn faction(self) -> &'static str {
        match self {
            Self::TdGdi01EastA | Self::TdGdiCampaign => "gdi",
            Self::TdNodCampaign => "nod",
        }
    }
}

#[derive(Debug, Clone)]
pub struct ConversionOptions {
    pub profile: ConversionProfile,
    pub package_id: String,
    pub created_at_unix_ms: u64,
    pub source_product: SourceProduct,
    pub provider: SourceProvider,
    pub locales: Vec<String>,
    pub compression_level: i64,
    pub limits: PackageLimits,
    pub mix_limits: MixLimits,
    pub aud_limits: AudLimits,
}

impl ConversionOptions {
    pub fn for_profile(
        profile: ConversionProfile,
        package_id: impl Into<String>,
        created_at_unix_ms: u64,
        provider: SourceProvider,
        locales: Vec<String>,
    ) -> Self {
        Self {
            profile,
            package_id: package_id.into(),
            created_at_unix_ms,
            source_product: SourceProduct::CncRemasteredCollection,
            provider,
            locales,
            compression_level: 6,
            limits: PackageLimits::browser_v1(),
            mix_limits: MixLimits::default(),
            aud_limits: AudLimits::default(),
        }
    }

    pub fn td_gdi_campaign(
        package_id: impl Into<String>,
        created_at_unix_ms: u64,
        provider: SourceProvider,
        locales: Vec<String>,
    ) -> Self {
        Self::for_profile(
            ConversionProfile::TdGdiCampaign,
            package_id,
            created_at_unix_ms,
            provider,
            locales,
        )
    }

    pub fn td_nod_campaign(
        package_id: impl Into<String>,
        created_at_unix_ms: u64,
        provider: SourceProvider,
        locales: Vec<String>,
    ) -> Self {
        Self::for_profile(
            ConversionProfile::TdNodCampaign,
            package_id,
            created_at_unix_ms,
            provider,
            locales,
        )
    }

    /// Compatibility constructor for the original one-mission owned-content
    /// acceptance profile.
    pub fn td_gdi_01(
        package_id: impl Into<String>,
        created_at_unix_ms: u64,
        provider: SourceProvider,
        locales: Vec<String>,
    ) -> Self {
        Self::for_profile(
            ConversionProfile::TdGdi01EastA,
            package_id,
            created_at_unix_ms,
            provider,
            locales,
        )
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum ConversionPhase {
    Locate,
    Validate,
    Copy,
    ExtractMission,
    ConvertAudio,
    WriteMetadata,
    Package,
    Verify,
    Complete,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct ConversionProgress {
    pub phase: ConversionPhase,
    pub current: usize,
    pub total: usize,
    pub item: String,
    pub message: String,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum SourceIssueCode {
    CdDirectoryMissing,
    AmbiguousName,
    RequiredFileMissing,
    NotAFile,
    InvalidMix,
    InvalidPalette,
    MissionEntryMissing,
    InvalidMissionIni,
    InvalidMissionMap,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct SourceIssue {
    pub code: SourceIssueCode,
    pub logical_name: String,
    pub message: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct SourceAssetProbe {
    pub logical_name: String,
    pub required: bool,
    pub byte_size: u64,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub mix_entries: Option<u16>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct MissionSourceProbe {
    pub id: String,
    pub scenario_root: String,
    pub theater: MissionTheater,
    pub ini_source: String,
    pub bin_source: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct SourceInspection {
    pub profile: ConversionProfile,
    pub valid: bool,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub cd_directory: Option<String>,
    pub assets: Vec<SourceAssetProbe>,
    pub optional_missing: Vec<String>,
    pub issues: Vec<SourceIssue>,
    pub missions: Vec<MissionSourceProbe>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
struct CanonicalMission {
    scenario: u32,
    direction: u32,
    variation: u32,
}

impl CanonicalMission {
    fn id(self, profile: ConversionProfile) -> String {
        mission_id(
            profile.faction(),
            self.scenario,
            self.direction,
            self.variation,
        )
        .expect("static campaign descriptor is valid")
    }

    fn root(self, profile: ConversionProfile) -> String {
        scenario_root(
            profile.faction(),
            self.scenario,
            self.direction,
            self.variation,
        )
        .expect("static campaign descriptor is valid")
    }

    fn runtime(
        self,
        profile: ConversionProfile,
        briefing: String,
        theater: MissionTheater,
    ) -> RuntimeMissionV1 {
        let direction = if self.direction == 0 { "East" } else { "West" };
        let variation = char::from(b'A' + self.variation as u8);
        let faction_title = if profile.faction() == "gdi" {
            "GDI"
        } else {
            "Nod"
        };
        let title = if self.scenario == 1 && self.direction == 0 && self.variation == 0 {
            format!("{faction_title} Mission 1")
        } else {
            format!(
                "{faction_title} Mission {} ({direction} {variation})",
                self.scenario
            )
        };
        RuntimeMissionV1 {
            id: self.id(profile),
            scenario_root: self.root(profile),
            scenario: self.scenario,
            variation: self.variation,
            direction: self.direction,
            build_level: self.scenario,
            sabotaged_structure: -1,
            faction: profile.faction().into(),
            title,
            briefing,
            theater,
        }
    }
}

// Deduplicated in table order from `CountryArray` in
// `tiberiandawn/mapsel.cpp`, preceded by the fixed East/A opening mission.
const GDI_CAMPAIGN_MISSIONS: &[CanonicalMission] = &[
    CanonicalMission {
        scenario: 1,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 2,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 3,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 4,
        direction: 1,
        variation: 0,
    },
    CanonicalMission {
        scenario: 4,
        direction: 1,
        variation: 1,
    },
    CanonicalMission {
        scenario: 4,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 5,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 5,
        direction: 1,
        variation: 0,
    },
    CanonicalMission {
        scenario: 5,
        direction: 1,
        variation: 1,
    },
    CanonicalMission {
        scenario: 6,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 7,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 8,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 8,
        direction: 0,
        variation: 1,
    },
    CanonicalMission {
        scenario: 9,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 10,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 10,
        direction: 0,
        variation: 1,
    },
    CanonicalMission {
        scenario: 11,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 12,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 12,
        direction: 0,
        variation: 1,
    },
    CanonicalMission {
        scenario: 13,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 13,
        direction: 0,
        variation: 1,
    },
    CanonicalMission {
        scenario: 14,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 15,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 15,
        direction: 0,
        variation: 1,
    },
    CanonicalMission {
        scenario: 15,
        direction: 0,
        variation: 2,
    },
];

const NOD_CAMPAIGN_MISSIONS: &[CanonicalMission] = &[
    CanonicalMission {
        scenario: 1,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 2,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 2,
        direction: 0,
        variation: 1,
    },
    CanonicalMission {
        scenario: 3,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 3,
        direction: 0,
        variation: 1,
    },
    CanonicalMission {
        scenario: 4,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 4,
        direction: 0,
        variation: 1,
    },
    CanonicalMission {
        scenario: 5,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 6,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 6,
        direction: 0,
        variation: 1,
    },
    CanonicalMission {
        scenario: 6,
        direction: 0,
        variation: 2,
    },
    CanonicalMission {
        scenario: 7,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 7,
        direction: 0,
        variation: 1,
    },
    CanonicalMission {
        scenario: 7,
        direction: 0,
        variation: 2,
    },
    CanonicalMission {
        scenario: 8,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 8,
        direction: 0,
        variation: 1,
    },
    CanonicalMission {
        scenario: 9,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 10,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 10,
        direction: 0,
        variation: 1,
    },
    CanonicalMission {
        scenario: 11,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 11,
        direction: 0,
        variation: 1,
    },
    CanonicalMission {
        scenario: 12,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 13,
        direction: 0,
        variation: 0,
    },
    CanonicalMission {
        scenario: 13,
        direction: 0,
        variation: 1,
    },
    CanonicalMission {
        scenario: 13,
        direction: 0,
        variation: 2,
    },
];

fn canonical_missions(profile: ConversionProfile) -> &'static [CanonicalMission] {
    match profile {
        ConversionProfile::TdGdi01EastA => &GDI_CAMPAIGN_MISSIONS[..1],
        ConversionProfile::TdGdiCampaign => GDI_CAMPAIGN_MISSIONS,
        ConversionProfile::TdNodCampaign => NOD_CAMPAIGN_MISSIONS,
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct RuntimeCatalogV1 {
    pub format: String,
    pub version: u32,
    pub engine: String,
    pub engine_root: String,
    pub missions: Vec<RuntimeMissionV1>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct RuntimeMissionV1 {
    pub id: String,
    pub scenario_root: String,
    pub scenario: u32,
    pub variation: u32,
    pub direction: u32,
    pub build_level: u32,
    pub sabotaged_structure: i32,
    pub faction: String,
    pub title: String,
    pub briefing: String,
    pub theater: MissionTheater,
}

impl RuntimeCatalogV1 {
    pub fn td_gdi_01(briefing: impl Into<String>) -> Self {
        Self {
            format: "cncweb-runtime".into(),
            version: 1,
            engine: "tiberian-dawn".into(),
            engine_root: ENGINE_ROOT.into(),
            missions: vec![RuntimeMissionV1 {
                id: MISSION_ID.into(),
                scenario_root: SCENARIO_ROOT.into(),
                scenario: 1,
                variation: 0,
                direction: 0,
                build_level: 1,
                sabotaged_structure: -1,
                faction: "gdi".into(),
                title: "GDI Mission 1".into(),
                briefing: briefing.into(),
                theater: MissionTheater::Temperate,
            }],
        }
    }

    fn with_missions(missions: Vec<RuntimeMissionV1>) -> Self {
        Self {
            format: "cncweb-runtime".into(),
            version: 1,
            engine: "tiberian-dawn".into(),
            engine_root: ENGINE_ROOT.into(),
            missions,
        }
    }

    pub fn validate(&self) -> Result<()> {
        if self.format != "cncweb-runtime"
            || self.version != 1
            || self.engine != "tiberian-dawn"
            || self.engine_root != ENGINE_ROOT
        {
            return Err(Error::Conversion(
                "runtime catalog has an unsupported identity or engine root".into(),
            ));
        }
        if self.missions.is_empty() || self.missions.len() > MAX_CATALOG_MISSIONS {
            return Err(Error::Conversion(format!(
                "runtime catalog must contain 1 to {MAX_CATALOG_MISSIONS} ordered missions"
            )));
        }

        let mut ids = BTreeSet::new();
        let mut roots = BTreeSet::new();
        for mission in &self.missions {
            if !ids.insert(mission.id.as_str()) {
                return Err(Error::Conversion(format!(
                    "runtime catalog contains duplicate mission id `{}`",
                    mission.id
                )));
            }
            if !roots.insert(mission.scenario_root.as_str()) {
                return Err(Error::Conversion(format!(
                    "runtime catalog contains duplicate scenario root `{}`",
                    mission.scenario_root
                )));
            }
            validate_runtime_mission(mission)?;
        }
        let serialized_bytes = serde_json::to_vec(self)?.len().saturating_add(1);
        if serialized_bytes > MAX_RUNTIME_CATALOG_BYTES {
            return Err(Error::Conversion(format!(
                "runtime catalog serializes to {serialized_bytes} bytes; browser limit is {MAX_RUNTIME_CATALOG_BYTES}"
            )));
        }
        Ok(())
    }
}

fn validate_runtime_mission(mission: &RuntimeMissionV1) -> Result<()> {
    if mission.id.is_empty()
        || mission.id.len() > 128
        || !mission.id.bytes().enumerate().all(|(index, byte)| {
            byte.is_ascii_lowercase()
                || byte.is_ascii_digit()
                || (index > 0 && matches!(byte, b'.' | b'_' | b'-'))
        })
        || !(1..=999).contains(&mission.scenario)
        || !matches!(mission.variation, 0..=3 | 5)
        || mission.direction > 1
        || mission.build_level > 255
        || !(-1..=255).contains(&mission.sabotaged_structure)
        || !matches!(mission.faction.as_str(), "gdi" | "nod")
    {
        return Err(Error::Conversion(format!(
            "runtime mission `{}` contains an out-of-range launch field",
            mission.id
        )));
    }
    let expected_root = scenario_root(
        mission.faction.as_str(),
        mission.scenario,
        mission.direction,
        mission.variation,
    )?;
    if mission.scenario_root != expected_root {
        return Err(Error::Conversion(format!(
            "runtime mission `{}` scenarioRoot `{}` does not match launch identity `{expected_root}`",
            mission.id, mission.scenario_root
        )));
    }
    if mission.title.trim().is_empty() || mission.title.len() > 128 {
        return Err(Error::Conversion(format!(
            "runtime mission `{}` title must contain 1 to 128 bytes",
            mission.id
        )));
    }
    if mission.briefing.trim().is_empty()
        || mission.briefing.len() > MAX_BRIEFING_BYTES
        || mission.briefing.contains(['\r', '@', '\0'])
    {
        return Err(Error::Conversion(format!(
            "runtime briefing must contain 1 to {MAX_BRIEFING_BYTES} normalized UTF-8 bytes"
        )));
    }
    Ok(())
}

fn scenario_root(faction: &str, scenario: u32, direction: u32, variation: u32) -> Result<String> {
    let faction = match faction {
        "gdi" => 'G',
        "nod" => 'B',
        _ => return Err(Error::Conversion("unsupported campaign faction".into())),
    };
    let direction = match direction {
        0 => 'E',
        1 => 'W',
        _ => return Err(Error::Conversion("unsupported scenario direction".into())),
    };
    let variation = match variation {
        0..=3 => char::from(b'A' + variation as u8),
        5 => 'L',
        _ => return Err(Error::Conversion("unsupported scenario variation".into())),
    };
    Ok(format!("SC{faction}{scenario:02}{direction}{variation}"))
}

fn mission_id(faction: &str, scenario: u32, direction: u32, variation: u32) -> Result<String> {
    let direction = match direction {
        0 => "east",
        1 => "west",
        _ => return Err(Error::Conversion("unsupported scenario direction".into())),
    };
    let variation = match variation {
        0..=3 => char::from(b'a' + variation as u8).to_string(),
        5 => "lose".into(),
        _ => return Err(Error::Conversion("unsupported scenario variation".into())),
    };
    Ok(format!("{faction}-{scenario:02}-{direction}-{variation}"))
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct ConvertedAsset {
    pub logical_name: String,
    pub destination: String,
    pub required: bool,
    pub byte_size: u64,
    pub sha256: Sha256Digest,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub mix_entries: Option<u16>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct OmittedAssetClass {
    pub id: String,
    pub reason: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ConvertedMissionSourceV1 {
    pub id: String,
    pub scenario_root: String,
    pub theater: MissionTheater,
    pub ini_source: String,
    pub bin_source: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct ConversionMetadataV1 {
    pub format: String,
    pub version: u32,
    pub profile: ConversionProfile,
    pub source_fingerprint_sha256: Sha256Digest,
    pub catalog: String,
    pub assets: Vec<ConvertedAsset>,
    pub missions: Vec<ConvertedMissionSourceV1>,
    pub audio_index: String,
    pub decoded_audio_assets: usize,
    pub omitted: Vec<OmittedAssetClass>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct ConversionReport {
    pub profile: ConversionProfile,
    pub package_path: String,
    pub package_id: String,
    pub source_fingerprint_sha256: Sha256Digest,
    pub content_sha256: Sha256Digest,
    pub files: usize,
    pub uncompressed_bytes: u64,
    pub mission_id: String,
    pub missions: usize,
    pub engine_root: String,
    pub decoded_audio_assets: usize,
    pub decoded_audio_wav_bytes: u64,
    pub missing_audio_candidates: usize,
    pub audio_decode_failures: usize,
}

struct LocatedSource {
    disc: PathBuf,
    files: BTreeMap<String, PathBuf>,
    required_files: BTreeSet<String>,
    inspection: SourceInspection,
}

pub fn inspect_conversion_source(
    root: impl AsRef<Path>,
    profile: ConversionProfile,
    mix_limits: MixLimits,
) -> Result<SourceInspection> {
    inspect_conversion_source_for_product(
        root,
        profile,
        SourceProduct::CncRemasteredCollection,
        mix_limits,
    )
}

pub fn inspect_conversion_source_for_product(
    root: impl AsRef<Path>,
    profile: ConversionProfile,
    source_product: SourceProduct,
    mix_limits: MixLimits,
) -> Result<SourceInspection> {
    match locate_and_validate(root.as_ref(), profile, source_product, mix_limits) {
        Ok(source) => Ok(source.inspection),
        Err(Error::Conversion(message)) if message.starts_with("DISC_NOT_FOUND:") => {
            Ok(SourceInspection {
                profile,
                valid: false,
                cd_directory: None,
                assets: Vec::new(),
                optional_missing: Vec::new(),
                issues: vec![SourceIssue {
                    code: SourceIssueCode::CdDirectoryMissing,
                    logical_name: format!("Data/CNCDATA/TIBERIAN_DAWN/{}", profile.disc()),
                    message: message
                        .split_once(':')
                        .map_or(message.as_str(), |(_, detail)| detail)
                        .trim()
                        .into(),
                }],
                missions: Vec::new(),
            })
        }
        Err(error) => Err(error),
    }
}

/// Creates and verifies a browser-v1 package. `progress` is called at stable
/// phase boundaries and after each source asset, making it suitable for a CLI
/// today and a cancellable desktop wrapper later.
pub fn convert_owned_content<F>(
    install_root: impl AsRef<Path>,
    output_path: impl AsRef<Path>,
    options: ConversionOptions,
    mut progress: F,
) -> Result<ConversionReport>
where
    F: FnMut(&ConversionProgress),
{
    if !(0..=9).contains(&options.compression_level) {
        return Err(Error::Conversion(
            "compression level must be between 0 and 9".into(),
        ));
    }
    if options.locales.len() != 1 || !options.locales[0].eq_ignore_ascii_case("en-US") {
        return Err(Error::Conversion(
            "Tiberian Dawn campaign conversion currently supports exactly the en-US classic content locale".into(),
        ));
    }
    let output_path = output_path.as_ref();
    if output_path.exists() {
        return Err(Error::OutputExists(output_path.to_path_buf()));
    }
    emit(
        &mut progress,
        ConversionPhase::Locate,
        0,
        1,
        options.profile.id(),
        format!(
            "locating {} Tiberian Dawn {} data",
            match options.source_product {
                SourceProduct::CncRemasteredCollection => "Remastered Collection",
                SourceProduct::TiberianDawnFreeware => "classic freeware",
            },
            options.profile.disc(),
        ),
    );
    let source = locate_and_validate(
        install_root.as_ref(),
        options.profile,
        options.source_product,
        options.mix_limits,
    )?;
    if !source.inspection.valid {
        let summary = source
            .inspection
            .issues
            .iter()
            .map(|issue| format!("{}: {}", issue.logical_name, issue.message))
            .collect::<Vec<_>>()
            .join("; ");
        return Err(Error::Conversion(format!(
            "source is not valid for {}: {summary}",
            options.profile.id()
        )));
    }
    emit(
        &mut progress,
        ConversionPhase::Validate,
        1,
        1,
        source.disc.to_string_lossy(),
        format!(
            "required MIX archives and {} mission descriptors are structurally valid",
            source.inspection.missions.len()
        ),
    );

    let staging = create_staging_directory(output_path)?;
    let conversion_result = (|| -> Result<ConversionReport> {
        let engine_root = staging.join("engine/td");
        fs::create_dir_all(&engine_root)?;

        let total_assets = source.files.len();
        let mut converted = Vec::with_capacity(total_assets);
        for (index, (logical_name, source_path)) in source.files.iter().enumerate() {
            let destination_logical = format!("{ENGINE_ROOT}/{logical_name}");
            let destination = engine_root.join(logical_name);
            let mut input = File::open(source_path)?;
            let mut output = OpenOptions::new()
                .write(true)
                .create_new(true)
                .open(&destination)?;
            let (sha256, byte_size) = copy_and_hash(&mut input, &mut output)?;
            output.sync_all()?;
            let mix_entries = if logical_name.ends_with(".MIX") {
                Some(
                    MixArchive::open_with_limits(&destination, options.mix_limits)?
                        .header()
                        .file_count,
                )
            } else {
                None
            };
            converted.push(ConvertedAsset {
                logical_name: logical_name.clone(),
                destination: destination_logical,
                required: source.required_files.contains(logical_name),
                byte_size,
                sha256,
                mix_entries,
            });
            emit(
                &mut progress,
                ConversionPhase::Copy,
                index + 1,
                total_assets,
                logical_name,
                "copied and hashed owned source asset",
            );
        }
        converted.sort_by(|left, right| left.logical_name.cmp(&right.logical_name));
        let source_fingerprint = source_fingerprint(options.profile, &converted);

        let descriptors = canonical_missions(options.profile);
        if descriptors.len() != source.inspection.missions.len() {
            return Err(Error::Conversion(
                "mission source resolution changed after validation".into(),
            ));
        }
        let mission_total = descriptors.len() * 2;
        let mut runtime_missions = Vec::with_capacity(descriptors.len());
        let mut converted_missions = Vec::with_capacity(descriptors.len());
        for (index, (descriptor, source_mission)) in descriptors
            .iter()
            .copied()
            .zip(&source.inspection.missions)
            .enumerate()
        {
            let root = descriptor.root(options.profile);
            let ini_name = format!("{root}.INI");
            let bin_name = format!("{root}.BIN");
            extract_resolved_entry(
                &engine_root,
                &source_mission.ini_source,
                &ini_name,
                &engine_root.join(&ini_name),
                options.mix_limits,
            )?;
            emit(
                &mut progress,
                ConversionPhase::ExtractMission,
                index * 2 + 1,
                mission_total,
                &ini_name,
                "extracted resolved scenario configuration",
            );
            extract_resolved_entry(
                &engine_root,
                &source_mission.bin_source,
                &bin_name,
                &engine_root.join(&bin_name),
                options.mix_limits,
            )?;
            let mission_ini =
                validate_extracted_mission(&engine_root, &root, options.source_product)?;
            if mission_ini.theater != source_mission.theater {
                return Err(Error::Conversion(format!(
                    "{ini_name} theater changed after source validation"
                )));
            }
            runtime_missions.push(descriptor.runtime(
                options.profile,
                mission_ini.briefing,
                mission_ini.theater,
            ));
            converted_missions.push(ConvertedMissionSourceV1 {
                id: source_mission.id.clone(),
                scenario_root: root,
                theater: source_mission.theater,
                ini_source: source_mission.ini_source.clone(),
                bin_source: source_mission.bin_source.clone(),
            });
            emit(
                &mut progress,
                ConversionPhase::ExtractMission,
                index * 2 + 2,
                mission_total,
                &bin_name,
                "extracted and validated the 64x64 scenario map",
            );
        }

        let audio = build_audio_index(
            &engine_root,
            &staging,
            AudioBuildOptions {
                aud_limits: options.aud_limits,
                mix_limits: options.mix_limits,
                allow_missing_outcome_speech: options.source_product
                    == SourceProduct::TiberianDawnFreeware,
            },
            |current, total, item| {
                emit(
                    &mut progress,
                    ConversionPhase::ConvertAudio,
                    current,
                    total,
                    item,
                    "processed browser-native audio candidate",
                );
            },
        )?;

        let catalog = RuntimeCatalogV1::with_missions(runtime_missions);
        catalog.validate()?;
        write_runtime_catalog(&staging.join(RUNTIME_CATALOG_PATH), &catalog)?;
        let mut omitted = vec![
            OmittedAssetClass {
                id: "mission-movies".into(),
                reason: "MOVIES.MIX is intentionally omitted: browser gameplay uses the remaster movie-callback boundary and browser-v1 currently limits individual files to 64 MiB".into(),
            },
            OmittedAssetClass {
                id: "enhanced-textures".into(),
                reason: "CONFIG/TEXTURES MEG conversion is a separate enhanced-rendering pipeline; this profile contains the complete classic-renderer mission assets".into(),
            },
        ];
        omitted.push(OmittedAssetClass {
            id: "music".into(),
            reason: "SCORES.MIX is excluded from browser packages; C&C music is not part of this modification".into(),
        });
        let metadata = ConversionMetadataV1 {
            format: "cncweb-conversion-report".into(),
            version: 1,
            profile: options.profile,
            source_fingerprint_sha256: source_fingerprint,
            catalog: RUNTIME_CATALOG_PATH.into(),
            assets: converted,
            missions: converted_missions,
            audio_index: AUDIO_INDEX_PATH.into(),
            decoded_audio_assets: audio.index.assets.len(),
            omitted,
        };
        write_json(&staging.join(CONVERSION_REPORT_PATH), &metadata)?;
        emit(
            &mut progress,
            ConversionPhase::WriteMetadata,
            2,
            2,
            RUNTIME_CATALOG_PATH,
            "wrote runtime catalog and conversion provenance",
        );

        emit(
            &mut progress,
            ConversionPhase::Package,
            0,
            1,
            output_path.to_string_lossy(),
            "creating deterministic browser-v1 package",
        );
        let manifest = create_package(
            &staging,
            output_path,
            CreateOptions {
                package_id: options.package_id.clone(),
                created_at_unix_ms: options.created_at_unix_ms,
                source: SourceV1 {
                    product: options.source_product,
                    provider: options.provider,
                    install_fingerprint_sha256: source_fingerprint,
                },
                content: ContentDescriptorV1 {
                    games: vec![GameId::TiberianDawn],
                    locales: options.locales.clone(),
                },
                compression_level: options.compression_level,
                limits: options.limits,
            },
        )?;
        emit(
            &mut progress,
            ConversionPhase::Verify,
            0,
            1,
            output_path.to_string_lossy(),
            "stream-verifying every packaged content hash",
        );
        let verified = verify_package(output_path, options.limits)?;
        if manifest != verified {
            return Err(Error::Conversion(
                "package manifest changed between creation and verification".into(),
            ));
        }
        let uncompressed_bytes = manifest.files.iter().try_fold(0_u64, |total, file| {
            total
                .checked_add(file.size)
                .ok_or_else(|| Error::Conversion("package byte count overflow".into()))
        })?;
        emit(
            &mut progress,
            ConversionPhase::Complete,
            1,
            1,
            options.profile.id(),
            "mission package is ready",
        );
        let first_mission = catalog
            .missions
            .first()
            .ok_or_else(|| Error::Conversion("generated catalog is empty".into()))?;
        Ok(ConversionReport {
            profile: options.profile,
            package_path: output_path.to_string_lossy().into_owned(),
            package_id: manifest.package_id,
            source_fingerprint_sha256: source_fingerprint,
            content_sha256: manifest.content_sha256,
            files: manifest.files.len(),
            uncompressed_bytes,
            mission_id: first_mission.id.clone(),
            missions: catalog.missions.len(),
            engine_root: ENGINE_ROOT.into(),
            decoded_audio_assets: audio.index.assets.len(),
            decoded_audio_wav_bytes: audio.wav_bytes,
            missing_audio_candidates: audio.index.diagnostics.missing_candidates,
            audio_decode_failures: audio.index.diagnostics.decode_failures.len(),
        })
    })();
    let cleanup_result = fs::remove_dir_all(&staging);
    match (conversion_result, cleanup_result) {
        (Ok(report), Ok(())) => Ok(report),
        (Ok(_), Err(error)) => {
            let _ = fs::remove_file(output_path);
            Err(Error::Io(error))
        }
        (Err(error), _) => {
            let _ = fs::remove_file(output_path);
            Err(error)
        }
    }
}

fn locate_and_validate(
    root: &Path,
    profile: ConversionProfile,
    source_product: SourceProduct,
    mix_limits: MixLimits,
) -> Result<LocatedSource> {
    let disc = locate_disc(root, profile.disc())?.ok_or_else(|| {
        Error::Conversion(
            format!(
                "DISC_NOT_FOUND: {}: no unambiguous directory containing CCLOCAL.MIX, CONQUER.MIX, and GENERAL.MIX was found at the selected path or its known Data/CNCDATA/TIBERIAN_DAWN/{} descendants",
                profile.disc(),
                profile.disc()
            ),
        )
    })?;

    let mut files = BTreeMap::new();
    let mut archives = BTreeMap::new();
    let mut required_files = BTreeSet::new();
    let mut assets = Vec::new();
    let mut optional_missing = Vec::new();
    let mut issues = Vec::new();

    for &logical_name in REQUIRED_COMMON_MIXES {
        if source_product == SourceProduct::TiberianDawnFreeware
            && matches!(logical_name, "LOCAL.MIX" | "UPDATA.MIX" | "UPDATE.MIX")
        {
            continue;
        }
        required_files.insert(logical_name.into());
        probe_mix(
            &disc,
            logical_name,
            true,
            mix_limits,
            &mut files,
            &mut archives,
            &mut assets,
            &mut issues,
        )?;
    }

    let mut missions = Vec::with_capacity(canonical_missions(profile).len());
    let mut theaters = BTreeSet::new();
    for descriptor in canonical_missions(profile).iter().copied() {
        let scenario_root = descriptor.root(profile);
        let ini_name = format!("{scenario_root}.INI");
        let bin_name = format!("{scenario_root}.BIN");
        let ini_source = resolve_mission_source(&archives, &ini_name)?;
        let bin_source = resolve_mission_source(&archives, &bin_name)?;
        if ini_source.is_none() {
            issues.push(SourceIssue {
                code: SourceIssueCode::MissionEntryMissing,
                logical_name: ini_name.clone(),
                message: "scenario INI was not found in patch-first MIX resolution order".into(),
            });
        }
        if bin_source.is_none() {
            issues.push(SourceIssue {
                code: SourceIssueCode::MissionEntryMissing,
                logical_name: bin_name.clone(),
                message: "scenario map was not found in patch-first MIX resolution order".into(),
            });
        }
        let (Some(ini_source), Some(bin_source)) = (ini_source, bin_source) else {
            continue;
        };

        let mission_ini = match archives
            .get_mut(&ini_source)
            .ok_or_else(|| Error::Conversion("resolved mission INI archive disappeared".into()))?
            .read_entry(&ini_name, MAX_SCENARIO_INI_BYTES)
        {
            Ok(bytes) => match validate_mission_ini_for_source(&bytes, &ini_name, source_product) {
                Ok(metadata) => Some(metadata),
                Err(error) => {
                    issues.push(SourceIssue {
                        code: SourceIssueCode::InvalidMissionIni,
                        logical_name: ini_name.clone(),
                        message: error.to_string(),
                    });
                    None
                }
            },
            Err(error) => {
                issues.push(SourceIssue {
                    code: SourceIssueCode::InvalidMissionIni,
                    logical_name: ini_name.clone(),
                    message: error.to_string(),
                });
                None
            }
        };
        let map_size = archives
            .get(&bin_source)
            .ok_or_else(|| Error::Conversion("resolved mission BIN archive disappeared".into()))?
            .entry(&bin_name)?
            .map(|entry| u64::from(entry.size));
        let valid_map = map_size == Some(EXPECTED_MAP_BYTES);
        if !valid_map {
            issues.push(SourceIssue {
                code: SourceIssueCode::InvalidMissionMap,
                logical_name: bin_name,
                message: format!(
                    "expected an {EXPECTED_MAP_BYTES}-byte classic map, found {}",
                    map_size.map_or_else(|| "no entry".into(), |size| format!("{size} bytes"))
                ),
            });
        }
        if let Some(mission_ini) = mission_ini {
            theaters.insert(mission_ini.theater);
            if valid_map {
                missions.push(MissionSourceProbe {
                    id: descriptor.id(profile),
                    scenario_root,
                    theater: mission_ini.theater,
                    ini_source,
                    bin_source,
                });
            }
        }
    }

    for theater in theaters {
        for logical_name in theater.archive_pair() {
            required_files.insert(logical_name.into());
            let use_shared_icons = source_product == SourceProduct::TiberianDawnFreeware
                && matches!(logical_name, "DESEICNH.MIX" | "WINTICNH.MIX")
                && find_case_insensitive(&disc, logical_name)?.is_none();
            if use_shared_icons {
                probe_mix_as(
                    &disc,
                    "TEMPICNH.MIX",
                    logical_name,
                    true,
                    mix_limits,
                    &mut files,
                    &mut archives,
                    &mut assets,
                    &mut issues,
                )?;
            } else {
                probe_mix(
                    &disc,
                    logical_name,
                    true,
                    mix_limits,
                    &mut files,
                    &mut archives,
                    &mut assets,
                    &mut issues,
                )?;
            }
        }
        probe_optional_file(
            &disc,
            theater.optional_palette(),
            &mut files,
            &mut assets,
            &mut optional_missing,
            &mut issues,
        )?;
    }
    assets.sort_by(|left, right| left.logical_name.cmp(&right.logical_name));
    optional_missing.sort();

    let inspection = SourceInspection {
        profile,
        valid: issues.is_empty()
            && required_files.iter().all(|name| files.contains_key(name))
            && missions.len() == canonical_missions(profile).len(),
        cd_directory: Some(disc.to_string_lossy().into_owned()),
        assets,
        optional_missing,
        issues,
        missions,
    };
    Ok(LocatedSource {
        disc,
        files,
        required_files,
        inspection,
    })
}

#[allow(clippy::too_many_arguments)]
fn probe_mix(
    directory: &Path,
    logical_name: &str,
    required: bool,
    mix_limits: MixLimits,
    files: &mut BTreeMap<String, PathBuf>,
    archives: &mut BTreeMap<String, MixArchive<File>>,
    assets: &mut Vec<SourceAssetProbe>,
    issues: &mut Vec<SourceIssue>,
) -> Result<()> {
    probe_mix_as(
        directory,
        logical_name,
        logical_name,
        required,
        mix_limits,
        files,
        archives,
        assets,
        issues,
    )
}

#[allow(clippy::too_many_arguments)]
fn probe_mix_as(
    directory: &Path,
    source_name: &str,
    logical_name: &str,
    required: bool,
    mix_limits: MixLimits,
    files: &mut BTreeMap<String, PathBuf>,
    archives: &mut BTreeMap<String, MixArchive<File>>,
    assets: &mut Vec<SourceAssetProbe>,
    issues: &mut Vec<SourceIssue>,
) -> Result<()> {
    let source_path = match find_case_insensitive(directory, source_name) {
        Ok(Some(path)) => path,
        Ok(None) => {
            if required {
                issues.push(SourceIssue {
                    code: SourceIssueCode::RequiredFileMissing,
                    logical_name: logical_name.into(),
                    message: "required legacy engine archive is missing".into(),
                });
            }
            return Ok(());
        }
        Err(error) => {
            issues.push(SourceIssue {
                code: SourceIssueCode::AmbiguousName,
                logical_name: logical_name.into(),
                message: error.to_string(),
            });
            return Ok(());
        }
    };
    let metadata = fs::metadata(&source_path)?;
    if !metadata.is_file() {
        issues.push(SourceIssue {
            code: SourceIssueCode::NotAFile,
            logical_name: logical_name.into(),
            message: "expected a regular file".into(),
        });
        return Ok(());
    }
    match MixArchive::open_with_limits(&source_path, mix_limits) {
        Ok(archive) => {
            assets.push(SourceAssetProbe {
                logical_name: logical_name.into(),
                required,
                byte_size: metadata.len(),
                mix_entries: Some(archive.header().file_count),
            });
            files.insert(logical_name.into(), source_path);
            archives.insert(logical_name.into(), archive);
        }
        Err(error) => issues.push(SourceIssue {
            code: SourceIssueCode::InvalidMix,
            logical_name: logical_name.into(),
            message: error.to_string(),
        }),
    }
    Ok(())
}

fn probe_optional_file(
    directory: &Path,
    logical_name: &str,
    files: &mut BTreeMap<String, PathBuf>,
    assets: &mut Vec<SourceAssetProbe>,
    optional_missing: &mut Vec<String>,
    issues: &mut Vec<SourceIssue>,
) -> Result<()> {
    let source_path = match find_case_insensitive(directory, logical_name) {
        Ok(Some(path)) => path,
        Ok(None) => {
            optional_missing.push(logical_name.into());
            return Ok(());
        }
        Err(error) => {
            issues.push(SourceIssue {
                code: SourceIssueCode::AmbiguousName,
                logical_name: logical_name.into(),
                message: error.to_string(),
            });
            return Ok(());
        }
    };
    let metadata = fs::metadata(&source_path)?;
    if !metadata.is_file() {
        optional_missing.push(logical_name.into());
        return Ok(());
    }
    if metadata.len() != 768 {
        issues.push(SourceIssue {
            code: SourceIssueCode::InvalidPalette,
            logical_name: logical_name.into(),
            message: format!(
                "optional loose theater palette is {} bytes; expected 768",
                metadata.len()
            ),
        });
        return Ok(());
    }
    assets.push(SourceAssetProbe {
        logical_name: logical_name.into(),
        required: false,
        byte_size: metadata.len(),
        mix_entries: None,
    });
    files.insert(logical_name.into(), source_path);
    Ok(())
}

fn resolve_mission_source(
    archives: &BTreeMap<String, MixArchive<File>>,
    logical_name: &str,
) -> Result<Option<String>> {
    for &layer in MISSION_RESOLUTION_ORDER {
        if archives
            .get(layer)
            .is_some_and(|archive| archive.entry(logical_name).ok().flatten().is_some())
        {
            return Ok(Some(layer.into()));
        }
    }
    Ok(None)
}

fn extract_resolved_entry(
    engine_root: &Path,
    source_archive: &str,
    logical_name: &str,
    destination: &Path,
    mix_limits: MixLimits,
) -> Result<()> {
    let path = engine_root.join(source_archive);
    let mut archive = MixArchive::open_with_limits(&path, mix_limits).map_err(|error| {
        Error::Conversion(format!(
            "copied resolved source archive `{source_archive}` is invalid or unavailable: {error}"
        ))
    })?;
    let mut output = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(destination)?;
    archive.copy_entry(logical_name, &mut output)?;
    output.sync_all()?;
    Ok(())
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct MissionIniMetadata {
    theater: MissionTheater,
    briefing: String,
}

fn validate_extracted_mission(
    engine_root: &Path,
    scenario_root: &str,
    source_product: SourceProduct,
) -> Result<MissionIniMetadata> {
    let ini_name = format!("{scenario_root}.INI");
    let bin_name = format!("{scenario_root}.BIN");
    let mut ini = Vec::new();
    File::open(engine_root.join(&ini_name))?
        .take(MAX_SCENARIO_INI_BYTES.saturating_add(1))
        .read_to_end(&mut ini)?;
    let metadata = validate_mission_ini_for_source(&ini, &ini_name, source_product)?;
    let map_size = fs::metadata(engine_root.join(&bin_name))?.len();
    if map_size != EXPECTED_MAP_BYTES {
        return Err(Error::Conversion(format!(
            "{bin_name} is {map_size} bytes; expected {EXPECTED_MAP_BYTES}"
        )));
    }
    Ok(metadata)
}

fn validate_mission_ini(bytes: &[u8], logical_name: &str) -> Result<MissionIniMetadata> {
    if bytes.len() as u64 > MAX_SCENARIO_INI_BYTES {
        return Err(Error::Conversion(format!(
            "{logical_name} exceeds {MAX_SCENARIO_INI_BYTES} bytes"
        )));
    }
    let text = std::str::from_utf8(bytes)
        .map_err(|_| Error::Conversion(format!("{logical_name} is not valid UTF-8/ASCII")))?;
    validate_mission_ini_text(text, logical_name, extract_briefing(bytes, logical_name)?)
}

fn validate_mission_ini_for_source(
    bytes: &[u8],
    logical_name: &str,
    source_product: SourceProduct,
) -> Result<MissionIniMetadata> {
    if source_product == SourceProduct::CncRemasteredCollection {
        return validate_mission_ini(bytes, logical_name);
    }
    if bytes.len() as u64 > MAX_SCENARIO_INI_BYTES {
        return Err(Error::Conversion(format!(
            "{logical_name} exceeds {MAX_SCENARIO_INI_BYTES} bytes"
        )));
    }
    validate_mission_ini_text(
        &String::from_utf8_lossy(bytes),
        logical_name,
        FREEWARE_BRIEFING.into(),
    )
}

fn validate_mission_ini_text(
    text: &str,
    logical_name: &str,
    briefing: String,
) -> Result<MissionIniMetadata> {
    let text = text.strip_prefix('\u{feff}').unwrap_or(text);
    let mut section = String::new();
    let mut has_basic = false;
    let mut theater = None;
    let mut theater_declarations = 0_u32;
    for raw_line in text.lines() {
        let line = strip_ini_comment(raw_line);
        if line.is_empty() {
            continue;
        }
        if line.starts_with('[') {
            if line.ends_with(']') {
                section = line[1..line.len() - 1].trim().to_ascii_lowercase();
                has_basic |= section == "basic";
            } else {
                section.clear();
            }
            continue;
        }
        if section == "map" {
            if let Some((key, value)) = line.split_once('=') {
                if key.trim().eq_ignore_ascii_case("Theater") {
                    theater_declarations += 1;
                    theater = Some(value.trim().to_ascii_lowercase());
                }
            }
        }
    }
    if !has_basic {
        return Err(Error::Conversion(format!(
            "{logical_name} does not contain a [Basic] section"
        )));
    }
    if theater_declarations != 1 {
        return Err(Error::Conversion(format!(
            "{logical_name} must declare [Map] Theater exactly once; found {theater_declarations}"
        )));
    }
    let theater_name = theater.expect("one theater declaration was counted");
    let theater = MissionTheater::from_ini(&theater_name).ok_or_else(|| {
        Error::Conversion(format!(
            "{logical_name} uses unsupported theater `{theater_name}`; expected TEMPERATE, DESERT, or WINTER"
        ))
    })?;
    Ok(MissionIniMetadata { theater, briefing })
}

fn strip_ini_comment(raw_line: &str) -> &str {
    let comment = raw_line
        .char_indices()
        .find_map(|(index, character)| matches!(character, ';' | '#').then_some(index));
    raw_line[..comment.unwrap_or(raw_line.len())].trim()
}

/// Reconstructs the engine text block without publishing any built-in retail
/// text. Wrapped INI entries are joined with spaces; `@` is treated as the
/// explicit paragraph separator used by mission data.
fn extract_briefing(bytes: &[u8], logical_name: &str) -> Result<String> {
    let text = std::str::from_utf8(bytes)
        .map_err(|_| Error::Conversion(format!("{logical_name} is not valid UTF-8/ASCII")))?;
    let text = text.strip_prefix('\u{feff}').unwrap_or(text);
    let mut section = String::new();
    let mut wrapped_values = Vec::new();
    for raw_line in text.lines() {
        let line = strip_ini_comment(raw_line);
        if line.is_empty() {
            continue;
        }
        if line.starts_with('[') {
            if line.ends_with(']') {
                section = line[1..line.len() - 1].trim().to_ascii_lowercase();
            } else {
                section.clear();
            }
            continue;
        }
        if section == "briefing" {
            if let Some((_, value)) = line.split_once('=') {
                wrapped_values.push(value.trim());
            }
        }
    }
    let raw = wrapped_values
        .join(" ")
        .replace("\r\n", "\n")
        .replace('\r', "\n");
    let paragraphs: Vec<String> = raw
        .split(['@', '\n'])
        .map(|paragraph| paragraph.split_whitespace().collect::<Vec<_>>().join(" "))
        .filter(|paragraph| !paragraph.is_empty())
        .collect();
    let briefing = paragraphs.join("\n");
    if briefing.is_empty() {
        return Err(Error::Conversion(format!(
            "{logical_name} does not contain non-empty [Briefing] text"
        )));
    }
    if briefing.len() > MAX_BRIEFING_BYTES {
        return Err(Error::Conversion(format!(
            "normalized [Briefing] text is {} bytes; limit is {MAX_BRIEFING_BYTES}",
            briefing.len()
        )));
    }
    Ok(briefing)
}

fn source_fingerprint(profile: ConversionProfile, assets: &[ConvertedAsset]) -> Sha256Digest {
    let mut hasher = Sha256::new();
    hasher.update(b"CNCWEB-TD-CAMPAIGN-SOURCE-V1\0");
    hasher.update(profile.id().as_bytes());
    hasher.update([0]);
    for asset in assets {
        hasher.update((asset.logical_name.len() as u64).to_le_bytes());
        hasher.update(asset.logical_name.as_bytes());
        hasher.update(asset.byte_size.to_le_bytes());
        hasher.update(asset.sha256.as_bytes());
    }
    digest_from_hasher(hasher)
}

fn write_json(path: &Path, value: &impl Serialize) -> Result<()> {
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }
    let mut bytes = serde_json::to_vec_pretty(value)?;
    bytes.push(b'\n');
    let mut output = OpenOptions::new().write(true).create_new(true).open(path)?;
    output.write_all(&bytes)?;
    output.sync_all()?;
    Ok(())
}

fn write_runtime_catalog(path: &Path, catalog: &RuntimeCatalogV1) -> Result<()> {
    let mut bytes = serde_json::to_vec(catalog)?;
    bytes.push(b'\n');
    if bytes.len() > MAX_RUNTIME_CATALOG_BYTES {
        return Err(Error::Conversion(format!(
            "runtime catalog serializes to {} bytes; browser limit is {MAX_RUNTIME_CATALOG_BYTES}",
            bytes.len()
        )));
    }
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }
    let mut output = OpenOptions::new().write(true).create_new(true).open(path)?;
    output.write_all(&bytes)?;
    output.sync_all()?;
    Ok(())
}

fn locate_disc(root: &Path, disc: &str) -> Result<Option<PathBuf>> {
    let metadata = fs::metadata(root).map_err(|error| {
        Error::Conversion(format!(
            "cannot read selected installation directory: {error}"
        ))
    })?;
    if !metadata.is_dir() {
        return Err(Error::Conversion(
            "selected installation path is not a directory".into(),
        ));
    }
    let selected_name = root.file_name().and_then(|name| name.to_str());
    let selected_is_other_disc = selected_name.is_some_and(|name| {
        matches!(name.to_ascii_uppercase().as_str(), "CD1" | "CD2")
            && !name.eq_ignore_ascii_case(disc)
    });
    let mut candidates = Vec::new();
    if !selected_is_other_disc {
        candidates.push(Vec::new());
    }
    candidates.extend([
        vec![disc],
        vec!["TIBERIAN_DAWN", disc],
        vec!["CNCDATA", "TIBERIAN_DAWN", disc],
        vec!["DATA", "CNCDATA", "TIBERIAN_DAWN", disc],
    ]);
    let mut matches = Vec::new();
    for components in &candidates {
        let Some(candidate) = resolve_components(root, components)? else {
            continue;
        };
        let mut has_markers = candidate.is_dir();
        if has_markers {
            for name in ["CCLOCAL.MIX", "CONQUER.MIX", "GENERAL.MIX"] {
                if find_case_insensitive(&candidate, name)?.is_none() {
                    has_markers = false;
                    break;
                }
            }
        }
        if has_markers && !matches.contains(&candidate) {
            matches.push(candidate);
        }
    }
    match matches.len() {
        0 => Ok(None),
        1 => Ok(matches.pop()),
        _ => Err(Error::Conversion(format!(
            "more than one candidate TD {disc} directory was found below `{}`",
            root.display()
        ))),
    }
}

fn resolve_components(root: &Path, components: &[&str]) -> Result<Option<PathBuf>> {
    let mut current = root.to_path_buf();
    for component in components {
        let Some(next) = find_case_insensitive(&current, component)? else {
            return Ok(None);
        };
        current = next;
    }
    Ok(Some(current))
}

fn find_case_insensitive(directory: &Path, name: &str) -> Result<Option<PathBuf>> {
    let mut found = None;
    for entry in fs::read_dir(directory)? {
        let entry = entry?;
        let Some(entry_name) = entry.file_name().to_str().map(str::to_owned) else {
            continue;
        };
        if entry_name.eq_ignore_ascii_case(name) {
            if found.is_some() {
                return Err(Error::Conversion(format!(
                    "more than one filesystem entry matches logical name `{name}` in `{}`",
                    directory.display()
                )));
            }
            found = Some(entry.path());
        }
    }
    Ok(found)
}

fn create_staging_directory(destination: &Path) -> Result<PathBuf> {
    let parent = destination.parent().unwrap_or_else(|| Path::new("."));
    fs::create_dir_all(parent)?;
    let stem = destination
        .file_name()
        .and_then(|name| name.to_str())
        .unwrap_or("content");
    for attempt in 0..100_u32 {
        let path = parent.join(format!(
            ".{stem}.converting-{}-{attempt}",
            std::process::id()
        ));
        match fs::create_dir(&path) {
            Ok(()) => return Ok(path),
            Err(error) if error.kind() == std::io::ErrorKind::AlreadyExists => continue,
            Err(error) => return Err(Error::Io(error)),
        }
    }
    Err(Error::Io(std::io::Error::new(
        std::io::ErrorKind::AlreadyExists,
        "could not allocate a unique conversion staging directory",
    )))
}

fn emit<F: FnMut(&ConversionProgress)>(
    progress: &mut F,
    phase: ConversionPhase,
    current: usize,
    total: usize,
    item: impl Into<String>,
    message: impl Into<String>,
) {
    progress(&ConversionProgress {
        phase,
        current,
        total,
        item: item.into(),
        message: message.into(),
    });
}

#[cfg(test)]
mod tests {
    use std::io::Cursor;

    use tempfile::tempdir;

    use super::*;
    use crate::mix::mix_name_hash;
    use crate::package::{extract_package, inspect_package};

    fn synthetic_mix(entries: &[(&str, &[u8])]) -> Vec<u8> {
        let mut indexed: Vec<_> = entries
            .iter()
            .map(|(name, bytes)| (mix_name_hash(name).unwrap(), *bytes))
            .collect();
        indexed.sort_by_key(|(hash, _)| *hash as i32);
        let data_size: usize = indexed.iter().map(|(_, bytes)| bytes.len()).sum();
        let mut output = Vec::new();
        output.extend((indexed.len() as u16).to_le_bytes());
        output.extend((data_size as u32).to_le_bytes());
        let mut offset = 0_u32;
        for (hash, bytes) in &indexed {
            output.extend(hash.to_le_bytes());
            output.extend(offset.to_le_bytes());
            output.extend((bytes.len() as u32).to_le_bytes());
            offset += bytes.len() as u32;
        }
        for (_, bytes) in indexed {
            output.extend(bytes);
        }
        output
    }

    fn synthetic_mix_owned(entries: &[(&str, Vec<u8>)]) -> Vec<u8> {
        let borrowed: Vec<_> = entries
            .iter()
            .map(|(name, bytes)| (*name, bytes.as_slice()))
            .collect();
        synthetic_mix(&borrowed)
    }

    fn synthetic_aud() -> Vec<u8> {
        let mut stored = Vec::new();
        stored.extend(1_u16.to_le_bytes());
        stored.extend(4_u16.to_le_bytes());
        stored.extend(0x0000_deaf_u32.to_le_bytes());
        stored.push(0xc3);
        let mut bytes = Vec::new();
        bytes.extend(22_050_u16.to_le_bytes());
        bytes.extend((stored.len() as i32).to_le_bytes());
        bytes.extend(4_i32.to_le_bytes());
        bytes.push(0);
        bytes.push(1);
        bytes.extend(stored);
        bytes
    }

    fn synthetic_install(root: &Path, patch_mission: bool) {
        let cd1 = root.join("Data/CNCData/Tiberian_Dawn/CD1");
        fs::create_dir_all(&cd1).unwrap();
        let ini = b"[Basic]\nName=Synthetic\n[Map]\nTheater=TEMPERATE\n[Briefing]\n1=Synthetic objective@Proceed safely.\n";
        let map = vec![0x2a; EXPECTED_MAP_BYTES as usize];
        for &name in REQUIRED_COMMON_MIXES
            .iter()
            .chain(["TEMPERAT.MIX", "TEMPICNH.MIX"].iter())
        {
            if name == "SOUNDS.MIX" {
                let entries = ["MGUN2.AUD", "BUTTON.AUD", "XPLOS.AUD", "ACKNO.V01"]
                    .map(|name| (name, synthetic_aud()));
                fs::write(cd1.join(name), synthetic_mix_owned(&entries)).unwrap();
                continue;
            }
            if name == "SPEECH.MIX" {
                let entries =
                    ["ACCOM1.AUD", "FAIL1.AUD", "REINFOR1.AUD"].map(|name| (name, synthetic_aud()));
                fs::write(cd1.join(name), synthetic_mix_owned(&entries)).unwrap();
                continue;
            }
            let entries: Vec<(&str, &[u8])> = if name == "GENERAL.MIX" {
                vec![("SCG01EA.INI", ini), ("SCG01EA.BIN", map.as_slice())]
            } else if name == "UPDATE.MIX" && patch_mission {
                vec![("SCG01EA.INI", ini)]
            } else {
                vec![("PROBE.BIN", b"synthetic")]
            };
            fs::write(cd1.join(name), synthetic_mix(&entries)).unwrap();
        }
        fs::write(cd1.join("TEMPERAT.PAL"), [0_u8; 768]).unwrap();
    }

    fn synthetic_mix_dynamic(entries: &[(String, Vec<u8>)]) -> Vec<u8> {
        let borrowed: Vec<_> = entries
            .iter()
            .map(|(name, bytes)| (name.as_str(), bytes.as_slice()))
            .collect();
        synthetic_mix(&borrowed)
    }

    fn theater_ini_name(theater: MissionTheater) -> &'static str {
        match theater {
            MissionTheater::Temperate => "TEMPERATE",
            MissionTheater::Desert => "DESERT",
            MissionTheater::Winter => "WINTER",
        }
    }

    fn synthetic_ini(root: &str, theater: MissionTheater, briefing: &str) -> Vec<u8> {
        format!(
            "[Basic]\nName={root}\n[Map]\nTheater={}\n[Briefing]\n1={briefing}\n",
            theater_ini_name(theater)
        )
        .into_bytes()
    }

    struct CampaignFixture {
        theaters: Vec<MissionTheater>,
        patch_ini: Option<(String, MissionTheater)>,
        omitted_entry: Option<String>,
        short_bin: Option<String>,
    }

    impl Default for CampaignFixture {
        fn default() -> Self {
            Self {
                theaters: vec![MissionTheater::Temperate],
                patch_ini: None,
                omitted_entry: None,
                short_bin: None,
            }
        }
    }

    fn synthetic_campaign_install(
        root: &Path,
        profile: ConversionProfile,
        fixture: &CampaignFixture,
    ) -> PathBuf {
        assert!(!fixture.theaters.is_empty());
        let disc = root.join("Data/CNCData/Tiberian_Dawn").join(profile.disc());
        fs::create_dir_all(&disc).unwrap();
        let mut general_entries = Vec::new();
        let mut patch_entries = Vec::new();
        let mut encountered_theaters = BTreeSet::new();
        for (index, descriptor) in canonical_missions(profile).iter().copied().enumerate() {
            let scenario_root = descriptor.root(profile);
            let ini_name = format!("{scenario_root}.INI");
            let bin_name = format!("{scenario_root}.BIN");
            let base_theater = fixture.theaters[index % fixture.theaters.len()];
            let final_theater = fixture
                .patch_ini
                .as_ref()
                .filter(|(root, _)| root == &scenario_root)
                .map_or(base_theater, |(_, theater)| *theater);
            if fixture.omitted_entry.as_deref() != Some(ini_name.as_str()) {
                general_entries.push((
                    ini_name.clone(),
                    synthetic_ini(
                        &scenario_root,
                        base_theater,
                        &format!("Synthetic briefing for {scenario_root}."),
                    ),
                ));
                if final_theater != base_theater {
                    patch_entries.push((
                        ini_name,
                        synthetic_ini(
                            &scenario_root,
                            final_theater,
                            &format!("Patched synthetic briefing for {scenario_root}@Patch wins."),
                        ),
                    ));
                }
                encountered_theaters.insert(final_theater);
            }
            if fixture.omitted_entry.as_deref() != Some(bin_name.as_str()) {
                let map_size = if fixture.short_bin.as_deref() == Some(scenario_root.as_str()) {
                    EXPECTED_MAP_BYTES as usize - 1
                } else {
                    EXPECTED_MAP_BYTES as usize
                };
                general_entries.push((bin_name, vec![index as u8; map_size]));
            }
        }

        for &name in REQUIRED_COMMON_MIXES {
            let bytes = if name == "GENERAL.MIX" {
                synthetic_mix_dynamic(&general_entries)
            } else if name == "UPDATE.MIX" && !patch_entries.is_empty() {
                synthetic_mix_dynamic(&patch_entries)
            } else if name == "SOUNDS.MIX" {
                let entries = ["MGUN2.AUD", "BUTTON.AUD", "XPLOS.AUD", "ACKNO.V01"]
                    .map(|name| (name, synthetic_aud()));
                synthetic_mix_owned(&entries)
            } else if name == "SPEECH.MIX" {
                let entries =
                    ["ACCOM1.AUD", "FAIL1.AUD", "REINFOR1.AUD"].map(|name| (name, synthetic_aud()));
                synthetic_mix_owned(&entries)
            } else {
                synthetic_mix(&[("PROBE.BIN", b"synthetic")])
            };
            fs::write(disc.join(name), bytes).unwrap();
        }
        for theater in encountered_theaters {
            for name in theater.archive_pair() {
                fs::write(
                    disc.join(name),
                    synthetic_mix(&[("PROBE.BIN", b"synthetic theater")]),
                )
                .unwrap();
            }
            fs::write(disc.join(theater.optional_palette()), [0_u8; 768]).unwrap();
        }
        disc
    }

    #[test]
    fn locates_case_insensitive_cd1_and_resolves_patch_order() {
        let temp = tempdir().unwrap();
        synthetic_install(temp.path(), true);
        let report = inspect_conversion_source(
            temp.path(),
            ConversionProfile::TdGdi01EastA,
            MixLimits::default(),
        )
        .unwrap();
        assert!(report.valid, "{:?}", report.issues);
        assert_eq!(report.assets.len(), REQUIRED_COMMON_MIXES.len() + 3);
        assert_eq!(report.missions[0].ini_source, "UPDATE.MIX");
        assert_eq!(report.missions[0].bin_source, "GENERAL.MIX");
    }

    #[test]
    fn emits_a_verified_mountable_browser_package() {
        let temp = tempdir().unwrap();
        synthetic_install(temp.path(), false);
        let package = temp.path().join("gdi-01.cncweb");
        let mut phases = Vec::new();
        let report = convert_owned_content(
            temp.path(),
            &package,
            ConversionOptions::td_gdi_01(
                "synthetic-gdi-01",
                1_700_000_000_000,
                SourceProvider::CopiedInstallation,
                vec!["en-US".into()],
            ),
            |progress| phases.push(progress.phase),
        )
        .unwrap();
        assert_eq!(report.mission_id, MISSION_ID);
        assert_eq!(report.missions, 1);
        assert_eq!(report.decoded_audio_assets, 7);
        assert!(report.decoded_audio_wav_bytes > 0);
        assert!(phases.contains(&ConversionPhase::Verify));
        assert!(phases.contains(&ConversionPhase::ConvertAudio));
        assert_eq!(phases.last(), Some(&ConversionPhase::Complete));

        let manifest = inspect_package(&package, PackageLimits::browser_v1()).unwrap();
        assert!(manifest
            .files
            .iter()
            .any(|file| file.path == RUNTIME_CATALOG_PATH));
        assert!(manifest
            .files
            .iter()
            .any(|file| file.path == "engine/td/SCG01EA.BIN"));
        assert!(manifest
            .files
            .iter()
            .any(|file| file.path == AUDIO_INDEX_PATH));
        assert!(manifest
            .files
            .iter()
            .any(|file| file.path == "audio/sfx/mgun2.wav"));
        assert!(!manifest
            .files
            .iter()
            .any(|file| file.path == "engine/td/SCORES.MIX"));

        let extracted = temp.path().join("extracted");
        extract_package(&package, &extracted, PackageLimits::browser_v1()).unwrap();
        let catalog: RuntimeCatalogV1 =
            serde_json::from_slice(&fs::read(extracted.join(RUNTIME_CATALOG_PATH)).unwrap())
                .unwrap();
        catalog.validate().unwrap();
        assert_eq!(catalog.missions.len(), 1);
        assert_eq!(catalog.missions[0].scenario_root, SCENARIO_ROOT);
        assert_eq!(catalog.missions[0].theater, MissionTheater::Temperate);
        let audio_index: crate::audio::AudioIndexV1 =
            serde_json::from_slice(&fs::read(extracted.join(AUDIO_INDEX_PATH)).unwrap()).unwrap();
        audio_index.validate().unwrap();
        assert_eq!(audio_index.assets.len(), 7);
        assert_eq!(
            catalog.missions[0].briefing,
            "Synthetic objective\nProceed safely."
        );
        assert_eq!(
            fs::metadata(extracted.join("engine/td/SCG01EA.BIN"))
                .unwrap()
                .len(),
            EXPECTED_MAP_BYTES
        );
    }

    #[test]
    fn freeware_package_records_provenance_and_excludes_music_archive() {
        let temp = tempdir().unwrap();
        synthetic_install(temp.path(), false);
        let inspection = inspect_conversion_source_for_product(
            temp.path(),
            ConversionProfile::TdGdi01EastA,
            SourceProduct::TiberianDawnFreeware,
            MixLimits::default(),
        )
        .unwrap();
        assert!(inspection.valid, "{:?}", inspection.issues);
        assert!(!inspection
            .assets
            .iter()
            .any(|asset| asset.logical_name == "SCORES.MIX"));

        let package = temp.path().join("classic-freeware.cncweb");
        let mut options = ConversionOptions::td_gdi_01(
            "classic-freeware-gdi",
            1_700_000_000_000,
            SourceProvider::EaFreeware,
            vec!["en-US".into()],
        );
        options.source_product = SourceProduct::TiberianDawnFreeware;
        convert_owned_content(temp.path(), &package, options, |_| {}).unwrap();

        let manifest = inspect_package(&package, PackageLimits::browser_v1()).unwrap();
        assert_eq!(manifest.source.product, SourceProduct::TiberianDawnFreeware);
        assert_eq!(manifest.source.provider, SourceProvider::EaFreeware);
        assert!(!manifest
            .files
            .iter()
            .any(|file| file.path == "engine/td/SCORES.MIX"));
    }

    #[test]
    fn reports_missing_and_invalid_mission_sources_without_writing() {
        let temp = tempdir().unwrap();
        let missing = inspect_conversion_source(
            temp.path(),
            ConversionProfile::TdGdi01EastA,
            MixLimits::default(),
        )
        .unwrap();
        assert!(!missing.valid);
        assert_eq!(missing.issues[0].code, SourceIssueCode::CdDirectoryMissing);

        let bytes = synthetic_mix(&[("SCG01EA.INI", b"[Basic]\n")]);
        assert!(MixArchive::parse(Cursor::new(bytes), MixLimits::default()).is_ok());
    }

    #[test]
    fn normalizes_and_caps_locally_derived_briefing_text() {
        let briefing = extract_briefing(
            b"[Briefing]\n1= First   line @ Second line\r\n2= wraps here \n",
            "SYNTHETIC.INI",
        )
        .unwrap();
        assert_eq!(briefing, "First line\nSecond line wraps here");

        let oversized = format!("[Briefing]\n1={}\n", "x".repeat(MAX_BRIEFING_BYTES + 1));
        assert!(extract_briefing(oversized.as_bytes(), "SYNTHETIC.INI").is_err());
    }

    #[test]
    fn mission_ini_matches_native_bom_comment_and_theater_rules() {
        let metadata = validate_mission_ini(
            b"\xef\xbb\xbf# generated fixture\n[Basic] ; section comment\nName=Synthetic # inline comment\n[Map] # section comment\nTheater=DESERT ; inline comment\n[Briefing]\n1=Synthetic objective. # not briefing text\n",
            "SYNTHETIC.INI",
        )
        .unwrap();
        assert_eq!(metadata.theater, MissionTheater::Desert);
        assert_eq!(metadata.briefing, "Synthetic objective.");

        let duplicate =
            b"[Basic]\n[Map]\nTheater=TEMPERATE\nTheater=WINTER\n[Briefing]\n1=Synthetic.\n";
        let error = validate_mission_ini(duplicate, "DUPLICATE.INI").unwrap_err();
        assert!(error.to_string().contains("exactly once"));

        let malformed_boundary =
            b"[Basic]\n[Map]\n[Broken\nTheater=TEMPERATE\n[Briefing]\n1=Synthetic.\n";
        let error = validate_mission_ini(malformed_boundary, "MALFORMED.INI").unwrap_err();
        assert!(error.to_string().contains("exactly once"));
    }

    #[test]
    fn canonical_profiles_preserve_legacy_mapsel_order_and_counts() {
        assert_eq!(canonical_missions(ConversionProfile::TdGdi01EastA).len(), 1);
        assert_eq!(
            canonical_missions(ConversionProfile::TdGdiCampaign).len(),
            25
        );
        assert_eq!(
            canonical_missions(ConversionProfile::TdNodCampaign).len(),
            25
        );
        let gdi_roots: Vec<_> = canonical_missions(ConversionProfile::TdGdiCampaign)
            .iter()
            .copied()
            .map(|mission| mission.root(ConversionProfile::TdGdiCampaign))
            .collect();
        assert_eq!(
            &gdi_roots[3..9],
            ["SCG04WA", "SCG04WB", "SCG04EA", "SCG05EA", "SCG05WA", "SCG05WB"]
        );
        assert_eq!(gdi_roots.last().map(String::as_str), Some("SCG15EC"));
        assert_eq!(
            canonical_missions(ConversionProfile::TdNodCampaign)
                .last()
                .copied()
                .unwrap()
                .root(ConversionProfile::TdNodCampaign),
            "SCB13EC"
        );
        assert_eq!(
            gdi_roots.iter().collect::<BTreeSet<_>>().len(),
            gdi_roots.len()
        );
    }

    #[test]
    fn selects_cd1_for_gdi_and_cd2_for_nod_with_profile_theaters() {
        let temp = tempdir().unwrap();
        let gdi_disc = synthetic_campaign_install(
            temp.path(),
            ConversionProfile::TdGdiCampaign,
            &CampaignFixture::default(),
        );
        synthetic_campaign_install(
            temp.path(),
            ConversionProfile::TdNodCampaign,
            &CampaignFixture {
                theaters: vec![MissionTheater::Desert],
                ..CampaignFixture::default()
            },
        );

        let gdi = inspect_conversion_source(
            temp.path(),
            ConversionProfile::TdGdiCampaign,
            MixLimits::default(),
        )
        .unwrap();
        let nod = inspect_conversion_source(
            temp.path(),
            ConversionProfile::TdNodCampaign,
            MixLimits::default(),
        )
        .unwrap();
        assert!(gdi.valid, "{:?}", gdi.issues);
        assert!(nod.valid, "{:?}", nod.issues);
        assert!(gdi.cd_directory.as_deref().unwrap().ends_with("/CD1"));
        assert!(nod.cd_directory.as_deref().unwrap().ends_with("/CD2"));
        assert_eq!(gdi.missions.len(), 25);
        assert_eq!(nod.missions.len(), 25);
        assert_eq!(nod.missions[0].scenario_root, "SCB01EA");
        assert_eq!(nod.missions[0].theater, MissionTheater::Desert);
        assert!(gdi
            .assets
            .iter()
            .any(|asset| asset.logical_name == "TEMPERAT.MIX"));
        assert!(!gdi
            .assets
            .iter()
            .any(|asset| asset.logical_name == "DESERT.MIX"));
        assert!(nod
            .assets
            .iter()
            .any(|asset| asset.logical_name == "DESERT.MIX"));
        assert!(!nod
            .assets
            .iter()
            .any(|asset| asset.logical_name == "TEMPERAT.MIX"));
        let wrong_disc = inspect_conversion_source(
            &gdi_disc,
            ConversionProfile::TdNodCampaign,
            MixLimits::default(),
        )
        .unwrap();
        assert!(!wrong_disc.valid);
        assert_eq!(
            wrong_disc.issues[0].code,
            SourceIssueCode::CdDirectoryMissing
        );
    }

    #[test]
    fn converts_every_campaign_mission_loose_with_patch_first_and_all_theaters() {
        let temp = tempdir().unwrap();
        synthetic_campaign_install(
            temp.path(),
            ConversionProfile::TdGdiCampaign,
            &CampaignFixture {
                theaters: vec![
                    MissionTheater::Temperate,
                    MissionTheater::Desert,
                    MissionTheater::Winter,
                ],
                patch_ini: Some(("SCG01EA".into(), MissionTheater::Winter)),
                ..CampaignFixture::default()
            },
        );
        let package = temp.path().join("gdi-campaign.cncweb");
        let report = convert_owned_content(
            temp.path(),
            &package,
            ConversionOptions::td_gdi_campaign(
                "synthetic-gdi-campaign",
                1_700_000_000_000,
                SourceProvider::CopiedInstallation,
                vec!["en-US".into()],
            ),
            |_| {},
        )
        .unwrap();
        assert_eq!(report.missions, 25);
        assert_eq!(report.mission_id, MISSION_ID);

        let extracted = temp.path().join("campaign-extracted");
        extract_package(&package, &extracted, PackageLimits::browser_v1()).unwrap();
        let catalog: RuntimeCatalogV1 =
            serde_json::from_slice(&fs::read(extracted.join(RUNTIME_CATALOG_PATH)).unwrap())
                .unwrap();
        catalog.validate().unwrap();
        assert_eq!(catalog.missions.len(), 25);
        assert_eq!(catalog.missions[0].scenario_root, "SCG01EA");
        assert_eq!(catalog.missions[0].theater, MissionTheater::Winter);
        assert_eq!(
            catalog.missions[0].briefing,
            "Patched synthetic briefing for SCG01EA\nPatch wins."
        );
        for mission in &catalog.missions {
            assert!(extracted
                .join(ENGINE_ROOT)
                .join(format!("{}.INI", mission.scenario_root))
                .is_file());
            assert_eq!(
                fs::metadata(
                    extracted
                        .join(ENGINE_ROOT)
                        .join(format!("{}.BIN", mission.scenario_root))
                )
                .unwrap()
                .len(),
                EXPECTED_MAP_BYTES
            );
        }
        for required in [
            "TEMPERAT.MIX",
            "TEMPICNH.MIX",
            "DESERT.MIX",
            "DESEICNH.MIX",
            "WINTER.MIX",
            "WINTICNH.MIX",
        ] {
            assert!(extracted.join(ENGINE_ROOT).join(required).is_file());
        }
        let metadata: ConversionMetadataV1 =
            serde_json::from_slice(&fs::read(extracted.join(CONVERSION_REPORT_PATH)).unwrap())
                .unwrap();
        assert_eq!(metadata.missions[0].ini_source, "UPDATE.MIX");
        assert_eq!(metadata.missions[0].bin_source, "GENERAL.MIX");
    }

    #[test]
    fn reports_missing_mission_theater_archive_and_invalid_bin_size() {
        let missing_mission = tempdir().unwrap();
        synthetic_campaign_install(
            missing_mission.path(),
            ConversionProfile::TdGdiCampaign,
            &CampaignFixture {
                omitted_entry: Some("SCG04WA.INI".into()),
                ..CampaignFixture::default()
            },
        );
        let report = inspect_conversion_source(
            missing_mission.path(),
            ConversionProfile::TdGdiCampaign,
            MixLimits::default(),
        )
        .unwrap();
        assert!(!report.valid);
        assert!(report.issues.iter().any(|issue| {
            issue.code == SourceIssueCode::MissionEntryMissing
                && issue.logical_name == "SCG04WA.INI"
        }));

        let missing_theater = tempdir().unwrap();
        let disc = synthetic_campaign_install(
            missing_theater.path(),
            ConversionProfile::TdNodCampaign,
            &CampaignFixture {
                theaters: vec![MissionTheater::Desert],
                ..CampaignFixture::default()
            },
        );
        fs::remove_file(disc.join("DESEICNH.MIX")).unwrap();
        let report = inspect_conversion_source(
            missing_theater.path(),
            ConversionProfile::TdNodCampaign,
            MixLimits::default(),
        )
        .unwrap();
        assert!(report.issues.iter().any(|issue| {
            issue.code == SourceIssueCode::RequiredFileMissing
                && issue.logical_name == "DESEICNH.MIX"
        }));

        let short_bin = tempdir().unwrap();
        synthetic_campaign_install(
            short_bin.path(),
            ConversionProfile::TdGdi01EastA,
            &CampaignFixture {
                short_bin: Some("SCG01EA".into()),
                ..CampaignFixture::default()
            },
        );
        let report = inspect_conversion_source(
            short_bin.path(),
            ConversionProfile::TdGdi01EastA,
            MixLimits::default(),
        )
        .unwrap();
        assert!(report.issues.iter().any(|issue| {
            issue.code == SourceIssueCode::InvalidMissionMap && issue.logical_name == "SCG01EA.BIN"
        }));

        let invalid_palette = tempdir().unwrap();
        let disc = synthetic_campaign_install(
            invalid_palette.path(),
            ConversionProfile::TdGdi01EastA,
            &CampaignFixture::default(),
        );
        fs::write(disc.join("TEMPERAT.PAL"), [0_u8; 767]).unwrap();
        let report = inspect_conversion_source(
            invalid_palette.path(),
            ConversionProfile::TdGdi01EastA,
            MixLimits::default(),
        )
        .unwrap();
        assert!(report.issues.iter().any(|issue| {
            issue.code == SourceIssueCode::InvalidPalette && issue.logical_name == "TEMPERAT.PAL"
        }));
    }

    #[test]
    fn catalog_accepts_one_to_256_unique_ordered_missions() {
        let missions: Vec<_> = (1..=256)
            .map(|scenario| RuntimeMissionV1 {
                id: format!("m{scenario}"),
                scenario_root: scenario_root("gdi", scenario, 0, 0).unwrap(),
                scenario,
                variation: 0,
                direction: 0,
                build_level: scenario.min(255),
                sabotaged_structure: -1,
                faction: "gdi".into(),
                title: "x".into(),
                briefing: "x".into(),
                theater: MissionTheater::Temperate,
            })
            .collect();
        RuntimeCatalogV1::with_missions(missions.clone())
            .validate()
            .unwrap();
        assert!(RuntimeCatalogV1::with_missions(Vec::new())
            .validate()
            .is_err());
        let mut overflow = missions.clone();
        overflow.push(RuntimeMissionV1 {
            id: "overflow".into(),
            scenario_root: "SCG257EA".into(),
            scenario: 257,
            ..missions[0].clone()
        });
        assert!(RuntimeCatalogV1::with_missions(overflow)
            .validate()
            .is_err());
        let mut duplicate = missions[..2].to_vec();
        duplicate[1].id = duplicate[0].id.clone();
        assert!(RuntimeCatalogV1::with_missions(duplicate)
            .validate()
            .is_err());

        let mut oversized = missions[..25].to_vec();
        for mission in &mut oversized {
            mission.briefing = "x".repeat(MAX_BRIEFING_BYTES);
        }
        let error = RuntimeCatalogV1::with_missions(oversized)
            .validate()
            .unwrap_err();
        assert!(error.to_string().contains("browser limit"));
    }
}
