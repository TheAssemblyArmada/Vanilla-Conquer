#!/usr/bin/env bash
set -euo pipefail

umask 077
export LC_ALL=C

readonly script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly repo_root="$(cd -- "$script_dir/.." && pwd -P)"
readonly packer_manifest="$repo_root/tools/content-packer/Cargo.toml"
readonly metadata_verifier="$script_dir/verify-owned-content-package.py"
readonly performance_verifier="$script_dir/verify-runtime-performance-report.py"
readonly owned_lifecycle_verifier="$repo_root/web/scripts/verify-owned-engine-lifecycle.mjs"
readonly profile="td-gdi-01-east-a"
readonly expected_node_version="$(tr -d '\r\n' <"$repo_root/.node-version")"
readonly expected_emscripten_version="$(tr -d '\r\n' <"$repo_root/emscripten-version.txt")"

provider="unknown"
package_id="td-gdi-01-local"
port=""
output_path=""
work_dir=""
dry_run=0
serve_only=0
server_pid=""
build_id=""
package_revision=""
mission_id=""
owned_browser_preflight=${CNCWEB_OWNED_BROWSER_PREFLIGHT:-0}
owned_browser_preflight_ran=0

usage() {
    cat <<'EOF'
Usage:
  ./scripts/run-owned-content-acceptance.sh [options] OWNED_INSTALL_PATH

Convert a legally owned C&C Remastered Collection installation into the first
GDI mission package, verify it, build and serve the integrated offline PWA,
then guide the operator through the manual milestone checkpoints.

Options:
  --provider VALUE    steam, ea-app, copied-installation, or unknown
                      (default: unknown)
  --package-id ID     Local package identity (default: td-gdi-01-local)
  --output FILE       New .cncweb output path; must be outside this repository
                      (default: inside a new private temporary work directory)
  --work-dir DIR      New directory for content-sensitive local reports; must
                      be outside this repository (default: private /tmp dir)
  --port PORT         Local preview port, 1024-65535 (default: a fresh free port)
  --serve-only        Print the checklist and serve until interrupted without
                      prompting for or recording manual pass/fail observations
  --dry-run           Validate arguments and print actions without writing,
                      converting, building, or starting a server
  -h, --help          Show this help

The owned installation, generated pack, conversion reports, and acceptance
record are required to remain outside the source tree. Output/work paths also
cannot be inside the owned installation, which is treated as read-only. The
harness never uploads content or records the locally derived mission briefing.

Optional private browser preflight:
  Set CNCWEB_OWNED_BROWSER_PREFLIGHT=1 to run the Playwright preflight against
  this harness's preview and external .cncweb package before the manual
  checklist. Its browser output and content-safe report remain in --work-dir.
  It exercises import/replacement, command controls, save/load, and online and
  offline resume; it does not assess audio, victory, or performance.

Example:
  ./scripts/run-owned-content-acceptance.sh --provider steam \
    "/games/CnC Remastered Collection"
EOF
}

die() {
    printf 'error: %s\n' "$*" >&2
    exit 2
}

require_option_value() {
    local option=$1
    local count=$2
    ((count >= 2)) || die "$option requires a value"
}

canonical_path() {
    python3 - "$1" <<'PY'
import os
import sys

print(os.path.realpath(os.path.abspath(sys.argv[1])))
PY
}

is_in_repo() {
    local path=$1
    [[ $path == "$repo_root" || $path == "$repo_root/"* ]]
}

guard_external_path() {
    local label=$1
    local path=$2
    if is_in_repo "$path"; then
        die "$label must be outside the repository"
    fi
}

is_within_path() {
    local candidate=$1
    local root=$2
    [[ $candidate == "$root" || $candidate == "$root/"* ]]
}

print_command() {
    printf '  '
    printf '%q ' "$@"
    printf '\n'
}

print_web_command() {
    printf '  (cd %q && ' "$repo_root/web"
    printf '%q ' "$@"
    printf ')\n'
}

cleanup_server() {
    if [[ -n $server_pid ]] && kill -0 "$server_pid" 2>/dev/null; then
        kill "$server_pid" 2>/dev/null || true
        wait "$server_pid" 2>/dev/null || true
    fi
}

on_signal() {
    exit 130
}

trap cleanup_server EXIT
trap on_signal INT TERM

positionals=()
while (($# > 0)); do
    case $1 in
        --provider)
            require_option_value "$1" "$#"
            provider=$2
            shift 2
            ;;
        --package-id)
            require_option_value "$1" "$#"
            package_id=$2
            shift 2
            ;;
        --output)
            require_option_value "$1" "$#"
            output_path=$2
            shift 2
            ;;
        --work-dir)
            require_option_value "$1" "$#"
            work_dir=$2
            shift 2
            ;;
        --port)
            require_option_value "$1" "$#"
            port=$2
            shift 2
            ;;
        --serve-only)
            serve_only=1
            shift
            ;;
        --dry-run)
            dry_run=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            positionals+=("$@")
            break
            ;;
        -*)
            die "unknown option: $1"
            ;;
        *)
            positionals+=("$1")
            shift
            ;;
    esac
done

case $owned_browser_preflight in
    0|1) ;;
    *) die "CNCWEB_OWNED_BROWSER_PREFLIGHT must be exactly 0 or 1" ;;
esac

((${#positionals[@]} == 1)) || die "exactly one OWNED_INSTALL_PATH is required; use --help for usage"
command -v python3 >/dev/null 2>&1 || die "python3 is required"

case $provider in
    steam|ea-app|copied-installation|unknown) ;;
    *) die "unsupported provider: $provider" ;;
esac

if ((${#package_id} == 0 || ${#package_id} > 128)) \
    || [[ ! $package_id =~ ^[[:alnum:]][[:alnum:]_.-]*$ ]]; then
    die "package ID must be 1-128 ASCII letters, digits, dots, underscores, or dashes and start alphanumeric"
fi

if [[ -z $port ]]; then
    # A new origin prevents an older localhost service worker/cache from
    # satisfying this run's offline checkpoints with stale build artifacts.
    port=$(python3 <<'PY'
import socket

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
    listener.bind(("127.0.0.1", 0))
    print(listener.getsockname()[1])
PY
    )
fi

[[ $port =~ ^[0-9]+$ && ${#port} -le 5 ]] || die "port must be an integer from 1024 to 65535"
port_number=$((10#$port))
((port_number >= 1024 && port_number <= 65535)) || die "port must be an integer from 1024 to 65535"
port=$port_number
acceptance_session=$(python3 <<'PY'
import secrets

print(secrets.token_hex(16))
PY
)

install_root=${positionals[0]}
[[ -d $install_root ]] || die "owned install path is not an existing directory"
install_root=$(canonical_path "$install_root")
guard_external_path "owned install path" "$install_root"

if [[ -n $work_dir ]]; then
    work_dir=$(canonical_path "$work_dir")
    guard_external_path "work directory" "$work_dir"
    if is_within_path "$work_dir" "$install_root"; then
        die "work directory must not modify the owned installation"
    fi
    [[ ! -e $work_dir ]] || die "work directory already exists; choose a new path"
elif ((dry_run)); then
    work_dir=$(canonical_path "${TMPDIR:-/tmp}/cncweb-acceptance.DRY-RUN")
    guard_external_path "temporary work directory" "$work_dir"
    if is_within_path "$work_dir" "$install_root"; then
        die "temporary work directory must not modify the owned installation"
    fi
else
    temp_root=$(canonical_path "${TMPDIR:-/tmp}")
    [[ -d $temp_root ]] || die "temporary directory root does not exist"
    guard_external_path "temporary directory root" "$temp_root"
    if is_within_path "$temp_root" "$install_root"; then
        die "temporary directory root must not be inside the owned installation"
    fi
    work_dir=$(mktemp -d "$temp_root/cncweb-acceptance.XXXXXXXX")
fi

if [[ -z $output_path ]]; then
    output_path="$work_dir/td-gdi-01.cncweb"
fi
output_path=$(canonical_path "$output_path")
guard_external_path "package output" "$output_path"
if is_within_path "$output_path" "$install_root"; then
    die "package output must not modify the owned installation"
fi
[[ $output_path == *.cncweb ]] || die "package output must end in .cncweb"
[[ ! -e $output_path ]] || die "package output already exists; refusing to overwrite it"

if ((!dry_run)); then
    if [[ ! -d $work_dir ]]; then
        work_parent=$(dirname -- "$work_dir")
        [[ -d $work_parent ]] || die "work directory parent does not exist"
        mkdir -m 700 -- "$work_dir"
    fi
    output_parent=$(dirname -- "$output_path")
    [[ -d $output_parent ]] || die "package output parent does not exist"
fi

readonly base_url="http://127.0.0.1:$port"
readonly acceptance_url="$base_url/?acceptance=$acceptance_session"
readonly plan_report="$work_dir/source-plan.json"
readonly plan_errors="$work_dir/source-plan.stderr"
readonly conversion_report="$work_dir/conversion-report.json"
readonly conversion_errors="$work_dir/conversion.stderr"
readonly verified_manifest="$work_dir/verified-manifest.json"
readonly runtime_preflight="$work_dir/runtime-preflight.json"
readonly acceptance_results="$work_dir/acceptance-results.tsv"
readonly toolchain_report="$work_dir/toolchain-versions.tsv"
readonly performance_report="$work_dir/runtime-performance-c09.json"
readonly performance_context="$work_dir/runtime-performance-c09-context.txt"
readonly owned_lifecycle_report="$work_dir/owned-engine-lifecycle.jsonl"
readonly owned_browser_report_dir="$work_dir/owned-browser-preflight"
readonly owned_browser_report="$owned_browser_report_dir/owned-browser-preflight.json"
readonly owned_browser_stdout="$owned_browser_report_dir/playwright.stdout"
readonly owned_browser_stderr="$owned_browser_report_dir/playwright.stderr"

print_checklist() {
    cat <<EOF

Manual browser acceptance checkpoints
=====================================

Use this pack in the browser file picker:
  $output_path

Open:
  $acceptance_url

Keep this exact URL, including its acceptance query, through every refresh.
It binds the observations and runtime report to this fresh local run.

The optional browser preflight does not replace any manual checkpoint and does
not assess command semantics, audio, victory, or sustained performance.

Record PASS only after directly observing every item:

  C01 import-disclosure
      Choose Import pack. Before confirming, verify the UI shows the archive
      size, available private-storage quota, the local-only storage promise,
      the consequence of clearing site data, and that installing ends the
      current simulation. Confirm the import.

  C02 real-mission-launch
      The compatible revision auto-selects and launches GDI Mission 1 in
      classic presentation. The locally derived briefing appears, the game is
      running, and no required-content or Wasm startup diagnostic is shown.

  C03 interaction-and-presentation
      Select and box-select units, issue a contextual order and Stop, pan and
      zoom the camera (including mouse/keyboard controls on desktop), and
      confirm the live minimap plus credits/power/combat telemetry update.
      After a user gesture, confirm both speech and sound.

  C04 manual-save-load
      Make a manual mid-mission save, issue state-changing commands, then load
      that save. Confirm the saved simulation state and camera pan/zoom return,
      and that play resumes without re-importing content.

  C05 online-refresh-resume
      Make or wait for a newer save and reload the page with networking on.
      Confirm the exact immutable package revision is selected and its newest
      matching save resumes automatically.

  C06 victory-terminal
      Finish GDI Mission 1. Confirm the victory/score UI appears and gameplay
      reaches a stable terminal state rather than continuing to advance.

  C07 post-victory-load
      From the terminal UI, choose Load latest manual to restore the earlier
      mid-mission save. Confirm victory state clears and the restored mission
      advances and accepts commands.

  C08 offline-refresh-resume
      First confirm the page is controlled by its service worker. In browser
      developer tools set Network to Offline, then use a normal reload of the
      exact acceptance URL above. Confirm the shell and Wasm engine load,
      the OPFS package is selected, the matching save resumes, and commands
      still work without a network response. Do not use a hard/shift reload,
      which may deliberately bypass the service worker and its cache.

  C09 owned-runtime-performance
      Restore browser networking, keep the game tab visible, and let the
      resumed real mission run for at least 60 uninterrupted seconds without
      loading, pausing, or forcing a graphics-context loss. In DevTools run:

        copy(JSON.stringify(window.__cncwebRuntimeMetrics.snapshot(60000)))

      At the C09 terminal prompt, enter pass and paste that one-line JSON when
      requested. The harness stores only content-safe metrics and the runtime,
      revision, mission, build, and acceptance-session identities in:
        $performance_report

      This manually pasted report is operator-attested evidence, not a
      cryptographic attestation. The verifier rejects stale identities,
      discontinuous windows, and internally inconsistent or malformed JSON,
      but a person with DevTools access can still fabricate browser values.

      Provisional single-device thresholds are 12.5-17.5 simulation Hz, at
      least 1,800 frames and 700 snapshots, at most 300 ms between snapshots
      or RAF samples (including window edges), at most 30/50 ms mean/p95 RAF
      intervals, at most three long tasks with none over 250 ms, and no more
      than one full classic baseline upload. Snapshot byte/rate fields are
      recorded for calibration, not treated as proven cross-device budgets.

Keep browser networking restored after C09. Do not copy the pack, screenshots,
browser profiles, or logs containing retail text into this repository.
EOF
}

validate_runtime_performance_report() {
    python3 "$performance_verifier" "$1" \
        --expected-build-id "$build_id" \
        --expected-package-revision "$package_revision" \
        --expected-mission-id "$mission_id" \
        --expected-acceptance-session "$acceptance_session"
}

capture_runtime_performance_report() {
    local report_json
    local context_note
    printf 'Paste one-line C09 JSON from DevTools: '
    IFS= read -r report_json || return 1
    [[ -n $report_json ]] || {
        printf 'C09 JSON cannot be empty.\n' >&2
        return 1
    }
    printf '%s\n' "$report_json" >"$performance_report"
    validate_runtime_performance_report "$performance_report" || return 1
    printf 'Browser version and device/profile note (content-safe, one line): '
    IFS= read -r context_note || return 1
    if [[ -z $context_note || ${#context_note} -gt 512 ]]; then
        printf 'C09 context note must contain 1-512 characters.\n' >&2
        return 1
    fi
    printf '%s\n' "$context_note" >"$performance_context"
}

if ((dry_run)); then
    cat <<EOF
Dry run only: no files, builds, packages, or servers will be created.

Pinned build toolchains:
  Node.js $expected_node_version
  Emscripten $expected_emscripten_version

Owned-content plan and conversion (stdout/stderr stay in the external work directory):
EOF
    print_command cargo run --quiet --locked --manifest-path "$packer_manifest" -- \
        plan-mission "$install_root" --profile "$profile"
    print_command cargo run --quiet --locked --manifest-path "$packer_manifest" -- \
        convert-mission "$install_root" "$output_path" --profile "$profile" \
        --package-id "$package_id" --provider "$provider" --locale en-US --quiet
    print_command cargo run --quiet --locked --manifest-path "$packer_manifest" -- \
        verify "$output_path"
    print_command python3 "$metadata_verifier" "$output_path" \
        --expected-package-id "$package_id" --expected-provider "$provider"
    cat <<'EOF'

Integrated engine and PWA build:
EOF
    print_command cmake --workflow --preset web-td
    print_web_command corepack pnpm install --frozen-lockfile
    print_web_command env REQUIRE_BROWSER_ENGINE=1 corepack pnpm build
    print_web_command corepack pnpm exec vite preview \
        --host 127.0.0.1 --port "$port" --strictPort
    print_command node "$owned_lifecycle_verifier" \
        "$repo_root/build/web-td/tiberiandawn.js" "$base_url/engine/" \
        "$output_path" VERIFIED_PACKAGE_REVISION
    if ((owned_browser_preflight)); then
        cat <<EOF

Optional private browser preflight (all runner output stays under $owned_browser_report_dir):
EOF
        print_web_command env \
            CNCWEB_OWNED_BROWSER_PREFLIGHT=1 \
            CNCWEB_OWNED_PREFLIGHT_PACKAGE_PATH="$output_path" \
            CNCWEB_OWNED_PREFLIGHT_REPORT_DIR="$owned_browser_report_dir" \
            CNCWEB_OWNED_PREFLIGHT_BASE_URL="$base_url" \
            CNCWEB_OWNED_PREFLIGHT_ACCEPTANCE_URL="$acceptance_url" \
            CNCWEB_OWNED_PREFLIGHT_PACKAGE_ID="$package_id" \
            CNCWEB_OWNED_PREFLIGHT_PACKAGE_REVISION=VERIFIED_PACKAGE_REVISION \
            CNCWEB_OWNED_PREFLIGHT_MISSION_ID=gdi-01-east-a \
            CNCWEB_OWNED_PREFLIGHT_BUILD_ID=VERIFIED_BUILD_ID \
            CNCWEB_OWNED_PREFLIGHT_ACCEPTANCE_SESSION="$acceptance_session" \
            corepack pnpm exec playwright test --config playwright.owned.config.mjs
    else
        cat <<'EOF'

Optional private browser preflight: skipped (set CNCWEB_OWNED_BROWSER_PREFLIGHT=1 to enable).
EOF
    fi
    print_checklist
    exit 0
fi

for required_command in cargo cmake ninja node corepack curl python3; do
    command -v "$required_command" >/dev/null 2>&1 || die "$required_command is required and was not found in PATH"
done
command -v emcc >/dev/null 2>&1 \
    || die "emcc is required; source the pinned Emscripten SDK environment before running this harness"
[[ -x $repo_root/scripts/check-no-retail-content.sh ]] || die "retail-content guard is missing or not executable"
[[ -f $metadata_verifier ]] || die "owned-content metadata verifier is missing"
[[ -f $performance_verifier ]] || die "runtime performance verifier is missing"
[[ -f $owned_lifecycle_verifier ]] || die "owned engine lifecycle verifier is missing"

actual_node_version=$(node --version)
actual_node_version=${actual_node_version#v}
[[ $actual_node_version == "$expected_node_version" ]] \
    || die "Node.js $expected_node_version is required; found $actual_node_version"
emcc_banner=$(emcc --version | sed -n '1p')
[[ " $emcc_banner " == *" $expected_emscripten_version "* ]] \
    || die "Emscripten $expected_emscripten_version is required; source the pinned SDK environment"
printf 'tool\tversion\nNode.js\t%s\nEmscripten\t%s\n' \
    "$actual_node_version" "$expected_emscripten_version" >"$toolchain_report"

printf '[1/10] Checking the source tree content boundary...\n'
"$repo_root/scripts/check-no-retail-content.sh"

printf '[2/10] Inspecting the owned installation (private report: %s)...\n' "$plan_report"
if ! cargo run --quiet --locked --manifest-path "$packer_manifest" -- \
    plan-mission "$install_root" --profile "$profile" \
    >"$plan_report" 2>"$plan_errors"; then
    printf 'Installation inspection failed. Content-sensitive details remain in:\n  %s\n' "$plan_errors" >&2
    exit 1
fi

printf '[3/10] Converting and verifying the local mission pack; this can take several minutes...\n'
if ! cargo run --quiet --locked --manifest-path "$packer_manifest" -- \
    convert-mission "$install_root" "$output_path" --profile "$profile" \
    --package-id "$package_id" --provider "$provider" --locale en-US --quiet \
    >"$conversion_report" 2>"$conversion_errors"; then
    printf 'Conversion failed. Content-sensitive details remain in:\n  %s\n' "$conversion_errors" >&2
    exit 1
fi
[[ -s $output_path ]] || die "converter reported success but did not create a package"

printf '[4/10] Streaming every packaged byte through the Rust verifier...\n'
cargo run --quiet --locked --manifest-path "$packer_manifest" -- \
    verify "$output_path" >"$verified_manifest"

printf '[5/10] Validating the mission catalog, audio index, and engine preflight inventory...\n'
python3 "$metadata_verifier" "$output_path" \
    --expected-package-id "$package_id" --expected-provider "$provider" \
    >"$runtime_preflight"
cat "$runtime_preflight"
mapfile -t runtime_identity < <(python3 - "$runtime_preflight" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as source:
    report = json.load(source)
print(report.get("packageRevision", ""))
print(report.get("missionId", ""))
PY
)
package_revision=${runtime_identity[0]:-}
mission_id=${runtime_identity[1]:-}
[[ $package_revision =~ ^[a-f0-9]{64}$ ]] || die "runtime preflight did not emit a full package revision"
[[ $mission_id == gdi-01-east-a ]] || die "runtime preflight emitted an unexpected mission identity"

printf '[6/10] Building the production WebAssembly engine and integrated PWA...\n'
cmake --workflow --preset web-td
(
    cd -- "$repo_root/web"
    corepack pnpm install --frozen-lockfile
    REQUIRE_BROWSER_ENGINE=1 corepack pnpm build
)

readonly engine_js="$repo_root/web/dist/engine/tiberiandawn.js"
readonly engine_wasm="$repo_root/web/dist/engine/tiberiandawn.wasm"
readonly service_worker="$repo_root/web/dist/sw.js"
readonly build_identity="$repo_root/web/dist/build-v1.json"
[[ -s $engine_js && -s $engine_wasm && -s $service_worker && -s $build_identity ]] \
    || die "integrated production build is missing an engine or service worker artifact"
build_id=$(python3 - "$build_identity" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as source:
    value = json.load(source)
if set(value) != {"format", "version", "id"} or value.get("format") != "cncweb-build" or value.get("version") != 1:
    raise SystemExit("invalid build identity schema")
print(value.get("id", ""))
PY
)
[[ $build_id =~ ^[a-f0-9]{16}$ ]] || die "production build identity is invalid"
grep -Fq 'engine/tiberiandawn.wasm' "$service_worker" \
    || die "service worker does not precache the Wasm engine"
if grep -Fq '__THEATER_' "$service_worker"; then
    die "service worker still contains an unexpanded build placeholder"
fi

printf '[7/10] Starting the localhost preview and probing production routes...\n'
(
    cd -- "$repo_root/web"
    exec corepack pnpm exec vite preview --host 127.0.0.1 --port "$port" --strictPort
) >"$work_dir/preview.log" 2>&1 &
server_pid=$!

server_ready=0
for _attempt in {1..100}; do
    if ! kill -0 "$server_pid" 2>/dev/null; then
        printf 'Preview server exited early. Source-only details are in:\n  %s\n' "$work_dir/preview.log" >&2
        exit 1
    fi
    if curl --fail --silent --output /dev/null "$base_url/"; then
        server_ready=1
        break
    fi
    sleep 0.1
done
((server_ready)) || die "preview server did not become ready on $base_url"

curl --fail --silent --show-error --dump-header "$work_dir/engine-js.headers" \
    --output "$work_dir/served-engine.js" "$base_url/engine/tiberiandawn.js"
curl --fail --silent --show-error --dump-header "$work_dir/engine-wasm.headers" \
    --output "$work_dir/served-engine.wasm" "$base_url/engine/tiberiandawn.wasm"
curl --fail --silent --show-error --output "$work_dir/served-sw.js" "$base_url/sw.js"
curl --fail --silent --show-error --output "$work_dir/served-build-v1.json" "$base_url/build-v1.json"
grep -Eiq '^content-type:[[:space:]]*(text|application)/javascript' "$work_dir/engine-js.headers" \
    || die "engine JavaScript route has the wrong media type"
grep -Eiq '^content-type:[[:space:]]*application/wasm' "$work_dir/engine-wasm.headers" \
    || die "engine Wasm route has the wrong media type"
cmp -s "$engine_js" "$work_dir/served-engine.js" \
    || die "served engine JavaScript differs from the production build"
cmp -s "$engine_wasm" "$work_dir/served-engine.wasm" \
    || die "served engine Wasm differs from the production build"
cmp -s "$service_worker" "$work_dir/served-sw.js" \
    || die "served service worker differs from the production build"
cmp -s "$build_identity" "$work_dir/served-build-v1.json" \
    || die "served build identity differs from the production build"
grep -Fq "$build_id" "$service_worker" \
    || die "service-worker and page build identities differ"

node "$repo_root/scripts/smoke-web-engine.mjs" \
    "$repo_root/build/web-td/tiberiandawn.js" "$base_url/engine/" \
    >"$work_dir/wasm-preflight-smoke.json"

printf '[8/10] Running the private real-engine save/replay/relaunch lifecycle...\n'
node "$owned_lifecycle_verifier" \
    "$repo_root/build/web-td/tiberiandawn.js" "$base_url/engine/" \
    "$output_path" "$package_revision" >"$owned_lifecycle_report"
python3 - "$owned_lifecycle_report" "$package_revision" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as source:
    lines = [line for line in source if line.strip()]
if not lines:
    raise SystemExit("owned lifecycle verifier emitted no report")
report = json.loads(lines[-1])
expected_keys = {
    "format",
    "version",
    "packageRevision",
    "missionId",
    "startupEvents",
    "saveBytes",
    "replayTicks",
    "sameInstanceReplay",
    "freshInstanceReplay",
}
if not isinstance(report, dict):
    raise SystemExit("owned lifecycle verifier emitted an invalid report")
counts = (report.get("startupEvents"), report.get("saveBytes"), report.get("replayTicks"))
if (
    set(report) != expected_keys
    or report.get("format") != "cncweb-owned-engine-lifecycle"
    or report.get("version") != 1
    or report.get("packageRevision") != sys.argv[2]
    or report.get("missionId") != "gdi-01-east-a"
    or any(isinstance(value, bool) or not isinstance(value, int) for value in counts)
    or report.get("startupEvents", 0) <= 0
    or report.get("saveBytes", 0) <= 60
    or report.get("replayTicks") != 30
    or report.get("sameInstanceReplay") is not True
    or report.get("freshInstanceReplay") is not True
):
    raise SystemExit("owned lifecycle verifier emitted an invalid report")
PY

printf '[9/10] Optional private owned-browser import/save/offline preflight...\n'
if ((owned_browser_preflight)); then
    mkdir -m 700 -- "$owned_browser_report_dir"
    if ! (
        cd -- "$repo_root/web"
        env \
            CNCWEB_OWNED_BROWSER_PREFLIGHT=1 \
            CNCWEB_OWNED_PREFLIGHT_PACKAGE_PATH="$output_path" \
            CNCWEB_OWNED_PREFLIGHT_REPORT_DIR="$owned_browser_report_dir" \
            CNCWEB_OWNED_PREFLIGHT_BASE_URL="$base_url" \
            CNCWEB_OWNED_PREFLIGHT_ACCEPTANCE_URL="$acceptance_url" \
            CNCWEB_OWNED_PREFLIGHT_PACKAGE_ID="$package_id" \
            CNCWEB_OWNED_PREFLIGHT_PACKAGE_REVISION="$package_revision" \
            CNCWEB_OWNED_PREFLIGHT_MISSION_ID="$mission_id" \
            CNCWEB_OWNED_PREFLIGHT_BUILD_ID="$build_id" \
            CNCWEB_OWNED_PREFLIGHT_ACCEPTANCE_SESSION="$acceptance_session" \
            corepack pnpm exec playwright test --config playwright.owned.config.mjs
    ) >"$owned_browser_stdout" 2>"$owned_browser_stderr"; then
        printf 'Owned-browser preflight failed. Private runner details remain in:\n  %s\n  %s\n' \
            "$owned_browser_stdout" "$owned_browser_stderr" >&2
        exit 1
    fi
    [[ -s $owned_browser_report ]] || die "owned-browser preflight did not create its private report"
    python3 - "$owned_browser_report" "$package_id" "$package_revision" "$mission_id" "$build_id" "$acceptance_session" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as source:
    report = json.load(source)
expected_top = {"format", "version", "identity", "checks", "ticks", "notAssessed"}
if set(report) != expected_top or report.get("format") != "cncweb-owned-browser-preflight" or report.get("version") != 1:
    raise SystemExit("owned-browser preflight report schema is invalid")
identity = report.get("identity")
if not isinstance(identity, dict) or set(identity) != {"packageId", "packageRevision", "missionId", "buildId", "acceptanceSession", "core"}:
    raise SystemExit("owned-browser preflight identity schema is invalid")
if identity != {
    "packageId": sys.argv[2],
    "packageRevision": sys.argv[3],
    "missionId": sys.argv[4],
    "buildId": sys.argv[5],
    "acceptanceSession": sys.argv[6],
    "core": "wasm",
}:
    raise SystemExit("owned-browser preflight identity does not match this harness run")
expected_checks = {
    "disclosureImport",
    "sameRevisionReplacement",
    "exactRuntimeMetricsIdentity",
    "commandControlsAndIngressExercised",
    "cameraControls",
    "manualSaveAdvanceLoad",
    "onlineRefreshResume",
    "serviceWorkerControl",
    "offlineExactUrlReloadResume",
}
checks = report.get("checks")
if not isinstance(checks, dict) or set(checks) != expected_checks or any(value is not True for value in checks.values()):
    raise SystemExit("owned-browser preflight checks are incomplete")
ticks = report.get("ticks")
expected_ticks = {"saved", "advanced", "loaded", "onlineResumed", "offlineResumed"}
if not isinstance(ticks, dict) or set(ticks) != expected_ticks or any(isinstance(value, bool) or not isinstance(value, int) or value < 0 for value in ticks.values()):
    raise SystemExit("owned-browser preflight tick evidence is invalid")
if not (
    ticks["saved"] <= ticks["loaded"] < ticks["advanced"]
    and ticks["saved"] <= ticks["onlineResumed"] < ticks["advanced"]
    and ticks["saved"] <= ticks["offlineResumed"] < ticks["advanced"]
):
    raise SystemExit("owned-browser preflight resume ticks do not describe the saved timeline")
if report.get("notAssessed") != ["audio", "command-semantics", "performance", "victory"]:
    raise SystemExit("owned-browser preflight must preserve the manual evidence boundary")
PY
    owned_browser_preflight_ran=1
    printf 'Private owned-browser preflight passed; it did not assess audio, victory, or performance.\n'
else
    printf 'Skipped; set CNCWEB_OWNED_BROWSER_PREFLIGHT=1 to enable this external-content check.\n'
fi

printf '[10/10] Rechecking that no owned/generated content entered the source tree...\n'
"$repo_root/scripts/check-no-retail-content.sh"

cat >"$acceptance_results" <<EOF
kind	checkpoint	result
automated	source-content-boundary	PASS
automated	owned-install-plan	PASS
automated	package-stream-verification	PASS
automated	runtime-preflight-metadata	PASS
automated	integrated-production-build	PASS
automated	localhost-engine-routes	PASS
automated	wasm-abi-preflight-diagnostic	PASS
automated	owned-wasm-save-replay-relaunch	PASS
automated	pinned-build-toolchains	PASS
$(if ((owned_browser_preflight_ran)); then printf 'automated\towned-browser-private-preflight\tPASS'; else printf 'optional\towned-browser-private-preflight\tNOT-RUN'; fi)
identity	build-id	$build_id
identity	package-revision	$package_revision
identity	mission-id	$mission_id
identity	acceptance-session	$acceptance_session
EOF

print_checklist

if ((serve_only)); then
    printf '\nPreview is running. No manual PASS observations will be recorded. Press Ctrl-C to stop.\n'
    wait "$server_pid"
    exit $?
fi

if [[ ! -t 0 ]]; then
    printf '\nA terminal is required for guided pass/fail recording. Re-run with --serve-only to serve without prompts.\n' >&2
    exit 2
fi

checkpoint_ids=(
    import-disclosure
    real-mission-launch
    interaction-and-presentation
    manual-save-load
    online-refresh-resume
    victory-terminal
    post-victory-load
    offline-refresh-resume
    owned-runtime-performance
)

printf '\nThe automated gates passed. Confirm each user-observed checkpoint explicitly.\n'
for checkpoint_id in "${checkpoint_ids[@]}"; do
    while true; do
        printf '%s [pass/fail/quit]: ' "$checkpoint_id"
        if ! IFS= read -r response; then
            response=quit
        fi
        response=${response,,}
        case $response in
            pass)
                if [[ $checkpoint_id == owned-runtime-performance ]] \
                    && ! capture_runtime_performance_report; then
                    printf 'C09 evidence was not accepted; capture a fresh uninterrupted 60-second report and retry.\n' >&2
                    continue
                fi
                printf 'manual\t%s\tPASS\n' "$checkpoint_id" >>"$acceptance_results"
                break
                ;;
            fail)
                printf 'manual\t%s\tFAIL\n' "$checkpoint_id" >>"$acceptance_results"
                printf 'Manual acceptance stopped at %s. No milestone success is claimed.\n' "$checkpoint_id" >&2
                exit 1
                ;;
            quit)
                printf 'manual\t%s\tNOT-RUN\n' "$checkpoint_id" >>"$acceptance_results"
                printf 'Manual acceptance was not completed. No milestone success is claimed.\n' >&2
                exit 1
                ;;
            *)
                printf 'Enter exactly pass, fail, or quit.\n'
                ;;
        esac
    done
done

printf 'summary\tall-required-checkpoints\tPASS\n' >>"$acceptance_results"
printf '\nAll automated and explicitly observed checkpoints are recorded PASS.\n'
printf 'Content-sensitive local workspace (do not commit or upload):\n  %s\n' "$work_dir"
printf 'Acceptance record:\n  %s\n' "$acceptance_results"
