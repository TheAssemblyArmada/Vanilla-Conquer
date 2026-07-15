# Browser-port implementation status

This branch keeps source and CI asset-free while supporting a separately built,
zero-install classic-freeware deployment sidecar. The source implementation
supports the canonical Tiberian Dawn GDI and Nod campaign catalogs using
classic presentation. The current zero-install profile contains all 25 GDI roots;
the guarded optional owned-content profile deliberately remains GDI mission 1,
east/variation A. Red Alert remains a later, separate engine module.

## Implemented

- The Tiberian Dawn gameplay core compiles as a modular Emscripten ES module
  without Asyncify, pthreads, native window/audio backends, or cross-origin
  isolation. Creation is content-independent; initialization is deferred until
  a mission start request has passed content preflight.
- The fixed-width browser/native ABI is version 2. The packed transport remains
  protocol version 1: `StartV1`, `CommandBatchV1`, `SnapshotV1`, `EventV1`, and
  the outer `WebSaveV1` layout are unchanged. The ABI starts and advances the
  15 Hz simulation, accepts atomic command batches, emits pointer-free snapshots
  and structured events/diagnostics, exposes the classic indexed surface and
  palette, and serializes and restores content-bound saves. Its additive
  campaign-transition call supplies raw cash and the three Nod nuke-piece bits
  before a campaign start. A correlated campaign-outcome event exports those
  values, the sabotage result, live scenario RNG, scenario, house, and canonical
  root immediately before game over.
- The unchanged outer `WebSaveV1` envelope contains TD's deterministic `TDWS`
  v2 payload. Version 2 preserves the live simulation RNG, first-update gate,
  and `SabotagedType`; the decoder remains compatible with v1 payloads, whose
  missing sabotage state is normalized to none. These private values contribute
  to the deterministic state hash, and resume cannot tick fresh mission state
  before its save loads. Victory/defeat is terminal until a save is loaded or
  the mission is restarted.
- `SnapshotV1` sends a complete STATIC_MAP cell array on start/load and after
  any logical map change. When it is unchanged, the wire section retains the
  preceding materialized `base_tick` cell array and sends only the fixed
  304-byte metadata instead of another 36-byte record for every map cell. A
  receiver cannot apply the retained form without the matching base, including
  when backpressure skips intermediate ticks. Full and retained forms hash as
  the same complete logical section, so state hashes are history-independent.
- The Rust companion discovers the Remastered Collection TD `CD1` or `CD2`
  directory case-insensitively, safely parses classic MIX archives, and applies
  the engine's first-match resolution order independently to every INI/BIN.
  `td-gdi-campaign` emits the 25 unique GDI descriptors for scenarios 1-15;
  `td-nod-campaign` emits the 25 unique Nod descriptors for scenarios 1-13.
  Catalog order and roots come from the legacy map-selection table. The
  original one-mission `td-gdi-01-east-a` profile remains stable and is still
  the default compatibility/owned-acceptance profile.
- The freeware conversion path accepts OpenRA's music-free normalized mirror
  only after its release archive is pinned by size and SHA-256. It tolerantly
  reads legacy scenario theater metadata, supplies neutral catalog briefing
  text where that stripped package has none, excludes SCORES/MOVIES entirely,
  and emits the complete 25-root GDI catalog. `LOCAL.MIX`, `UPDATA.MIX`, and
  `UPDATE.MIX` are optional in the browser build; `CCLOCAL.MIX` and
  `UPDATEC.MIX` remain required. The two winter missions currently reuse the
  verified temperate icon archive under the winter logical name.
- Every generated mission catalog record declares its canonical scenario root
  and INI-derived temperate, desert, or winter theater. Conversion includes
  only the encountered fixed theater data/icon pairs, plus an optional loose
  palette when available. Native startup independently reads a bounded loose
  mission INI, requires exactly one supported `[Map] Theater`, validates the
  exact 8192-byte BIN, and preflights the selected fixed archive pair without
  deriving a filesystem path from untrusted INI text.
- Mission conversion decodes supported Westwood AUD formats into checksummed
  PCM WAV files and writes a strict `runtime/audio-v1.json` callback index. A
  completeness gate prevents a structurally valid but effectively silent first
  mission package. Freeware packs may omit terminal outcome speech because the
  browser always presents the authoritative victory/defeat dialog and score.
- The companion creates and re-verifies a browser-v1 `.cncweb` package. The
  browser independently validates its manifest, paths, sizes, compression
  limits, and SHA-256 values before an index-last OPFS commit.
- A strict `classic-freeware-v1.json` descriptor pins package identity,
  aggregate content digest, source provenance, archive byte length, and outer
  SHA-256. Startup fetches only same-origin, non-redirected descriptor/archive
  URLs; it skips downloading an exact installed revision and falls back to the
  synthetic demo when a sidecar is not deployed or cannot be verified.
- The web library distinguishes compatible and incompatible installed
  revisions. A compatible revision's engine files are rechecked in the worker,
  exposed as OPFS-backed `File`/`Blob` objects, and mounted read-only through
  Emscripten WORKERFS before `StartV1` launches the selected catalog mission.
  The library derives each mission's loose INI/BIN and the package's union of
  required theater pairs from strict catalog metadata, then reads each bounded,
  hash-verified INI and rejects any catalog/theater mismatch before listing the
  pack as compatible. A shared
  lifetime lock prevents package replacement from deleting mounted files;
  immutable catalog/audio reads share that lease while installs and removals
  remain exclusive;
  corrupt content/save indexes are isolated rather than poisoning the library,
  and a corrupt newest save falls back to the next readable matching save.
- The real mission renders the engine's classic indexed surface through WebGL2.
  Mission canvases use a 0.375 backing-buffer scale after the bounded
  device-pixel ratio. A RAF callback skips its WebGL draw when the snapshot
  buffer, graphics mode, camera, and canvas size are unchanged, while every
  ordered classic-surface update is still ingested. Pointer/touch selection,
  box selection, contextual orders, stop, touch and
  desktop camera pan/zoom, pause, diagnostics, locally derived briefing, and
  terminal score UI with deterministic manual-save recovery are connected. A
  downsampled live classic minimap and engine-exported credits/power/combat
  telemetry populate the mission panel. DOM telemetry and contextual hover are
  coalesced near 5 Hz, while the full-surface minimap repaints near 1 Hz with
  immediate launch/load/terminal updates. The DOM construction panel exposes
  category-grouped production with progress, start/hold/resume/cancel actions,
  completed-building placement with footprint/proximity feedback, structure
  repair, structure/wall selling, and targeted superweapons. Target hit tests
  enforce ownership/capability constraints before commands cross the adapter.
  Pointer hover renders a contextual action label and cursor. The resolver
  returns **Explore** for an unrevealed cell before it inspects any object or
  terrain action, preventing hover from becoming a shroud oracle; object-level
  **Select**, **Move**, **Attack**, and related actions are presented only for
  visible cells. On visible terrain it consumes the engine's ordered,
  duplicate-preserving occupier grid and native object centers instead of
  guessing from sprite rectangles: current/adjacent cells, building footprint
  centers, Dragon-Strike distance, the 192-lepton cutoff, command-coordinate
  rounding, and altitude-adjusted aircraft are mirrored. Per-snapshot visible
  root/identity caches and frame-coalesced pointer work keep that lookup off the
  raw pointer-event path. Exact parity for asymmetric owner-side alliances,
  aircraft heap ties, and subpixel camera residuals would require additional
  engine-authored fields; current handling fails closed at shroud boundaries.
  An accessible control-group panel reports the current
  selection and all ten legacy groups with pointer/touch assignment and
  selection. Keyboard 1–0 selects, Ctrl+1–0 replaces, Shift+1–0 adds, and
  repeating an active group key centers it; Q/E switch select/order mode and X
  stops selected units. Hotkeys are suppressed in editable controls and
  dialogs. The enhanced sprite renderer remains available to the synthetic
  demo but mission packs are locked to classic mode. A persistent first-run
  guide identifies black areas as unexplored shroud, points players to Mission
  1's lower-right start, documents mouse/keyboard/touch pan, zoom, selection,
  and orders, and retains an accessible Controls launcher after dismissal.
- Missions 1–5 have exact, reviewed objective presentations gated to their
  canonical GDI/SCG01EA, GDI/SCG02EA, GDI/SCG03EA, GDI/SCG04WA,
  GDI/SCG04WB, GDI/SCG04EA, GDI/SCG05EA, GDI/SCG05WA, and GDI/SCG05WB
  identities. Mission 1 eliminates the Nod force
  while retaining a GDI ground force; Mission 2 eliminates the Nod occupation
  while retaining a GDI force. Mission 3 must destroy every counted Nod unit and
  structure, including targets added by Nod production, rebuilding, and attack
  teams, and fails when no counted GDI structure, infantry, or ground vehicle
  remains. Mission 4 West A and East A recover the marked crate while retaining
  counted GDI infantry or a ground vehicle. West B eliminates the active Nod
  force, retains at least one of the four protected village structures,
  and retains counted GDI infantry or a ground vehicle. Mission 5 destroys
  every counted Nod unit and structure and retains a counted GDI unit or
  structure. Before each authored relief zone is crossed, it also fails if the
  corresponding protected starting group—field force or base structures—is
  eliminated. Repair remains briefing guidance, not a native completion rule.
  Missions 1–3 and Mission 5 use engine-exported unit/structure totals as
  progress context; Missions 4 and 5 use cause-neutral state for conditions
  that sidebar totals cannot prove. Only the authoritative game-over result
  marks objectives complete or failed. Mission 6 and later do not inherit
  guessed rules.
- Object snapshots also preserve the engine-authored contextual action for
  every player house. The visible selection model turns the selected MCV's
  `Self` action into an enabled **Deploy** command, presents the engine's
  cannot-deploy action as **Blocked**, and maps engine object identifiers to
  semantic labels such as “Mobile Construction Vehicle” and “Construction
  Yard.” The command still crosses the normal contextual-order adapter; the
  browser does not synthesize a deployment result.
- On a correlated single-player victory, the browser resolves only canonical
  continuation edges present in the immutable catalog. It reproduces GDI and
  Nod branch choices, including the GDI mission-six sabotaged-airstrip skip,
  and recognizes both final scenarios. The next start carries raw cash, Nod
  nuke pieces, scoped GDI sabotage state, and the terminal RNG. Catalog content
  cannot invent an edge, and a missing or mismatched destination fails closed.
- Browser-native positional sound effects and serialized speech consume the
  generated audio index. C&C music and movies are excluded from the
  distributable freeware profile; eventual music must be original or otherwise
  separately licensed. Movie callbacks are surfaced as presentation notices.
  Read-only OPFS reads retry only `NotFoundError`, with 25, 100, and 250 ms
  delays. If the audio index remains unavailable after those retries, the app
  reports degraded audio and continues to engine startup.
- Manual saves, one rotating 30-second autosave, and a backgrounding autosave
  are stored separately from content. Save identity includes package ID,
  immutable content revision, mission ID, and a fresh campaign run ID, so a
  replay of the same mission cannot consume saves from another attempt. The
  version-2 runtime-session record persists incoming carry state and a
  correlated pending victory independently of a simulation save. Refresh can
  therefore restore the victory branch chooser without relaunching terminal
  mission state, while normal resume selects only the exact run/revision/
  mission save. Session epochs prevent a late asynchronous save from becoming
  the remembered resume point after a restart or campaign transition; one
  narrowly gated path accepts pre-run-ID v1 sessions and legacy save metadata.
- Production builds register an offline service worker that caches only
  source-derived shell and engine assets. Hosted/imported packages and saves
  stay in origin-private storage and are never written to the Cache API. The
  page compares its controlling worker with cache-busted deployment metadata,
  rechecks after reconnect/foreground and periodically, and requests an update
  when those build IDs differ. Applying a waiting update pauses and autosaves a
  live game before activation; a bounded activation failure resumes play, an
  unsavable active game refuses the reload, and failed installs delete their
  partial shell cache.
- A local acceptance harness rejects owned/generated paths inside the source
  tree, converts and byte-verifies a real package externally, validates its
  exact mission/audio/engine preflight metadata without printing briefing
  text, builds and probes the served production PWA, runs a private real-Wasm
  save/replay/destroy/relaunch gate, and records manual results only after
  explicit pass/fail confirmation. Each run uses a fresh localhost origin and
  binds C09 telemetry to the Wasm core, full package revision, mission,
  production build, and a per-run acceptance identity. Its v3 metrics reset on
  run, visibility, load, terminal, timeline, and identity discontinuities and
  carry independent counters used for strict verifier cross-checks; the manual
  DevTools handoff is documented as operator-attested rather than
  cryptographically attested.
- Deterministic production bundle budgets cover raw/gzip app JavaScript, CSS,
  the simulation worker, and the optional or required engine pair. A repeated
  headless-Chromium gate measures asset-free demo startup, 15 Hz tick progress,
  frame cadence, and long tasks; its source-only evidence boundary is
  documented separately.
- Tests construct synthetic archives and media at runtime. CI rejects common
  retail and generated package/save extensions before exercising the Rust,
  TypeScript, and Wasm layers; native adapter tests are also available through
  the CMake testing preset.

## Verification state

The source-only path is exercised by generated Rust/TypeScript tests, native
adapter tests, an Emscripten lifecycle smoke test, and integrated web builds.
This includes synthetic coverage for both 25-mission campaign catalogs,
canonical continuation, carry-over, pending-victory correlation, run isolation,
TDWS v1/v2 decoding, theater-aware preflight, construction/tool commands,
control-group keyboard routing/focus guards, and full/retained STATIC_MAP
canonical hashing and chain validation.
Bundle and browser scheduler baselines are described in
[browser performance gates](browser-performance-gates.md); they do not measure
owned-content mission performance.
The real hash-pinned freeware mirror has been converted successfully into a
10.7 MB archive containing a manifest and 128 declared payload files, including
all 25 GDI roots, 64 decoded browser audio assets, and no music/movie payloads.
The packer re-verified every entry and
emitted the strict same-origin bootstrap descriptor. The opt-in Chromium gate
proves exactly one download, real Wasm GDI mission-one startup, manual
save/load, online/offline resume, and that no content archive enters the Cache
API. It starts GDI mission 8 in a 390×844 coarse-pointer viewport and captures
the rendered winter battlefield. A narrow loopback-only hook routes the
engine's existing end-game debug request through a paused worker, then proves
genuine campaign-outcome/game-over events, pending-victory refresh recovery,
canonical GDI mission-two carry state, and offline continuation through Mission
3 without another archive download. The Mission 2 → Mission 3 handoff also
fault-injects repeated audio-index `NotFoundError` failures matching the class
observed after a long run, exhausts the bounded retry, and proves degraded audio
cannot prevent startup. This
validates the terminal pipeline but is not evidence of
human combat-objective completion. Independently, a public-ABI real-engine
verifier mounts the freeware package and wins Mission 1 through public
clear-selection, select-object, and contextual-order ABI calls only. Its
deterministic baseline reaches authoritative terminal victory at tick 2,137
with 13 Nod kills and two GDI losses, without the terminal debug hook. The same
verifier wins Mission 2 at tick 16,123 through public production, repair,
selection, and contextual-order calls, with 26 production starts, two repair
orders, 62 Nod unit kills, five Nod structure kills, and 29 GDI losses. The
Mission 3 mode of the same verifier deploys the MCV through its contextual
action, constructs and places the Power Plant (`NUKE`) → Barracks (`PYLE`) →
Refinery (`PROC`) chain, trains infantry, completes reviewed scout and assault
route milestones, and requires both the authoritative victory and zero
remaining counted Nod combatants. It is part of release acceptance and can be
run independently with
`corepack pnpm verify:classic-freeware:mission-three`; the alias selects
canonical `SCG03EA` in
`web/scripts/verify-classic-freeware-mission-one.mjs`. The Mission 4 mode
produces deterministic public-ABI victories for all three canonical variants.
West A and East A complete every reviewed recovery-route milestone, reach
authoritative victory with a surviving GDI force, and leave counted Nod
combatants alive; East A also loads and unloads its authored APC cargo and
reaches the eastern staging area. West B receives its authored GDI
reinforcements, eliminates every counted Nod combatant, and wins with at least
one protected village structure surviving. It is part of release acceptance
and can be run independently with
`corepack pnpm verify:classic-freeware:mission-four`; the alias exercises
canonical `SCG04WA`, `SCG04WB`, and `SCG04EA` in the same verifier. A second
browser-visible acceptance selects the MCV through the canvas, observes its
engine-authored deploy action and semantic selection label, invokes **Deploy**,
and waits for the real Construction Yard production menu. Together these prove
combat completion and the ordinary browser deployment path, but the engine
verifier has object-level battlefield knowledge; they do not replace a
human-played fog-of-war victory.

The Mission 5 mode produces deterministic public-ABI victories for canonical
`SCG05EA`, `SCG05WA`, and `SCG05WB`. It routes the protected starting force
through both authored relief zones, repairs the seven damaged base structures,
produces infantry and vehicles, launches a normal-command assault, exercises
the Nod production/counterattack trigger, and requires both authoritative
victory and zero remaining counted Nod combatants. Cloaked SAM cleanup uses
ordinary move and Ctrl force-fire orders at authored sites; the verifier does
not mutate mission state or invoke the debug victory hook. It can be run with
`corepack pnpm verify:classic-freeware:mission-five`.

An additional long, opt-in Chromium Playwright acceptance supplies a
scenario-scripted browser-control victory path. It follows fixed reviewed
patrol waypoints, finds candidate targets in composited battlefield
screenshots, and uses rendered DOM status to validate targets and orders before
issuing ordinary pointer and keyboard controls. It does not read the
snapshot/object ABI, engine debug state, or any acceptance or forced-victory
API. The authored patrol means this is visible-only target-acquisition evidence,
not autonomous or human-blind fog-of-war exploration. The hardened baseline
completed GDI Mission 1 in about 10 minutes, selected **GDI Mission 2 (East
A)** from the authoritative victory dialog, and verified that the next mission
launched. A separate longer route reached authoritative Mission 2 victory using
ordinary browser controls and rendered targeting/order evidence. Independently,
the fast offline continuation test proved Mission 3 startup while
fault-injecting the OPFS audio failure observed after the long session. A single
uninterrupted post-fix replay through Mission 3 remains useful soak evidence,
not a release blocker. Run these only after
placing the integrated build and classic-freeware sidecar in `web/dist`:

```sh
cd web
corepack pnpm test:classic-freeware:genuine-victory
corepack pnpm test:classic-freeware:genuine-mission-two
```

This automated acceptance is intentionally outside the default fast/release
set and is not a substitute for the remaining human hardware review.

A separate 10-second real-Wasm gate keeps an MCV selected while enforcing
cold-start, 15 Hz cadence, RAF, snapshot-gap, long-task, and classic-upload
budgets. Chromium and headed Firefox pass hosted package/save/reload coverage.
Playwright Linux WebKit lacks its OPFS surface and passes the explicit demo
fallback; physical Safari and detailed human winter review remain open.
Remote-matrix URL handling now supports an HTTPS deployment directory at an
origin root or subpath and fails closed on unsafe URL forms. The generic static
host requirements and atomic rollback flow are documented in the
[staging deployment contract](staging-deployment.md). A fail-closed verifier
checks the complete same-origin HTTP graph, MIME/security/cache headers,
service-worker allowlists, build identity, descriptor/archive hashes, the
package manifest and all 128 declared payload files, and optional byte parity
with local `dist`. A single
staging command runs that verifier before the remote matrix. No hosting target
has been supplied, so no external publication or remote-origin run has been
completed; the Chromium/Firefox facts above are from local release acceptance.
The zero-install GDI freeware release does not depend on a retail installation.
Separately, the optional retail-content C01-C09 acceptance is unrun because no
retail installation is present in this checkout. Campaign continuation and
real CD1/CD2 codec, theater, and content variations therefore remain
unmeasured for that optional profile. These are validation gates, not
permission to add retail fixtures to the repository or public CI. Its
local-only evidence boundary is documented in
[owned-content acceptance](owned-content-acceptance.md).

## Remaining product work

1. Publish an immutable candidate to an operator-controlled HTTPS target, run
   the staging gate, then complete physical Safari and representative
   desktop/mobile hardware review, including a human-played Mission 1 victory
   through normal commands and detailed GDI 8/9 winter presentation. Then
   source the missing Nod freeware campaign without importing music or movies.
2. Run the optional owned-content C01-C09 acceptance sequence and private
   continuation coverage without weakening the stable one-mission contract.
   Any future music must use original or independently licensed assets.
3. Produce enhanced texture/terrain atlases and asset tables, stream them to
   the existing sprite renderer, and add presentation parity. The current
   mission package contains classic MIX data only.
4. Extend the implemented classic-surface dirty-region and STATIC_MAP retained
   protocols to the remaining frequently changing snapshot sections, then add
   visual interpolation. Extend the automated real-Wasm Chromium baseline with
   memory, package-import, and mission-start budgets on broader representative
   desktop and mobile hardware.
5. Extend the exact authored Mission 1–5 objectives to reviewed rules for
   Mission 6 and later, and extend the present mission-information/construction
   panel and live classic-surface minimap with semantic radar behavior,
   production icon art, and complete gamepad equivalents.
6. Add semantic screen-reader unit selection/orders, remappable controls,
   content conversion/import cancellation, deeper recovery UX, compatibility
   checks, and cross-browser assistive-technology soak testing.

Multiplayer, Red Alert, public hosting of Remastered/non-freeware content, and a
polished desktop conversion UI are intentionally outside that first slice.
