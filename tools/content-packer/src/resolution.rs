//! Explicit first-layer-wins resource resolution.
//!
//! Both engines register archives in a deliberate order. Keeping that order in
//! the conversion plan avoids depending on filesystem enumeration or hash-map
//! iteration when the same logical resource is present in multiple patches.

use std::io::{Read, Seek};

use crate::error::Result;
use crate::meg::{MegArchive, MegEntry};
use crate::mix::{mix_name_hash, MixArchive, MixEntry};

#[derive(Clone, Copy)]
pub struct MegLayer<'a, R> {
    pub id: &'a str,
    pub archive: &'a MegArchive<R>,
}

#[derive(Clone, Copy)]
pub struct ResolvedMegEntry<'a> {
    pub layer_id: &'a str,
    pub entry: &'a MegEntry,
}

pub fn resolve_meg_entry<'a, R: Read + Seek>(
    layers: &'a [MegLayer<'a, R>],
    logical_name: &str,
) -> Option<ResolvedMegEntry<'a>> {
    layers.iter().find_map(|layer| {
        layer
            .archive
            .entry(logical_name)
            .map(|entry| ResolvedMegEntry {
                layer_id: layer.id,
                entry,
            })
    })
}

#[derive(Clone, Copy)]
pub struct MixLayer<'a, R> {
    pub id: &'a str,
    pub archive: &'a MixArchive<R>,
}

#[derive(Debug, Clone, Copy)]
pub struct ResolvedMixEntry<'a> {
    pub layer_id: &'a str,
    pub entry: &'a MixEntry,
}

pub fn resolve_mix_entry<'a, R: Read + Seek>(
    layers: &'a [MixLayer<'a, R>],
    logical_name: &str,
) -> Result<Option<ResolvedMixEntry<'a>>> {
    let hash = mix_name_hash(logical_name)?;
    Ok(layers.iter().find_map(|layer| {
        layer
            .archive
            .entry_by_hash(hash)
            .map(|entry| ResolvedMixEntry {
                layer_id: layer.id,
                entry,
            })
    }))
}

#[cfg(test)]
mod tests {
    use std::io::Cursor;

    use super::*;
    use crate::meg::{MegLimits, MEG_MAGIC};
    use crate::mix::MixLimits;

    fn meg(name: &str, data: &[u8]) -> MegArchive<Cursor<Vec<u8>>> {
        let string_size = 2 + name.len();
        let header_size = 24 + string_size + 20;
        let mut bytes = Vec::new();
        bytes.extend(MEG_MAGIC.to_le_bytes());
        bytes.extend(0.99_f32.to_le_bytes());
        bytes.extend((header_size as u32).to_le_bytes());
        bytes.extend(1_u32.to_le_bytes());
        bytes.extend(1_u32.to_le_bytes());
        bytes.extend((string_size as u32).to_le_bytes());
        bytes.extend((name.len() as u16).to_le_bytes());
        bytes.extend(name.as_bytes());
        bytes.extend(0_u16.to_le_bytes());
        bytes.extend(0_u32.to_le_bytes());
        bytes.extend(0_i32.to_le_bytes());
        bytes.extend((data.len() as u32).to_le_bytes());
        bytes.extend((header_size as u32).to_le_bytes());
        bytes.extend(0_u16.to_le_bytes());
        bytes.extend(data);
        MegArchive::parse(Cursor::new(bytes), MegLimits::default()).unwrap()
    }

    fn mix(name: &str, data: &[u8]) -> MixArchive<Cursor<Vec<u8>>> {
        let hash = mix_name_hash(name).unwrap();
        let mut bytes = Vec::new();
        bytes.extend(1_u16.to_le_bytes());
        bytes.extend((data.len() as u32).to_le_bytes());
        bytes.extend(hash.to_le_bytes());
        bytes.extend(0_u32.to_le_bytes());
        bytes.extend((data.len() as u32).to_le_bytes());
        bytes.extend(data);
        MixArchive::parse(Cursor::new(bytes), MixLimits::default()).unwrap()
    }

    #[test]
    fn meg_resolution_is_case_insensitive_and_first_layer_wins() {
        let patch = meg("DATA/PROBE.BIN", b"patch");
        let base = meg("data/probe.bin", b"base");
        let layers = [
            MegLayer {
                id: "patch",
                archive: &patch,
            },
            MegLayer {
                id: "base",
                archive: &base,
            },
        ];
        let resolved = resolve_meg_entry(&layers, "Data/Probe.bin").unwrap();
        assert_eq!(resolved.layer_id, "patch");
        assert_eq!(resolved.entry.size, 5);
    }

    #[test]
    fn mix_resolution_uses_engine_hash_and_first_layer_wins() {
        let patch = mix("SCG01EA.INI", b"patch");
        let base = mix("SCG01EA.INI", b"base");
        let layers = [
            MixLayer {
                id: "UPDATE.MIX",
                archive: &patch,
            },
            MixLayer {
                id: "GENERAL.MIX",
                archive: &base,
            },
        ];
        let resolved = resolve_mix_entry(&layers, "scg01ea.ini").unwrap().unwrap();
        assert_eq!(resolved.layer_id, "UPDATE.MIX");
        assert_eq!(resolved.entry.size, 5);
    }
}
