use std::fmt;
use std::fs::File;
use std::io::{Read, Write};
use std::path::Path;
use std::str::FromStr;

use serde::{de, Deserialize, Deserializer, Serialize, Serializer};
use sha2::{Digest, Sha256};

use crate::error::{Error, Result};

/// A SHA-256 digest serialized as 64 lowercase hexadecimal characters.
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
pub struct Sha256Digest([u8; 32]);

impl Sha256Digest {
    pub const ZERO: Self = Self([0; 32]);

    pub const fn from_bytes(bytes: [u8; 32]) -> Self {
        Self(bytes)
    }

    pub const fn as_bytes(&self) -> &[u8; 32] {
        &self.0
    }

    pub fn to_hex(self) -> String {
        const HEX: &[u8; 16] = b"0123456789abcdef";
        let mut output = String::with_capacity(64);
        for byte in self.0 {
            output.push(HEX[(byte >> 4) as usize] as char);
            output.push(HEX[(byte & 0x0f) as usize] as char);
        }
        output
    }
}

impl fmt::Display for Sha256Digest {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        formatter.write_str(&self.to_hex())
    }
}

impl fmt::Debug for Sha256Digest {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        formatter
            .debug_tuple("Sha256Digest")
            .field(&self.to_hex())
            .finish()
    }
}

impl FromStr for Sha256Digest {
    type Err = Error;

    fn from_str(value: &str) -> Result<Self> {
        if value.len() != 64 {
            return Err(Error::InvalidDigest(
                "expected exactly 64 hexadecimal characters".into(),
            ));
        }

        let mut bytes = [0_u8; 32];
        for (index, pair) in value.as_bytes().chunks_exact(2).enumerate() {
            let high = decode_hex(pair[0]).ok_or_else(|| {
                Error::InvalidDigest(format!("non-hexadecimal character at byte {}", index * 2))
            })?;
            let low = decode_hex(pair[1]).ok_or_else(|| {
                Error::InvalidDigest(format!(
                    "non-hexadecimal character at byte {}",
                    index * 2 + 1
                ))
            })?;
            bytes[index] = (high << 4) | low;
        }
        Ok(Self(bytes))
    }
}

fn decode_hex(byte: u8) -> Option<u8> {
    match byte {
        b'0'..=b'9' => Some(byte - b'0'),
        b'a'..=b'f' => Some(byte - b'a' + 10),
        b'A'..=b'F' => Some(byte - b'A' + 10),
        _ => None,
    }
}

impl Serialize for Sha256Digest {
    fn serialize<S>(&self, serializer: S) -> std::result::Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_str(&self.to_hex())
    }
}

impl<'de> Deserialize<'de> for Sha256Digest {
    fn deserialize<D>(deserializer: D) -> std::result::Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let value = String::deserialize(deserializer)?;
        if value.bytes().any(|byte| matches!(byte, b'A'..=b'F')) {
            return Err(de::Error::custom(
                "SHA-256 digests in JSON must use lowercase hexadecimal",
            ));
        }
        value.parse().map_err(de::Error::custom)
    }
}

/// Hashes a stream without buffering it in memory, returning its byte count too.
pub fn hash_reader<R: Read>(reader: &mut R) -> Result<(Sha256Digest, u64)> {
    let mut hasher = Sha256::new();
    let mut total = 0_u64;
    let mut buffer = [0_u8; 64 * 1024];

    loop {
        let read = reader.read(&mut buffer)?;
        if read == 0 {
            break;
        }
        total = total
            .checked_add(read as u64)
            .ok_or_else(|| Error::PackageLimit("stream length overflowed u64".into()))?;
        hasher.update(&buffer[..read]);
    }

    Ok((digest_from_hasher(hasher), total))
}

pub fn hash_file(path: impl AsRef<Path>) -> Result<(Sha256Digest, u64)> {
    let mut file = File::open(path)?;
    hash_reader(&mut file)
}

pub(crate) fn copy_and_hash<R: Read, W: Write>(
    reader: &mut R,
    writer: &mut W,
) -> Result<(Sha256Digest, u64)> {
    let mut hasher = Sha256::new();
    let mut total = 0_u64;
    let mut buffer = [0_u8; 64 * 1024];

    loop {
        let read = reader.read(&mut buffer)?;
        if read == 0 {
            break;
        }
        writer.write_all(&buffer[..read])?;
        hasher.update(&buffer[..read]);
        total = total
            .checked_add(read as u64)
            .ok_or_else(|| Error::PackageLimit("stream length overflowed u64".into()))?;
    }

    Ok((digest_from_hasher(hasher), total))
}

pub(crate) fn digest_from_hasher(hasher: Sha256) -> Sha256Digest {
    let bytes: [u8; 32] = hasher.finalize().into();
    Sha256Digest::from_bytes(bytes)
}

#[cfg(test)]
mod tests {
    use std::io::Cursor;

    use super::*;

    #[test]
    fn hashes_known_vector() {
        let (digest, size) = hash_reader(&mut Cursor::new(b"abc")).unwrap();
        assert_eq!(size, 3);
        assert_eq!(
            digest.to_string(),
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
        );
    }

    #[test]
    fn digest_json_is_canonical_lowercase() {
        let upper = "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD";
        let digest: Sha256Digest = upper.parse().unwrap();
        assert_eq!(
            serde_json::to_string(&digest).unwrap(),
            format!("\"{}\"", upper.to_lowercase())
        );
    }
}
