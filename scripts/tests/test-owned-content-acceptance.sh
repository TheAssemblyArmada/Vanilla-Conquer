#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C

readonly test_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly repo_root="$(cd -- "$test_dir/../.." && pwd -P)"
readonly harness="$repo_root/scripts/run-owned-content-acceptance.sh"
readonly performance_verifier="$repo_root/scripts/verify-runtime-performance-report.py"
readonly temporary="$(mktemp -d "${TMPDIR:-/tmp}/cncweb-harness-test.XXXXXXXX")"
trap 'rm -rf -- "$temporary"' EXIT

mkdir -- "$temporary/install"

expect_failure() {
    local expected=$1
    shift
    local output
    local status
    set +e
    output=$("$harness" "$@" 2>&1)
    status=$?
    set -e
    if ((status == 0)); then
        printf 'expected command to fail: %q ' "$harness" "$@" >&2
        printf '\n' >&2
        exit 1
    fi
    if [[ $output != *"$expected"* ]]; then
        printf 'expected failure containing %q, got:\n%s\n' "$expected" "$output" >&2
        exit 1
    fi
}

help_output=$("$harness" --help)
[[ $help_output == *"--dry-run"* ]]
[[ $help_output == *"must be outside this repository"* ]]
[[ $help_output == *"CNCWEB_OWNED_BROWSER_PREFLIGHT=1"* ]]
[[ $help_output == *"does not assess audio, victory, or performance"* ]]

expect_failure "exactly one OWNED_INSTALL_PATH" --dry-run
expect_failure "unknown option" --not-an-option
expect_failure "not an existing directory" --dry-run "$temporary/missing-install"
expect_failure "unsupported provider" --dry-run --provider retail "$temporary/install"
expect_failure "port must be an integer" --dry-run --port 999 "$temporary/install"
expect_failure "port must be an integer" --dry-run --port 999999999999999999 "$temporary/install"
expect_failure "package ID must be" --dry-run --package-id '/bad' "$temporary/install"
expect_failure "owned install path must be outside" --dry-run "$repo_root/scripts"

set +e
invalid_preflight_output=$(CNCWEB_OWNED_BROWSER_PREFLIGHT=yes "$harness" --dry-run "$temporary/install" 2>&1)
invalid_preflight_status=$?
set -e
if ((invalid_preflight_status == 0)) || [[ $invalid_preflight_output != *"must be exactly 0 or 1"* ]]; then
    printf 'invalid owned-browser opt-in was not rejected:\n%s\n' "$invalid_preflight_output" >&2
    exit 1
fi

expect_failure "package output must be outside" \
    --dry-run --work-dir "$temporary/work-output-guard" \
    --output "$repo_root/should-never-exist.cncweb" "$temporary/install"
expect_failure "work directory must be outside" \
    --dry-run --work-dir "$repo_root/should-never-exist" "$temporary/install"
expect_failure "work directory must not modify" \
    --dry-run --work-dir "$temporary/install/work" "$temporary/install"
expect_failure "package output must not modify" \
    --dry-run --work-dir "$temporary/work-install-guard" \
    --output "$temporary/install/pack.cncweb" "$temporary/install"

set +e
tmpdir_guard_output=$(TMPDIR="$temporary/install" "$harness" --dry-run "$temporary/install" 2>&1)
tmpdir_guard_status=$?
set -e
if ((tmpdir_guard_status == 0)) || [[ $tmpdir_guard_output != *"must not modify the owned installation"* ]]; then
    printf 'default temporary work path was not rejected inside the owned installation:\n%s\n' \
        "$tmpdir_guard_output" >&2
    exit 1
fi

touch "$temporary/existing.cncweb"
expect_failure "package output already exists" \
    --dry-run --work-dir "$temporary/work-existing" \
    --output "$temporary/existing.cncweb" "$temporary/install"

ln -s "$repo_root" "$temporary/repo-link"
expect_failure "package output must be outside" \
    --dry-run --work-dir "$temporary/work-symlink" \
    --output "$temporary/repo-link/symlink-leak.cncweb" "$temporary/install"

dry_run_output="$temporary/dry-run.txt"
"$harness" --dry-run \
    --provider steam \
    --package-id synthetic-acceptance \
    --work-dir "$temporary/guided-work" \
    --output "$temporary/guided-pack.cncweb" \
    --port 48173 \
    "$temporary/install" >"$dry_run_output"

grep -Fq "Dry run only" "$dry_run_output"
grep -Fq "Node.js 24.15.0" "$dry_run_output"
grep -Fq "Emscripten 6.0.2" "$dry_run_output"
grep -Fq "plan-mission" "$dry_run_output"
grep -Fq "convert-mission" "$dry_run_output"
grep -Fq "REQUIRE_BROWSER_ENGINE=1" "$dry_run_output"
grep -Fq "C01 import-disclosure" "$dry_run_output"
grep -Fq "verify the UI shows the archive" "$dry_run_output"
grep -Fq "size, available private-storage quota" "$dry_run_output"
grep -Fq "C06 victory-terminal" "$dry_run_output"
grep -Fq "C08 offline-refresh-resume" "$dry_run_output"
grep -Fq "C09 owned-runtime-performance" "$dry_run_output"
grep -Fq "window.__cncwebRuntimeMetrics.snapshot(60000)" "$dry_run_output"
grep -Fq "Provisional single-device thresholds" "$dry_run_output"
grep -Fq "Snapshot byte/rate fields are" "$dry_run_output"
grep -Fq "normal reload of the" "$dry_run_output"
grep -Fq "including its acceptance query" "$dry_run_output"
grep -Fq "Do not use a hard/shift reload" "$dry_run_output"
grep -Fq "Optional private browser preflight: skipped" "$dry_run_output"
if grep -Fq "playwright.owned.config.mjs" "$dry_run_output"; then
    printf 'owned-browser command appeared without its explicit opt-in\n' >&2
    exit 1
fi
[[ ! -e $temporary/guided-work ]]
[[ ! -e $temporary/guided-pack.cncweb ]]
[[ ! -e $repo_root/should-never-exist.cncweb ]]

owned_dry_run_output="$temporary/owned-browser-dry-run.txt"
CNCWEB_OWNED_BROWSER_PREFLIGHT=1 "$harness" --dry-run \
    --provider steam \
    --package-id synthetic-acceptance \
    --work-dir "$temporary/owned-browser-work" \
    --output "$temporary/owned-browser-pack.cncweb" \
    --port 48174 \
    "$temporary/install" >"$owned_dry_run_output"
grep -Fq "Optional private browser preflight (all runner output stays under" "$owned_dry_run_output"
grep -Fq "CNCWEB_OWNED_PREFLIGHT_PACKAGE_PATH=" "$owned_dry_run_output"
grep -Fq "CNCWEB_OWNED_PREFLIGHT_REPORT_DIR=" "$owned_dry_run_output"
grep -Fq "CNCWEB_OWNED_PREFLIGHT_ACCEPTANCE_URL=" "$owned_dry_run_output"
grep -Fq "playwright.owned.config.mjs" "$owned_dry_run_output"
grep -Fq "not assess command semantics, audio, victory, or sustained performance" "$owned_dry_run_output"
[[ ! -e $temporary/owned-browser-work ]]
[[ ! -e $temporary/owned-browser-pack.cncweb ]]

valid_performance_report="$temporary/runtime-performance-valid.json"
readonly expected_performance_build_id="0123456789abcdef"
readonly expected_performance_revision="$(printf 'ab%.0s' {1..32})"
readonly expected_performance_mission="gdi-01-east-a"
readonly expected_performance_session="$(printf '01%.0s' {1..16})"
performance_verifier_arguments=(
    --expected-build-id "$expected_performance_build_id"
    --expected-package-revision "$expected_performance_revision"
    --expected-mission-id "$expected_performance_mission"
    --expected-acceptance-session "$expected_performance_session"
)
python3 - "$valid_performance_report" <<'PY'
import json
import sys

report = {
    "format": "cncweb-runtime-performance",
    "version": 3,
    "core": "wasm",
    "packageRevision": "ab" * 32,
    "missionId": "gdi-01-east-a",
    "buildId": "0123456789abcdef",
    "acceptanceSession": "01" * 16,
    "running": True,
    "visibilityState": "visible",
    "requestedWindowMs": 60000,
    "observedWindowMs": 60000,
    "frames": 3600,
    "rafSpanMs": 59983.33,
    "meanRafIntervalMs": 16.67,
    "p95RafIntervalMs": 20,
    "maximumRafIntervalMs": 80,
    "approximateRafFps": 59.99,
    "observedTickHz": 15,
    "maximumSnapshotGapMs": 90,
    "snapshotSamples": 900,
    "snapshotTickDelta": 899,
    "snapshotSpanMs": 59933.33,
    "latestTick": 1000,
    "longTaskSupported": True,
    "longTasks": 1,
    "maximumLongTaskMs": 70,
    "totalLongTaskMs": 70,
    "classicBaselineUploads": 1,
    "classicDeltaUploads": 700,
    "classicUnchangedUpdates": 199,
    "classicUploadSamples": 900,
    "classicPixelsUploaded": 0,
    "latestSnapshotDeclaredBytes": 200000,
    "maximumSnapshotDeclaredBytes": 200000,
    "meanSnapshotDeclaredBytes": 200000,
    "maximumSnapshotBufferBytes": 262144,
    "snapshotDeclaredBytes": 180000000,
    "snapshotDeclaredBytesPerSecond": 3000000,
    "classicPixelsUploadedPerSecond": 0,
}
with open(sys.argv[1], "w", encoding="utf-8") as destination:
    json.dump(report, destination)
PY
"$performance_verifier" "$valid_performance_report" \
    "${performance_verifier_arguments[@]}" >"$temporary/runtime-performance-valid.txt"
grep -Fq "C09 runtime metrics validated" "$temporary/runtime-performance-valid.txt"

set +e
missing_identity_output=$("$performance_verifier" "$valid_performance_report" 2>&1)
missing_identity_status=$?
set -e
if ((missing_identity_status == 0)) || [[ $missing_identity_output != *"--expected-build-id"* ]]; then
    printf 'C09 verifier accepted evidence without harness identities:\n%s\n' "$missing_identity_output" >&2
    exit 1
fi

expect_performance_rejection() {
    local field=$1
    local json_value=$2
    local expected=$3
    local invalid_performance_report="$temporary/runtime-performance-invalid.json"
    local invalid_performance_output
    local invalid_performance_status
    python3 - "$valid_performance_report" "$invalid_performance_report" "$field" "$json_value" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as source:
    report = json.load(source)
report[sys.argv[3]] = json.loads(sys.argv[4])
with open(sys.argv[2], "w", encoding="utf-8") as destination:
    json.dump(report, destination)
PY
    set +e
    invalid_performance_output=$("$performance_verifier" "$invalid_performance_report" \
        "${performance_verifier_arguments[@]}" 2>&1)
    invalid_performance_status=$?
    set -e
    if ((invalid_performance_status == 0)) || [[ $invalid_performance_output != *"$expected"* ]]; then
        printf 'invalid C09 performance report was not rejected:\n%s\n' "$invalid_performance_output" >&2
        exit 1
    fi
}

expect_performance_rejection p95RafIntervalMs 51 \
    "p95RafIntervalMs must be greater than 0 and at most 50"
expect_performance_rejection maximumRafIntervalMs 301 \
    "maximumRafIntervalMs must be at most 300"
expect_performance_rejection frames 1799 \
    "at least 1800 rendered frames are required"
expect_performance_rejection snapshotSamples 699 \
    "at least 700 snapshot samples are required"
expect_performance_rejection maximumSnapshotGapMs 301 \
    "maximumSnapshotGapMs must be at most 300"
expect_performance_rejection running false \
    "running must be true at report capture"
expect_performance_rejection visibilityState '"hidden"' \
    "visibilityState must be visible at report capture"
expect_performance_rejection observedWindowMs 59999 \
    "observedWindowMs must cover exactly the requested 60000 ms"
expect_performance_rejection meanRafIntervalMs 1 \
    "meanRafIntervalMs is inconsistent with rafSpanMs and frames"
expect_performance_rejection snapshotDeclaredBytes 1 \
    "snapshotDeclaredBytes is inconsistent with the sample maximum"
expect_performance_rejection classicUploadSamples 899 \
    "classicUploadSamples does not equal its upload categories"
expect_performance_rejection classicPixelsUploadedPerSecond 1 \
    "classicPixelsUploadedPerSecond is inconsistent with the observed window"
expect_performance_rejection acceptanceSession '"02020202020202020202020202020202"' \
    "acceptanceSession does not match this acceptance run"
expect_performance_rejection core '"demo"' \
    "core must be wasm"
expect_performance_rejection packageRevision 'null' \
    "packageRevision must be a full lowercase SHA-256 digest"
expect_performance_rejection retailText '"forbidden"' \
    "unexpected report keys: retailText"

duplicate_performance_report="$temporary/runtime-performance-duplicate.json"
python3 - "$valid_performance_report" "$duplicate_performance_report" <<'PY'
import sys

with open(sys.argv[1], "r", encoding="utf-8") as source:
    report = source.read().rstrip()
with open(sys.argv[2], "w", encoding="utf-8") as destination:
    destination.write(report[:-1] + ',"frames":999999}\n')
PY
set +e
duplicate_output=$("$performance_verifier" "$duplicate_performance_report" \
    "${performance_verifier_arguments[@]}" 2>&1)
duplicate_status=$?
set -e
if ((duplicate_status == 0)) || [[ $duplicate_output != *"duplicate JSON key: frames"* ]]; then
    printf 'C09 verifier accepted duplicate JSON keys:\n%s\n' "$duplicate_output" >&2
    exit 1
fi

oversized_performance_report="$temporary/runtime-performance-oversized.json"
python3 - "$oversized_performance_report" <<'PY'
import sys

with open(sys.argv[1], "w", encoding="utf-8") as destination:
    destination.write('{"padding":"' + ('x' * 33000) + '"}\n')
PY
set +e
oversized_output=$("$performance_verifier" "$oversized_performance_report" \
    "${performance_verifier_arguments[@]}" 2>&1)
oversized_status=$?
set -e
if ((oversized_status == 0)) || [[ $oversized_output != *"report exceeds 32768 bytes"* ]]; then
    printf 'C09 verifier accepted an oversized JSON report:\n%s\n' "$oversized_output" >&2
    exit 1
fi

mkdir -- "$temporary/fake-toolchain"
printf '#!/bin/sh\nprintf "emcc (Emscripten synthetic) 0.0.0\\n"\n' >"$temporary/fake-toolchain/emcc"
printf '#!/bin/sh\nexit 0\n' >"$temporary/fake-toolchain/cargo"
chmod +x "$temporary/fake-toolchain/emcc" "$temporary/fake-toolchain/cargo"
PATH="$temporary/fake-toolchain:$PATH" expect_failure "Emscripten 6.0.2 is required" \
    --work-dir "$temporary/wrong-emcc-work" "$temporary/install"

mkdir -- "$temporary/fake-node-toolchain"
printf '#!/bin/sh\nprintf "v0.0.0\\n"\n' >"$temporary/fake-node-toolchain/node"
printf '#!/bin/sh\nprintf "emcc (Emscripten synthetic) 6.0.2\\n"\n' >"$temporary/fake-node-toolchain/emcc"
printf '#!/bin/sh\nexit 0\n' >"$temporary/fake-node-toolchain/cargo"
chmod +x "$temporary/fake-node-toolchain/node" "$temporary/fake-node-toolchain/emcc" "$temporary/fake-node-toolchain/cargo"
PATH="$temporary/fake-node-toolchain:$PATH" expect_failure "Node.js 24.15.0 is required" \
    --work-dir "$temporary/wrong-node-work" "$temporary/install"

node --test "$repo_root/web/scripts/test-owned-browser-preflight-env.mjs" >/dev/null

printf 'Owned-content acceptance harness argument/path guards passed.\n'
