# Browser-native audio v1

Mission conversion writes `runtime/audio-v1.json` independently of the mission
catalog. The web client discovers this exact path while inspecting an imported
package; it must not infer asset paths from the legacy MIX files.

The authoritative synthetic example is
[`fixtures/audio-v1.example.json`](../fixtures/audio-v1.example.json) and the
machine-readable contract is
[`schema/audio-v1.schema.json`](../schema/audio-v1.schema.json). Unknown fields
are rejected.

## Event keys

The converter's tables reproduce the active entries and numeric order of
`SoundEffectName[VOC_COUNT]` and `Speech[VOX_COUNT]` in the released
`tiberiandawn/audio.cpp`.

- Normal sound callbacks use the uppercase base name, such as `MGUN2` or
  `BUTTON`. Their source MIX entry has the `.AUD` suffix.
- Infantry variation callbacks include the exact suffix emitted by the engine:
  `ACKNO.V00`, `ACKNO.V01`, `ACKNO.V02`, or `ACKNO.V03`. All four are probed.
- Speech callbacks use the uppercase base name, such as `ACCOM1`; their source
  entry is `ACCOM1.AUD`.

Lookup is an exact, case-sensitive match on the pair `(kind, eventName)`.
`eventIds` records the callback's numeric enum indexes and handles the one
released duplicate base (`BLEEP2`) without duplicating its WAV.

Each decoded asset records its WAV path, source archive/name/compression,
sample rate, channel count, sample width, PCM frame count, and SHA-256. Paths
are lowercase and live below `audio/sfx/` or `audio/speech/`.

## Resolution and completeness

Speech candidates resolve only from `SPEECH.MIX`; sound and infantry-variation
candidates resolve only from `SOUNDS.MIX`. This preserves the archive identity
used by the engine callbacks even if both archives contain the same entry
name. A missing individual candidate is counted and skipped. A present but
malformed or unsupported candidate is listed in
`diagnostics.decodeFailures` without copying its bytes.

Package creation still fails unless decoded assets cover all of these first
mission groups:

- a GDI weapon;
- interface feedback;
- an explosion;
- a unit-response variant;
- mission-accomplished and mission-failed speech;
- gameplay speech such as reinforcements or unit ready.

This gate prevents a structurally valid but effectively silent package.

## AUD decoding

The decoder follows the released `common/audio.h`, `auduncmp.cpp`,
`soundio_common.cpp`, and `soscodec.cpp` implementations. It bounds source and
decoded sizes, chunk counts, all offset arithmetic, frame alignment, chunk
magic, and exact input/output consumption before emitting canonical RIFF/WAVE
PCM.

Supported source compression:

- `0`: uncompressed 8- or 16-bit PCM, mono or stereo;
- `1`: Westwood 8-bit delta compression, mono;
- `99`: SOS/IMA ADPCM, mono 16-bit, with predictor state preserved across
  chunks.

Unsupported audio is diagnosed precisely: stereo or 16-bit compression-1,
stereo or 8-bit compression-99, unknown compression identifiers, malformed
chunks, and samples exceeding the configured ceilings. Juvenile `.JUV` assets
are not candidates because the campaign profiles do not enable juvenile
mode. Music in `SCORES.MIX`, movie/VQA audio, and localized Remastered audio
banks are not converted by audio-v1.
