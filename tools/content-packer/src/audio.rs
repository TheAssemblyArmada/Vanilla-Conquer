//! Browser-native audio extraction for engine sound and speech callbacks.

use std::collections::{BTreeMap, BTreeSet};
use std::fs::{self, OpenOptions};
use std::io::{Cursor, Write};
use std::path::Path;

use serde::{Deserialize, Serialize};

use crate::aud::{decode_aud, AudLimits};
use crate::error::{Error, Result};
use crate::hash::{hash_reader, Sha256Digest};
use crate::mix::{MixArchive, MixLimits};
use crate::path::normalize_package_path;

pub const AUDIO_INDEX_PATH: &str = "runtime/audio-v1.json";
const AUDIO_LAYERS: &[&str] = &["SPEECH.MIX", "SOUNDS.MIX"];

fn source_archive(kind: AudioEventKind) -> &'static str {
    match kind {
        AudioEventKind::Sound => "SOUNDS.MIX",
        AudioEventKind::Speech => "SPEECH.MIX",
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum AudioEventKind {
    Sound,
    Speech,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct AudioAssetV1 {
    pub kind: AudioEventKind,
    pub event_name: String,
    pub event_ids: Vec<u32>,
    pub path: String,
    pub source_archive: String,
    pub source_name: String,
    pub source_compression: u8,
    pub sample_rate: u32,
    pub channels: u16,
    pub bits_per_sample: u16,
    pub frames: u64,
    pub sha256: Sha256Digest,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct AudioDecodeFailureV1 {
    pub kind: AudioEventKind,
    pub event_name: String,
    pub source_archive: String,
    pub source_name: String,
    pub reason: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct AudioDiagnosticsV1 {
    pub candidate_count: usize,
    pub missing_candidates: usize,
    pub decode_failures: Vec<AudioDecodeFailureV1>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct AudioIndexV1 {
    pub format: String,
    pub version: u32,
    pub encoding: String,
    pub assets: Vec<AudioAssetV1>,
    pub diagnostics: AudioDiagnosticsV1,
}

impl AudioIndexV1 {
    pub fn validate(&self) -> Result<()> {
        if self.format != "cncweb-audio" || self.version != 1 || self.encoding != "wav-pcm" {
            return Err(Error::Conversion(
                "unsupported runtime audio index identity".into(),
            ));
        }
        if self.assets.is_empty() {
            return Err(Error::Conversion(
                "runtime audio index contains no decoded assets".into(),
            ));
        }
        if self.assets.len() > 1_000
            || self.diagnostics.candidate_count == 0
            || self.diagnostics.candidate_count > 1_000
            || self.diagnostics.missing_candidates > 1_000
            || self.diagnostics.decode_failures.len() > 1_000
        {
            return Err(Error::Conversion(
                "runtime audio index exceeds v1 collection limits".into(),
            ));
        }
        if self.diagnostics.candidate_count
            != self.assets.len()
                + self.diagnostics.missing_candidates
                + self.diagnostics.decode_failures.len()
        {
            return Err(Error::Conversion(
                "audio diagnostics do not account for every candidate".into(),
            ));
        }

        let mut previous: Option<(AudioEventKind, &str)> = None;
        let mut paths = BTreeSet::new();
        for asset in &self.assets {
            let key = (asset.kind, asset.event_name.as_str());
            if previous.is_some_and(|value| value >= key) {
                return Err(Error::Conversion(
                    "audio assets must be strictly sorted by kind and eventName".into(),
                ));
            }
            previous = Some(key);
            validate_event_name(&asset.event_name)?;
            if asset.event_ids.is_empty()
                || asset.event_ids.len() > 4
                || asset.event_ids.iter().any(|id| *id > 65_535)
                || asset.event_ids.windows(2).any(|pair| pair[0] >= pair[1])
            {
                return Err(Error::Conversion(
                    "audio eventIds must be non-empty, sorted, and unique".into(),
                ));
            }
            let normalized = normalize_package_path(&asset.path)?;
            let expected_prefix = match asset.kind {
                AudioEventKind::Sound => "audio/sfx/",
                AudioEventKind::Speech => "audio/speech/",
            };
            if !normalized.starts_with(expected_prefix)
                || !normalized.ends_with(".wav")
                || !paths.insert(normalized)
                || asset.path != audio_path(asset.kind, &asset.event_name)?
            {
                return Err(Error::Conversion(
                    "audio asset path is misplaced, duplicated, or not WAV".into(),
                ));
            }
            if asset.source_archive != source_archive(asset.kind)
                || !valid_source_name(&asset.source_name)
                || asset.source_name != expected_source_name(asset.kind, &asset.event_name)
            {
                return Err(Error::Conversion(
                    "audio asset has an unsupported source identity".into(),
                ));
            }
            if !matches!(asset.source_compression, 0 | 1 | 99)
                || !(4_000..=192_000).contains(&asset.sample_rate)
                || !matches!(asset.channels, 1 | 2)
                || !matches!(asset.bits_per_sample, 8 | 16)
                || asset.frames == 0
            {
                return Err(Error::Conversion(
                    "audio asset PCM metadata is invalid".into(),
                ));
            }
        }

        let mut previous_failure: Option<(AudioEventKind, &str)> = None;
        for failure in &self.diagnostics.decode_failures {
            let key = (failure.kind, failure.event_name.as_str());
            if previous_failure.is_some_and(|value| value >= key) {
                return Err(Error::Conversion(
                    "audio decode failures must be strictly sorted".into(),
                ));
            }
            previous_failure = Some(key);
            validate_event_name(&failure.event_name)?;
            if failure.source_archive != source_archive(failure.kind)
                || !valid_source_name(&failure.source_name)
                || failure.source_name != expected_source_name(failure.kind, &failure.event_name)
                || failure.reason.is_empty()
                || failure.reason.len() > 1024
            {
                return Err(Error::Conversion(
                    "audio decode failure source or reason is invalid".into(),
                ));
            }
        }
        Ok(())
    }
}

#[derive(Debug, Clone, Copy, Default)]
pub struct AudioBuildOptions {
    pub aud_limits: AudLimits,
    pub mix_limits: MixLimits,
    /// Music-free freeware mirrors may omit terminal campaign speech. The
    /// browser still presents the authoritative win/loss dialog and score.
    pub allow_missing_outcome_speech: bool,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct AudioBuildReport {
    pub index: AudioIndexV1,
    pub wav_bytes: u64,
}

struct Candidate {
    kind: AudioEventKind,
    event_name: String,
    source_name: String,
    event_ids: Vec<u32>,
}

/// Resolves the released engine's sound tables against the copied legacy MIX
/// layers and emits only locally decoded WAV files plus `runtime/audio-v1.json`.
pub fn build_audio_index<F>(
    engine_root: impl AsRef<Path>,
    package_root: impl AsRef<Path>,
    options: AudioBuildOptions,
    mut progress: F,
) -> Result<AudioBuildReport>
where
    F: FnMut(usize, usize, &str),
{
    let engine_root = engine_root.as_ref();
    let package_root = package_root.as_ref();
    let mut archives = BTreeMap::new();
    for &name in AUDIO_LAYERS {
        archives.insert(
            name,
            MixArchive::open_with_limits(engine_root.join(name), options.mix_limits)?,
        );
    }

    let candidates = candidates();
    let total = candidates.len();
    let mut assets = Vec::new();
    let mut decode_failures = Vec::new();
    let mut missing_candidates = 0_usize;
    let mut wav_bytes = 0_u64;
    for (index, candidate) in candidates.into_values().enumerate() {
        /* Callback kind determines the legacy archive. Cross-archive name
         * collisions must not change a sound into speech (or vice versa), and
         * the browser independently enforces the same identity contract. */
        let layer = source_archive(candidate.kind);
        if archives
            .get(layer)
            .expect("known kind-specific layer")
            .entry(&candidate.source_name)?
            .is_none()
        {
            missing_candidates += 1;
            progress(index + 1, total, &candidate.event_name);
            continue;
        }
        let source = match archives
            .get_mut(layer)
            .expect("resolved layer remains present")
            .read_entry(&candidate.source_name, options.aud_limits.max_source_bytes)
        {
            Ok(source) => source,
            Err(Error::MixLimit(reason)) => {
                decode_failures.push(AudioDecodeFailureV1 {
                    kind: candidate.kind,
                    event_name: candidate.event_name,
                    source_archive: layer.into(),
                    source_name: candidate.source_name,
                    reason,
                });
                progress(
                    index + 1,
                    total,
                    &decode_failures.last().unwrap().event_name,
                );
                continue;
            }
            Err(error) => return Err(error),
        };
        let decoded = match decode_aud(&source, options.aud_limits) {
            Ok(decoded) => decoded,
            Err(Error::InvalidAud(reason)) | Err(Error::AudLimit(reason)) => {
                decode_failures.push(AudioDecodeFailureV1 {
                    kind: candidate.kind,
                    event_name: candidate.event_name,
                    source_archive: layer.into(),
                    source_name: candidate.source_name,
                    reason,
                });
                progress(
                    index + 1,
                    total,
                    &decode_failures.last().unwrap().event_name,
                );
                continue;
            }
            Err(error) => return Err(error),
        };
        let wav = decoded.to_wav()?;
        let logical_path = audio_path(candidate.kind, &candidate.event_name)?;
        let destination = package_root.join(&logical_path);
        if let Some(parent) = destination.parent() {
            fs::create_dir_all(parent)?;
        }
        let mut output = OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(&destination)?;
        output.write_all(&wav)?;
        output.sync_all()?;
        let (sha256, size) = hash_reader(&mut Cursor::new(&wav))?;
        wav_bytes = wav_bytes
            .checked_add(size)
            .ok_or_else(|| Error::AudLimit("total WAV byte count overflow".into()))?;
        assets.push(AudioAssetV1 {
            kind: candidate.kind,
            event_name: candidate.event_name,
            event_ids: candidate.event_ids,
            path: logical_path,
            source_archive: layer.into(),
            source_name: candidate.source_name,
            source_compression: decoded.source_compression,
            sample_rate: decoded.sample_rate,
            channels: decoded.channels,
            bits_per_sample: decoded.bits_per_sample,
            frames: decoded.frames,
            sha256,
        });
        progress(index + 1, total, &assets.last().unwrap().event_name);
    }
    assets.sort_by(|left, right| {
        (left.kind, left.event_name.as_str()).cmp(&(right.kind, right.event_name.as_str()))
    });
    decode_failures.sort_by(|left, right| {
        (left.kind, left.event_name.as_str()).cmp(&(right.kind, right.event_name.as_str()))
    });
    ensure_core_gdi1_audio(&assets, options.allow_missing_outcome_speech)?;
    let index = AudioIndexV1 {
        format: "cncweb-audio".into(),
        version: 1,
        encoding: "wav-pcm".into(),
        assets,
        diagnostics: AudioDiagnosticsV1 {
            candidate_count: total,
            missing_candidates,
            decode_failures,
        },
    };
    index.validate()?;
    write_json(&package_root.join(AUDIO_INDEX_PATH), &index)?;
    Ok(AudioBuildReport { index, wav_bytes })
}

fn validate_event_name(name: &str) -> Result<()> {
    if name.is_empty()
        || name.len() > 16
        || !name
            .bytes()
            .all(|byte| byte.is_ascii_uppercase() || byte.is_ascii_digit() || byte == b'.')
    {
        return Err(Error::Conversion(format!(
            "invalid engine audio event name `{name}`"
        )));
    }
    Ok(())
}

fn audio_path(kind: AudioEventKind, event_name: &str) -> Result<String> {
    validate_event_name(event_name)?;
    let directory = match kind {
        AudioEventKind::Sound => "audio/sfx",
        AudioEventKind::Speech => "audio/speech",
    };
    normalize_package_path(&format!(
        "{directory}/{}.wav",
        event_name.to_ascii_lowercase()
    ))
}

fn expected_source_name(kind: AudioEventKind, event_name: &str) -> String {
    if kind == AudioEventKind::Sound
        && [".V00", ".V01", ".V02", ".V03"]
            .iter()
            .any(|suffix| event_name.ends_with(suffix))
    {
        event_name.into()
    } else {
        format!("{event_name}.AUD")
    }
}

fn valid_source_name(name: &str) -> bool {
    name.len() <= 16
        && name
            .bytes()
            .all(|byte| byte.is_ascii_uppercase() || byte.is_ascii_digit() || byte == b'.')
        && (name.ends_with(".AUD")
            || [".V00", ".V01", ".V02", ".V03"]
                .iter()
                .any(|suffix| name.ends_with(suffix)))
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

fn candidates() -> BTreeMap<(AudioEventKind, String), Candidate> {
    let mut candidates = BTreeMap::new();
    for (event_id, definition) in SFX_TABLE.iter().enumerate() {
        if definition.has_variants {
            for extension in ["V00", "V01", "V02", "V03"] {
                let event_name = format!("{}.{}", definition.name, extension);
                add_candidate(
                    &mut candidates,
                    AudioEventKind::Sound,
                    &event_name,
                    &event_name,
                    event_id as u32,
                );
            }
        } else {
            add_candidate(
                &mut candidates,
                AudioEventKind::Sound,
                definition.name,
                &format!("{}.AUD", definition.name),
                event_id as u32,
            );
        }
    }
    for (event_id, name) in SPEECH_TABLE.iter().enumerate() {
        add_candidate(
            &mut candidates,
            AudioEventKind::Speech,
            name,
            &format!("{name}.AUD"),
            event_id as u32,
        );
    }
    for candidate in candidates.values_mut() {
        candidate.event_ids.sort_unstable();
        candidate.event_ids.dedup();
    }
    candidates
}

fn add_candidate(
    candidates: &mut BTreeMap<(AudioEventKind, String), Candidate>,
    kind: AudioEventKind,
    event_name: &str,
    source_name: &str,
    event_id: u32,
) {
    candidates
        .entry((kind, event_name.into()))
        .and_modify(|candidate| candidate.event_ids.push(event_id))
        .or_insert_with(|| Candidate {
            kind,
            event_name: event_name.into(),
            source_name: source_name.into(),
            event_ids: vec![event_id],
        });
}

fn ensure_core_gdi1_audio(
    assets: &[AudioAssetV1],
    allow_missing_outcome_speech: bool,
) -> Result<()> {
    let available: BTreeSet<(AudioEventKind, &str)> = assets
        .iter()
        .map(|asset| (asset.kind, asset.event_name.as_str()))
        .collect();
    let has_exact =
        |kind, names: &[&str]| names.iter().any(|name| available.contains(&(kind, *name)));
    let has_prefix = |kind, prefixes: &[&str]| {
        available.iter().any(|(asset_kind, name)| {
            *asset_kind == kind
                && prefixes.iter().any(|prefix| {
                    *name == *prefix
                        || name
                            .strip_prefix(prefix)
                            .is_some_and(|suffix| suffix.starts_with(".V0"))
                })
        })
    };
    let gates = [
        (
            "weapon",
            has_exact(AudioEventKind::Sound, &["MGUN2", "GUN18", "BAZOOK1"]),
        ),
        (
            "interface-feedback",
            has_exact(AudioEventKind::Sound, &["BUTTON", "SCOLD2", "BLEEP2"]),
        ),
        (
            "explosion",
            has_exact(
                AudioEventKind::Sound,
                &["XPLOS", "XPLODE", "XPLOSML2", "XPLOBIG4"],
            ),
        ),
        (
            "unit-response",
            has_prefix(
                AudioEventKind::Sound,
                &["ACKNO", "AFFIRM1", "MOVOUT1", "REPORT1", "UNIT1", "YESSIR1"],
            ),
        ),
        (
            "mission-accomplished",
            has_exact(AudioEventKind::Speech, &["ACCOM1"]),
        ),
        (
            "mission-failed",
            has_exact(AudioEventKind::Speech, &["FAIL1"]),
        ),
        (
            "gameplay-speech",
            has_exact(
                AudioEventKind::Speech,
                &["REINFOR1", "UNITREDY", "NEWOPT1", "BASEATK1"],
            ),
        ),
    ];
    let missing: Vec<_> = gates
        .iter()
        .filter_map(|(name, present)| {
            (!present
                && !(allow_missing_outcome_speech
                    && matches!(*name, "mission-accomplished" | "mission-failed")))
            .then_some(*name)
        })
        .collect();
    if !missing.is_empty() {
        return Err(Error::Conversion(format!(
            "decoded audio is insufficient for Tiberian Dawn campaign gameplay; missing core groups: {}",
            missing.join(", ")
        )));
    }
    Ok(())
}

struct SfxDefinition {
    name: &'static str,
    has_variants: bool,
}

macro_rules! sfx {
    ($name:literal) => {
        SfxDefinition {
            name: $name,
            has_variants: false,
        }
    };
    ($name:literal, variants) => {
        SfxDefinition {
            name: $name,
            has_variants: true,
        }
    };
}

// Active entries from `SoundEffectName[VOC_COUNT]` in tiberiandawn/audio.cpp,
// in enum/index order. Juvenile mode is outside the campaign profiles, so
// the normal `.AUD` candidate is used for IN_JUV entries.
const SFX_TABLE: &[SfxDefinition] = &[
    sfx!("BOMBIT1"),
    sfx!("CMON1"),
    sfx!("GOTIT1"),
    sfx!("KEEPEM1"),
    sfx!("LAUGH1"),
    sfx!("LEFTY1"),
    sfx!("NOPRBLM1"),
    sfx!("ONIT1"),
    sfx!("RAMYELL1"),
    sfx!("ROKROLL1"),
    sfx!("TUFFGUY1"),
    sfx!("YEAH1"),
    sfx!("YES1"),
    sfx!("YO1"),
    sfx!("GIRLOKAY"),
    sfx!("GIRLYEAH"),
    sfx!("GUYOKAY1"),
    sfx!("GUYYEAH1"),
    sfx!("2DANGR1", variants),
    sfx!("ACKNO", variants),
    sfx!("AFFIRM1", variants),
    sfx!("AWAIT1", variants),
    sfx!("MOVOUT1", variants),
    sfx!("NEGATV1", variants),
    sfx!("NOPROB", variants),
    sfx!("READY", variants),
    sfx!("REPORT1", variants),
    sfx!("RITAWAY", variants),
    sfx!("ROGER", variants),
    sfx!("UGOTIT", variants),
    sfx!("UNIT1", variants),
    sfx!("VEHIC1", variants),
    sfx!("YESSIR1", variants),
    sfx!("BAZOOK1"),
    sfx!("BLEEP2"),
    sfx!("BOMB1"),
    sfx!("BUTTON"),
    sfx!("COMCNTR1"),
    sfx!("CONSTRU2"),
    sfx!("CRUMBLE"),
    sfx!("FLAMER2"),
    sfx!("GUN18"),
    sfx!("GUN19"),
    sfx!("GUN20"),
    sfx!("GUN5"),
    sfx!("GUN8"),
    sfx!("GUNCLIP1"),
    sfx!("HVYDOOR1"),
    sfx!("HVYGUN10"),
    sfx!("ION1"),
    sfx!("MGUN11"),
    sfx!("MGUN2"),
    sfx!("NUKEMISL"),
    sfx!("NUKEXPLO"),
    sfx!("OBELRAY1"),
    sfx!("OBELPOWR"),
    sfx!("POWRDN1"),
    sfx!("RAMGUN2"),
    sfx!("ROCKET1"),
    sfx!("ROCKET2"),
    sfx!("SAMMOTR2"),
    sfx!("SCOLD2"),
    sfx!("SIDBAR1C"),
    sfx!("SIDBAR2C"),
    sfx!("SQUISH2"),
    sfx!("TNKFIRE2"),
    sfx!("TNKFIRE3"),
    sfx!("TNKFIRE4"),
    sfx!("TNKFIRE6"),
    sfx!("TONE15"),
    sfx!("TONE16"),
    sfx!("TONE2"),
    sfx!("TONE5"),
    sfx!("TOSS"),
    sfx!("TRANS1"),
    sfx!("TREEBRN1"),
    sfx!("TURRFIR5"),
    sfx!("XPLOBIG4"),
    sfx!("XPLOBIG6"),
    sfx!("XPLOBIG7"),
    sfx!("XPLODE"),
    sfx!("XPLOS"),
    sfx!("XPLOSML2"),
    sfx!("NUYELL1"),
    sfx!("NUYELL3"),
    sfx!("NUYELL4"),
    sfx!("NUYELL5"),
    sfx!("NUYELL6"),
    sfx!("NUYELL7"),
    sfx!("NUYELL10"),
    sfx!("NUYELL11"),
    sfx!("NUYELL12"),
    sfx!("YELL1"),
    sfx!("MYES1"),
    sfx!("MCOMND1"),
    sfx!("MHELLO1"),
    sfx!("MHMMM1"),
    sfx!("MPLAN3"),
    sfx!("MCOURSE1"),
    sfx!("MYESYES1"),
    sfx!("MTIBER1"),
    sfx!("MTHANKS1"),
    sfx!("CASHTURN"),
    sfx!("BLEEP2"),
    sfx!("DINOMOUT"),
    sfx!("DINOYES"),
    sfx!("DINOATK1"),
    sfx!("DINODIE1"),
    sfx!("BEACON"),
];

// Active entries from `Speech[VOX_COUNT]` in tiberiandawn/audio.cpp, in
// callback index order.
const SPEECH_TABLE: &[&str] = &[
    "ACCOM1", "FAIL1", "BLDG1", "CONSTRU1", "UNITREDY", "NEWOPT1", "DEPLOY1", "GDIDEAD1",
    "NODDEAD1", "CIVDEAD1", "NOCASH1", "BATLCON1", "REINFOR1", "CANCEL1", "BLDGING1", "LOPOWER1",
    "NOPOWER1", "MOCASH1", "BASEATK1", "INCOME1", "ENEMYA", "NUKE1", "NOBUILD1", "PRIBLDG1",
    "NODCAPT1", "GDICAPT1", "IONCHRG1", "IONREDY1", "NUKAVAIL", "NUKLNCH1", "UNITLOST", "STRCLOST",
    "NEEDHARV", "SELECT1", "AIRREDY1", "NOREDY1", "TRANSSEE", "TRANLOAD", "ENMYAPP1", "SILOS1",
    "ONHOLD1", "REPAIR1", "ESTRUCX", "GSTRUC1", "NSTRUC1", "ENMYUNIT",
];

#[cfg(test)]
mod tests {
    use std::io::Cursor;

    use tempfile::tempdir;

    use super::*;
    use crate::mix::mix_name_hash;

    fn compressed_aud(value: u8) -> Vec<u8> {
        let payload = [0xc3];
        let mut stored = Vec::new();
        stored.extend(1_u16.to_le_bytes());
        stored.extend(4_u16.to_le_bytes());
        stored.extend(0x0000_deaf_u32.to_le_bytes());
        stored.extend(payload);
        let mut bytes = Vec::new();
        bytes.extend(22_050_u16.to_le_bytes());
        bytes.extend((stored.len() as i32).to_le_bytes());
        bytes.extend(4_i32.to_le_bytes());
        bytes.push(0);
        bytes.push(1);
        // Change the initial sample through a one-sample signed delta, then
        // repeat is unnecessary for the contract test; deterministic bytes are enough.
        let _ = value;
        bytes.extend(stored);
        bytes
    }

    fn mix(entries: &[(&str, Vec<u8>)]) -> Vec<u8> {
        let mut indexed: Vec<_> = entries
            .iter()
            .map(|(name, bytes)| (mix_name_hash(name).unwrap(), bytes))
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
            output.extend_from_slice(bytes);
        }
        output
    }

    #[test]
    fn source_tables_cover_released_active_names_and_variants() {
        assert_eq!(SFX_TABLE.len(), 109);
        assert_eq!(SPEECH_TABLE.len(), 46);
        assert_eq!(
            SFX_TABLE.iter().filter(|entry| entry.has_variants).count(),
            15
        );
        let released = include_str!("../../../tiberiandawn/audio.cpp");
        let sfx_section = released
            .split("} SoundEffectName[VOC_COUNT] = {")
            .nth(1)
            .unwrap()
            .split("\n};")
            .next()
            .unwrap();
        let released_sfx: Vec<_> = sfx_section
            .lines()
            .filter_map(|line| {
                let line = line.trim();
                let name = line.strip_prefix("{\"")?.split('"').next()?;
                Some((name, line.contains("IN_VAR")))
            })
            .collect();
        let expected_sfx: Vec<_> = SFX_TABLE
            .iter()
            .map(|entry| (entry.name, entry.has_variants))
            .collect();
        assert_eq!(released_sfx, expected_sfx);

        let speech_section = released
            .split("char const* Speech[VOX_COUNT] = {")
            .nth(1)
            .unwrap()
            .split("\n};")
            .next()
            .unwrap();
        let released_speech: Vec<_> = speech_section
            .lines()
            .filter_map(|line| line.trim().strip_prefix('"')?.split('"').next())
            .collect();
        assert_eq!(released_speech, SPEECH_TABLE);
        let map = candidates();
        assert!(map.contains_key(&(AudioEventKind::Sound, "ACKNO.V00".into())));
        assert!(map.contains_key(&(AudioEventKind::Sound, "ACKNO.V03".into())));
        assert_eq!(
            map[&(AudioEventKind::Sound, "BLEEP2".into())].event_ids,
            [34, 103]
        );
    }

    #[test]
    fn resolves_decodes_and_indexes_a_core_synthetic_audio_set() {
        let temp = tempdir().unwrap();
        let engine = temp.path().join("engine/td");
        fs::create_dir_all(&engine).unwrap();
        let sound_entries = [
            "MGUN2.AUD",
            "BUTTON.AUD",
            "XPLOS.AUD",
            "ACKNO.V01",
            /* Wrong-kind collision: speech must still resolve from SPEECH.MIX. */
            "ACCOM1.AUD",
        ]
        .map(|name| (name, compressed_aud(128)));
        let speech_entries = [
            "ACCOM1.AUD",
            "FAIL1.AUD",
            "REINFOR1.AUD",
            /* Wrong-kind collision: effects must still resolve from SOUNDS.MIX. */
            "MGUN2.AUD",
        ]
        .map(|name| (name, compressed_aud(128)));
        fs::write(engine.join("SOUNDS.MIX"), mix(&sound_entries)).unwrap();
        fs::write(engine.join("SPEECH.MIX"), mix(&speech_entries)).unwrap();

        let mut progress = Vec::new();
        let report = build_audio_index(
            &engine,
            temp.path(),
            AudioBuildOptions::default(),
            |current, total, name| progress.push((current, total, name.to_owned())),
        )
        .unwrap();
        assert_eq!(report.index.assets.len(), 7);
        assert_eq!(progress.last().unwrap().0, progress.last().unwrap().1);
        report.index.validate().unwrap();
        assert!(temp.path().join(AUDIO_INDEX_PATH).is_file());
        assert!(temp.path().join("audio/sfx/mgun2.wav").is_file());
        assert!(temp.path().join("audio/speech/accom1.wav").is_file());
        assert_eq!(
            report
                .index
                .assets
                .iter()
                .find(|asset| asset.event_name == "MGUN2")
                .unwrap()
                .source_archive,
            "SOUNDS.MIX"
        );
        assert_eq!(
            report
                .index
                .assets
                .iter()
                .find(|asset| asset.event_name == "ACCOM1")
                .unwrap()
                .source_archive,
            "SPEECH.MIX"
        );

        let serialized = fs::read(temp.path().join(AUDIO_INDEX_PATH)).unwrap();
        let decoded: AudioIndexV1 = serde_json::from_slice(&serialized).unwrap();
        assert_eq!(decoded, report.index);
        let wav = fs::read(temp.path().join("audio/sfx/mgun2.wav")).unwrap();
        assert_eq!(&wav[..4], b"RIFF");
        assert_eq!(&wav[8..12], b"WAVE");
        assert!(MixArchive::parse(Cursor::new(mix(&sound_entries)), MixLimits::default()).is_ok());
    }

    #[test]
    fn refuses_a_silent_or_non_core_audio_index() {
        let asset = |kind, name: &str| AudioAssetV1 {
            kind,
            event_name: name.into(),
            event_ids: vec![0],
            path: "audio/sfx/probe.wav".into(),
            source_archive: "SOUNDS.MIX".into(),
            source_name: "PROBE.AUD".into(),
            source_compression: 1,
            sample_rate: 22_050,
            channels: 1,
            bits_per_sample: 8,
            frames: 1,
            sha256: Sha256Digest::ZERO,
        };
        assert!(ensure_core_gdi1_audio(&[asset(AudioEventKind::Sound, "BUTTON")], false).is_err());
    }

    #[test]
    fn rejects_cross_archive_audio_identity() {
        let index = AudioIndexV1 {
            format: "cncweb-audio".into(),
            version: 1,
            encoding: "wav-pcm".into(),
            assets: vec![AudioAssetV1 {
                kind: AudioEventKind::Sound,
                event_name: "BUTTON".into(),
                event_ids: vec![1],
                path: "audio/sfx/button.wav".into(),
                source_archive: "SPEECH.MIX".into(),
                source_name: "BUTTON.AUD".into(),
                source_compression: 1,
                sample_rate: 22_050,
                channels: 1,
                bits_per_sample: 8,
                frames: 1,
                sha256: Sha256Digest::ZERO,
            }],
            diagnostics: AudioDiagnosticsV1 {
                candidate_count: 1,
                missing_candidates: 0,
                decode_failures: vec![],
            },
        };
        assert!(index.validate().is_err());
    }
}
