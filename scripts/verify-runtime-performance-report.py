#!/usr/bin/env python3
"""Validate content-safe C09 owned-mission runtime telemetry."""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Any


MAXIMUM_REPORT_BYTES = 32 * 1024
MAXIMUM_SAFE_INTEGER = (1 << 53) - 1
EXPECTED_KEYS = frozenset(
    {
        "format",
        "version",
        "core",
        "packageRevision",
        "missionId",
        "buildId",
        "acceptanceSession",
        "running",
        "visibilityState",
        "requestedWindowMs",
        "observedWindowMs",
        "frames",
        "rafSpanMs",
        "meanRafIntervalMs",
        "p95RafIntervalMs",
        "maximumRafIntervalMs",
        "approximateRafFps",
        "snapshotSamples",
        "snapshotTickDelta",
        "snapshotSpanMs",
        "observedTickHz",
        "maximumSnapshotGapMs",
        "latestTick",
        "latestSnapshotDeclaredBytes",
        "maximumSnapshotDeclaredBytes",
        "meanSnapshotDeclaredBytes",
        "maximumSnapshotBufferBytes",
        "snapshotDeclaredBytes",
        "snapshotDeclaredBytesPerSecond",
        "classicBaselineUploads",
        "classicDeltaUploads",
        "classicUnchangedUpdates",
        "classicUploadSamples",
        "classicPixelsUploaded",
        "classicPixelsUploadedPerSecond",
        "longTaskSupported",
        "longTasks",
        "maximumLongTaskMs",
        "totalLongTaskMs",
    }
)
TEXT_KEYS = frozenset(
    {
        "format",
        "core",
        "packageRevision",
        "missionId",
        "buildId",
        "acceptanceSession",
        "visibilityState",
    }
)
BOOLEAN_KEYS = frozenset({"running", "longTaskSupported"})
NUMERIC_KEYS = EXPECTED_KEYS - TEXT_KEYS - BOOLEAN_KEYS
INTEGER_KEYS = frozenset(
    {
        "version",
        "requestedWindowMs",
        "observedWindowMs",
        "frames",
        "snapshotSamples",
        "snapshotTickDelta",
        "latestTick",
        "latestSnapshotDeclaredBytes",
        "maximumSnapshotDeclaredBytes",
        "maximumSnapshotBufferBytes",
        "snapshotDeclaredBytes",
        "classicBaselineUploads",
        "classicDeltaUploads",
        "classicUnchangedUpdates",
        "classicUploadSamples",
        "classicPixelsUploaded",
        "longTasks",
    }
)


class DuplicateJsonKey(ValueError):
    """Raised when manually supplied evidence repeats a JSON member name."""


def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise DuplicateJsonKey(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def reject_nonstandard_number(value: str) -> None:
    raise ValueError(f"non-standard JSON number: {value}")


def close_enough(actual: float, expected: float, tolerance: float) -> bool:
    return abs(actual - expected) <= tolerance


def validate(
    report: Any,
    *,
    expected_build_id: str | None = None,
    expected_package_revision: str | None = None,
    expected_mission_id: str | None = None,
    expected_acceptance_session: str | None = None,
) -> list[str]:
    if not isinstance(report, dict):
        return ["report root must be a JSON object"]

    errors: list[str] = []
    missing = sorted(EXPECTED_KEYS - report.keys())
    unexpected = sorted(report.keys() - EXPECTED_KEYS)
    if missing:
        errors.append(f"missing report keys: {', '.join(missing)}")
    if unexpected:
        errors.append(f"unexpected report keys: {', '.join(unexpected)}")

    values: dict[str, float] = {}
    for name in NUMERIC_KEYS:
        value = report.get(name)
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            errors.append(f"{name} must be a finite number")
            values[name] = 0.0
            continue
        try:
            values[name] = float(value)
        except OverflowError:
            errors.append(f"{name} must be a finite number")
            values[name] = 0.0
            continue
        if not math.isfinite(values[name]):
            errors.append(f"{name} must be a finite number")
            values[name] = 0.0
            continue
        if values[name] < 0:
            errors.append(f"{name} cannot be negative")
        if name in INTEGER_KEYS:
            if not values[name].is_integer():
                errors.append(f"{name} must be an integer")
            elif values[name] > MAXIMUM_SAFE_INTEGER:
                errors.append(f"{name} exceeds the browser's safe integer range")

    if report.get("format") != "cncweb-runtime-performance" or report.get("version") != 3:
        errors.append("format/version must be cncweb-runtime-performance v3")
    if report.get("core") != "wasm":
        errors.append("core must be wasm; demo telemetry is not owned-mission evidence")
    if report.get("running") is not True:
        errors.append("running must be true at report capture")
    if report.get("visibilityState") != "visible":
        errors.append("visibilityState must be visible at report capture")

    package_revision = report.get("packageRevision")
    if not isinstance(package_revision, str) or len(package_revision) != 64 or any(
        character not in "0123456789abcdef" for character in package_revision
    ):
        errors.append("packageRevision must be a full lowercase SHA-256 digest")
    mission_id = report.get("missionId")
    if mission_id != "gdi-01-east-a":
        errors.append("missionId must identify the owned GDI mission 1 profile")
    build_id = report.get("buildId")
    if not isinstance(build_id, str) or len(build_id) != 16 or any(
        character not in "0123456789abcdef" for character in build_id
    ):
        errors.append("buildId must be the production build's 16-digit identity")
    acceptance_session = report.get("acceptanceSession")
    if not isinstance(acceptance_session, str) or not 32 <= len(acceptance_session) <= 64 or any(
        character not in "0123456789abcdef" for character in acceptance_session
    ):
        errors.append("acceptanceSession must be the fresh harness session identity")
    for name, actual, expected in (
        ("buildId", build_id, expected_build_id),
        ("packageRevision", package_revision, expected_package_revision),
        ("missionId", mission_id, expected_mission_id),
        ("acceptanceSession", acceptance_session, expected_acceptance_session),
    ):
        if expected is not None and actual != expected:
            errors.append(f"{name} does not match this acceptance run")

    requested_window = values["requestedWindowMs"]
    observed_window = values["observedWindowMs"]
    if requested_window != 60_000:
        errors.append("requestedWindowMs must equal 60000")
    if observed_window != requested_window:
        errors.append("observedWindowMs must cover exactly the requested 60000 ms")

    frames = values["frames"]
    raf_span = values["rafSpanMs"]
    mean_raf = values["meanRafIntervalMs"]
    p95_raf = values["p95RafIntervalMs"]
    maximum_raf = values["maximumRafIntervalMs"]
    approximate_fps = values["approximateRafFps"]
    if frames < 1_800:
        errors.append("at least 1800 rendered frames are required")
    if raf_span <= 0 or raf_span > observed_window:
        errors.append("rafSpanMs must be positive and no greater than observedWindowMs")
    if frames >= 2 and raf_span > 0:
        expected_mean_raf = raf_span / (frames - 1)
        if not close_enough(mean_raf, expected_mean_raf, 0.03):
            errors.append("meanRafIntervalMs is inconsistent with rafSpanMs and frames")
    if mean_raf <= 0 or mean_raf > 30:
        errors.append("meanRafIntervalMs must be greater than 0 and at most 30")
    if p95_raf <= 0 or p95_raf > 50:
        errors.append("p95RafIntervalMs must be greater than 0 and at most 50")
    if maximum_raf > 300:
        errors.append("maximumRafIntervalMs must be at most 300")
    if p95_raf > maximum_raf or mean_raf > maximum_raf:
        errors.append("RAF interval metrics are inconsistent")
    if approximate_fps <= 0 or mean_raf <= 0 or not close_enough(1000 / approximate_fps, mean_raf, 0.05):
        errors.append("approximateRafFps is inconsistent with meanRafIntervalMs")
    if frames >= 2 and observed_window > 0:
        minimum_possible_maximum = max(
            raf_span / (frames - 1),
            max(0.0, observed_window - raf_span) / 2,
        )
        if maximum_raf + 0.02 < minimum_possible_maximum:
            errors.append("maximumRafIntervalMs is inconsistent with the measured RAF window edges")

    snapshot_samples = values["snapshotSamples"]
    snapshot_tick_delta = values["snapshotTickDelta"]
    snapshot_span = values["snapshotSpanMs"]
    tick_hz = values["observedTickHz"]
    maximum_snapshot_gap = values["maximumSnapshotGapMs"]
    if snapshot_samples < 700:
        errors.append("at least 700 snapshot samples are required")
    if snapshot_span <= 0 or snapshot_span > observed_window:
        errors.append("snapshotSpanMs must be positive and no greater than observedWindowMs")
    if snapshot_tick_delta <= 0 or snapshot_tick_delta > values["latestTick"]:
        errors.append("snapshotTickDelta must be positive and no greater than latestTick")
    if snapshot_span > 0:
        expected_tick_hz = snapshot_tick_delta / (snapshot_span / 1000)
        if not close_enough(tick_hz, expected_tick_hz, 0.002):
            errors.append("observedTickHz is inconsistent with snapshotTickDelta and snapshotSpanMs")
    if tick_hz < 12.5 or tick_hz > 17.5:
        errors.append("observedTickHz must be between 12.5 and 17.5")
    if maximum_snapshot_gap > 300:
        errors.append("maximumSnapshotGapMs must be at most 300")
    if snapshot_samples >= 2 and observed_window > 0:
        minimum_possible_gap = max(
            snapshot_span / (snapshot_samples - 1),
            max(0.0, observed_window - snapshot_span) / 2,
        )
        if maximum_snapshot_gap + 0.02 < minimum_possible_gap:
            errors.append("maximumSnapshotGapMs is inconsistent with the measured snapshot window edges")

    if report.get("longTaskSupported") is not True:
        errors.append("the browser Long Tasks API must be supported")
    long_tasks = values["longTasks"]
    maximum_long_task = values["maximumLongTaskMs"]
    total_long_task = values["totalLongTaskMs"]
    if long_tasks > 3:
        errors.append("longTasks must be at most 3")
    if maximum_long_task > 250:
        errors.append("maximumLongTaskMs must be at most 250")
    if total_long_task < maximum_long_task or total_long_task > long_tasks * maximum_long_task + 0.02:
        errors.append("long-task duration metrics are inconsistent")
    if (long_tasks == 0 and (maximum_long_task != 0 or total_long_task != 0)) or (
        long_tasks > 0 and maximum_long_task <= 0
    ):
        errors.append("long-task count and duration metrics are inconsistent")

    baseline_uploads = values["classicBaselineUploads"]
    delta_uploads = values["classicDeltaUploads"]
    unchanged_updates = values["classicUnchangedUpdates"]
    upload_samples = values["classicUploadSamples"]
    if baseline_uploads > 1:
        errors.append("classicBaselineUploads must be at most 1")
    if upload_samples != baseline_uploads + delta_uploads + unchanged_updates:
        errors.append("classicUploadSamples does not equal its upload categories")
    if upload_samples < max(2, snapshot_samples - 2) or upload_samples > snapshot_samples + 2:
        errors.append("classic upload samples do not track the delivered snapshots")

    latest_declared = values["latestSnapshotDeclaredBytes"]
    maximum_declared = values["maximumSnapshotDeclaredBytes"]
    mean_declared = values["meanSnapshotDeclaredBytes"]
    maximum_buffer = values["maximumSnapshotBufferBytes"]
    total_declared = values["snapshotDeclaredBytes"]
    declared_rate = values["snapshotDeclaredBytesPerSecond"]
    if latest_declared <= 0 or maximum_declared < latest_declared:
        errors.append("snapshot declared-byte metrics are inconsistent")
    if mean_declared <= 0 or mean_declared > maximum_declared:
        errors.append("mean snapshot declared-byte metric is inconsistent")
    if maximum_buffer < maximum_declared:
        errors.append("snapshot buffer envelope is smaller than a declared snapshot")
    if snapshot_samples > 0:
        if total_declared < maximum_declared or total_declared > maximum_declared * snapshot_samples:
            errors.append("snapshotDeclaredBytes is inconsistent with the sample maximum")
        if not close_enough(mean_declared, total_declared / snapshot_samples, 0.011):
            errors.append("meanSnapshotDeclaredBytes is inconsistent with snapshotDeclaredBytes")
    expected_declared_rate = total_declared / (observed_window / 1000) if observed_window > 0 else 0
    if declared_rate <= 0 or not close_enough(declared_rate, expected_declared_rate, 0.011):
        errors.append("snapshotDeclaredBytesPerSecond is inconsistent with the observed window")

    classic_pixels = values["classicPixelsUploaded"]
    classic_pixel_rate = values["classicPixelsUploadedPerSecond"]
    expected_pixel_rate = classic_pixels / (observed_window / 1000) if observed_window > 0 else 0
    # A completely unchanged classic surface can legitimately have no pixels
    # uploaded during the rolling window (the retained baseline may predate
    # it). Pixel rate is calibration data, so verify accounting but impose no
    # non-zero performance threshold.
    if not close_enough(classic_pixel_rate, expected_pixel_rate, 0.011):
        errors.append("classicPixelsUploadedPerSecond is inconsistent with the observed window")
    return errors


def load_report(path: Path) -> Any:
    data = path.read_bytes()
    if len(data) > MAXIMUM_REPORT_BYTES:
        raise ValueError(f"report exceeds {MAXIMUM_REPORT_BYTES} bytes")
    return json.loads(
        data.decode("utf-8"),
        object_pairs_hook=reject_duplicate_keys,
        parse_constant=reject_nonstandard_number,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate content-safe C09 owned-mission runtime telemetry.")
    parser.add_argument("report", type=Path)
    parser.add_argument("--expected-build-id", required=True)
    parser.add_argument("--expected-package-revision", required=True)
    parser.add_argument("--expected-mission-id", required=True)
    parser.add_argument("--expected-acceptance-session", required=True)
    args = parser.parse_args()
    try:
        report = load_report(args.report)
    except (OSError, UnicodeError, json.JSONDecodeError, ValueError) as error:
        print(f"C09 report is not valid bounded UTF-8 JSON: {error}", file=sys.stderr)
        return 1

    errors = validate(
        report,
        expected_build_id=args.expected_build_id,
        expected_package_revision=args.expected_package_revision,
        expected_mission_id=args.expected_mission_id,
        expected_acceptance_session=args.expected_acceptance_session,
    )
    if errors:
        for error in errors:
            print(f"C09 threshold failed: {error}", file=sys.stderr)
        return 1

    print(
        "C09 runtime metrics validated: "
        f"{float(report['observedTickHz']):.2f} Hz, "
        f"{float(report['p95RafIntervalMs']):.2f} ms RAF p95, "
        f"{int(report['snapshotSamples'])} snapshots, "
        f"{int(report['classicBaselineUploads'])} baselines, "
        f"build {str(report['buildId'])}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
