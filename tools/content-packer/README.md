# C&C Web content packer

This directory contains the local content-packaging foundation for the browser
port. It is intentionally asset-free: it does not ship, download, unlock, or
upload Command & Conquer data. The mission converter copies only the legacy
files needed by its selected profile, generates a versioned runtime catalog,
and packages everything locally. The repository-level freeware build script
handles a separately hash-pinned download and passes only its extracted
music-free base files to this tool.

The current deliverable is a reusable Rust library plus the
`cncweb-content` CLI. Its progress callback and structured diagnostics can be
wrapped by a desktop UI without changing the conversion graph.

## Build and test

Rust 1.85 or newer is required. The repository toolchain pins Rust 1.97.

```sh
cargo build --locked
cargo test --locked --all-targets
cargo clippy --locked --all-targets -- -D warnings
```

All tests generate synthetic archives at runtime. There are no fixture files
copied from a retail installation.

## CLI workflow

Validate the official archives and compute a deterministic installation
fingerprint:

```sh
cncweb-content validate-install "/path/to/CnC Remastered" \
  --profile tiberian-dawn --hash
```

Profiles are deliberately explicit:

- `map-editor` checks the five archives loaded by EA's published map editor.
- `tiberian-dawn` checks common data, TD textures, and `MOVIES_TD.MEG`.
- `red-alert` checks common data, RA textures, and `MOVIES_RA.MEG`.
- `collection` checks the union of both game profiles.

The validator accepts either the installation root or its `DATA` directory,
resolves known archive names case-insensitively, parses their metadata with
safety limits, and never writes to the installation. Full hashing is opt-in
because the archives can be large. Validation is a compatibility check, not an
entitlement mechanism.

Inspect an archive or extract one explicitly named entry:

```sh
cncweb-content meg-list DATA/CONFIG.MEG
cncweb-content meg-extract DATA/CONFIG.MEG \
  DATA/XML/TILESETS.XML ./work/tilesets.xml
```

Plan a campaign without writing anything. The selected path may be the
collection root, `Data`, `CNCDATA`, `TIBERIAN_DAWN`, or the matching `CD1`/`CD2`
directory:

```sh
cncweb-content plan-mission "/path/to/CnC Remastered" \
  --profile td-gdi-campaign
```

Conversion profiles are explicit:

- `td-gdi-campaign` selects the 25 unique GDI roots reachable through the
  legacy map-selection table and reads CD1.
- `td-nod-campaign` selects the 25 unique Nod roots reachable through that
  table and reads CD2.
- `td-gdi-01-east-a` remains the default compatibility profile and still emits
  exactly the original single GDI mission. The owned-content acceptance
  harness depends on that stable behavior.

### Zero-install classic-freeware release

From the repository root, build the current 25-mission GDI deployment sidecar:

```sh
./scripts/build-classic-freeware.sh .cache/classic-freeware
```

The script verifies the 7,911,636-byte OpenRA mirror archive against SHA-256
`a55b2c160b534f6d1b865ad6120e1f4fde8c418d47bb2fb1a9c72c586a5e1603`,
then invokes this CLI with `--source-product tiberian-dawn-freeware --provider
ea-freeware`. The converter accepts the normalized classic files, uses neutral
catalog briefing text where the stripped freeware package has none, and never
reads or packages `SCORES.MIX` or movies. `LOCAL.MIX`, `UPDATA.MIX`, and
`UPDATE.MIX` are optional in the browser profile. The current mirror supplies
temperate icons for the two winter missions, so the generated package aliases
that verified icon archive as `WINTICNH.MIX`; obtaining the authentic winter
icon archive is a presentation follow-up.

After conversion, `emit-freeware-bootstrap` verifies the entire package again,
rejects music/movie paths and non-freeware provenance, hashes the outer archive,
and writes the strict descriptor consumed by the browser:

```sh
cncweb-content emit-freeware-bootstrap ./classic-freeware-gdi-v1.cncweb \
  ./classic-freeware-v1.json \
  --archive-url ./classic-freeware-gdi-v1.cncweb
```

Place both generated files beside the deployed `index.html`. They are release
sidecars, not source files, and must never be committed. The current normalized
mirror contains the complete GDI campaign but only the first Nod mission; the
Nod zero-install campaign remains a later Gold-freeware extraction task.

Create the owned-content package directly. Progress is written to stderr and
the final conversion report is JSON on stdout; use `--json-progress` for
newline-delimited machine-readable progress:

```sh
cncweb-content convert-mission "/path/to/CnC Remastered" \
  ./td-gdi-campaign.cncweb \
  --profile td-gdi-campaign \
  --package-id td-gdi-campaign-en-us \
  --provider steam \
  --locale en-US
```

The converter finds the selected profile's
`Data/CNCDATA/TIBERIAN_DAWN/CD1` or `CD2` case-insensitively and emits this
stable runtime layout:

```text
runtime/catalog-v1.json
runtime/audio-v1.json
metadata/conversion-report-v1.json
audio/sfx/*.wav
audio/speech/*.wav
engine/td/CCLOCAL.MIX
engine/td/CONQUER.MIX
engine/td/GENERAL.MIX
engine/td/LOCAL.MIX
engine/td/SOUNDS.MIX
engine/td/SPEECH.MIX
engine/td/TRANSIT.MIX
engine/td/UPDATA.MIX
engine/td/UPDATE.MIX
engine/td/UPDATEC.MIX
engine/td/TEMPERAT.MIX       # required when a mission is TEMPERATE
engine/td/TEMPICNH.MIX       # matching theater-icon archive
engine/td/DESERT.MIX         # required when a mission is DESERT
engine/td/DESEICNH.MIX       # matching theater-icon archive
engine/td/WINTER.MIX         # required when a mission is WINTER
engine/td/WINTICNH.MIX       # matching theater-icon archive
engine/td/TEMPERAT.PAL       # copied when matching theater is encountered and file exists
engine/td/DESERT.PAL         # likewise optional
engine/td/WINTER.PAL         # likewise optional
engine/td/SCG01EA.INI        # one loose pair per selected GDI root
engine/td/SCG01EA.BIN
engine/td/SCB01EA.INI        # Nod profiles use SCB roots
engine/td/SCB01EA.BIN
```

Only roots selected by the profile are extracted. Each INI and BIN is resolved
independently with patch archives before base archives, then copied loose using
its canonical uppercase root. The converter derives `briefing` and `theater`
from each resolved INI. Every encountered theater requires its engine data and
icon archive pair: `TEMPERAT`/`TEMPICNH`, `DESERT`/`DESEICNH`, or
`WINTER`/`WINTICNH`. Matching loose palettes are optional because the palettes
are also addressable through theater data. Conversion metadata marks the
common and encountered theater archives as required; the runtime catalog
declares the canonical scenario root and theater from which consumers derive
the exact preflight inventory.

The runtime profile also requires the fonts/UI, patch layers, core object art,
speech, and sound archives needed by engine startup and classic mission
gameplay. `SCORES.MIX` and expansion-only `ZOUNDS.MIX` are not included; C&C
music is outside the browser package contract.

## Campaign/runtime contract

The campaign profiles describe content; they do not trust package metadata to
define progression. The browser owns a fixed copy of the original Tiberian
Dawn map-selection graph and intersects each canonical destination with exact
missions in the active immutable catalog. This preserves the GDI and Nod
direction/variation choices, the GDI mission-six sabotaged-airstrip skip, and
the GDI 15/Nod 13 endings. A missing catalog destination is unavailable rather
than replaced by a guessed edge.

At import/library inspection, the browser derives every loose INI/BIN and the
union of required theater archive pairs from the catalog, then re-parses each
INI to ensure its `[Map] Theater` matches the declared value. At mission start,
the native adapter independently derives the canonical scenario root from the
start tuple, reads at most 1 MiB of INI, requires exactly one supported theater
declaration, requires an 8192-byte BIN, and validates the corresponding fixed
theater data/icon pair. Theater text is matched against a closed table and is
never used to construct an arbitrary path.

After a correlated victory, the browser carries the engine-exported raw cash,
three Nod nuke-piece bits, scoped GDI sabotage state, and live scenario RNG to
the chosen destination. This progression logic and the multi-mission profiles
have asset-free synthetic coverage. They have not been exercised here against
a legally owned retail installation.

`MOVIES.MIX` is intentionally omitted: the retail file exceeds
the browser-v1 per-file cap, and the browser engine uses the remaster movie
callback boundary instead of reading VQA during gameplay. Enhanced remaster
textures/atlases are also not produced yet; CONFIG and texture MEG conversion
is an independent follow-up pipeline. SHP, ICN, and other visual classic
entries remain inside their MIX containers. Candidate AUD entries are decoded
locally into browser-native PCM WAV while the original MIX remains available
to the engine.

These profiles support the `en-US` classic content locale only; localized
Remastered speech/text MEG conversion remains unsupported.

After conversion, follow the browser workflow in
[`../../web/README.md`](../../web/README.md): build/serve the Wasm web app,
choose **Import pack**, and select the generated file. The browser verifies the
container independently, stores it in origin-private storage, rechecks and
mounts only `engine/td/*` in its worker, and launches the catalog mission. Keep
the generated package outside this repository and do not upload or share it.
For the complete guarded conversion, integrated build, route probes, and
manual victory/save/refresh/offline checklist, use the repository's
[`owned-content acceptance guide`](../../docs/owned-content-acceptance.md).
That harness intentionally converts `td-gdi-01-east-a`, not a full campaign,
so its exact mission preflight remains stable. No owned game data is present in
this checkout, and the real owned-content C01-C09 sequence remains unrun.

For development or a future independent conversion stage, package a populated
staging directory using the fingerprint emitted by validation. `pack` always
targets the browser-v1 import profile; it inventories sizes before hashing and
fails without creating output if the staging tree cannot be imported by the
browser:

```sh
cncweb-content pack ./staging ./td-slice.cncweb \
  --package-id td-slice-en-us \
  --source-fingerprint 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef \
  --provider copied-installation \
  --game tiberian-dawn \
  --locale en-US
```

Inspecting checks the manifest, paths, declared sizes, entry types, compression
methods, and resource ceilings. Verification additionally streams every byte
through SHA-256. Extraction performs the same verification in a fresh sibling
directory and renames it into place only after every file succeeds:

```sh
cncweb-content inspect ./td-slice.cncweb
cncweb-content verify ./td-slice.cncweb
cncweb-content extract ./td-slice.cncweb ./imported/td-slice
```

Existing package files and extraction directories are never overwritten.

## Security model

- MEG counts, lengths, table arithmetic, string indices, entry indices, data
  offsets, and reads are bounds-checked before allocation or seeking.
- Package paths use one portable `/`-separated namespace. Absolute paths,
  traversal, backslashes, empty components, Windows device names, control
  characters, and case-fold collisions are rejected.
- ZIP directories, symlinks, devices, encryption, undeclared entries, duplicate
  paths, unsupported compression, oversized entries, excessive total output,
  and extreme compression ratios are rejected before extraction.
- Every content file is size- and SHA-256-verified. Package creation hashes the
  staging file twice (inventory and write) so a concurrent change cannot make
  the manifest describe different bytes.
- ZIP64 size fields are emitted for every created entry, avoiding a late failure
  if future packs cross the classic 4 GiB limit. Creation and extraction use
  hidden temporary siblings and atomic rename.

The default browser-v1 ceilings are 100,000 content entries, 4 MiB of manifest
JSON, 64 MiB per file, 2 GiB total uncompressed content, and a 1,000:1
compression ratio. They intentionally match the browser importer while it
materializes one entry at a time for hashing. Package creation also inspects
the completed temporary ZIP before publishing it, so an unsafe actual
compression ratio cannot produce a browser-incompatible output.

The machine-readable
[`browser-package-profile-v1.json`](fixtures/browser-package-profile-v1.json)
fixture is checked by both the Rust and TypeScript test suites. The library
exposes `BROWSER_PACKAGE_LIMITS_V1`, `PackageLimits::browser_v1()`, `MegLimits`,
and customizable `PackageLimits`; callers that loosen the package limits are
creating an offline/custom archive that is not guaranteed to import in the web
runtime.

See [MEG format notes](docs/meg-format.md), the
[classic MIX notes](docs/mix-format.md), the
[browser-native audio contract](docs/audio-v1.md), the
[`.cncweb` v1 contract](docs/cncweb-v1.md), the authoritative
[package JSON Schema](schema/manifest-v1.schema.json), the
[classic-freeware bootstrap schema](schema/classic-freeware-v1.schema.json), the
[runtime catalog contract](docs/runtime-catalog-v1.md), and a
[validated example manifest](fixtures/manifest-v1.example.json). The schema and
example are the integration contract for browser consumers.

## Provenance and licensing

The MEG layout is based on EA's GPL-licensed official map-editor implementation:

- [`Megafile.cs`](https://github.com/electronicarts/CnC_Remastered_Collection/blob/master/CnCTDRAMapEditor/Utility/Megafile.cs)
- [`MegafileBuilder.cs`](https://github.com/electronicarts/CnC_Remastered_Collection/blob/master/CnCTDRAMapEditor/Utility/MegafileBuilder.cs)

This implementation is written in Rust with additional validation and streaming
I/O; it contains no EA game data. It is distributed under the repository's GPL
terms. Any public product must also comply with the upstream additional terms
and obtain the legal review described in the project plan.
