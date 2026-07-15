# Theater Runtime web vertical slice

Neutral-branded React/Vite PWA shell for the browser port. With a compatible
same-origin classic-freeware sidecar, it performs a zero-install verified
bootstrap, mounts the content read-only, and launches the real Tiberian Dawn
engine in a dedicated worker. The source implementation
supports the canonical 25-descriptor GDI and Nod campaign catalogs as well as
the stable one-mission GDI compatibility profile. Without a compatible package
it labels and runs a deterministic synthetic demo fallback.

## Run it

Build the browser engine from the repository root after sourcing the pinned
Emscripten SDK environment:

```sh
cmake --workflow --preset web-td
```

Then install and run the web workspace:

```sh
corepack pnpm install --frozen-lockfile
corepack pnpm dev
corepack pnpm typecheck
corepack pnpm test
corepack pnpm test:bundle-budget
corepack pnpm test:classic-freeware:deployment-tools
corepack pnpm verify:classic-freeware:mission-one
corepack pnpm verify:classic-freeware:mission-two
corepack pnpm verify:classic-freeware:mission-three
corepack pnpm build
corepack pnpm check:bundle-budget
```

If `../build/web-td/tiberiandawn.js` and `.wasm` exist, the production build
includes them under `dist/engine/` and the service worker treats them as
optional source-derived offline assets. The Vite development server serves the
same artifacts at `/engine/`; it returns an actionable 503 if only part of the
engine build exists. A source-only production build is permitted for
demo/testing, while `REQUIRE_BROWSER_ENGINE=1 corepack pnpm build` requires and
bundles both artifacts.

The deterministic bundle budget checker measures raw and level-9 gzip totals
for app JavaScript/CSS, the simulation worker, and—when present—the engine
JavaScript/Wasm pair. `corepack pnpm check:bundle-budget:integrated` requires
the pair. With Playwright Chromium installed, `corepack pnpm test:performance`
repeats the source-only demo's startup, 15 Hz tick, RAF-callback cadence, and long-task
gate twice. See the repository-level
[browser performance gates](../docs/browser-performance-gates.md) for budgets,
calibration values, and the evidence boundary.

Source builds contain no EA game data. To attach the GDI classic-freeware
campaign to an integrated build, run this from the repository root after the
web build:

```sh
./scripts/build-classic-freeware.sh web/dist
```

The browser fetches `classic-freeware-v1.json`, requires a same-origin archive
with the declared size/SHA-256, independently validates its inner identity and
every file hash, then commits it to OPFS. See the
[classic-freeware deployment guide](../docs/classic-freeware.md). An uploaded
candidate can be checked from this directory with:

```sh
export CNCWEB_CLASSIC_FREEWARE_BASE_URL="https://<host>/<release-directory>/"
corepack pnpm test:classic-freeware:staging
```

That command verifies the remote HTTP graph, security/cache headers, package
integrity, and local `dist` parity before starting the cross-browser matrix.
The [static-host contract](../docs/staging-deployment.md) lists the exact
requirements. With the integrated build and classic-freeware sidecar already
in `dist`, the long opt-in visible-control Mission 1 acceptance is:

```sh
corepack pnpm test:classic-freeware:genuine-victory
```

It follows a fixed scenario-specific patrol with ordinary pointer/keyboard
controls, finds candidate targets in composited battlefield screenshots, and
validates them through rendered DOM status; it does not inspect simulation
snapshots or invoke debug, acceptance, or forced-victory APIs. It wins Mission
1 naturally and verifies the visible continuation into GDI Mission 2 (East
A). The authored patrol makes this visible-only target-acquisition evidence,
not an autonomous fog-of-war playthrough. It is not part of the default fast
test set. Mission 2 has a separate, longer ordinary-control acceptance:

```sh
corepack pnpm test:classic-freeware:genuine-mission-two
```

That route trains the real Mission 2 force through visible production controls,
uses only rendered targeting/order evidence during the assault, and reaches the
authoritative victory. The fast release suite independently verifies the
Mission 2 → Mission 3 handoff, including offline startup when the optional
audio-index read remains unavailable through all bounded OPFS retries.

Mission 3 also has a deterministic normal-command engine acceptance:

```sh
corepack pnpm verify:classic-freeware:mission-three
```

The shared `scripts/verify-classic-freeware-mission-one.mjs` verifier deploys
the MCV, constructs and places Power Plant, Barracks, and Refinery, trains
infantry, completes the reviewed scout and assault routes, and requires both
the authoritative win and zero remaining counted Nod combatants. It does not
use the debug victory hook.

You may also generate a personal package with the companion documented in
[`../tools/content-packer/README.md`](../tools/content-packer/README.md), open
the local development or production URL, and choose **Import pack**. Import
first confirms the archive size, available origin-private quota, local-only
storage, and clearing-site-data consequences, then validates the package and
commits it to OPFS before launch. A package that does not satisfy the runtime
catalog, audio index, per-mission INI/BIN set, or catalog-derived theater
archives is listed as incompatible with its reason. The repository-level
[owned-content acceptance guide](../docs/owned-content-acceptance.md) provides
the integrated build/serve and victory/save/refresh/offline procedure.

The compatible package launches automatically after import. On later visits,
the app selects the remembered package revision and mission, then resumes the
requested or newest matching save. Replacing a package cannot silently attach
an old save because saves include package ID, content revision, and mission ID.

## Integration boundaries

- `src/simulation/protocol.ts` defines main-thread/worker messages and
  normalized command batches. The browser/native ABI negotiation version is 2;
  the packed message protocol and named `StartV1`, `CommandBatchV1`,
  `SnapshotV1`, `EventV1`, and `WebSaveV1` layouts remain version 1.
- `src/simulation/snapshot.ts` owns the bounds-checked `SnapshotV1` binary
  layout consumed by WebGL. A full STATIC_MAP payload establishes the cell
  baseline after start/load or a logical map change. An unchanged retained
  payload carries only its fixed 304-byte metadata and reuses the complete cell
  array from the preceding materialized `base_tick`; a missing or mismatched
  base is rejected.
- `src/simulation/WasmCore.ts` adapts the generated modular Emscripten ES
  module to the worker's `SimulationCore` interface. Initialize it with the
  JavaScript loader URL (for example, `./engine/tiberiandawn.js`); the
  accompanying `.wasm` file is resolved by Emscripten and cannot be loaded
  directly.
- `src/simulation/runtimeLibrary.ts` validates installed runtime catalogs,
  audio indexes, every declared mission pair, and the union of required theater
  archives before a package can be selected.
- `src/simulation/contentMount.ts` rechecks the immutable physical OPFS
  revision, verifies engine-file size and SHA-256, and mounts OPFS-backed blobs
  read-only through Emscripten WORKERFS before engine initialization.
- `src/storage/ContentStore.ts` mirrors the Rust packer's `ManifestV1`,
  including its aggregate content digest, and commits immutable OPFS revisions
  by updating an index last.
- `src/storage/PackageImporter.ts` imports ZIP64 `.cncweb` files with
  entry/path/size/ratio/hash checks. Until incremental hashing and streaming
  OPFS writes are implemented, individual compressed entries are capped at 64
  MiB to avoid multi-gigabyte allocations on mobile.
- `src/bootstrap/classicFreewareBootstrap.ts` owns the strict same-origin,
  hash-pinned zero-install descriptor/archive acquisition contract.
- `src/storage/SaveStore.ts` uses revisioned data and an index-last commit for
  manual/autosave data, separate from imported content. Mission saves are bound
  to package ID, immutable revision, mission ID, and campaign run ID.
- `src/audio/RuntimeAudio.ts` consumes exact engine sound/speech callback names
  from `runtime/audio-v1.json` and lazily reads the corresponding WAV from
  OPFS. If the launch-time audio read remains unavailable after bounded retries,
  the mission reports degraded audio and continues starting.

Extracted-folder import is exposed only as a development aid. Production
manual import uses the `.cncweb` picker; the classic-freeware path bootstraps
its same-origin sidecar without a picker.

The production service worker caches only the application shell and
source-derived engine assets. Hosted/imported content and saves stay in OPFS;
the large archive is never added to the Cache API. Serve the build through localhost or HTTPS and visit
it once online before testing offline; `file://` is unsupported. Clearing site
data removes both imported packages and saves. A cache-bypassing build check
detects a stale controlling worker. **Save & update** pauses and autosaves the
active game before activating and reloading; if activation fails or the game
cannot be preserved, the old build remains available and play resumes.

## Controls and saves

- Select mode plus tap/click selects; alternate/right click issues a contextual
  order.
- Order mode plus tap/click issues a contextual move/attack order.
- Hover renders the current contextual action and cursor. Unrevealed cells
  expose only **Explore** before object or terrain actions are inspected, so
  shroud cannot be used as a hidden-object oracle. Visible targets use the
  engine's ordered cell occupiers, footprint/center distance, and airborne
  altitude projection instead of sprite-rectangle guesses.
- Drag box-selects.
- Two fingers pan and pinch zoom. Desktop users can middle-drag or use
  WASD/arrow keys to pan, and use the wheel, +/-, or visible camera controls to
  zoom; Home or the visible reset control restores the view.
- On the first mission launch, the Battlefield basics guide explains shroud
  and the primary controls. Dismissal is remembered, and the compact
  **Controls** button reopens it.
- Canonical GDI Missions 1–5 show exact reviewed objectives. Mission 1
  eliminates the Nod force while retaining a GDI ground force; Mission 2
  eliminates the Nod occupation while retaining a GDI force; Mission 3
  eliminates every counted Nod unit and structure while requiring at least one
  counted GDI structure, infantry unit, or ground vehicle to remain. Mission 4
  West A and East A recover the marked crate while retaining counted GDI
  infantry or a ground vehicle. West B eliminates the active Nod force, prevents
  all four protected village structures from being destroyed, and retains
  counted GDI infantry or a ground vehicle. Mission 4 uses cause-neutral rule
  state because GDI sidebar losses cannot represent the protected Neutral
  structures. Only the engine's terminal result marks objectives complete or
  failed. Mission 5 eliminates every counted Nod unit and structure while
  retaining a counted GDI unit or structure. Before the two authored relief
  zones are crossed, it also fails if the last member of either protected
  starting group—the field force or base structures—is destroyed. Repairs are
  briefing guidance rather than a native completion condition. Mission 6 and
  later do not receive inferred rules.
- Q selects, E enters contextual order mode, and X or **Stop** stops selected
  units.
- When one deployable unit is selected, the engine-authored contextual action
  exposes **Deploy** (or **Blocked** when deployment is invalid). The selection
  status uses semantic names such as “Mobile Construction Vehicle” and
  “Construction Yard”; activation is sent through the ordinary contextual
  order path.
- The accessible control-group panel reports current selection and membership
  for all ten legacy groups. Number keys 1–0 select a group, Ctrl+1–0 replaces
  it with the current mobile-unit selection, Shift+1–0 adds it to the selection,
  and pressing an already selected group number again centers the camera. Alt
  plus a number selects and centers immediately. The panel provides equivalent
  pointer/touch assignment and selection controls.
- Escape pauses or resumes. Space switches graphics mode only in the synthetic
  demo; every real mission—including classic-freeware and manually imported
  packages—stays in classic mode.
- **Save** writes a manual local save. **Load** restores the newest save for the
  exact active content revision and mission. The terminal score panel exposes
  **Load latest manual** so a rotating autosave cannot hide the known-good
  pre-victory save.

While a mission is running, the app updates a rotating autosave every 30
seconds. Backgrounding pauses and writes a lifecycle autosave; returning to the
page resumes without wall-clock catch-up. Gameplay is landscape-first on
coarse-pointer mobile devices.

## Current limitations

- Enhanced atlases are not generated, so the real mission is classic-only.
- Sound effects and speech play through Web Audio. C&C music and movies are
  excluded from the distributable freeware profile; any future music must be
  original or independently licensed.
- Production, placement, repair, sell, targeted superweapons, battle telemetry,
  control groups, and exact Mission 1–5 objectives are connected. Mission 6 and
  later still need reviewed objective rules, and the mission panel's live
  downsampled classic surface is not yet a semantic radar implementation.
- The engine sends one full indexed-surface bootstrap after start/load, then
  minimal dirty rectangles at 15 Hz. STATIC_MAP likewise sends a full baseline
  and then retains unchanged cell data, removing 36 bytes per map cell from
  steady snapshots. The other snapshot sections are still complete payloads;
  broader state deltas, visual interpolation, and gamepad controls remain open.
  A bounded real-Wasm desktop Chromium performance gate now covers cold start,
  cadence, frame intervals, snapshot gaps, long tasks, and classic uploads;
  broader physical-device measurements remain open.
- The canvas and control panel have keyboard-accessible controls, but semantic
  screen-reader unit targeting/orders and remappable controls remain open.
- The integration is covered with synthetic data, but a complete real-install
  C01-C09 victory/save/reload/refresh/offline acceptance run has not been
  recorded in this checkout. The separately generated classic-freeware GDI
  sidecar does have real-Wasm save/offline, forced terminal continuation,
  portrait-touch, Firefox, and selected-unit performance acceptance. Separate
  ordinary-command verifiers win Missions 1–3 through the public engine ABI.
  Mission 3's verifier additionally deploys the MCV, builds and places the
  Power Plant/Barracks/Refinery chain, trains infantry, and completes its
  reviewed scout and assault milestones. The browser test separately selects
  and deploys the real MCV and observes production. A long opt-in Chromium
  acceptance additionally wins Mission 1 from rendered
  screenshot/DOM information through ordinary controls and continues to
  Mission 2 without snapshot, debug, or acceptance APIs. A separate long route
  wins Mission 2 the same way, while a short offline fault-injected acceptance
  verifies continuation into Mission 3. This is automated, scenario-scripted
  visible-target evidence; a recorded human-played hardware run remains open.

## Legal

This modified project was first identified as modified on 10 July 2026. See the
source checkout's `License.txt` and the visible About & legal panel. EA has not
endorsed and does not support this product. It is provided without warranty.
