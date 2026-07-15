# Remastered MEG reader contract

The parser follows the layout emitted and consumed by EA's published C&C
Remastered map editor. All integers are little-endian.

## Remastered header

| Offset | Type | Meaning |
| ---: | --- | --- |
| 0 | `u32` | `0xffffffff` or `0x8fffffff` magic |
| 4 | `f32` | format version (`0.99` in the official builder) |
| 8 | `u32` | declared end of header/metadata |
| 12 | `u32` | file-record count |
| 16 | `u32` | string count |
| 20 | `u32` | string-table byte size |

EA's reader also accepts a legacy form whose first four bytes are an opaque
prefix and whose three counts begin at offset 4. The Rust reader retains this
compatibility while applying the same limits.

The string table follows the counts. Each string is a `u16` byte length followed
by that many UTF-8 bytes. Retail names are normally uppercase ASCII with
backslash separators. The parser canonicalizes separators to `/` and rejects
unsafe paths. The declared table size must be consumed exactly.

## Packed file record

Records immediately follow the string table and are 20 bytes each (`Pack = 2`
in the official C# source):

| Field | Type |
| --- | --- |
| flags | `u16` |
| CRC value | `u32` |
| subfile index | `i32` |
| subfile size | `u32` |
| absolute data offset | `u32` |
| string-table index | `u16` |

The reader requires every subfile index to be unique and within the record
count, every name index to exist, and every `offset + size` range to remain
inside the archive and outside its metadata. Case-insensitive logical-name
collisions are rejected because they are ambiguous on supported platforms.

The stored CRC is exposed as metadata but is not treated as a security digest;
package content uses SHA-256.

## Deliberate differences from the official reader

EA's reader memory-maps ranges after trusting most header fields. This reader
checks all arithmetic and resource limits first, streams payloads, limits
in-memory entry reads, rejects traversal names, and reports malformed data as
recoverable errors. It does not write MEG archives or mutate an installation.
