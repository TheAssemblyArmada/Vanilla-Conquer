# Owned-content milestone acceptance

This acceptance run is deliberately local. It consumes a legally owned C&C
Remastered Collection installation but must not add, upload, or publish retail
data, generated packages, browser profiles, screenshots, or content-sensitive
logs. Public CI continues to use synthetic data only.

## Guided run

Source the pinned Emscripten SDK environment, then run the harness from the
repository root. The input may be the collection root or its `Data`,
`CNCDATA`, `TIBERIAN_DAWN`, or `CD1` directory:

```sh
source /path/to/emsdk/emsdk_env.sh

./scripts/run-owned-content-acceptance.sh \
  --provider steam \
  "/path/to/CnC Remastered Collection"
```

To run the optional content-private browser preflight before the manual
checklist, opt in explicitly:

```sh
CNCWEB_OWNED_BROWSER_PREFLIGHT=1 \
  ./scripts/run-owned-content-acceptance.sh \
  --provider steam \
  "/path/to/CnC Remastered Collection"
```

Use `ea-app`, `copied-installation`, or `unknown` when appropriate. Run
`./scripts/run-owned-content-acceptance.sh --help` for explicit external output
and work-directory options. `--dry-run` exercises argument and path guards and
prints the complete workflow without creating anything.

By default, the harness creates a mode-0700 temporary directory outside the
repository. The `.cncweb` pack, source inspection, conversion report, verified
manifest, content-safe runtime summary, server log, and acceptance record stay
there. A content-safe toolchain report records the exact pinned Node.js and
Emscripten versions used for the build. The script rejects install, output,
and work paths that resolve inside the checkout, including paths that enter it
through a symlink. It checks the
retail-content boundary both before conversion and after the integrated build.
It also rejects output/work paths inside the selected installation, which is
treated as read-only.

The automated phase:

1. Requires the repository-pinned Node.js and Emscripten versions.
2. Plans the mission conversion without extracting into the checkout.
3. Creates the owned-content package and re-verifies every declared byte with
   the Rust packer.
4. Independently checks the exact GDI mission 1 catalog fields, browser-audio
   references and seven required cue groups, required MIX inventory, scenario
   file sizes, and manifest inventory digest without printing the locally
   derived briefing.
5. Builds the production Wasm engine and integrated PWA.
6. Serves only on `127.0.0.1`, compares the served JavaScript, Wasm, and service
   worker with the production artifacts, checks their media types, and runs the
   Wasm ABI/missing-mount diagnostic smoke test against the served engine.
7. Privately mounts the verified owned GDI1 payload in the real Wasm engine,
   starts it, acknowledges startup movies, saves after live ticks, proves a
   same-instance deterministic branch/replay at every tick (including hashes
   of each complete encoded event sequence), destroys and recreates the
   runtime, then proves load-before-first-tick replay again. The state hashes
   include the private RNG/first-update state. Only a content-safe
   identity/count report is retained in the external work directory; replay
   traces remain in memory and event draining fails closed if its bound is
   exceeded.
8. When `CNCWEB_OWNED_BROWSER_PREFLIGHT=1` is supplied, reuses the private
   preview to import through the real disclosure dialog, re-imports the active
   immutable revision to exercise lease teardown/replacement, verifies the
   exact package, mission, build, acceptance-session, and Wasm metrics identity,
   exercises command controls and camera input, performs save/advance/load,
   resumes after an online refresh, verifies service-worker control, and
   resumes after an offline reload of the exact acceptance URL.

The optional Playwright runner does not start another server. Screenshots,
video, traces, and retries are disabled. Its stdout, stderr, test output, and a
schema-bounded report containing only identities, booleans, and ticks remain
under the external mode-0700 harness work directory. It never records the
briefing or other retail text. The report explicitly leaves audio, victory,
performance, and real command semantics to the manual checkpoints.

These automated checks do not claim that the game was played. The guided phase
records `PASS` only after the operator explicitly confirms each browser
checkpoint: the import disclosure, real mission launch, interaction and audio,
manual save/load, online refresh/resume, victory terminal state, load after
victory, offline normal-reload/resume, and a 60-second real-mission performance
sample. Entering `fail`, `quit`, or ending input leaves the overall milestone
unpassed. `--serve-only` prints the same checklist and keeps the preview running
without recording manual results.

Before import, the app shows the archive size, available origin-private quota,
the local-only storage promise, the effect of clearing site data, and that an
installation ends the current simulation. After the
initial online visit, use the browser's developer tools to confirm service
worker control before selecting Network **Offline** and performing a normal
reload of the exact acceptance URL printed by the harness. Do not use a
hard/shift reload for this
gate because browsers may deliberately bypass the service worker and its cache.

For C09, restore networking, keep the game visible, and let the resumed mission
run uninterrupted for at least 60 seconds. Then run this in DevTools:

```js
copy(JSON.stringify(window.__cncwebRuntimeMetrics.snapshot(60000)))
```

Enter `pass` at the C09 terminal prompt and paste the copied one-line JSON. The
harness validates it, asks for a content-safe browser/device note, and writes
both into the private external work directory. The report contains no mission
text or retail data; it does include content-safe core, full package revision,
mission, production-build, and fresh acceptance-session identities. The
verifier requires all five to match the current harness run.

The v3 report also says whether the worker is currently running and whether
the document is visible. A pause/resume, load attempt, terminal event,
visibility transition, runtime/build identity change, or unannounced tick
rewind starts a new measurement segment, so none can borrow time or samples
from the preceding segment. Independent RAF and snapshot spans, tick delta,
declared-byte total, and upload-sample total let the verifier recompute the
reported means and rates and cross-check upload categories.

This is operator-attested evidence, not a cryptographic browser attestation.
The fresh acceptance identity, exact build/content identities, bounded strict
JSON parser, and arithmetic checks reject stale, malformed, and casually
edited reports, but a person controlling DevTools can still fabricate values.
Keep the browser/device context note with the private report and do not present
one manual run as tamper-proof evidence.

The current C09 thresholds are provisional: 12.5–17.5 simulation Hz, no more
than 300 ms between delivered snapshots, at most 30/50 ms mean/p95 animation
frame intervals, no more than three long tasks with none over 250 ms, and at
most one full classic baseline upload during the uninterrupted window. It also
requires the full 60 seconds of observations, 1,800 RAF callbacks, 700
snapshot samples, no RAF gap over 300 ms (including either window edge), and a
continuous incremental-upload record. Snapshot envelope, byte-rate, and pixel-
upload-rate values are captured for calibration but do not yet have a pass/fail
budget. See the [browser performance gates](browser-performance-gates.md) for
the evidence boundary; one passing machine does not establish a supported
device matrix, memory-pressure behavior, or thermal stability.

## Harness checks

The harness and its metadata validator have asset-free tests:

```sh
./scripts/tests/test-owned-content-acceptance.sh
python3 -m unittest scripts/tests/test_verify_owned_content_package.py
(cd web && pnpm test:owned-preflight-config)
./scripts/check-no-retail-content.sh
```

The Python test creates a tiny synthetic ZIP in the operating system's
temporary directory and removes it after the test. No package fixture is kept
in the repository.
