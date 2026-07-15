# Browser Port Development

> **Modified project — 2026-07-11.** This browser port is an independent,
> experimental modification of code released by Electronic Arts and maintained
> by the Vanilla Conquer project. EA has not endorsed and does not support this
> product.

The `browser-port` branch adds a neutral browser host around the original game
simulation. The target architecture is a 15 Hz C++ simulation compiled to
WebAssembly, a 60 Hz `requestAnimationFrame` presentation loop with
change-driven WebGL2 draws, an offline PWA, and a verified same-origin
classic-freeware content sidecar. A local companion remains available for
optional personal packages.

No EA game data belongs in this repository. Do not commit art,
music, speech, movies, maps, localized text, MEG/MIX archives, or generated
`.cncweb` packs. See [the browser architecture](docs/browser-architecture.md),
[classic-freeware deployment](docs/classic-freeware.md),
[static-host staging contract](docs/staging-deployment.md),
[implementation status](docs/implementation-status.md),
[content policy](docs/content-policy.md),
[browser performance gates](docs/browser-performance-gates.md), and
[NOTICE.md](NOTICE.md).

The implemented source slice wires the real Tiberian Dawn simulation to the
browser for the canonical GDI and Nod campaigns. The zero-install release
profile converts OpenRA's hash-pinned, music-free mirror of EA's Tiberian Dawn
freeware data into the 25-mission GDI catalog. A local Rust companion can also
convert either campaign's 25 unique mission descriptors from an optional
Remastered Collection installation. The PWA verifies and stores each compatible
package in origin-private storage, mounts its engine files read-only in the Wasm worker,
and launches catalog missions in classic graphics. It provides production,
placement, repair, sell, and superweapon controls; follows the original
campaign branch table after victory; and carries cash, Nod nuke pieces, the
mission-six GDI sabotage result, and the live RNG into the next operation.
The accessible mission panel also exposes all ten legacy control groups.
Keyboard users can select them with 1–0, replace them with Ctrl+1–0, add them
with Shift+1–0, and center an already selected group by pressing its number
again. Q/E switch select/order mode and X stops selected units. Pointer hover
renders the current contextual action and cursor. Unrevealed shroud resolves
only to **Explore** before object or terrain actions are inspected; **Select**,
**Move**, **Attack**, and other actions are exposed only for visible cells.
Visible targets are chosen from the engine's ordered cell occupiers and native
object centers rather than WebGL sprite rectangles.
Saves are isolated by immutable package revision, mission, and campaign run.
If no compatible package is installed, the UI runs an explicit synthetic demo
fallback.

The browser/native negotiation version is ABI v2. The packed message protocol,
`StartV1`, `CommandBatchV1`, `SnapshotV1`, `EventV1`, and outer `WebSaveV1`
remain version 1. ABI v2 adds history-aware STATIC_MAP transport: start/load or
any map change sends the complete cell array, while an unchanged snapshot keeps
the fixed 304-byte metadata and retains the cells from its matching
`base_tick`. Receivers reject a retained section when that materialized base is
missing; deterministic hashes still cover the complete logical map.

This is a narrow engineering slice, not a finished browser edition. Newly
composed replacement music, enhanced graphics, reviewed objective rules beyond
Mission 4, semantic radar, gamepad and screen-reader order parity, broader state
deltas, and presentation interpolation remain open. C&amp;C music and movies are
deliberately excluded from the distributable freeware profile. Source and CI
remain asset-free; release assets are generated only into ignored or external
deployment output. The optional owned-content C01-C09 sequence remains a
separate private acceptance profile. See
[implementation status](docs/implementation-status.md) for the exact boundary.

## Browser vertical-slice quick start

Prerequisites are CMake 3.25+, Ninja, the Emscripten version in
[`emscripten-version.txt`](emscripten-version.txt), the Node version in
[`.node-version`](.node-version), Corepack, and the Rust toolchain in
[`rust-toolchain.toml`](rust-toolchain.toml).

Build the source-derived Wasm engine and web shell, then generate the
zero-install classic-freeware sidecar directly into the deployment directory:

```sh
source /path/to/emsdk/emsdk_env.sh
cmake --workflow --preset web-td

cd web
corepack pnpm install --frozen-lockfile
REQUIRE_BROWSER_ENGINE=1 corepack pnpm build
cd ..

./scripts/build-classic-freeware.sh web/dist
cd web
corepack pnpm preview
```

The script downloads an 8 MB source archive, requires its pinned byte length
and SHA-256, converts a deterministic GDI campaign package, verifies every
entry, rejects music/movie content, and emits `classic-freeware-v1.json` next
to the `.cncweb` archive. Neither file is committed. On first visit the browser
downloads and verifies the sidecar, commits it to origin-private storage, and
launches the first mission; later launches and offline reloads use that stored
revision. The default script output is `.cache/classic-freeware` when no
deployment directory is supplied.

Publishing that directory to external staging is a separate operator action.
Follow the [static-host staging contract](docs/staging-deployment.md) for
atomic deployment, subpath-safe URLs, MIME/cache/security headers, and the
fail-closed verifier plus remote browser matrix. Once a deployment-directory
URL exists, `cd web && corepack pnpm test:classic-freeware:staging` verifies it
before running browsers. No hosting target is configured and no external
publication has been performed from this checkout.

The older local-import workflow remains optional. For its guarded conversion
and browser checklist, see the
[owned-content acceptance guide](docs/owned-content-acceptance.md).

The individual development steps follow.

Build the Wasm engine and install the web dependencies:

```sh
# First source the pinned emsdk environment in this shell.
source /path/to/emsdk/emsdk_env.sh
cmake --workflow --preset web-td

cd web
corepack pnpm install --frozen-lockfile
cd ..
```

Build a personal mission pack outside the repository. The selected path may be
the collection root or its `Data`, `CNCDATA`, `TIBERIAN_DAWN`, or `CD1`
directory:

```sh
export CNC_REMASTERED_ROOT="/path/to/CnC Remastered"

cargo run --locked --manifest-path tools/content-packer/Cargo.toml -- \
  plan-mission "$CNC_REMASTERED_ROOT" --profile td-gdi-01-east-a

cargo run --locked --manifest-path tools/content-packer/Cargo.toml -- \
  convert-mission "$CNC_REMASTERED_ROOT" "$HOME/td-gdi-01.cncweb" \
  --profile td-gdi-01-east-a \
  --package-id td-gdi-01-en-us \
  --provider steam \
  --locale en-US
```

Choose the provider value that describes the owned source: `steam`, `ea-app`,
`copied-installation`, or `unknown`.

The compatibility profile above deliberately remains the one-mission package
used by the guarded acceptance harness. For the full source-implemented
campaign catalogs, use `td-gdi-campaign` with CD1 or `td-nod-campaign` with
CD2, for example:

```sh
cargo run --locked --manifest-path tools/content-packer/Cargo.toml -- \
  convert-mission "$CNC_REMASTERED_ROOT" "$HOME/td-gdi-campaign.cncweb" \
  --profile td-gdi-campaign \
  --package-id td-gdi-campaign-en-us \
  --provider steam \
  --locale en-US
```

Each mission is extracted as its canonical loose INI/BIN pair. Its declared
temperate, desert, or winter theater determines the corresponding data/icon
archive pair included in the package and revalidated by the engine at launch.

Run the development server, open the printed local URL, and optionally choose
**Import pack** to select a personal `.cncweb` file:

```sh
cd web
corepack pnpm dev
```

Import is local and transactional. Before it begins, the confirmation shows
the archive size, available origin-private quota, the local-only storage
promise, and the effect of clearing site data. The compatible package launches
automatically; later visits select the same immutable revision and resume the
latest matching save when one exists. Manual saves, 30-second autosaves, and a
backgrounding autosave stay in origin-private storage. Clearing the site's
browser data removes both imported packages and saves.

For an integrated production/offline bundle without the hosted sidecar:

```sh
cd web
REQUIRE_BROWSER_ENGINE=1 corepack pnpm build
corepack pnpm preview
```

Use `localhost` or HTTPS rather than opening `dist/index.html` through
`file://`. Visit the production build once while online so its service worker
can cache the source-derived shell and engine. Content packages are never put
in that cache; hosted and imported packages remain in origin-private storage.

## Browser-port verification

The same asset-free checks used by CI can be run locally:

```sh
./scripts/check-no-retail-content.sh
./scripts/tests/test-owned-content-acceptance.sh
python3 -m unittest scripts/tests/test_verify_owned_content_package.py

cargo fmt --manifest-path tools/content-packer/Cargo.toml --all -- --check
cargo clippy --locked --manifest-path tools/content-packer/Cargo.toml \
  --all-targets -- -D warnings
cargo test --locked --manifest-path tools/content-packer/Cargo.toml --all-targets

cd web
corepack pnpm typecheck
corepack pnpm test
corepack pnpm test:bundle-budget
corepack pnpm test:classic-freeware:deployment-tools
REQUIRE_BROWSER_ENGINE=1 corepack pnpm build
corepack pnpm check:bundle-budget:integrated
```

The opt-in real freeware conversion gate is:

```sh
./scripts/build-classic-freeware.sh .cache/classic-freeware-acceptance
```

It writes only ignored/generated output and verifies the source archive,
package contents, provenance, and deployment descriptor. Do not commit that
output. After building the integrated web bundle and placing the sidecar in
`web/dist`, `cd web && corepack pnpm test:classic-freeware:release` additionally
proves real Wasm startup, save/load and online/offline recovery, deterministic
public-ABI victories for Missions 1–5, including all three Mission 4 and Mission
5 variants,
genuine native terminal events, canonical continuation through Mission 3,
portrait winter play, single-download behavior, Cache API exclusion, desktop
Chromium/Firefox coverage, and the real-mission performance budget. Linux
Playwright WebKit lacks OPFS and verifies the explicit fallback; physical
Safari remains a device gate.

Run the Mission 3 verifier or the complete Mission 4 and Mission 5 variant
suites independently with:

```sh
cd web
corepack pnpm verify:classic-freeware:mission-three
corepack pnpm verify:classic-freeware:mission-four
corepack pnpm verify:classic-freeware:mission-five
```

The shared `web/scripts/verify-classic-freeware-mission-one.mjs` verifier selects
canonical `SCG03EA`, deploys the MCV, constructs and places the Power Plant →
Barracks → Refinery chain, trains infantry, completes its scout and assault
route milestones, and requires an authoritative GDI victory with no remaining
counted Nod combatants. The Mission 4 alias exercises canonical `SCG04WA`,
`SCG04WB`, and `SCG04EA`. West A and East A complete every reviewed recovery
route milestone, reach authoritative victory with a surviving GDI force, and
leave counted Nod combatants alive; East A also loads and unloads its authored
APC cargo and reaches the eastern staging area. West B receives its authored
GDI reinforcements, eliminates every counted Nod combatant, and wins with at
least one protected village structure surviving. These verifiers use the
public engine ABI rather than the debug victory hook.

The Mission 5 alias exercises canonical `SCG05EA`, `SCG05WA`, and `SCG05WB`.
Each playthrough moves the protected starting force through both authored
relief zones before either early-loss group is eliminated, repairs the damaged
field base, produces infantry and vehicles, triggers the Nod counterattack,
and reaches authoritative victory only after every counted Nod unit and
structure is gone. Repair and force assembly are playthrough guidance; the
native terminal rules remain Nod elimination, GDI survival, and the two
pre-relief special-loss conditions.

A separate Chromium Mission 1 playthrough is deliberately long and opt-in. It
requires the integrated build and classic-freeware sidecar already present in
`web/dist`:

```sh
cd web
corepack pnpm test:classic-freeware:genuine-victory
```

This acceptance follows a fixed, scenario-specific patrol through ordinary
pointer and keyboard controls, finds candidate targets in composited
battlefield screenshots, and validates them through rendered DOM status before
attacking. It does not read simulation snapshots or object/debug state and
does not invoke an acceptance or forced-victory API. The recorded baseline
reached authoritative GDI Mission 1 victory in about 10 minutes, then selected
**GDI Mission 2 (East A)** and verified that Mission 2 launched. It remains outside the
default release command because of that runtime. Because the patrol itself is
scenario-authored, this is visible-only target-acquisition evidence rather
than an autonomous or human-blind fog-of-war playthrough.

Mission 2 has its own longer ordinary-control acceptance:

```sh
cd web
corepack pnpm test:classic-freeware:genuine-mission-two
```

Its recorded run reached authoritative Mission 2 victory using rendered
production and targeting/order evidence. The fast release suite separately
proves the post-victory Mission 3 handoff, including a fault-injected offline
audio-storage failure; a single uninterrupted post-fix long run remains useful
soak evidence rather than a release blocker.

With the Playwright Chromium browser installed, `corepack pnpm
test:performance` builds the source-only shell and repeats its asset-free
startup/tick/frame/long-task gate twice. Its limited evidence boundary and
calibrated values are documented in
[browser performance gates](docs/browser-performance-gates.md).

The content-specific acceptance run must remain on a user-controlled machine.
The guided harness above covers conversion, package/runtime preflight, the
integrated PWA build, and exact C01-C09 manual gates for GDI mission 1 import,
interaction, save/load, victory, refresh/resume, offline refresh/resume, and
the 60-second owned-mission performance sample. Those real owned-content gates
remain unrun in this checkout. Never commit the generated pack, captures,
browser profiles, logs containing retail text, or extracted content.

The portable desktop engine documentation follows.

# Vanilla Conquer
Vanilla Conquer is a fully portable version of the first generation C&C engine and is capable of running both Tiberian Dawn and Red Alert on multiple platforms. It can also be used for mod development for the Remastered Collection.

The main focus of Vanilla Conquer is to keep the default out-of-box experience faithful to what the games were back when they were released and work as a drop-in replacement for the original executables while also providing bug fixes, compatibility and quality of life improvements.

Current project goals are tracked as [GitHub issues with the goal label](https://github.com/Vanilla-Conquer/Vanilla-Conquer/issues?q=is%3Aissue+is%3Aopen+label%3Agoal).

## Chat with us

There are rooms on multiple platforms for discussion:

- [The Assembly Armada](https://discord.gg/UnWK2Tw) on [Discord](https://discord.gg)
- [#vanilla-conquer:vi.fi](https://matrix.to/#/#vanilla-conquer:vi.fi) on [Matrix](https://matrix.org)
- [#vanilla-conquer](https://web.libera.chat/?channel=#vanilla-conquer) on [Libera.Chat](https://libera.chat]) IRC network

All of these rooms are bridged together so people can choose their preferred service. Please be nice to each other.

## Building

We support wide variety of compilers and platforms to target. Vanilla Conquer is known to compile with recent enough gcc, MSVC, mingw-w64 or clang and known to run on Windows, Linux, macOS and BSDs.

### Presets

A [CMakePresets.json](CMakePresets.json) file is provided that contains presets for common build configurations and is used by our CI scripts to build the release builds. These presets require the [Ninja](https://ninja-build.org/) build tool to be available in the system PATH in order to be used.

We also provide an example [CMakeUserPresets.json](resources/CMakeUserPresets.json.example) that can be copied to the root source directory and renamed. You can edit this file to create your own development presets that won't be included in git commits. A few example presets are provided which override the release presets to build a "debug" configuration.

To build using a preset, add `--preset preset_name` to the CMake command line examples below.

### Windows

#### Requirements

The following components are needed to build Vanilla Conquer executables:

 - [MSVC v142 C++ x86/x64 build tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)
 - Windows 10 SDK
 - CMake (installable from MSVC build tools)
 - [SDL1 or SDL2 development libraries, Visual C++](https://libsdl.org/download-2.0.php)
 - [OpenAL Core SDK](https://www.openal.org/downloads/)

Extract SDL2 and OpenAL somewhere you know. If you are building only Remastered dlls you can skip installing SDL2 and OpenAL.

#### Building

In a VS command line window in the Vanilla Conquer source directory:

```sh
cmake -DSDL2_ROOT_DIR=C:\path\to\SDL2 -DOPENAL_ROOT=C:\path\to\OpenAL -B build .
cmake --build build
```

This will build Vanilla Conquer executables in the build directory. If you are building Remastered dlls you need to configure cmake with `-A win32` and ensure your VS command line is x86.

### Linux / macOS / BSD

#### Requirements

- GNU C++ Compiler (g++) or Clang
- CMake
- SDL1 or SDL2
- OpenAL

On Debian/Ubuntu you can install the build requirements as follows:

```sh
sudo apt update
sudo apt install g++ cmake libsdl2-dev libopenal-dev
or
sudo apt install g++ cmake libsdl1.2-dev libopenal-dev
```

On Fedora/RedHat based system you can install the build requirements as follows:

```sh
sudo dnf install gcc-c++ cmake SDL2-devel openal-soft-devel
or
sudo dnf install gcc-c++ cmake SDL-devel openal-soft-devel
```

#### Building

```sh
cmake -B build .
cmake --build build
```

This will build Vanilla Conquer executables in the build directory.

#### macOS considerations

To create a portable bundle for macOS we run [macdylibbundler](https://github.com/auriamg/macdylibbundler) in our CI builds as an extra step to add the SDL2 and OpenAL libraries to the bundle. If you wish to create a portable bundle yourself, you will need to do this step manually as CMake will not currently do it for you.

### Icons

CMake will attempt to generate icons in an appropriate format for Windows and macOS if ImageMagick is found in the system PATH. Otherwise you will end up with generic "program" icons.

## Releases

Binary releases of the latest commit are available from [here](https://github.com/TheAssemblyArmada/Vanilla-Conquer/releases/tag/latest), which is updated whenever new code is merged into the main branch.

## Running

### VanillaTD and VanillaRA

Copy the Vanilla executable (`vanillatd.exe` or `vanillara.exe`) to your legacy game directory, on Windows also copy `SDL2.dll` and `OpenAL32.dll`.

For Tiberian Dawn the final freeware Gold CD release ([GDI](https://www.moddb.com/games/cc-gold/downloads/command-conquer-gold-free-game-gdi-iso), [NOD](https://www.moddb.com/games/cc-gold/downloads/command-conquer-gold-free-game-nod-iso)) works fine.

For Red Alert the freeware [CD release](https://web.archive.org/web/20080901183216/http://www.ea.com/redalert/news-detail.jsp?id=62) works fine as well.
The official [Red Alert demo](https://www.moddb.com/games/cc-red-alert/downloads/command-conquer-red-alert-demo) is also fully playable.
The demo supports custom skirmish maps (except interior) and includes one campaign mission for both Allied and Soviet from the retail game.

While it is possible to use the game data from the Remastered Collection, The Ultimate Collection or The First Decade they are currently _not_ supported.
Any repackaged version that you may already have from any unofficial source is _not_ supported.
If you encounter a bug that may be data related like invisible things or crashing when using a certain unit please retest with the retail data first before submitting a bug report.

### Remastered

The build process will produce _Vanilla_TD_ and _Vanilla_RA_ directories in your build directory if you enable them with `-DBUILD_REMASTERTD=ON` and `-DBUILD_REMASTERRA=ON`.
These work as mods for the Remastered Collection.

To manually install a local Remastered mod, launch both games once then head to _My Documents/CnCRemastered/CnCRemastered/Mods_.
You should see _Tiberian\_Dawn_ and _Red\_Alert_ directories.

#### Tiberian Dawn

Copy the _Vanilla_TD_ directory to the _Tiberian\_Dawn_ directory.

The directory structure should look like this:

    My Documents/CnCRemastered/CnCRemastered/Mods/Tiberian_Dawn/Vanilla_TD/Data/TiberianDawn.dll
    My Documents/CnCRemastered/CnCRemastered/Mods/Tiberian_Dawn/Vanilla_TD/ccmod.json
    My Documents/CnCRemastered/CnCRemastered/Mods/Tiberian_Dawn/Vanilla_TD/GameConstants_Mod.xml

You should now see the new mod in the mods list of Tiberian Dawn Remastered.

#### Red Alert

Copy the _Vanilla_RA_ directory to the _Red\_Alert_ directory.

The directory structure should look like this:

    My Documents/CnCRemastered/CnCRemastered/Mods/Red_Alert/Vanilla_RA/Data/RedAlert.dll
    My Documents/CnCRemastered/CnCRemastered/Mods/Red_Alert/Vanilla_RA/ccmod.json

You should now see the new mod in the mods list of Red Alert Remastered.
