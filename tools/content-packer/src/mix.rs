//! Bounds-checked reader for the classic Westwood MIX container.
//!
//! MIX indexes contain filename hashes rather than names. Callers must know the
//! logical filename they are looking for; resolution intentionally uses the
//! same uppercase C&C hash as the Tiberian Dawn engine.

use std::collections::BTreeMap;
use std::fs::File;
use std::io::{Read, Seek, SeekFrom, Write};
use std::path::Path;

use crate::error::{Error, Result};

const FILE_HEADER_BYTES: u64 = 6;
const EXTENDED_PREFIX_BYTES: u64 = 4;
const INDEX_ENTRY_BYTES: u64 = 12;
const SHA1_DIGEST_BYTES: u64 = 20;
const FLAG_DIGEST: u16 = 0x0001;
const FLAG_ENCRYPTED: u16 = 0x0002;
const KNOWN_FLAGS: u16 = FLAG_DIGEST | FLAG_ENCRYPTED;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MixLimits {
    pub max_files: u16,
    pub max_entry_bytes: u64,
    pub max_archive_bytes: u64,
}

impl Default for MixLimits {
    fn default() -> Self {
        Self {
            max_files: u16::MAX,
            max_entry_bytes: 64 * 1024 * 1024,
            max_archive_bytes: 2 * 1024 * 1024 * 1024,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MixFormat {
    Plain,
    Extended { has_digest: bool },
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MixHeader {
    pub format: MixFormat,
    pub file_count: u16,
    pub declared_data_size: u32,
    pub data_start: u64,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MixEntry {
    /// Unsigned representation of the engine's signed 32-bit filename hash.
    pub name_hash: u32,
    /// Offset relative to the MIX data region.
    pub relative_offset: u32,
    pub size: u32,
    /// Absolute byte offset in the source archive.
    pub offset: u64,
}

pub struct MixArchive<R> {
    reader: R,
    archive_len: u64,
    header: MixHeader,
    entries: Vec<MixEntry>,
    entries_by_hash: BTreeMap<u32, usize>,
}

impl MixArchive<File> {
    pub fn open(path: impl AsRef<Path>) -> Result<Self> {
        Self::open_with_limits(path, MixLimits::default())
    }

    pub fn open_with_limits(path: impl AsRef<Path>, limits: MixLimits) -> Result<Self> {
        Self::parse(File::open(path)?, limits)
    }
}

impl<R: Read + Seek> MixArchive<R> {
    pub fn parse(mut reader: R, limits: MixLimits) -> Result<Self> {
        let archive_len = reader.seek(SeekFrom::End(0))?;
        if archive_len > limits.max_archive_bytes {
            return Err(Error::MixLimit(format!(
                "{archive_len}-byte archive exceeds limit {}",
                limits.max_archive_bytes
            )));
        }
        reader.seek(SeekFrom::Start(0))?;
        if archive_len < FILE_HEADER_BYTES {
            return Err(Error::InvalidMix(
                "archive is shorter than a plain MIX header".into(),
            ));
        }

        let mut first_four = [0_u8; 4];
        reader.read_exact(&mut first_four)?;
        let first = u16::from_le_bytes([first_four[0], first_four[1]]);
        let second = u16::from_le_bytes([first_four[2], first_four[3]]);

        let (format, file_count, declared_data_size, mut cursor) = if first == 0 {
            if second & !KNOWN_FLAGS != 0 {
                return Err(Error::InvalidMix(format!(
                    "extended header uses unknown flags 0x{:04x}",
                    second & !KNOWN_FLAGS
                )));
            }
            if second & FLAG_ENCRYPTED != 0 {
                return Err(Error::InvalidMix(
                    "encrypted MIX indexes are not supported by browser-v1 conversion".into(),
                ));
            }
            let mut header = [0_u8; FILE_HEADER_BYTES as usize];
            reader.read_exact(&mut header)?;
            (
                MixFormat::Extended {
                    has_digest: second & FLAG_DIGEST != 0,
                },
                u16::from_le_bytes([header[0], header[1]]),
                u32::from_le_bytes([header[2], header[3], header[4], header[5]]),
                EXTENDED_PREFIX_BYTES + FILE_HEADER_BYTES,
            )
        } else {
            let mut size_tail = [0_u8; 2];
            reader.read_exact(&mut size_tail)?;
            (
                MixFormat::Plain,
                first,
                u32::from_le_bytes([first_four[2], first_four[3], size_tail[0], size_tail[1]]),
                FILE_HEADER_BYTES,
            )
        };

        if file_count > limits.max_files {
            return Err(Error::MixLimit(format!(
                "{file_count} files exceeds limit {}",
                limits.max_files
            )));
        }
        let index_bytes = u64::from(file_count)
            .checked_mul(INDEX_ENTRY_BYTES)
            .ok_or_else(|| Error::InvalidMix("index length overflow".into()))?;
        let data_start = cursor
            .checked_add(index_bytes)
            .ok_or_else(|| Error::InvalidMix("data offset overflow".into()))?;
        let data_end = data_start
            .checked_add(u64::from(declared_data_size))
            .ok_or_else(|| Error::InvalidMix("declared data length overflow".into()))?;
        let digest_bytes = match format {
            MixFormat::Extended { has_digest: true } => SHA1_DIGEST_BYTES,
            _ => 0,
        };
        let expected_len = data_end
            .checked_add(digest_bytes)
            .ok_or_else(|| Error::InvalidMix("archive length overflow".into()))?;
        if expected_len != archive_len {
            return Err(Error::InvalidMix(format!(
                "header describes {expected_len} bytes but archive contains {archive_len}"
            )));
        }

        let mut entries = Vec::with_capacity(file_count as usize);
        let mut entries_by_hash = BTreeMap::new();
        let mut previous_signed_hash = None;
        for index in 0..file_count {
            let mut raw = [0_u8; INDEX_ENTRY_BYTES as usize];
            reader.read_exact(&mut raw)?;
            cursor += INDEX_ENTRY_BYTES;
            let name_hash = u32::from_le_bytes(raw[0..4].try_into().expect("fixed slice"));
            let relative_offset = u32::from_le_bytes(raw[4..8].try_into().expect("fixed slice"));
            let size = u32::from_le_bytes(raw[8..12].try_into().expect("fixed slice"));

            if u64::from(size) > limits.max_entry_bytes {
                return Err(Error::MixLimit(format!(
                    "entry {index} is {size} bytes; limit is {}",
                    limits.max_entry_bytes
                )));
            }
            let signed_hash = name_hash as i32;
            if previous_signed_hash.is_some_and(|previous| previous >= signed_hash) {
                return Err(Error::InvalidMix(format!(
                    "index is not strictly sorted by signed filename hash at entry {index}"
                )));
            }
            previous_signed_hash = Some(signed_hash);

            let offset = data_start
                .checked_add(u64::from(relative_offset))
                .ok_or_else(|| Error::InvalidMix(format!("entry {index} offset overflow")))?;
            let end = offset
                .checked_add(u64::from(size))
                .ok_or_else(|| Error::InvalidMix(format!("entry {index} length overflow")))?;
            if end > data_end {
                return Err(Error::InvalidMix(format!(
                    "entry {index} ends at {end}, beyond data end {data_end}"
                )));
            }

            if entries_by_hash.insert(name_hash, entries.len()).is_some() {
                return Err(Error::InvalidMix(format!(
                    "duplicate filename hash 0x{name_hash:08x}"
                )));
            }
            entries.push(MixEntry {
                name_hash,
                relative_offset,
                size,
                offset,
            });
        }
        debug_assert_eq!(cursor, data_start);

        Ok(Self {
            reader,
            archive_len,
            header: MixHeader {
                format,
                file_count,
                declared_data_size,
                data_start,
            },
            entries,
            entries_by_hash,
        })
    }

    pub const fn archive_len(&self) -> u64 {
        self.archive_len
    }

    pub const fn header(&self) -> &MixHeader {
        &self.header
    }

    pub fn entries(&self) -> &[MixEntry] {
        &self.entries
    }

    pub fn entry_by_hash(&self, name_hash: u32) -> Option<&MixEntry> {
        self.entries_by_hash
            .get(&name_hash)
            .map(|index| &self.entries[*index])
    }

    pub fn entry(&self, name: &str) -> Result<Option<&MixEntry>> {
        Ok(self.entry_by_hash(mix_name_hash(name)?))
    }

    pub fn copy_entry<W: Write>(&mut self, name: &str, output: &mut W) -> Result<u64> {
        let hash = mix_name_hash(name)?;
        let entry = self.entry_by_hash(hash).ok_or_else(|| {
            Error::InvalidMix(format!("entry `{name}` (hash 0x{hash:08x}) does not exist"))
        })?;
        let (offset, size) = (entry.offset, u64::from(entry.size));
        self.reader.seek(SeekFrom::Start(offset))?;
        let copied = std::io::copy(&mut self.reader.by_ref().take(size), output)?;
        if copied != size {
            return Err(Error::InvalidMix(format!(
                "entry `{name}` truncated while reading: expected {size}, got {copied}"
            )));
        }
        Ok(copied)
    }

    pub fn read_entry(&mut self, name: &str, max_bytes: u64) -> Result<Vec<u8>> {
        let size = self
            .entry(name)?
            .ok_or_else(|| Error::InvalidMix(format!("entry `{name}` does not exist")))?
            .size as u64;
        if size > max_bytes || size > usize::MAX as u64 {
            return Err(Error::MixLimit(format!(
                "entry `{name}` is {size} bytes; read limit is {max_bytes}"
            )));
        }
        let mut bytes = Vec::with_capacity(size as usize);
        self.copy_entry(name, &mut bytes)?;
        Ok(bytes)
    }

    pub fn into_inner(self) -> R {
        self.reader
    }
}

/// Reproduces `CRCEngine` from the released Tiberian Dawn source. MIX names
/// are ASCII and are uppercased before hashing.
pub fn mix_name_hash(name: &str) -> Result<u32> {
    if name.is_empty() || !name.is_ascii() {
        return Err(Error::InvalidMix(
            "MIX logical names must be non-empty ASCII".into(),
        ));
    }
    let uppercase = name.to_ascii_uppercase();
    let bytes = uppercase.as_bytes();
    let mut hash = 0_u32;
    let mut chunks = bytes.chunks_exact(4);
    for chunk in &mut chunks {
        let word = u32::from_le_bytes(chunk.try_into().expect("four-byte chunk"));
        hash = hash.rotate_left(1).wrapping_add(word);
    }
    let remainder = chunks.remainder();
    if !remainder.is_empty() {
        let mut padded = [0_u8; 4];
        padded[..remainder.len()].copy_from_slice(remainder);
        hash = hash.rotate_left(1).wrapping_add(u32::from_le_bytes(padded));
    }
    Ok(hash)
}

#[cfg(test)]
mod tests {
    use std::io::Cursor;

    use super::*;

    fn synthetic_mix(entries: &[(&str, &[u8])], extended_digest: bool) -> Vec<u8> {
        let mut indexed: Vec<_> = entries
            .iter()
            .map(|(name, bytes)| (mix_name_hash(name).unwrap(), *bytes))
            .collect();
        indexed.sort_by_key(|(hash, _)| *hash as i32);
        let data_size: usize = indexed.iter().map(|(_, bytes)| bytes.len()).sum();
        let mut output = Vec::new();
        if extended_digest {
            output.extend(0_u16.to_le_bytes());
            output.extend(FLAG_DIGEST.to_le_bytes());
        }
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
        if extended_digest {
            output.extend([0x5a; SHA1_DIGEST_BYTES as usize]);
        }
        output
    }

    #[test]
    fn hash_is_ascii_case_insensitive_and_matches_engine_vectors() {
        assert_eq!(mix_name_hash("SCG01EA.INI").unwrap(), 0x1de8_e5f8);
        assert_eq!(mix_name_hash("scg01ea.ini").unwrap(), 0x1de8_e5f8);
        assert_eq!(mix_name_hash("SCG01EA.BIN").unwrap(), 0x1ded_e0f1);
    }

    #[test]
    fn parses_plain_and_extended_archives_and_reads_by_name() {
        for extended in [false, true] {
            let bytes = synthetic_mix(
                &[("SCG01EA.INI", b"[Basic]\n"), ("SCG01EA.BIN", b"map")],
                extended,
            );
            let mut archive = MixArchive::parse(Cursor::new(bytes), MixLimits::default()).unwrap();
            assert_eq!(archive.entries().len(), 2);
            assert_eq!(archive.read_entry("scg01ea.bin", 10).unwrap(), b"map");
            assert_eq!(
                archive.read_entry("SCG01EA.INI", 100).unwrap(),
                b"[Basic]\n"
            );
        }
    }

    #[test]
    fn rejects_encryption_bad_ranges_and_unsorted_indexes() {
        let mut encrypted = synthetic_mix(&[("A", b"x")], true);
        encrypted[2..4].copy_from_slice(&FLAG_ENCRYPTED.to_le_bytes());
        assert!(MixArchive::parse(Cursor::new(encrypted), MixLimits::default()).is_err());

        let mut range = synthetic_mix(&[("A", b"x")], false);
        range[10..14].copy_from_slice(&u32::MAX.to_le_bytes());
        assert!(MixArchive::parse(Cursor::new(range), MixLimits::default()).is_err());

        let mut unsorted = synthetic_mix(&[("A", b"x"), ("B", b"y")], false);
        let first = unsorted[6..18].to_vec();
        let second = unsorted[18..30].to_vec();
        unsorted[6..18].copy_from_slice(&second);
        unsorted[18..30].copy_from_slice(&first);
        assert!(MixArchive::parse(Cursor::new(unsorted), MixLimits::default()).is_err());
    }
}
