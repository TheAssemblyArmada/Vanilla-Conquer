# Browser architecture

This document describes the current Tiberian Dawn campaign source slice and the
target product boundary. GDI/Nod campaign conversion, canonical progression,
real mission launch, and the construction/tool UI are wired; enhanced assets,
delta coverage beyond the classic surface and retained static map, objective
review beyond Mission 4, semantic radar, and physical-device/human-played
acceptance remain open. The hosted freeware path now has automated real-Wasm
launch, save, continuation, offline, mobile-viewport, and performance gates. See
[implementation status](implementation-status.md) for the evidence boundary.

## Runtime boundary

The released game code is used as a deterministic simulation, not as a browser
UI. Tiberian Dawn and Red Alert remain separate modules. Each module exposes a
fixed-width C ABI that starts a scenario, receives high-level commands,
advances exactly one 15 Hz tick, emits a pointer-free binary snapshot, drains
events, and serializes save data.

The current browser/native negotiation boundary is ABI v2. This does not rename
or revise the packed message family: the protocol version and `StartV1`,
`CommandBatchV1`, `SnapshotV1`, `EventV1`, and outer `WebSaveV1` layouts remain
version 1. The ABI bump makes history-aware snapshot retention an explicit
capability while keeping persisted saves and command/event framing compatible.

The existing remaster DLL interface is deliberately kept behind that adapter.
Its packed structs contain native pointers, reference parameters, compiler ABI
types, and variable-length tails that are not safe as a JavaScript contract.

The simulation runs in a dedicated worker. The worker owns WebAssembly memory,
ticks at 15 Hz, transfers reusable snapshot buffers to the UI thread, and stops
advancing after a terminal victory/defeat event. The UI renders the latest
snapshot at display cadence. The classic surface uses dirty-rectangle deltas;
STATIC_MAP uses a full/retained representation; deltas for the remaining
snapshot sections and visual interpolation are still planned.

A bootstrap after start/load uses `base_tick == tick` and carries the complete
STATIC_MAP metadata and row-major cell records. Each later materialized
snapshot names the preceding materialized snapshot tick as `base_tick`; transfer
backpressure may therefore skip simulation ticks without breaking the chain.
When the logical static map is unchanged, RETAINED sends the fixed 304-byte
metadata and keeps the base's cell array instead of retransmitting 36 bytes per
cell. A metadata, dimension, order, template, or icon change sends a FULL
replacement. The client rejects RETAINED when the named base is unavailable,
and start/load clears retained history. Canonical hashing always covers the
complete logical payload, so the section hash for one logical state is the
same whether it is encoded FULL or RETAINED; whole-snapshot hashes still
include the simulation tick.

Campaign transition state uses an additive C ABI call rather than changing the
version-1 start message. Before a continued campaign start, the host supplies
the original raw cash balance and three-bit Nod nuke-piece state. The start
message still carries the scoped sabotage result and exact terminal RNG. On
victory, a campaign-outcome event is emitted immediately before game over and
contains those values plus scenario, house, and canonical root, allowing the
host to correlate the boundary without running the legacy native campaign UI.

## Rendering

WebGL2 is the first production backend.

- Classic presentation uploads the engine's indexed map surface as an R8
  texture and the active 256-color palette as a second texture. The engine
  sends a full bootstrap after start/load, then a minimal bounding rectangle
  (or an empty unchanged delta) at each later snapshot. Full-image canonical
  hashing keeps deterministic state hashes independent of delta history; the
  hash also incorporates engine-private RNG/first-update state that is not
  exposed as a render section.
- The renderer has a sorted, instanced sprite path and atlas-page upload API,
  which the generated demo exercises. The owned-content converter does not yet
  produce enhanced atlases or asset tables, so real mission launches are
  intentionally locked to classic mode.
- Menus, dialogs, the mission panel, settings, import flows, and accessible
  controls are DOM UI. React is not used in the frame loop. The current mission
  panel presents package/mission selection, briefing, diagnostics, storage,
  save controls, engine-exported battle telemetry, and a downsampled live view
  of the classic surface. Its construction section groups engine-exported
  production entries and exposes start, hold, resume, cancel, completed-building
  placement, repair, sell, and superweapon targeting. Placement is drawn in a
  separate canvas overlay from the engine's expanded cell grid; ownership and
  capability-aware object/wall hit tests keep prospective targets aligned with
  the adapter contract. Contextual battlefield targeting parses the snapshot's
  variable-length cell occupiers and resolves visible roots with the native
  current-plus-eight-cell order, center/footprint distance, cutoff, and airborne
  altitude projection; pointer updates are coalesced to animation frames,
  snapshot-driven hover refresh is limited to the 5 Hz DOM cadence, and each
  evaluated snapshot reuses an identity map. The accessible control-group panel derives membership,
  selection, and group centroids from the latest snapshot. It supports all ten
  groups through DOM controls and keyboard 1–0 selection, Ctrl assignment,
  Shift additive selection, and repeat-to-center behavior. Q/E switch
  select/order mode and X stops the current selection; hotkeys do not escape
  editable controls or dialogs. The full-surface minimap refresh is limited to
  roughly 1 Hz with immediate timeline-boundary updates; it is not yet a
  semantic radar implementation.

The first implementation does not require WebGPU, Asyncify, Emscripten
pthreads, SharedArrayBuffer, or cross-origin isolation.

## Content flow

A release build can create a music-free, 25-mission GDI package from a
hash-pinned normalized mirror of EA's Tiberian Dawn freeware data. The
repository script downloads and verifies that source outside Git, while the
Rust converter supplies neutral catalog briefings for stripped missions and
emits no C&C music or movies. A local command-line companion also supports an
optional installation chosen by the user. Its full GDI and Nod profiles locate
`Data/CNCDATA/TIBERIAN_DAWN/CD1` or `CD2`
case-insensitively, validate MIX indexes and file bounds, and apply the engine's
archive resolution order independently to each selected INI and BIN. They
extract the 25 unique mission descriptors for GDI scenarios 1-15 or Nod
scenarios 1-13 in legacy map-selection order. Each catalog entry contains its
canonical scenario root, scenario/variation/direction/build level, locally
derived briefing, and INI-derived theater. Only fixed temperate, desert, and
winter theater archive pairs encountered by the selected catalog are included;
matching loose palettes are optional. The original single GDI mission profile
remains the default compatibility and guarded-acceptance path. The companion
also decodes supported AUD entries to PCM WAV, writes versioned runtime
catalog/audio metadata, and creates a verified `.cncweb` package. It does not
contact a content service itself. MEG inspection is implemented for installation
validation and later enhanced-data conversion, but the current profiles do not
convert remaster textures.

At startup, a deployment may expose the strict same-origin
`classic-freeware-v1.json` descriptor beside the application. The browser pins
the outer archive's byte length/SHA-256 and its inner package identity, content
digest, and freeware provenance. An exact installed revision avoids another
download; absence or failure falls back to the synthetic demo. The PWA installs
hosted or manually imported packages transactionally into Origin Private File
System storage and commits a small index only after all declared files pass independent
size and SHA-256 verification. On mission launch the worker checks the selected
physical revision again, opens only `engine/td/*` as OPFS `File` objects, and
holds a shared package-revision lock while those blobs remain mounted read-only
with Emscripten WORKERFS. Immutable catalog and browser-audio reads join that
shared lock; installs, replacements, and removals are exclusive, so lazy audio
cannot deadlock behind its own active mission. Corrupt indexes are reported per
item, and corrupt save payloads are quarantined so valid packages and older
saves remain usable. Library inspection also cross-checks every catalog theater
against its bounded, hash-verified loose scenario INI before marking a pack
compatible. Browser-native WAV files are read separately by Web Audio.
Saves use separate, versioned storage and are never written into an imported
content pack. Replay storage is a future extension.

The mission `StartV1` includes both the mounted content root and a stable hash
of the package revision. The engine derives the expected canonical loose root
from the start tuple, reads the bounded mission INI, and requires exactly one
supported `[Map] Theater` declaration. It then preflights the exact loose
INI/8192-byte BIN and fixed theater data/icon pair before initializing global
game state. It never uses the INI value as a path fragment. An optional loose
palette is validated when present and reported as a warning when absent.
Failures are recoverable and emitted as structured runtime diagnostics rather
than partial mission startup.

## Campaign progression

Campaign navigation lives in the browser host, while mission simulation and
carry semantics remain in the original TD engine. The host has a fixed copy of
the legacy `CountryArray` continuation graph and intersects its destinations
with exact missions in the active immutable catalog. It cannot accept a forged
catalog entry or invent an edge. This reproduces the direction/variation
choices for both factions, the GDI mission-six sabotaged-airstrip jump to
mission eight, and the GDI 15/Nod 13 endings.

The game-over callback alone is insufficient for a safe boundary, so the host
offers continuation only after the preceding campaign-outcome event matches
the active mission's scenario, root, faction/house, tick, and sabotage result.
Raw cash is passed to `Scen.CarryOverMoney` before the destination INI applies
its percentage/cap; the nuke-piece bits are restored after the new house is
constructed. The mission-six sabotage value is carried only into GDI mission
seven when that branch is used. The terminal RNG is preserved bit-for-bit,
including zero.

## Lifecycle and updates

The production service worker caches only first-party source-derived
application and engine files. Hosted/imported packages remain in OPFS and the
large `.cncweb` archive is not duplicated in the Cache API. The UI
allows a pending update to apply only while gameplay and content/save
transactions are inactive. Backgrounding pauses the single-player simulation,
writes an autosave, and foregrounding resumes without wall-clock catch-up.

The version-2 mission session record contains the package ID, immutable content
revision, mission ID, random seed, fresh campaign run ID, optional save ID,
optional incoming transition, and optional correlated pending victory. The run
ID isolates separate campaign attempts even when they revisit the same mission.
Startup will resume only a save whose complete run/revision/mission identity
matches; stale, replaced, or previous-run content cannot be silently paired
with it. Session epochs prevent a save operation started before restart or
continuation from later overwriting the remembered resume point. A narrow
compatibility path can consume a version-1 session and run-ID-less save once.

A pending victory is not a simulation save. It persists the matched game-over
and campaign-outcome records so refresh can restore the branch chooser without
starting or ticking terminal mission state. The schema forbids combining a
pending victory with a resume save and rejects mismatched ticks or sabotage
values. Choosing a continuation persists the next mission and its incoming
carry state before launch while retaining the same campaign run ID.

A deferred run gate makes ordinary startup plus resume atomic: a fresh mission
cannot advance before the matching save replaces it. The outer `WebSaveV1`
wire layout remains unchanged. Its TD-specific deterministic payload is now
`TDWS` v2, which retains the live RNG, first-update flag, and `SabotagedType`;
the decoder also accepts v1 and supplies the previously absent sabotage value
as none. Graceful worker disposal waits briefly for native shutdown and
mount-lock release.

## Audio and media

The engine emits named sound and speech callbacks across the fixed-width event
boundary. `runtime/audio-v1.json` maps those exact callback keys to locally
decoded WAV files. The browser lazily decodes them, pans positional sound
effects relative to the view, and serializes speech. A browser audio context is
unlocked on the first pointer or keyboard gesture. User/lifecycle pauses stop
active voices and drop paused-time cues; terminal victory/defeat audio is
allowed to finish.

C&C theme/music and VQA/movie media are excluded from distributable browser
packages. `SCORES.MIX` is optional and the WebAssembly build skips registration
when it is absent; movie callbacks become UI notices. Eventual music must use
original or independently licensed replacement assets.

## Performance boundary

The worker uses a reusable three-buffer pool and the WebGL renderer caps device
pixel ratio. Classic surface traffic is bounded to a full start/load bootstrap
plus minimal dirty rectangles. STATIC_MAP traffic is bounded to a full
bootstrap or logical change and otherwise only 304 bytes, rather than
`304 + 36 × cell_count`; the remaining simulation sections still materialize
complete payloads at 15 Hz. FPS is visible in the UI. A bounded rolling metrics
API exposes tick cadence, animation-frame intervals, snapshot and
transfer-buffer sizes, classic texture uploads, and browser long tasks for
owned-mission acceptance; representative-hardware evidence still has to be
captured with retail content, and the C01-C09 owned-content sequence has not
been run in this checkout.

## Multiplayer boundary

Networking is outside the first playable slice. The command and snapshot
protocols are transport-neutral so a later native Linux server can run the same
15 Hz simulation and exchange commands/state deltas over WebSocket. Deployment
of retail maps or other protected data on a server is a separate legal gate.
