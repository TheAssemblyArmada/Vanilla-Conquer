# Classic MIX reader notes

The mission converter implements the classic container consumed by
`MixFileClass` in the released Tiberian Dawn source.

## Supported layouts

- Plain header: 16-bit file count, 32-bit data size, then 12-byte index rows.
- Extended unencrypted header: zero, a 16-bit flags word, the plain header and
  index. The optional 20-byte digest trailer is bounds-checked.
- Index rows: 32-bit C&C filename hash, 32-bit data-relative offset and 32-bit
  size. Rows must be strictly ordered by the signed hash because the engine
  performs a signed binary search.

All arithmetic, archive lengths, entry lengths, offsets, duplicate hashes and
read sizes are checked. Extended encrypted indexes are reported explicitly and
are not supported by the browser-v1 converter. None of the CD1/CD2 files
required by the supported campaign profiles are expected to use encryption.

MIX files do not store names. `mix_name_hash` reproduces the engine's uppercase
ASCII `CRCEngine`; a caller can resolve only a name it already knows. Layered
resolution is first-match-wins and uses an explicit, deterministic layer list,
not directory enumeration order. Mission conversion places patch archives
(`UPDATE.MIX`, `UPDATA.MIX`, `UPDATEC.MIX`) before base data, so either half of
every selected INI/BIN pair shadows `GENERAL.MIX` when the patch contains the
same scenario hash.
