#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import importlib.util
import json
import tempfile
import unittest
import zipfile
from pathlib import Path
from typing import Any, Dict, Optional


REPO_ROOT = Path(__file__).resolve().parents[2]
VERIFIER_PATH = REPO_ROOT / "scripts" / "verify-owned-content-package.py"
SPEC = importlib.util.spec_from_file_location("owned_content_verifier", VERIFIER_PATH)
assert SPEC is not None and SPEC.loader is not None
verifier = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(verifier)


def _json_bytes(value: Any) -> bytes:
    return (json.dumps(value, separators=(",", ":"), sort_keys=True) + "\n").encode("utf-8")


def _role(path: str) -> str:
    if path.startswith("engine/"):
        return "engine-data"
    if path.startswith("audio/"):
        return "audio"
    return "configuration"


def _write_fixture(
    directory: Path,
    *,
    bin_size: int = 8192,
    direction: int = 0,
    omit_audio_event: Optional[str] = None,
    wrong_audio_archive: Optional[str] = None,
) -> Path:
    files: Dict[str, bytes] = {
        f"engine/td/{name}": b"MIX123" for name in verifier.REQUIRED_MIXES
    }
    files["engine/td/SCG01EA.INI"] = b"[Briefing]\n1=Synthetic acceptance fixture\n"
    files["engine/td/SCG01EA.BIN"] = bytes(bin_size)
    files[verifier.CONVERSION_REPORT_PATH] = _json_bytes(
        {"format": "cncweb-conversion-report", "version": 1}
    )
    files[verifier.CATALOG_PATH] = _json_bytes(
        {
            "format": "cncweb-runtime",
            "version": 1,
            "engine": "tiberian-dawn",
            "engineRoot": "engine/td",
            "missions": [
                {
                    "id": "gdi-01-east-a",
                    "scenario": 1,
                    "variation": 0,
                    "direction": direction,
                    "buildLevel": 1,
                    "sabotagedStructure": -1,
                    "faction": "gdi",
                    "title": "GDI Mission 1",
                    "briefing": "Synthetic acceptance fixture",
                }
            ],
        }
    )

    audio_assets = []
    audio_specs = (
        ("sound", "ACKNO.V01"),
        ("sound", "BUTTON"),
        ("sound", "MGUN2"),
        ("sound", "XPLOS"),
        ("speech", "ACCOM1"),
        ("speech", "FAIL1"),
        ("speech", "REINFOR1"),
    )
    for event_id, (kind, event_name) in enumerate(audio_specs):
        if event_name == omit_audio_event:
            continue
        directory_name = "sfx" if kind == "sound" else "speech"
        path = f"audio/{directory_name}/{event_name.lower()}.wav"
        wav = b"RIFF" + event_name.encode("ascii") + bytes(64)
        files[path] = wav
        audio_assets.append(
            {
                "kind": kind,
                "eventName": event_name,
                "eventIds": [event_id],
                "path": path,
                "sourceArchive": (
                    "SPEECH.MIX"
                    if event_name == wrong_audio_archive
                    else "SOUNDS.MIX"
                )
                if kind == "sound"
                else (
                    "SOUNDS.MIX"
                    if event_name == wrong_audio_archive
                    else "SPEECH.MIX"
                ),
                "sourceName": event_name
                if kind == "sound" and ".V0" in event_name
                else f"{event_name}.AUD",
                "sourceCompression": 1,
                "sampleRate": 22050,
                "channels": 1,
                "bitsPerSample": 8,
                "frames": 64,
                "sha256": hashlib.sha256(wav).hexdigest(),
            }
        )
    files[verifier.AUDIO_INDEX_PATH] = _json_bytes(
        {
            "format": "cncweb-audio",
            "version": 1,
            "encoding": "wav-pcm",
            "assets": audio_assets,
            "diagnostics": {
                "candidateCount": len(audio_assets),
                "missingCandidates": 0,
                "decodeFailures": [],
            },
        }
    )

    manifest_files = [
        {
            "path": path,
            "size": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
            "role": _role(path),
        }
        for path, data in sorted(files.items())
    ]
    manifest = {
        "format": "cncweb-content",
        "version": 1,
        "package_id": "synthetic-acceptance",
        "created_at_unix_ms": 1,
        "source": {
            "product": "cnc-remastered-collection",
            "provider": "unknown",
            "install_fingerprint_sha256": "00" * 32,
        },
        "content": {"games": ["tiberian-dawn"], "locales": ["en-US"]},
        "content_sha256": verifier._content_digest(manifest_files),
        "files": manifest_files,
    }

    package = directory / "synthetic.cncweb"
    with zipfile.ZipFile(package, "w", compression=zipfile.ZIP_STORED) as archive:
        for path, data in files.items():
            archive.writestr(path, data)
        archive.writestr("manifest.json", _json_bytes(manifest))
    return package


class OwnedContentVerifierTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="cncweb-verifier-test-")
        self.directory = Path(self.temporary.name)

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def test_accepts_exact_milestone_runtime_contract_without_returning_briefing(self) -> None:
        package = _write_fixture(self.directory)
        summary = verifier.validate_package(
            package,
            expected_package_id="synthetic-acceptance",
            expected_provider="unknown",
        )
        self.assertEqual(summary["missionId"], "gdi-01-east-a")
        with zipfile.ZipFile(package, "r") as archive:
            manifest = json.loads(archive.read("manifest.json"))
        browser_bytes = json.dumps(manifest, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
        self.assertEqual(summary["packageRevision"], hashlib.sha256(browser_bytes).hexdigest())
        self.assertEqual(summary["requiredEngineFiles"], 14)
        self.assertEqual(summary["soundAssets"], 4)
        self.assertEqual(summary["speechAssets"], 3)
        self.assertEqual(summary["requiredAudioGroups"], 7)
        self.assertNotIn("briefing", json.dumps(summary).lower())

    def test_rejects_map_size_that_engine_preflight_would_reject(self) -> None:
        package = _write_fixture(self.directory, bin_size=8191)
        with self.assertRaisesRegex(verifier.ValidationError, "invalid size"):
            verifier.validate_package(package)

    def test_rejects_noncanonical_mission_launch_fields(self) -> None:
        package = _write_fixture(self.directory, direction=1)
        with self.assertRaisesRegex(verifier.ValidationError, "launch fields"):
            verifier.validate_package(package)

    def test_rejects_each_missing_first_playable_audio_group(self) -> None:
        gate_events = {
            "MGUN2": "weapon",
            "BUTTON": "interface-feedback",
            "XPLOS": "explosion",
            "ACKNO.V01": "unit-response",
            "ACCOM1": "mission-accomplished",
            "FAIL1": "mission-failed",
            "REINFOR1": "gameplay-speech",
        }
        for event_name, group in gate_events.items():
            with self.subTest(group=group):
                package = _write_fixture(self.directory, omit_audio_event=event_name)
                with self.assertRaisesRegex(verifier.ValidationError, group):
                    verifier.validate_package(package)

    def test_rejects_wrong_requested_identity_without_exposing_content(self) -> None:
        package = _write_fixture(self.directory)
        with self.assertRaisesRegex(verifier.ValidationError, "package ID differs"):
            verifier.validate_package(package, expected_package_id="different")

    def test_rejects_cross_archive_audio_identity(self) -> None:
        package = _write_fixture(self.directory, wrong_audio_archive="MGUN2")
        with self.assertRaisesRegex(verifier.ValidationError, "cross-archive"):
            verifier.validate_package(package)


if __name__ == "__main__":
    unittest.main()
