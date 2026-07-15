use std::collections::{BTreeMap, BTreeSet};
use std::fs::File;
use std::io::{Read, Seek, SeekFrom, Write};
use std::path::Path;

use crate::error::{Error, Result};
use crate::path::{collision_key, normalize_meg_path};

pub const MEG_MAGIC: u32 = 0xffff_ffff;
pub const MEG_MAGIC_ALTERNATE: u32 = 0x8fff_ffff;
const SUBFILE_DATA_SIZE: u64 = 20;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MegLimits {
    pub max_files: u32,
    pub max_strings: u32,
    pub max_string_table_bytes: u64,
    pub max_name_bytes: u16,
    pub max_entry_bytes: u64,
}

impl Default for MegLimits {
    fn default() -> Self {
        Self {
            max_files: 1_000_000,
            // The packed entry stores a u16 string index.
            max_strings: u16::MAX as u32 + 1,
            max_string_table_bytes: 64 * 1024 * 1024,
            max_name_bytes: 16 * 1024,
            max_entry_bytes: 32 * 1024 * 1024 * 1024,
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub enum MegFormat {
    /// Older MEG layout: a four-byte prefix followed directly by the counts.
    Legacy { prefix: u32 },
    /// Layout emitted by the official Remastered Collection map editor.
    Remastered {
        magic: u32,
        version: f32,
        declared_header_size: u32,
    },
}

#[derive(Debug, Clone, PartialEq)]
pub struct MegHeader {
    pub format: MegFormat,
    pub file_count: u32,
    pub string_count: u32,
    pub string_table_size: u32,
    pub metadata_end: u64,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MegEntry {
    pub flags: u16,
    pub crc32: u32,
    pub index: u32,
    /// Canonical `/`-separated logical name.
    pub name: String,
    pub size: u64,
    pub offset: u64,
}

pub struct MegArchive<R> {
    reader: R,
    archive_len: u64,
    header: MegHeader,
    entries: Vec<MegEntry>,
    entries_by_name: BTreeMap<String, usize>,
}

impl MegArchive<File> {
    pub fn open(path: impl AsRef<Path>) -> Result<Self> {
        Self::open_with_limits(path, MegLimits::default())
    }

    pub fn open_with_limits(path: impl AsRef<Path>, limits: MegLimits) -> Result<Self> {
        Self::parse(File::open(path)?, limits)
    }
}

impl<R: Read + Seek> MegArchive<R> {
    pub fn parse(mut reader: R, limits: MegLimits) -> Result<Self> {
        let archive_len = reader.seek(SeekFrom::End(0))?;
        reader.seek(SeekFrom::Start(0))?;
        let mut offset = 0_u64;

        let prefix = read_u32(&mut reader, &mut offset, archive_len)?;
        let format = if prefix == MEG_MAGIC || prefix == MEG_MAGIC_ALTERNATE {
            let version = f32::from_bits(read_u32(&mut reader, &mut offset, archive_len)?);
            let declared_header_size = read_u32(&mut reader, &mut offset, archive_len)?;
            if !version.is_finite() {
                return Err(Error::InvalidMeg("version is not a finite number".into()));
            }
            MegFormat::Remastered {
                magic: prefix,
                version,
                declared_header_size,
            }
        } else {
            MegFormat::Legacy { prefix }
        };

        let file_count = read_u32(&mut reader, &mut offset, archive_len)?;
        let string_count = read_u32(&mut reader, &mut offset, archive_len)?;
        let string_table_size = read_u32(&mut reader, &mut offset, archive_len)?;

        if file_count > limits.max_files {
            return Err(Error::MegLimit(format!(
                "{file_count} files exceeds limit {}",
                limits.max_files
            )));
        }
        if string_count > limits.max_strings {
            return Err(Error::MegLimit(format!(
                "{string_count} strings exceeds limit {}",
                limits.max_strings
            )));
        }
        if string_table_size as u64 > limits.max_string_table_bytes {
            return Err(Error::MegLimit(format!(
                "{}-byte string table exceeds limit {}",
                string_table_size, limits.max_string_table_bytes
            )));
        }

        let string_table_end = checked_add(offset, string_table_size as u64, "string table")?;
        if string_table_end > archive_len {
            return Err(Error::InvalidMeg(format!(
                "string table ends at {string_table_end}, beyond {archive_len}-byte archive"
            )));
        }

        let mut strings = Vec::with_capacity(string_count as usize);
        for string_index in 0..string_count {
            if checked_add(offset, 2, "string length")? > string_table_end {
                return Err(Error::InvalidMeg(format!(
                    "string {string_index} has no complete length inside the string table"
                )));
            }
            let byte_len = read_u16(&mut reader, &mut offset, string_table_end)?;
            if byte_len > limits.max_name_bytes {
                return Err(Error::MegLimit(format!(
                    "string {string_index} is {byte_len} bytes; limit is {}",
                    limits.max_name_bytes
                )));
            }
            let name_end = checked_add(offset, byte_len as u64, "string data")?;
            if name_end > string_table_end {
                return Err(Error::InvalidMeg(format!(
                    "string {string_index} extends beyond the declared string table"
                )));
            }
            let mut bytes = vec![0_u8; byte_len as usize];
            read_exact_at(&mut reader, &mut offset, name_end, &mut bytes)?;
            let source_name = String::from_utf8(bytes).map_err(|_| {
                Error::InvalidMeg(format!("string {string_index} is not valid UTF-8"))
            })?;
            let name = normalize_meg_path(&source_name).map_err(|error| {
                Error::InvalidMeg(format!("unsafe name at string {string_index}: {error}"))
            })?;
            strings.push(name);
        }

        if offset != string_table_end {
            return Err(Error::InvalidMeg(format!(
                "string table contains {} unclaimed bytes",
                string_table_end - offset
            )));
        }

        let file_table_bytes = (file_count as u64)
            .checked_mul(SUBFILE_DATA_SIZE)
            .ok_or_else(|| Error::InvalidMeg("file table length overflow".into()))?;
        let metadata_end = checked_add(offset, file_table_bytes, "file table")?;
        if metadata_end > archive_len {
            return Err(Error::InvalidMeg(format!(
                "file table ends at {metadata_end}, beyond {archive_len}-byte archive"
            )));
        }

        let data_floor = match format {
            MegFormat::Remastered {
                declared_header_size,
                ..
            } => {
                let declared = declared_header_size as u64;
                if declared < metadata_end {
                    return Err(Error::InvalidMeg(format!(
                        "declared header size {declared} is smaller than metadata end {metadata_end}"
                    )));
                }
                if declared > archive_len {
                    return Err(Error::InvalidMeg(format!(
                        "declared header size {declared} exceeds archive size {archive_len}"
                    )));
                }
                declared
            }
            MegFormat::Legacy { .. } => metadata_end,
        };

        let mut entries = Vec::with_capacity(file_count as usize);
        let mut entries_by_name = BTreeMap::new();
        let mut entry_indices = BTreeSet::new();

        for table_index in 0..file_count {
            let flags = read_u16(&mut reader, &mut offset, metadata_end)?;
            let crc32 = read_u32(&mut reader, &mut offset, metadata_end)?;
            let signed_index = read_i32(&mut reader, &mut offset, metadata_end)?;
            let size = read_u32(&mut reader, &mut offset, metadata_end)? as u64;
            let data_offset = read_u32(&mut reader, &mut offset, metadata_end)? as u64;
            let name_index = read_u16(&mut reader, &mut offset, metadata_end)? as usize;

            if signed_index < 0 || signed_index as u32 >= file_count {
                return Err(Error::InvalidMeg(format!(
                    "file table record {table_index} has invalid subfile index {signed_index}"
                )));
            }
            let index = signed_index as u32;
            if !entry_indices.insert(index) {
                return Err(Error::InvalidMeg(format!(
                    "duplicate subfile index {index}"
                )));
            }
            if size > limits.max_entry_bytes {
                return Err(Error::MegLimit(format!(
                    "file table record {table_index} is {size} bytes; limit is {}",
                    limits.max_entry_bytes
                )));
            }
            let name = strings.get(name_index).ok_or_else(|| {
                Error::InvalidMeg(format!(
                    "file table record {table_index} refers to missing string {name_index}"
                ))
            })?;
            if data_offset < data_floor {
                return Err(Error::InvalidMeg(format!(
                    "entry `{name}` begins inside archive metadata"
                )));
            }
            let data_end = checked_add(data_offset, size, "entry data")?;
            if data_end > archive_len {
                return Err(Error::InvalidMeg(format!(
                    "entry `{name}` ends at {data_end}, beyond {archive_len}-byte archive"
                )));
            }

            let key = collision_key(name);
            if entries_by_name.insert(key, entries.len()).is_some() {
                return Err(Error::DuplicatePath(name.clone()));
            }
            entries.push(MegEntry {
                flags,
                crc32,
                index,
                name: name.clone(),
                size,
                offset: data_offset,
            });
        }

        debug_assert_eq!(offset, metadata_end);
        Ok(Self {
            reader,
            archive_len,
            header: MegHeader {
                format,
                file_count,
                string_count,
                string_table_size,
                metadata_end,
            },
            entries,
            entries_by_name,
        })
    }

    pub const fn archive_len(&self) -> u64 {
        self.archive_len
    }

    pub const fn header(&self) -> &MegHeader {
        &self.header
    }

    pub fn entries(&self) -> &[MegEntry] {
        &self.entries
    }

    pub fn entry(&self, name: &str) -> Option<&MegEntry> {
        let normalized = normalize_meg_path(name).ok()?;
        self.entries_by_name
            .get(&collision_key(&normalized))
            .map(|index| &self.entries[*index])
    }

    pub fn copy_entry<W: Write>(&mut self, name: &str, output: &mut W) -> Result<u64> {
        let entry = self
            .entry(name)
            .ok_or_else(|| Error::InvalidMeg(format!("entry `{name}` does not exist")))?;
        let (offset, size) = (entry.offset, entry.size);
        self.reader.seek(SeekFrom::Start(offset))?;
        let copied = std::io::copy(&mut self.reader.by_ref().take(size), output)?;
        if copied != size {
            return Err(Error::InvalidMeg(format!(
                "entry `{name}` truncated while reading: expected {size}, got {copied}"
            )));
        }
        Ok(copied)
    }

    pub fn read_entry(&mut self, name: &str, max_bytes: u64) -> Result<Vec<u8>> {
        let size = self
            .entry(name)
            .ok_or_else(|| Error::InvalidMeg(format!("entry `{name}` does not exist")))?
            .size;
        if size > max_bytes || size > usize::MAX as u64 {
            return Err(Error::MegLimit(format!(
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

fn checked_add(left: u64, right: u64, field: &str) -> Result<u64> {
    left.checked_add(right)
        .ok_or_else(|| Error::InvalidMeg(format!("{field} offset overflow")))
}

fn read_exact_at<R: Read>(
    reader: &mut R,
    offset: &mut u64,
    bound: u64,
    bytes: &mut [u8],
) -> Result<()> {
    let end = checked_add(*offset, bytes.len() as u64, "read")?;
    if end > bound {
        return Err(Error::InvalidMeg(format!(
            "read at {} for {} bytes exceeds bound {bound}",
            *offset,
            bytes.len()
        )));
    }
    reader.read_exact(bytes).map_err(|error| {
        if error.kind() == std::io::ErrorKind::UnexpectedEof {
            Error::InvalidMeg(format!(
                "archive truncated at byte {} while reading {} bytes",
                *offset,
                bytes.len()
            ))
        } else {
            Error::Io(error)
        }
    })?;
    *offset = end;
    Ok(())
}

fn read_u16<R: Read>(reader: &mut R, offset: &mut u64, bound: u64) -> Result<u16> {
    let mut bytes = [0_u8; 2];
    read_exact_at(reader, offset, bound, &mut bytes)?;
    Ok(u16::from_le_bytes(bytes))
}

fn read_u32<R: Read>(reader: &mut R, offset: &mut u64, bound: u64) -> Result<u32> {
    let mut bytes = [0_u8; 4];
    read_exact_at(reader, offset, bound, &mut bytes)?;
    Ok(u32::from_le_bytes(bytes))
}

fn read_i32<R: Read>(reader: &mut R, offset: &mut u64, bound: u64) -> Result<i32> {
    let mut bytes = [0_u8; 4];
    read_exact_at(reader, offset, bound, &mut bytes)?;
    Ok(i32::from_le_bytes(bytes))
}

#[cfg(test)]
mod tests {
    use std::io::Cursor;

    use super::*;

    fn synthetic_meg(names_and_data: &[(&str, &[u8])]) -> Vec<u8> {
        let string_size: usize = names_and_data.iter().map(|(name, _)| 2 + name.len()).sum();
        let header_size = 24 + string_size + names_and_data.len() * SUBFILE_DATA_SIZE as usize;
        let mut output = Vec::new();
        output.extend(MEG_MAGIC.to_le_bytes());
        output.extend(0.99_f32.to_le_bytes());
        output.extend((header_size as u32).to_le_bytes());
        output.extend((names_and_data.len() as u32).to_le_bytes());
        output.extend((names_and_data.len() as u32).to_le_bytes());
        output.extend((string_size as u32).to_le_bytes());
        for (name, _) in names_and_data {
            output.extend((name.len() as u16).to_le_bytes());
            output.extend(name.as_bytes());
        }
        let mut data_offset = header_size as u32;
        for (index, (_, data)) in names_and_data.iter().enumerate() {
            output.extend(0_u16.to_le_bytes());
            output.extend((index as u32).to_le_bytes());
            output.extend((index as i32).to_le_bytes());
            output.extend((data.len() as u32).to_le_bytes());
            output.extend(data_offset.to_le_bytes());
            output.extend((index as u16).to_le_bytes());
            data_offset += data.len() as u32;
        }
        for (_, data) in names_and_data {
            output.extend(*data);
        }
        output
    }

    #[test]
    fn parses_official_remastered_layout_and_reads_entries() {
        let bytes = synthetic_meg(&[(r"DATA\ONE.TXT", b"one"), (r"DATA\TWO.BIN", b"two!")]);
        let mut archive = MegArchive::parse(Cursor::new(bytes), MegLimits::default()).unwrap();
        assert_eq!(archive.entries().len(), 2);
        assert_eq!(archive.entry("data/one.txt").unwrap().size, 3);
        assert_eq!(archive.read_entry(r"DATA\TWO.BIN", 100).unwrap(), b"two!");
        assert!(matches!(
            archive.header().format,
            MegFormat::Remastered { .. }
        ));
    }

    #[test]
    fn rejects_truncated_and_out_of_bounds_archives() {
        let bytes = synthetic_meg(&[("DATA/ONE.TXT", b"one")]);
        for truncated_len in [0, 3, 23, bytes.len() - 1] {
            assert!(MegArchive::parse(
                Cursor::new(bytes[..truncated_len].to_vec()),
                MegLimits::default()
            )
            .is_err());
        }

        let mut bad_offset = bytes;
        let entry_offset_field = 24 + (2 + "DATA/ONE.TXT".len()) + 14;
        bad_offset[entry_offset_field..entry_offset_field + 4]
            .copy_from_slice(&u32::MAX.to_le_bytes());
        assert!(MegArchive::parse(Cursor::new(bad_offset), MegLimits::default()).is_err());
    }

    #[test]
    fn rejects_unsafe_and_case_colliding_names() {
        assert!(MegArchive::parse(
            Cursor::new(synthetic_meg(&[("../ESCAPE", b"x")])),
            MegLimits::default()
        )
        .is_err());
        assert!(MegArchive::parse(
            Cursor::new(synthetic_meg(&[("DATA/A", b"x"), ("data/a", b"y")])),
            MegLimits::default()
        )
        .is_err());
    }
}
