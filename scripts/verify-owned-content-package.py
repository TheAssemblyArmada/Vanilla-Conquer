#!/usr/bin/env python3
"""Validate the milestone-specific metadata in a verified local .cncweb pack.

This intentionally prints only content-safe identifiers and counts. It never
extracts assets or emits the locally derived mission briefing.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
import zipfile
from pathlib import Path
from typing import Any, Dict, Iterable, List, Mapping, Optional, Sequence, Tuple


MANIFEST_PATH = "manifest.json"
CATALOG_PATH = "runtime/catalog-v1.json"
AUDIO_INDEX_PATH = "runtime/audio-v1.json"
CONVERSION_REPORT_PATH = "metadata/conversion-report-v1.json"
ENGINE_ROOT = "engine/td"
MISSION_ID = "gdi-01-east-a"
SCENARIO_ROOT = "SCG01EA"
MAX_JSON_BYTES = 4 * 1024 * 1024
HEX_DIGITS = frozenset("0123456789abcdef")
EVENT_NAME_PATTERN = re.compile(r"^[A-Z0-9]+(?:\.V0[0-3])?$")

REQUIRED_MIXES: Tuple[str, ...] = (
    "CCLOCAL.MIX",
    "CONQUER.MIX",
    "GENERAL.MIX",
    "LOCAL.MIX",
    "SOUNDS.MIX",
    "SPEECH.MIX",
    "TEMPERAT.MIX",
    "TEMPICNH.MIX",
    "TRANSIT.MIX",
    "UPDATA.MIX",
    "UPDATE.MIX",
    "UPDATEC.MIX",
)


class ValidationError(Exception):
    """A content-safe validation failure."""


def _object_without_duplicate_keys(pairs: Iterable[Tuple[str, Any]]) -> Dict[str, Any]:
    value: Dict[str, Any] = {}
    for key, item in pairs:
        if key in value:
            raise ValidationError("JSON metadata contains a duplicate object key")
        value[key] = item
    return value


def _load_json_bytes(data: bytes, logical_name: str) -> Mapping[str, Any]:
    if not data or len(data) > MAX_JSON_BYTES:
        raise ValidationError(f"{logical_name} is empty or exceeds the metadata size limit")
    try:
        decoded = data.decode("utf-8")
        value = json.loads(decoded, object_pairs_hook=_object_without_duplicate_keys)
    except UnicodeDecodeError as error:
        raise ValidationError(f"{logical_name} is not UTF-8 JSON") from error
    except json.JSONDecodeError as error:
        raise ValidationError(f"{logical_name} is not valid JSON") from error
    if not isinstance(value, dict):
        raise ValidationError(f"{logical_name} must contain a JSON object")
    return value


def _is_sha256(value: Any) -> bool:
    return (
        isinstance(value, str)
        and len(value) == 64
        and all(character in HEX_DIGITS for character in value)
    )


def _content_digest(files: Sequence[Mapping[str, Any]]) -> str:
    digest = hashlib.sha256()
    digest.update(b"CNCWEB-CONTENT-MANIFEST-V1\0")
    for entry in files:
        path = entry.get("path")
        size = entry.get("size")
        sha256 = entry.get("sha256")
        role = entry.get("role")
        if (
            not isinstance(path, str)
            or not isinstance(size, int)
            or isinstance(size, bool)
            or size < 0
            or not _is_sha256(sha256)
            or not isinstance(role, str)
        ):
            raise ValidationError("manifest file metadata is malformed")
        path_bytes = path.encode("utf-8")
        role_bytes = role.encode("utf-8")
        digest.update(struct.pack("<Q", len(path_bytes)))
        digest.update(path_bytes)
        digest.update(struct.pack("<Q", size))
        digest.update(bytes.fromhex(sha256))
        digest.update(struct.pack("<Q", len(role_bytes)))
        digest.update(role_bytes)
    return digest.hexdigest()


def _browser_manifest_revision(manifest: Mapping[str, Any]) -> str:
    """Match TextEncoder(JSON.stringify(validatedManifest)) in ContentStore."""
    compact = json.dumps(manifest, ensure_ascii=False, separators=(",", ":"))
    return hashlib.sha256(compact.encode("utf-8")).hexdigest()


def _require_file(
    files: Mapping[str, Mapping[str, Any]],
    path: str,
    role: str,
    minimum_size: int = 1,
    exact_size: Optional[int] = None,
    maximum_size: Optional[int] = None,
) -> Mapping[str, Any]:
    entry = files.get(path)
    if entry is None:
        raise ValidationError(f"required milestone file is missing: {path}")
    size = entry.get("size")
    if entry.get("role") != role or not isinstance(size, int) or isinstance(size, bool):
        raise ValidationError(f"required milestone file has invalid metadata: {path}")
    if exact_size is not None and size != exact_size:
        raise ValidationError(f"required milestone file has an invalid size: {path}")
    if size < minimum_size or (maximum_size is not None and size > maximum_size):
        raise ValidationError(f"required milestone file has an invalid size: {path}")
    return entry


def _read_verified_metadata(
    archive: zipfile.ZipFile,
    zip_entries: Mapping[str, zipfile.ZipInfo],
    files: Mapping[str, Mapping[str, Any]],
    path: str,
) -> Mapping[str, Any]:
    entry = _require_file(files, path, "configuration", maximum_size=MAX_JSON_BYTES)
    info = zip_entries[path]
    if info.file_size != entry["size"]:
        raise ValidationError(f"ZIP and manifest sizes differ for {path}")
    data = archive.read(info)
    if hashlib.sha256(data).hexdigest() != entry["sha256"]:
        raise ValidationError(f"metadata digest differs from the manifest for {path}")
    return _load_json_bytes(data, path)


def _validate_catalog(catalog: Mapping[str, Any]) -> None:
    if (
        catalog.get("format") != "cncweb-runtime"
        or catalog.get("version") != 1
        or catalog.get("engine") != "tiberian-dawn"
        or catalog.get("engineRoot") != ENGINE_ROOT
    ):
        raise ValidationError("runtime catalog does not target the milestone engine")
    missions = catalog.get("missions")
    if not isinstance(missions, list) or len(missions) != 1 or not isinstance(missions[0], dict):
        raise ValidationError("runtime catalog must contain one milestone mission")
    mission = missions[0]
    expected = {
        "id": MISSION_ID,
        "scenario": 1,
        "variation": 0,
        "direction": 0,
        "buildLevel": 1,
        "sabotagedStructure": -1,
        "faction": "gdi",
        "title": "GDI Mission 1",
    }
    if any(mission.get(key) != value for key, value in expected.items()):
        raise ValidationError("runtime catalog launch fields do not match GDI mission 1")
    briefing = mission.get("briefing")
    if (
        not isinstance(briefing, str)
        or not briefing.strip()
        or len(briefing.encode("utf-8")) > 4096
        or any(character in briefing for character in ("\r", "@", "\0"))
    ):
        raise ValidationError("runtime catalog has an invalid locally derived briefing")


def _validate_audio(
    audio: Mapping[str, Any], files: Mapping[str, Mapping[str, Any]]
) -> Tuple[int, int, int, int, int]:
    if (
        audio.get("format") != "cncweb-audio"
        or audio.get("version") != 1
        or audio.get("encoding") != "wav-pcm"
    ):
        raise ValidationError("audio index does not match the milestone format")
    assets = audio.get("assets")
    if not isinstance(assets, list) or not assets:
        raise ValidationError("audio index contains no browser-native assets")

    seen_paths = set()
    available = set()
    previous_asset_key = None
    sound_count = 0
    speech_count = 0
    expected_asset_keys = {
        "kind",
        "eventName",
        "eventIds",
        "path",
        "sourceArchive",
        "sourceName",
        "sourceCompression",
        "sampleRate",
        "channels",
        "bitsPerSample",
        "frames",
        "sha256",
    }
    for asset in assets:
        if not isinstance(asset, dict):
            raise ValidationError("audio index contains a malformed asset")
        if set(asset) != expected_asset_keys:
            raise ValidationError("audio index asset fields do not match the v1 contract")
        path = asset.get("path")
        kind = asset.get("kind")
        event_name = asset.get("eventName")
        sha256 = asset.get("sha256")
        event_ids = asset.get("eventIds")
        source_archive = asset.get("sourceArchive")
        source_name = asset.get("sourceName")
        source_compression = asset.get("sourceCompression")
        sample_rate = asset.get("sampleRate")
        channels = asset.get("channels")
        bits_per_sample = asset.get("bitsPerSample")
        frames = asset.get("frames")
        if (
            not isinstance(path, str)
            or path in seen_paths
            or kind not in ("sound", "speech")
            or not isinstance(event_name, str)
            or len(event_name) > 16
            or EVENT_NAME_PATTERN.fullmatch(event_name) is None
            or not _is_sha256(sha256)
        ):
            raise ValidationError("audio index contains invalid callback metadata")
        asset_key = (kind, event_name)
        if previous_asset_key is not None and previous_asset_key >= asset_key:
            raise ValidationError("audio assets are not strictly sorted")
        previous_asset_key = asset_key
        if (
            not isinstance(event_ids, list)
            or not 1 <= len(event_ids) <= 4
            or any(
                not isinstance(event_id, int)
                or isinstance(event_id, bool)
                or not 0 <= event_id <= 65_535
                for event_id in event_ids
            )
            or any(left >= right for left, right in zip(event_ids, event_ids[1:]))
        ):
            raise ValidationError("audio index contains invalid callback event IDs")
        seen_paths.add(path)
        available.add((kind, event_name))
        expected_directory = "sfx" if kind == "sound" else "speech"
        expected_path = f"audio/{expected_directory}/{event_name.lower()}.wav"
        if path != expected_path:
            raise ValidationError("audio index contains a noncanonical callback path")
        expected_archive = "SOUNDS.MIX" if kind == "sound" else "SPEECH.MIX"
        expected_source_name = (
            event_name
            if kind == "sound" and any(event_name.endswith(f".V0{index}") for index in range(4))
            else f"{event_name}.AUD"
        )
        if source_archive != expected_archive or source_name != expected_source_name:
            raise ValidationError("audio index contains a cross-archive or noncanonical source")
        if (
            source_compression not in (0, 1, 99)
            or not isinstance(sample_rate, int)
            or isinstance(sample_rate, bool)
            or not 4_000 <= sample_rate <= 192_000
            or channels not in (1, 2)
            or bits_per_sample not in (8, 16)
            or not isinstance(frames, int)
            or isinstance(frames, bool)
            or frames <= 0
        ):
            raise ValidationError("audio index contains invalid PCM metadata")
        manifest_entry = _require_file(files, path, "audio", minimum_size=44)
        if manifest_entry.get("sha256") != sha256:
            raise ValidationError("audio index and manifest digest metadata differ")
        sound_count += int(kind == "sound")
        speech_count += int(kind == "speech")
    if sound_count == 0 or speech_count == 0:
        raise ValidationError("audio index must include both sound and speech callbacks")

    def has_exact(kind: str, names: Sequence[str]) -> bool:
        return any((kind, name) in available for name in names)

    def has_variant(kind: str, prefixes: Sequence[str]) -> bool:
        return any(
            asset_kind == kind
            and any(
                event_name == prefix
                or EVENT_NAME_PATTERN.fullmatch(event_name) is not None
                and event_name.startswith(f"{prefix}.V0")
                for prefix in prefixes
            )
            for asset_kind, event_name in available
        )

    core_gates = (
        ("weapon", has_exact("sound", ("MGUN2", "GUN18", "BAZOOK1"))),
        (
            "interface-feedback",
            has_exact("sound", ("BUTTON", "SCOLD2", "BLEEP2")),
        ),
        (
            "explosion",
            has_exact("sound", ("XPLOS", "XPLODE", "XPLOSML2", "XPLOBIG4")),
        ),
        (
            "unit-response",
            has_variant(
                "sound",
                ("ACKNO", "AFFIRM1", "MOVOUT1", "REPORT1", "UNIT1", "YESSIR1"),
            ),
        ),
        ("mission-accomplished", has_exact("speech", ("ACCOM1",))),
        ("mission-failed", has_exact("speech", ("FAIL1",))),
        (
            "gameplay-speech",
            has_exact("speech", ("REINFOR1", "UNITREDY", "NEWOPT1", "BASEATK1")),
        ),
    )
    missing_core_groups = [name for name, present in core_gates if not present]
    if missing_core_groups:
        raise ValidationError(
            "audio index is insufficient for GDI mission 1; missing core groups: "
            + ", ".join(missing_core_groups)
        )

    diagnostics = audio.get("diagnostics")
    if not isinstance(diagnostics, dict) or set(diagnostics) != {
        "candidateCount",
        "missingCandidates",
        "decodeFailures",
    }:
        raise ValidationError("audio index diagnostics are missing")
    missing = diagnostics.get("missingCandidates")
    candidate_count = diagnostics.get("candidateCount")
    failures = diagnostics.get("decodeFailures")
    if (
        not isinstance(candidate_count, int)
        or isinstance(candidate_count, bool)
        or not 1 <= candidate_count <= 1_000
        or not isinstance(missing, int)
        or isinstance(missing, bool)
        or missing < 0
        or not isinstance(failures, list)
        or candidate_count != len(assets) + missing + len(failures)
    ):
        raise ValidationError("audio index diagnostics are malformed")
    previous_failure_key = None
    expected_failure_keys = {
        "kind",
        "eventName",
        "sourceArchive",
        "sourceName",
        "reason",
    }
    for failure in failures:
        if not isinstance(failure, dict) or set(failure) != expected_failure_keys:
            raise ValidationError("audio decode failure fields are malformed")
        kind = failure.get("kind")
        event_name = failure.get("eventName")
        source_archive = failure.get("sourceArchive")
        source_name = failure.get("sourceName")
        reason = failure.get("reason")
        if (
            kind not in ("sound", "speech")
            or not isinstance(event_name, str)
            or EVENT_NAME_PATTERN.fullmatch(event_name) is None
            or len(event_name) > 16
        ):
            raise ValidationError("audio decode failure callback is malformed")
        failure_key = (kind, event_name)
        if previous_failure_key is not None and previous_failure_key >= failure_key:
            raise ValidationError("audio decode failures are not strictly sorted")
        previous_failure_key = failure_key
        expected_archive = "SOUNDS.MIX" if kind == "sound" else "SPEECH.MIX"
        expected_source_name = (
            event_name
            if kind == "sound" and any(event_name.endswith(f".V0{index}") for index in range(4))
            else f"{event_name}.AUD"
        )
        if (
            source_archive != expected_archive
            or source_name != expected_source_name
            or not isinstance(reason, str)
            or not 1 <= len(reason) <= 1_024
        ):
            raise ValidationError("audio decode failure source or reason is malformed")
    return len(assets), sound_count, speech_count, missing, len(failures)


def validate_package(
    package: Path,
    expected_package_id: Optional[str] = None,
    expected_provider: Optional[str] = None,
) -> Mapping[str, Any]:
    if not package.is_file():
        raise ValidationError("package is not a regular file")

    with zipfile.ZipFile(package, "r") as archive:
        infos = archive.infolist()
        names = [info.filename for info in infos]
        if len(names) != len(set(names)) or any(info.is_dir() for info in infos):
            raise ValidationError("package contains duplicate names or directory entries")
        if any(
            info.flag_bits & 0x1
            or info.compress_type not in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED)
            for info in infos
        ):
            raise ValidationError("package uses encryption or an unsupported compression method")
        zip_entries = {info.filename: info for info in infos}
        manifest_info = zip_entries.get(MANIFEST_PATH)
        if manifest_info is None or manifest_info.file_size > MAX_JSON_BYTES:
            raise ValidationError("manifest.json is missing or oversized")
        manifest = _load_json_bytes(archive.read(manifest_info), MANIFEST_PATH)

        package_id = manifest.get("package_id")
        source = manifest.get("source")
        content = manifest.get("content")
        manifest_files = manifest.get("files")
        if (
            manifest.get("format") != "cncweb-content"
            or manifest.get("version") != 1
            or not isinstance(package_id, str)
            or not isinstance(source, dict)
            or source.get("product") != "cnc-remastered-collection"
            or not isinstance(content, dict)
            or content.get("games") != ["tiberian-dawn"]
            or content.get("locales") != ["en-US"]
            or not isinstance(manifest_files, list)
        ):
            raise ValidationError("manifest does not describe the milestone content profile")
        if expected_package_id is not None and package_id != expected_package_id:
            raise ValidationError("manifest package ID differs from the requested package ID")
        if expected_provider is not None and source.get("provider") != expected_provider:
            raise ValidationError("manifest provider differs from the requested provider")
        if not _is_sha256(source.get("install_fingerprint_sha256")):
            raise ValidationError("manifest installation fingerprint is malformed")

        paths: List[str] = []
        files: Dict[str, Mapping[str, Any]] = {}
        for entry in manifest_files:
            if not isinstance(entry, dict) or not isinstance(entry.get("path"), str):
                raise ValidationError("manifest contains malformed file metadata")
            path = entry["path"]
            paths.append(path)
            files[path] = entry
        if paths != sorted(paths) or len(files) != len(paths):
            raise ValidationError("manifest file paths are not unique and strictly sorted")
        if set(zip_entries) != set(files) | {MANIFEST_PATH}:
            raise ValidationError("ZIP member inventory differs from the manifest")
        for path, entry in files.items():
            info = zip_entries[path]
            if info.file_size != entry.get("size") or not _is_sha256(entry.get("sha256")):
                raise ValidationError("ZIP and manifest file metadata differ")
        calculated_content_digest = _content_digest(manifest_files)
        if manifest.get("content_sha256") != calculated_content_digest:
            raise ValidationError("manifest content inventory digest is invalid")

        for name in REQUIRED_MIXES:
            _require_file(files, f"{ENGINE_ROOT}/{name}", "engine-data", minimum_size=6)
        _require_file(
            files,
            f"{ENGINE_ROOT}/{SCENARIO_ROOT}.INI",
            "engine-data",
            minimum_size=1,
            maximum_size=1024 * 1024,
        )
        _require_file(
            files,
            f"{ENGINE_ROOT}/{SCENARIO_ROOT}.BIN",
            "engine-data",
            exact_size=8192,
        )
        palette_path = f"{ENGINE_ROOT}/TEMPERAT.PAL"
        if palette_path in files:
            _require_file(files, palette_path, "engine-data", exact_size=768)
        _require_file(files, CONVERSION_REPORT_PATH, "configuration", maximum_size=MAX_JSON_BYTES)

        catalog = _read_verified_metadata(archive, zip_entries, files, CATALOG_PATH)
        audio = _read_verified_metadata(archive, zip_entries, files, AUDIO_INDEX_PATH)
        _validate_catalog(catalog)
        (
            audio_assets,
            sound_assets,
            speech_assets,
            missing_audio_candidates,
            decode_failures,
        ) = _validate_audio(audio, files)

    return {
        "format": "cncweb-owned-content-preflight",
        "version": 1,
        "packageId": package_id,
        "packageRevision": _browser_manifest_revision(manifest),
        "contentSha256": calculated_content_digest,
        "engineRoot": ENGINE_ROOT,
        "missionId": MISSION_ID,
        "scenarioRoot": SCENARIO_ROOT,
        "requiredEngineFiles": len(REQUIRED_MIXES) + 2,
        "optionalPalettePresent": palette_path in files,
        "audioAssets": audio_assets,
        "soundAssets": sound_assets,
        "speechAssets": speech_assets,
        "requiredAudioGroups": 7,
        "missingAudioCandidates": missing_audio_candidates,
        "audioDecodeFailures": decode_failures,
    }


def _parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Validate content-safe GDI mission 1 runtime metadata in a local .cncweb pack."
    )
    parser.add_argument("package", type=Path)
    parser.add_argument("--expected-package-id")
    parser.add_argument(
        "--expected-provider",
        choices=("steam", "ea-app", "copied-installation", "unknown"),
    )
    return parser.parse_args(argv)


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = _parse_args(argv)
    try:
        summary = validate_package(
            args.package,
            expected_package_id=args.expected_package_id,
            expected_provider=args.expected_provider,
        )
    except (OSError, zipfile.BadZipFile, ValidationError) as error:
        print(f"error: owned-content preflight failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(summary, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
