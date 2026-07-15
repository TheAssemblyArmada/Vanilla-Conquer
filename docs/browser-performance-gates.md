# Browser performance and bundle gates

The browser port has two asset-free regression gates. They provide an early,
repeatable baseline for source changes; they do not establish the performance
of a real mission or a supported-device matrix.

## Production bundle budgets

[`performance-budgets.json`](../web/performance-budgets.json) is the checked-in
budget contract. After a production build, the deterministic checker discovers
sorted delivery artifacts, aggregates split app JavaScript and CSS, compresses
each HTTP artifact independently with Node's level-9 gzip implementation, and
checks both raw and gzip totals. Sourcemaps are development diagnostics and are
not included.

| Artifact class | Raw limit | Gzip limit | Calibrated value |
| --- | ---: | ---: | ---: |
| App JavaScript | 532 KiB | 190 KiB | 531.2 / 182.1 KiB |
| App CSS | 27 KiB | 6 KiB | 26.9 / 6.0 KiB |
| Simulation worker | 32 KiB | 11 KiB | 30.1 / 10.0 KiB |
| Engine JavaScript | 92 KiB | 25 KiB | 73.4 / 19.2 KiB |
| Engine Wasm | 1,152 KiB | 420 KiB | 975.2 / 349.1 KiB |

The source-only profile requires app JavaScript, CSS, and exactly one worker;
it permits both engine files to be absent. If either engine artifact is
present, the pair must be complete and is measured. The integrated profile
requires both engine files. These are regression ceilings rather than size
estimates. The 2026-07-14 integrated build intentionally leaves narrow raw
JavaScript/CSS headroom while gzip and engine artifacts retain more room. The
CSS ceiling was recalibrated from 25 to 27 KiB for the reviewed Mission 1
objective state and persistent visible selection feedback; the 6 KiB transfer
ceiling did not change. The app JavaScript ceiling moved from 525 to 528 KiB
for strict variable-length occupier parsing and native-style contextual target
selection. It moved from 528 to 532 KiB when the exact, engine-authoritative
Mission 4 objective presentations were added for all three canonical variants;
the measured gzip size remains below the unchanged 190 KiB transfer ceiling.
The real-Wasm cadence gate keeps the MCV selected so
engine contextual-action export and selection presentation remain on its hot
path. New UI work must justify and record any calibrated
ceiling change rather than silently growing the delivery bundle.

```sh
cd web
corepack pnpm build
corepack pnpm test:bundle-budget
corepack pnpm check:bundle-budget

# After REQUIRE_BROWSER_ENGINE=1 corepack pnpm build:
corepack pnpm check:bundle-budget:integrated
```

The checker catches payload growth, missing artifact classes, duplicate worker
outputs, and incomplete engine pairs. It is not a network benchmark: deployed
servers may use Brotli, different compression levels, or HTTP caching.

## Asset-free browser performance

The Playwright performance spec installs its probe before application code,
then uses the explicit synthetic demo fallback. “Playable” means the demo
notice is present, the Pause control is enabled, and the worker has delivered
tick 2. After that point it observes a four-second steady window.

| Signal | CI budget |
| --- | ---: |
| Startup to playable | at most 2,500 ms |
| Observed simulation rate | 12.5–17.5 Hz |
| Longest tick-observation gap | at most 300 ms |
| `requestAnimationFrame` samples in four seconds | at least 120 |
| Mean / p95 frame interval | at most 30 / 50 ms |
| Startup long tasks / longest | at most 12 / 1,000 ms |
| Steady long tasks / longest | at most 3 / 250 ms |

Chromium's Long Tasks API must be available; unsupported instrumentation is a
failure rather than a zero count. Metrics are printed as
`CNCWEB_PERF_METRICS` JSON. The spec also reads the production rolling API,
prints it as `CNCWEB_RUNTIME_METRICS`, and attaches both reports to the
Playwright result. CI repeats the test twice in fresh browser contexts. The
recorded 2026-07-11 local CI-form two-repeat run passed both runs with:

- startup: 346.1–348.7 ms;
- observed simulation rate: 15.06 Hz, with 83.4–100 ms maximum gaps;
- frame cadence: 155–158 samples, 25.32–25.70 ms mean, and 33.4 ms p95;
- startup long tasks: one, at most 55 ms; and
- steady long tasks: zero.

Run the same repeated gate with:

```sh
cd web
corepack pnpm test:performance

# Additional local soak:
corepack pnpm exec playwright test e2e/performance.spec.ts --repeat-each=5
```

## Classic-freeware real-mission gate

The opt-in classic-freeware performance spec uses the generated, hash-pinned
sidecar and production Wasm engine rather than the synthetic demo. It separates
the one-time package download/verification/import from a 10-second uninterrupted
visible mission window. Budgets live in `web/performance-budgets.json`:

| Signal | Desktop Chromium budget |
| --- | ---: |
| Cold package bootstrap to playable | at most 30,000 ms |
| Observed simulation rate | 12.5–17.5 Hz |
| Longest snapshot gap | at most 300 ms |
| RAF / snapshot samples in ten seconds | at least 250 / 115 |
| Mean / p95 / maximum RAF interval | at most 30 / 50 / 300 ms |
| Long tasks / longest | at most 3 / 250 ms |
| Full classic-surface uploads | at most 1 |

RAF telemetry counts animation callbacks, not WebGL draw submissions. Real
missions ingest every ordered classic-surface update, but duplicate RAF
callbacks do not redraw when snapshot, mode, camera, and canvas size are
unchanged. The mission canvas uses a 0.375 backing-buffer scale after the
bounded device-pixel ratio and nearest-neighbor presentation. The performance
spec disables Playwright video, trace, and screenshots because recording a
software-rendered WebGL canvas materially perturbs cadence.

DOM-facing tick, selection, sidebar, and contextual-hover presentation is
coalesced near 5 Hz. The full-surface minimap repaint runs near 1 Hz, with
immediate launch/load/terminal refreshes. Neither throttle changes the 15 Hz
simulation, ordered texture ingestion, or RAF cadence. The complete release
command runs this cold benchmark before its CPU-heavy deterministic mission
soak so the startup measurement begins from an idle runner.

It also reconciles baseline, delta, and unchanged upload counts and requires
the exact Wasm/package/mission identity. Run it alone with
`corepack pnpm test:classic-freeware:performance`, or as part of
`corepack pnpm test:classic-freeware:release`. Two consecutive isolated runs
and the subsequent complete release run on 2026-07-15 observed 4.37–4.48-second
cold startups, 15.000–15.001 Hz, 600 RAF samples in ten seconds (16.67 ms mean,
16.71 ms p95, 16.81 ms maximum), 150 snapshots with at most a 70.1 ms gap, and
zero long tasks.

## Owned-mission C09 gate

The production page exposes a bounded, read-only diagnostic API:

```js
window.__cncwebRuntimeMetrics.snapshot(60000)
```

It reports the runtime core, package revision, mission, production build, and
fresh acceptance-session identity alongside current running/visibility state,
rolling simulation cadence, snapshot-delivery gaps and byte envelopes, RAF
mean/p95/max intervals, long tasks, and classic baseline/delta/unchanged upload
counts and pixels. Report format v3 includes independent RAF/snapshot spans,
snapshot tick delta, declared-byte total, and total upload samples. The
RAF-sample ring holds 32,768 samples, enough
for a full minute even at 500 Hz; other rings hold 8,192 samples and are also
bounded for long-running sessions.

The local owned-content harness makes C09 a recorded checkpoint. After an
uninterrupted visible 60-second GDI mission 1 window, the operator pastes the
API's one-line JSON into the harness. A content-safe verifier requires:

| Signal | Provisional C09 threshold |
| --- | ---: |
| Observed window | the full 60,000 ms |
| Simulation rate | 12.5–17.5 Hz |
| Longest snapshot-delivery gap | at most 300 ms |
| RAF callback samples | at least 1,800 |
| Snapshot samples | at least 700 |
| Mean / p95 RAF interval | at most 30 / 50 ms |
| Longest RAF gap, including window edges | at most 300 ms |
| Long tasks / longest | at most 3 / 250 ms |
| Full classic baseline uploads | at most 1 |
| Incremental classic uploads | cover delivered snapshots, allowing two boundary samples |

The verifier requires `running: true`, `visibilityState: "visible"`, exact
harness-supplied build/content/mission/session identities, and a complete
60,000 ms segment. Pause/resume, load attempts, terminal state, visibility
changes, identity/build changes, and tick rewinds reset every series. It also
recomputes rates and means from the independent spans/deltas/totals, checks
window-edge lower bounds, reconciles all upload categories, synchronously
drains pending Long Tasks observer entries, and rejects duplicate-key or
oversized JSON.

Declared snapshot sizes, transfer-buffer envelopes, byte rate, and classic
pixel-upload rate are recorded but deliberately have no threshold until real
content has been measured across representative hardware. The harness keeps
the JSON and a browser/device note in its private external work directory.

## Evidence boundary

The always-on performance spec measures the generated demo core in headless
desktop Chromium at a 1440×900 viewport. The opt-in classic-freeware gate adds
Tiberian Dawn Wasm, OPFS package import/mounting, real classic-surface uploads,
and mission startup on the same local browser class. Neither gate measures
owned-content variations, mobile GPU behavior, thermals, heap growth, or memory
pressure. Those still require measurements on representative desktop and
mobile hardware. C09 closes the owned-mission cadence/upload observation
for the one browser and device on which it is run, but its thresholds remain
provisional and it does not measure heap growth, GPU memory, thermals, battery,
or a supported-device matrix. Neither the demo budgets nor one C09 pass may be
reported as general proof that the retail-content milestone is performant.
Because the operator manually copies the DevTools value, it is not a
cryptographic attestation: strict identity/schema/arithmetic validation makes
stale or accidentally edited evidence fail closed, but cannot prevent a person
with page/DevTools control from fabricating a coherent report.
