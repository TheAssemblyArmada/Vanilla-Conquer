# `.cncweb` package format v1

A `.cncweb` file is a ZIP archive with ZIP64 size fields and exactly one
top-level `manifest.json`. Directory entries are not used. File names are UTF-8,
relative, `/`-separated portable paths.

## Manifest

The v1 manifest has this shape (digest values abbreviated here only):

```json
{
  "format": "cncweb-content",
  "version": 1,
  "package_id": "td-slice-en-us",
  "created_at_unix_ms": 1700000000000,
  "source": {
    "product": "cnc-remastered-collection",
    "provider": "copied-installation",
    "install_fingerprint_sha256": "<64 lowercase hex characters>"
  },
  "content": {
    "games": ["tiberian-dawn"],
    "locales": ["en-US"]
  },
  "content_sha256": "<64 lowercase hex characters>",
  "files": [
    {
      "path": "config/tilesets.json",
      "size": 1234,
      "sha256": "<64 lowercase hex characters>",
      "role": "configuration"
    }
  ]
}
```

Unknown manifest fields are rejected. `files` is strictly sorted by path and
must declare every non-manifest ZIP entry exactly once. Its allowed roles are
`engine-data`, `texture-atlas`, `audio`, `video`, `map`, `configuration`, and
`other`. Package creation infers a role from the first staging path component;
a future converter API can supply more precise roles without changing the wire
schema.

`content_sha256` is initialized with the bytes
`CNCWEB-CONTENT-MANIFEST-V1` followed by one zero byte. For every sorted file,
the hasher then receives its path byte length as little-endian `u64`, UTF-8 path
bytes, size as little-endian `u64`, 32 raw content-digest bytes, role-name byte
length as little-endian `u64`, and UTF-8 role-name bytes. This authenticates the
file inventory against accidental mutation; it is not a signature. Each file
digest is verified against the decompressed bytes during verification/import.

The installation fingerprint is a domain-separated SHA-256 over the required
archive logical name, byte size, and full-file SHA-256 for the selected profile.
It lets the client identify which locally owned source set produced a pack
without embedding absolute install paths or retail bytes.

Manifest v1 also accepts the paired freeware provenance values
`product: "tiberian-dawn-freeware"` and `provider: "ea-freeware"`. No other
provider may be paired with that product, and `ea-freeware` may not be paired
with `cnc-remastered-collection`. The legacy field name
`install_fingerprint_sha256` is retained for wire compatibility; for freeware
it fingerprints the normalized, hash-pinned source archive set.

## Import requirements

Importers must enforce the same path and resource rules before writing any
output, reject undeclared or missing files, and verify decompressed size and
SHA-256. The native implementation extracts to a fresh sibling directory and
atomically renames it. The browser importer applies the equivalent transaction
using a temporary OPFS namespace, verifies every declared byte, and commits the
package index last.

The browser-v1 resource profile is part of the current producer/consumer
contract:

- at most 100,000 content entries, plus `manifest.json`;
- at most 4 MiB of manifest JSON;
- at most 64 MiB per ZIP entry;
- at most 2 GiB total uncompressed content, excluding `manifest.json`; and
- at most a 1,000:1 uncompressed-to-compressed ratio per non-empty entry.

The canonical numeric values live in
[`../fixtures/browser-package-profile-v1.json`](../fixtures/browser-package-profile-v1.json).
The CLI's `pack`, `inspect`, `verify`, and `extract` commands use this profile by
default. A library caller may select different `PackageLimits`, but such an
archive is outside the direct-browser compatibility contract.

Only `stored` and `deflate` entries are accepted in v1. Already-compressed
textures, images, audio, and VQA media are stored; other staged files use
deflate. Retail files are never automatically placed in a service-worker cache,
uploaded, or distributed with the application.
