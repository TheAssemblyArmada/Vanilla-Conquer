//! Safe decoder for the Westwood AUD samples used by Tiberian Dawn.
//!
//! The layout and codecs follow `common/audio.h`, `common/auduncmp.cpp`,
//! `common/soundio_common.cpp`, and `common/soscodec.cpp` in the released
//! engine source. Decoding is bounded before allocating output.

use crate::error::{Error, Result};

const HEADER_BYTES: usize = 12;
const CHUNK_HEADER_BYTES: usize = 8;
const CHUNK_MAGIC: u32 = 0x0000_deaf;
const FLAG_STEREO: u8 = 0x01;
const FLAG_16_BIT: u8 = 0x02;
const KNOWN_FLAGS: u8 = FLAG_STEREO | FLAG_16_BIT;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct AudLimits {
    pub max_source_bytes: u64,
    pub max_output_bytes: u64,
    pub max_chunks: usize,
    pub minimum_sample_rate: u16,
    pub maximum_sample_rate: u32,
}

impl Default for AudLimits {
    fn default() -> Self {
        Self {
            max_source_bytes: 16 * 1024 * 1024,
            max_output_bytes: 64 * 1024 * 1024,
            max_chunks: 100_000,
            minimum_sample_rate: 4_000,
            maximum_sample_rate: 192_000,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct DecodedAud {
    pub sample_rate: u32,
    pub channels: u16,
    pub bits_per_sample: u16,
    pub source_compression: u8,
    pub frames: u64,
    /// WAV-compatible PCM: unsigned bytes for 8-bit, little-endian signed
    /// samples for 16-bit.
    pub pcm: Vec<u8>,
}

impl DecodedAud {
    pub fn to_wav(&self) -> Result<Vec<u8>> {
        let bytes_per_sample = u32::from(self.bits_per_sample) / 8;
        let block_align = u32::from(self.channels)
            .checked_mul(bytes_per_sample)
            .ok_or_else(|| Error::InvalidAud("WAV block alignment overflow".into()))?;
        if block_align == 0 || self.pcm.len() as u64 % u64::from(block_align) != 0 {
            return Err(Error::InvalidAud(
                "PCM byte count is not aligned to complete sample frames".into(),
            ));
        }
        let data_size = u32::try_from(self.pcm.len())
            .map_err(|_| Error::AudLimit("PCM does not fit in a WAV data chunk".into()))?;
        let padding = data_size & 1;
        let riff_size = 36_u32
            .checked_add(data_size)
            .and_then(|size| size.checked_add(padding))
            .ok_or_else(|| Error::AudLimit("WAV RIFF size overflow".into()))?;
        let byte_rate = self
            .sample_rate
            .checked_mul(block_align)
            .ok_or_else(|| Error::InvalidAud("WAV byte rate overflow".into()))?;
        let block_align_u16 = u16::try_from(block_align)
            .map_err(|_| Error::InvalidAud("WAV block alignment does not fit u16".into()))?;

        let capacity = 44_usize
            .checked_add(self.pcm.len())
            .and_then(|size| size.checked_add(padding as usize))
            .ok_or_else(|| Error::AudLimit("WAV allocation overflow".into()))?;
        let mut output = Vec::with_capacity(capacity);
        output.extend(b"RIFF");
        output.extend(riff_size.to_le_bytes());
        output.extend(b"WAVEfmt ");
        output.extend(16_u32.to_le_bytes());
        output.extend(1_u16.to_le_bytes());
        output.extend(self.channels.to_le_bytes());
        output.extend(self.sample_rate.to_le_bytes());
        output.extend(byte_rate.to_le_bytes());
        output.extend(block_align_u16.to_le_bytes());
        output.extend(self.bits_per_sample.to_le_bytes());
        output.extend(b"data");
        output.extend(data_size.to_le_bytes());
        output.extend(&self.pcm);
        if padding != 0 {
            output.push(0);
        }
        debug_assert_eq!(output.len(), capacity);
        Ok(output)
    }
}

pub fn decode_aud(bytes: &[u8], limits: AudLimits) -> Result<DecodedAud> {
    if bytes.len() as u64 > limits.max_source_bytes {
        return Err(Error::AudLimit(format!(
            "{}-byte source exceeds limit {}",
            bytes.len(),
            limits.max_source_bytes
        )));
    }
    if bytes.len() < HEADER_BYTES {
        return Err(Error::InvalidAud(
            "sample is shorter than the 12-byte AUD header".into(),
        ));
    }

    let sample_rate = u16::from_le_bytes([bytes[0], bytes[1]]);
    let stored_size = read_positive_i32(&bytes[2..6], "stored size")? as usize;
    let output_size = read_positive_i32(&bytes[6..10], "uncompressed size")? as usize;
    let flags = bytes[10];
    let compression = bytes[11];
    if flags & !KNOWN_FLAGS != 0 {
        return Err(Error::InvalidAud(format!(
            "unknown audio flags 0x{:02x}",
            flags & !KNOWN_FLAGS
        )));
    }
    if stored_size == 0 || output_size == 0 {
        return Err(Error::InvalidAud(
            "stored and uncompressed sizes must be non-zero".into(),
        ));
    }
    if sample_rate < limits.minimum_sample_rate
        || u32::from(sample_rate) > limits.maximum_sample_rate
    {
        return Err(Error::InvalidAud(format!(
            "sample rate {sample_rate} is outside {}..={} Hz",
            limits.minimum_sample_rate, limits.maximum_sample_rate
        )));
    }
    if output_size as u64 > limits.max_output_bytes {
        return Err(Error::AudLimit(format!(
            "{output_size}-byte decoded sample exceeds limit {}",
            limits.max_output_bytes
        )));
    }
    let expected_source_size = HEADER_BYTES
        .checked_add(stored_size)
        .ok_or_else(|| Error::InvalidAud("stored size overflow".into()))?;
    if expected_source_size != bytes.len() {
        return Err(Error::InvalidAud(format!(
            "header describes {expected_source_size} bytes but sample contains {}",
            bytes.len()
        )));
    }

    let channels = if flags & FLAG_STEREO != 0 { 2 } else { 1 };
    let bits_per_sample = if flags & FLAG_16_BIT != 0 { 16 } else { 8 };
    let frame_bytes = channels * (bits_per_sample / 8);
    if output_size % frame_bytes != 0 {
        return Err(Error::InvalidAud(
            "uncompressed size is not aligned to complete PCM frames".into(),
        ));
    }

    let data = &bytes[HEADER_BYTES..];
    let pcm = match compression {
        0 => {
            if stored_size != output_size {
                return Err(Error::InvalidAud(
                    "uncompressed AUD stored and output sizes differ".into(),
                ));
            }
            data.to_vec()
        }
        1 => {
            if channels != 1 || bits_per_sample != 8 {
                return Err(Error::InvalidAud(
                    "Westwood compression 1 is supported only for mono 8-bit PCM".into(),
                ));
            }
            decode_chunked(data, output_size, limits.max_chunks, ChunkCodec::Westwood)?
        }
        99 => {
            if channels != 1 || bits_per_sample != 16 {
                return Err(Error::InvalidAud(
                    "SOS/IMA compression 99 is supported only for mono 16-bit PCM".into(),
                ));
            }
            decode_chunked(data, output_size, limits.max_chunks, ChunkCodec::Ima)?
        }
        other => {
            return Err(Error::InvalidAud(format!(
                "unsupported AUD compression {other}; expected 0, 1, or 99"
            )))
        }
    };
    if pcm.len() != output_size {
        return Err(Error::InvalidAud(format!(
            "decoder produced {} bytes; header declares {output_size}",
            pcm.len()
        )));
    }
    let playback_rate = if (20_000..24_000).contains(&sample_rate) {
        22_050
    } else {
        u32::from(sample_rate)
    };
    Ok(DecodedAud {
        // Matches Play_Sample_Handle's normalization in the released engine.
        sample_rate: playback_rate,
        channels: channels as u16,
        bits_per_sample: bits_per_sample as u16,
        source_compression: compression,
        frames: (output_size / frame_bytes) as u64,
        pcm,
    })
}

fn read_positive_i32(bytes: &[u8], field: &str) -> Result<u32> {
    let raw = i32::from_le_bytes(bytes.try_into().expect("four-byte header field"));
    if raw < 0 {
        return Err(Error::InvalidAud(format!("{field} is negative")));
    }
    Ok(raw as u32)
}

#[derive(Clone, Copy)]
enum ChunkCodec {
    Westwood,
    Ima,
}

fn decode_chunked(
    data: &[u8],
    expected_output: usize,
    max_chunks: usize,
    codec: ChunkCodec,
) -> Result<Vec<u8>> {
    let mut cursor = 0_usize;
    let mut output = Vec::with_capacity(expected_output);
    let mut chunks = 0_usize;
    let mut ima = ImaState::default();
    while cursor < data.len() {
        chunks += 1;
        if chunks > max_chunks {
            return Err(Error::AudLimit(format!(
                "AUD contains more than {max_chunks} chunks"
            )));
        }
        let header_end = cursor
            .checked_add(CHUNK_HEADER_BYTES)
            .ok_or_else(|| Error::InvalidAud("chunk header offset overflow".into()))?;
        if header_end > data.len() {
            return Err(Error::InvalidAud("truncated AUD chunk header".into()));
        }
        let stored = u16::from_le_bytes([data[cursor], data[cursor + 1]]) as usize;
        let decoded = u16::from_le_bytes([data[cursor + 2], data[cursor + 3]]) as usize;
        let magic = u32::from_le_bytes(
            data[cursor + 4..cursor + 8]
                .try_into()
                .expect("four-byte chunk magic"),
        );
        if magic != CHUNK_MAGIC {
            return Err(Error::InvalidAud(format!(
                "chunk {chunks} has magic 0x{magic:08x}, expected 0x{CHUNK_MAGIC:08x}"
            )));
        }
        if decoded == 0 {
            return Err(Error::InvalidAud(format!(
                "chunk {chunks} declares no output"
            )));
        }
        let payload_end = header_end
            .checked_add(stored)
            .ok_or_else(|| Error::InvalidAud("chunk payload offset overflow".into()))?;
        if payload_end > data.len() {
            return Err(Error::InvalidAud(format!(
                "chunk {chunks} payload extends beyond the sample"
            )));
        }
        let output_end = output
            .len()
            .checked_add(decoded)
            .ok_or_else(|| Error::InvalidAud("chunk output length overflow".into()))?;
        if output_end > expected_output {
            return Err(Error::InvalidAud(format!(
                "chunk {chunks} exceeds declared uncompressed size"
            )));
        }
        let payload = &data[header_end..payload_end];
        if stored == decoded {
            output.extend_from_slice(payload);
        } else {
            match codec {
                ChunkCodec::Westwood => decode_westwood_chunk(payload, decoded, &mut output)?,
                ChunkCodec::Ima => decode_ima_chunk(payload, decoded, &mut ima, &mut output)?,
            }
        }
        if output.len() != output_end {
            return Err(Error::InvalidAud(format!(
                "chunk {chunks} produced an unexpected output size"
            )));
        }
        cursor = payload_end;
    }
    if chunks == 0 || output.len() != expected_output {
        return Err(Error::InvalidAud(format!(
            "AUD chunks produced {} of {expected_output} declared bytes",
            output.len()
        )));
    }
    Ok(output)
}

fn decode_westwood_chunk(input: &[u8], expected: usize, output: &mut Vec<u8>) -> Result<()> {
    const STEP_2: [i16; 4] = [-2, -1, 0, 1];
    const STEP_4: [i16; 16] = [-9, -8, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 8];
    let target = output
        .len()
        .checked_add(expected)
        .ok_or_else(|| Error::InvalidAud("Westwood output length overflow".into()))?;
    let mut cursor = 0_usize;
    let mut sample = 0x80_i16;
    while output.len() < target {
        let command = *input
            .get(cursor)
            .ok_or_else(|| Error::InvalidAud("truncated Westwood command".into()))?;
        cursor += 1;
        let count = (command & 0x3f) as usize;
        match command >> 6 {
            0 => {
                for _ in 0..=count {
                    let code = *input.get(cursor).ok_or_else(|| {
                        Error::InvalidAud("truncated Westwood 2-bit delta run".into())
                    })?;
                    cursor += 1;
                    for shift in [0, 2, 4, 6] {
                        push_sample(
                            output,
                            target,
                            &mut sample,
                            STEP_2[((code >> shift) & 0x03) as usize],
                        )?;
                    }
                }
            }
            1 => {
                for _ in 0..=count {
                    let code = *input.get(cursor).ok_or_else(|| {
                        Error::InvalidAud("truncated Westwood 4-bit delta run".into())
                    })?;
                    cursor += 1;
                    push_sample(output, target, &mut sample, STEP_4[(code & 0x0f) as usize])?;
                    push_sample(output, target, &mut sample, STEP_4[(code >> 4) as usize])?;
                }
            }
            2 if count & 0x20 != 0 => {
                let delta = (((count as u8) << 2) as i8 >> 2) as i16;
                push_sample(output, target, &mut sample, delta)?;
            }
            2 => {
                let run = count + 1;
                let end = cursor
                    .checked_add(run)
                    .ok_or_else(|| Error::InvalidAud("Westwood raw run overflow".into()))?;
                let bytes = input
                    .get(cursor..end)
                    .ok_or_else(|| Error::InvalidAud("truncated Westwood raw sample run".into()))?;
                if output.len() + run > target {
                    return Err(Error::InvalidAud(
                        "Westwood raw run exceeds chunk output size".into(),
                    ));
                }
                output.extend_from_slice(bytes);
                sample = i16::from(*bytes.last().expect("non-empty run"));
                cursor = end;
            }
            3 => {
                let run = count + 1;
                if output.len() + run > target {
                    return Err(Error::InvalidAud(
                        "Westwood repeat run exceeds chunk output size".into(),
                    ));
                }
                output.resize(output.len() + run, sample as u8);
            }
            _ => unreachable!(),
        }
    }
    if output.len() != target || cursor != input.len() {
        return Err(Error::InvalidAud(format!(
            "Westwood chunk consumed {cursor}/{} input bytes",
            input.len()
        )));
    }
    Ok(())
}

fn push_sample(output: &mut Vec<u8>, target: usize, sample: &mut i16, delta: i16) -> Result<()> {
    if output.len() >= target {
        return Err(Error::InvalidAud(
            "Westwood delta run exceeds chunk output size".into(),
        ));
    }
    *sample = (*sample + delta).clamp(0, 255);
    output.push(*sample as u8);
    Ok(())
}

#[derive(Default)]
struct ImaState {
    predictor: i32,
    index: usize,
}

fn decode_ima_chunk(
    input: &[u8],
    expected: usize,
    state: &mut ImaState,
    output: &mut Vec<u8>,
) -> Result<()> {
    if expected % 2 != 0 {
        return Err(Error::InvalidAud(
            "IMA chunk output is not aligned to 16-bit samples".into(),
        ));
    }
    let samples = expected / 2;
    let needed = samples.div_ceil(2);
    if input.len() != needed {
        return Err(Error::InvalidAud(format!(
            "IMA chunk needs {needed} bytes for {samples} samples, contains {}",
            input.len()
        )));
    }
    for sample_index in 0..samples {
        let byte = input[sample_index / 2];
        let nibble = if sample_index & 1 == 0 {
            byte & 0x0f
        } else {
            byte >> 4
        };
        let sample = decode_ima_nibble(nibble, state);
        output.extend(sample.to_le_bytes());
    }
    Ok(())
}

fn decode_ima_nibble(nibble: u8, state: &mut ImaState) -> i16 {
    const INDEX: [i32; 8] = [-1, -1, -1, -1, 2, 4, 6, 8];
    const STEP: [i32; 89] = [
        7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60,
        66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371,
        408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878,
        2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845,
        8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086,
        29794, 32767,
    ];
    let magnitude = usize::from(nibble & 7);
    let step = STEP[state.index];
    let mut delta = step * magnitude as i32 / 4 + step / 8;
    if nibble & 8 != 0 {
        delta = -delta;
    }
    state.predictor = (state.predictor + delta).clamp(i16::MIN as i32, i16::MAX as i32);
    state.index = (state.index as i32 + INDEX[magnitude]).clamp(0, 88) as usize;
    state.predictor as i16
}

#[cfg(test)]
mod tests {
    use super::*;

    fn aud(compression: u8, flags: u8, stored: &[u8], output_size: usize) -> Vec<u8> {
        let mut bytes = Vec::new();
        bytes.extend(22_050_u16.to_le_bytes());
        bytes.extend((stored.len() as i32).to_le_bytes());
        bytes.extend((output_size as i32).to_le_bytes());
        bytes.push(flags);
        bytes.push(compression);
        bytes.extend(stored);
        bytes
    }

    fn chunk(payload: &[u8], output_size: u16) -> Vec<u8> {
        let mut bytes = Vec::new();
        bytes.extend((payload.len() as u16).to_le_bytes());
        bytes.extend(output_size.to_le_bytes());
        bytes.extend(CHUNK_MAGIC.to_le_bytes());
        bytes.extend(payload);
        bytes
    }

    #[test]
    fn decodes_uncompressed_pcm_and_writes_a_canonical_wav() {
        let mut source = aud(0, 0, &[0, 64, 128, 255], 4);
        source[0..2].copy_from_slice(&22_051_u16.to_le_bytes());
        let decoded = decode_aud(&source, AudLimits::default()).unwrap();
        assert_eq!(decoded.pcm, [0, 64, 128, 255]);
        assert_eq!(decoded.frames, 4);
        assert_eq!(decoded.sample_rate, 22_050);
        let wav = decoded.to_wav().unwrap();
        assert_eq!(&wav[0..4], b"RIFF");
        assert_eq!(&wav[8..12], b"WAVE");
        assert_eq!(&wav[36..40], b"data");
        assert_eq!(&wav[44..48], &[0, 64, 128, 255]);
    }

    #[test]
    fn decodes_westwood_raw_delta_and_repeat_commands() {
        // Raw run: command 10xxxxxx followed by count + 1 literal samples.
        let raw = chunk(&[0x83, 10, 20, 30, 40], 4);
        let decoded = decode_aud(&aud(1, 0, &raw, 4), AudLimits::default()).unwrap();
        assert_eq!(decoded.pcm, [10, 20, 30, 40]);

        // Repeat the initial 0x80 sample four times.
        let repeat = chunk(&[0xc3], 4);
        let decoded = decode_aud(&aud(1, 0, &repeat, 4), AudLimits::default()).unwrap();
        assert_eq!(decoded.pcm, [128; 4]);

        let delta_2bit = chunk(&[0x00, 0xe4], 4);
        let decoded = decode_aud(&aud(1, 0, &delta_2bit, 4), AudLimits::default()).unwrap();
        assert_eq!(decoded.pcm, [126, 125, 125, 126]);

        let delta_4bit = chunk(&[0x40, 0xf0, 0xc1], 4);
        let decoded = decode_aud(&aud(1, 0, &delta_4bit, 4), AudLimits::default()).unwrap();
        assert_eq!(decoded.pcm, [119, 127, 127, 127]);

        let signed_delta = chunk(&[0xbf, 0xc1], 3);
        let decoded = decode_aud(&aud(1, 0, &signed_delta, 3), AudLimits::default()).unwrap();
        assert_eq!(decoded.pcm, [127, 127, 127]);
    }

    #[test]
    fn decodes_sos_ima_adpcm_and_preserves_state_between_chunks() {
        let mut stored = chunk(&[0x77], 4);
        stored.extend(chunk(&[0x88], 4));
        let decoded = decode_aud(&aud(99, FLAG_16_BIT, &stored, 8), AudLimits::default()).unwrap();
        assert_eq!(decoded.bits_per_sample, 16);
        assert_eq!(decoded.frames, 4);
        assert_eq!(decoded.pcm.len(), 8);
        assert_eq!(decoded.pcm, [12, 0, 42, 0, 38, 0, 35, 0]);
    }

    #[test]
    fn rejects_bad_lengths_magic_flags_and_decompression_bombs() {
        let mut bad_magic = chunk(&[0xc0], 1);
        bad_magic[4] = 0;
        assert!(decode_aud(&aud(1, 0, &bad_magic, 1), AudLimits::default()).is_err());
        assert!(decode_aud(&aud(7, 0, &[0], 1), AudLimits::default()).is_err());
        assert!(decode_aud(&aud(1, 0x80, &[0], 1), AudLimits::default()).is_err());

        let limits = AudLimits {
            max_output_bytes: 3,
            ..AudLimits::default()
        };
        assert!(decode_aud(&aud(0, 0, &[0; 4], 4), limits).is_err());
    }
}
