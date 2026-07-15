# Classic-freeware deployment

This is the zero-install content path. Players do not select a local game
installation or package: the deployed PWA fetches one versioned package from
its own origin, verifies it, commits it to origin-private storage, and launches
the first mission.

**EA has not endorsed and does not support this product.**

The deployment must remain free and noncommercial. EA assets remain EA
property and are used under the revocable permission described by the
[Command & Conquer franchise modding guidelines](https://www.ea.com/games/command-and-conquer/command-and-conquer-remastered/news/modding-faq).
The package contains no C&C music or movies.

## Build the sidecar

From the repository root:

```sh
./scripts/build-classic-freeware.sh .cache/classic-freeware
```

The source is OpenRA's music-free “Base Freeware Content” package. The script
uses a mirror URL from OpenRA's published package set and pins both:

- byte length: `7,911,636`;
- SHA-256: `a55b2c160b534f6d1b865ad6120e1f4fde8c418d47bb2fb1a9c72c586a5e1603`.

`CNCWEB_FREEWARE_SOURCE_URL` may select another OpenRA mirror, but the pinned
size and digest cannot be overridden. OpenRA documents the asset boundary in
its [legal notice](https://www.openra.net/legal/) and publishes the current
[mirror list](https://www.openra.net/packages/cnc-mirrors.txt).

The build produces:

```text
classic-freeware-v1.json
classic-freeware-gdi-v1.cncweb
```

Place both files beside the deployed `index.html`. For an integrated build:

```sh
source /path/to/emsdk/emsdk_env.sh
cmake --workflow --preset web-td

cd web
corepack pnpm install --frozen-lockfile
REQUIRE_BROWSER_ENGINE=1 corepack pnpm build
cd ..

./scripts/build-classic-freeware.sh web/dist
```

For external staging, publish the complete `web/dist` tree under one immutable
deployment-directory URL and follow the provider-neutral
[static-host staging contract](staging-deployment.md). The URL may be a
subpath but must resolve relative assets from its trailing `/`. Building and
local acceptance do not themselves publish a release; this checkout has no
configured external hosting target. The staging gate verifies HTTP headers,
every served byte and package entry, local `dist` parity, then runs the remote
desktop browser matrix.

With the Playwright browser dependencies installed, run the complete local
browser/Wasm release acceptance:

```sh
cd web
corepack pnpm test:classic-freeware:release
```

The release gate proves one archive download, real Wasm mission startup,
manual save/load and online/offline resume, exclusion from the Cache API,
coarse-pointer portrait play, the winter mission, and reuse of the installed
OPFS revision. Its engine verifiers mount the same package and produce
deterministic public-ABI victories for GDI Missions 1–5, including all three
Mission 4 and Mission 5 variants. Missions 1–3 use public production, placement, repair,
clear-selection, select-object, and contextual-order commands. Mission 1
reaches terminal victory at tick 2,137 after 13 Nod kills and two GDI losses.
Mission 2 reaches terminal victory at tick 16,123 after 26 production starts,
two repair orders, 62 Nod unit kills, five Nod structure kills, and 29 GDI
losses. Mission 3 deploys its MCV, constructs and places Power Plant, Barracks,
and Refinery, trains infantry, completes reviewed scout and assault route
milestones, and requires an authoritative win with zero remaining counted Nod
combatants. Mission 4 West A and East A complete every reviewed recovery-route
milestone, reach authoritative victory with a surviving GDI force, and leave
counted Nod combatants alive; East A also loads and unloads its authored APC
cargo and reaches the eastern staging area. West B receives its authored GDI
reinforcements, eliminates every counted Nod combatant, and wins with at least
one protected village structure surviving. Mission 5 completes both authored
relief zones before either protected starting group is eliminated, repairs the
field base, produces a strike force, triggers the Nod counterattack, and wins
only after every counted Nod unit and structure is gone. None uses the debug
victory hook.

Run the Mission 3 verifier or the complete Mission 4 and Mission 5 variant
suites independently with:

```sh
cd web
corepack pnpm verify:classic-freeware:mission-three
corepack pnpm verify:classic-freeware:mission-four
corepack pnpm verify:classic-freeware:mission-five
```

The Mission 3 alias selects canonical `SCG03EA`; the Mission 4 alias exercises
canonical `SCG04WA`, `SCG04WB`, and `SCG04EA`; and the Mission 5 alias exercises
canonical `SCG05EA`, `SCG05WA`, and `SCG05WB` in the shared
`web/scripts/verify-classic-freeware-mission-one.mjs` verifier.

The browser also displays exact reviewed rules for Missions 1–5. Mission 3's
canonical orders are:

- **Eliminate the Nod force:** “Destroy every counted Nod unit and structure in
  the operation area. Nod production, rebuilt structures, and attack teams can
  add targets.”
- **Keep GDI operational:** “The operation fails if no counted GDI structure,
  infantry, or ground vehicle remains.”

Mission 4's three canonical variants use two reviewed rule sets:

- **West A / East A — Recover the GDI crate:** “Reach the marked recovery area.
  The operation completes when a GDI unit enters the crate cell; destroying Nod
  is not required.” The recovery force must retain counted GDI infantry or a
  ground vehicle; a transport aircraft alone does not prevent defeat.
- **West B — Eliminate the Nod force:** destroy every active counted Nod unit,
  including triggered assault groups. The operation also fails if all four
  protected village structures are destroyed or if no counted GDI infantry or
  ground vehicle remains.

Mission 5's three canonical variants share one reviewed rule set:

- **Eliminate the Nod force:** destroy every counted Nod unit and structure,
  including production, rebuilding, patrols, and timed attack teams.
- **Relieve the separated GDI base:** cross both authored relief zones. Until
  its corresponding zone is crossed, losing the last member of the protected
  starting field force or protected starting base structures immediately
  fails the operation.
- **Keep GDI operational:** the operation fails if every counted GDI unit and
  structure is destroyed. Repairing the damaged base is briefing guidance, not
  an engine completion rule.

Missions 1–3 and Mission 5 use combat totals as progress context. Missions 4
and 5 display cause-neutral state for conditions that cannot be inferred from
sidebar totals. In every reviewed mission, the engine's terminal result alone
decides success or failure. In Mission 1 the browser-visible deploy
acceptance selects the real MCV, consumes its engine-authored deploy action, and
uses **Deploy** until Construction Yard production is available. In Mission 2 it trains a
Minigunner and exercises pause/resume through visible production controls.
Semantic labels identify the selected MCV and resulting Construction Yard
without changing the engine command path.

A separate loopback-only acceptance hook uses the engine's existing
end-game debug request to produce genuine campaign-outcome/game-over events;
the test restores that result after refresh, starts canonical GDI mission 2
with carry state, reloads the continuation offline, and continues through a
second terminal result into GDI mission 3. A fault-injected audio-index read
exhausts the bounded OPFS retry during that offline handoff and proves optional
audio cannot prevent the engine from starting. Normal/public origins never
expose this hook. The normal-command engine verifiers prove combat and
mission completion through the public ABI, the deploy acceptance proves real
canvas selection/order/deployment behavior, and the hook proves browser
terminal recovery and continuation. The combat verifier has engine-level
object knowledge rather than a person's fog-of-war view, so a recorded
human-played combat victory remains open.

The same command runs a 10-second real-mission performance budget and a
desktop browser matrix. Chromium and headed Firefox exercise the hosted
package, save, and OPFS reload. Playwright's Linux WebKit port currently lacks
`navigator.storage`, so it verifies the explicit demo fallback; WebKit
[documents Safari's OPFS support](https://webkit.org/blog/12257/the-file-system-access-api-with-origin-private-file-system/),
and a physical macOS/iOS acceptance run is still required.

The source archive, extracted files, `.cncweb`, and descriptor are generated
or downloaded content. They are ignored release outputs and must not be added
to Git.

## Trust and storage flow

The browser accepts only the fixed `classic-freeware-v1.json` schema. The
descriptor and archive must be same-origin and may not redirect, use
credentials, or contain URL queries/fragments. Before parsing the ZIP, the
browser requires its declared byte length and SHA-256. Before committing any
file, the importer independently checks the package ID, aggregate content
digest, freeware provenance, ZIP limits, manifest inventory, and every file
digest. A failed candidate never replaces a working revision.

The service worker deliberately does not cache the large content archive. The
verified, expanded revision lives once in origin-private storage. A later
online or offline launch uses that installed revision without downloading the
archive again.

## Current content boundary

The normalized mirror contains all 25 canonical GDI campaign roots and only
the opening Nod mission. This release profile therefore publishes the complete
GDI campaign first. Nod requires an additional music-free Gold freeware source
and is not fabricated from missing data.

The mirror strips mission briefing prose, so the catalog uses a neutral
non-story instruction while preserving each original scenario INI/BIN for
simulation. The browser supplies exact objective prose only for the reviewed,
canonical Mission 1–5 identities. Missions 1–3 and Mission 5 use engine-exported
combat totals as progress context; Missions 4 and 5 use cause-neutral state for
conditions that sidebar totals cannot prove. Completion/failure comes only from
the engine result. Mission 6 and later retain the neutral
instruction until their rules are authored and reviewed.
The mirror also omits winter-specific icon data; the current MVP aliases
the verified temperate icon archive for the two winter missions. Authentic
winter icons or fully original replacement art are the presentation follow-up.
The winter gate captures the live classic battlefield on a 390×844
coarse-pointer viewport and checks that controls remain usable. A detailed
human visual review on representative phones remains release follow-up.

The optional local Remastered converter and its private acceptance harness
remain supported, but they are not required by the classic-freeware deployment.
