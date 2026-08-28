"""Canonical recorded-run directories."""

from __future__ import annotations

import json
import shutil
from pathlib import Path
from typing import Any, Iterable

from .common import ToolError, file_fingerprint, read_json, write_json
from .state import PROFILES, compare_state, load_state_jsonl

RUN_SCHEMA = "quiky.recorded-run-v3"
RUN_FILES = ("input.jsonl", "expected-state.jsonl", "actual-state.jsonl")


def validate_input_row(value: Any, *, label: str = "input row") -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ToolError(f"{label}: expected object")
    sequence, flags = value.get("sequence"), value.get("input_flags")
    guest_frame = value.get("guest_frame", sequence)
    if not isinstance(sequence, int) or sequence < 1:
        raise ToolError(f"{label}: sequence must be a positive integer")
    if not isinstance(guest_frame, int) or guest_frame < 0:
        raise ToolError(f"{label}: guest_frame must be a non-negative integer")
    if not isinstance(flags, int) or not 0 <= flags <= 0xffff:
        raise ToolError(f"{label}: input_flags must be a uint16")
    camera = value.get("camera")
    if camera is not None and (not isinstance(camera, dict) or
                               not isinstance(camera.get("x"), int) or
                               not isinstance(camera.get("y"), int)):
        raise ToolError(f"{label}: camera must contain integer x and y")
    unknown = set(value).difference({"sequence", "guest_frame", "input_flags", "camera"})
    if unknown:
        raise ToolError(f"{label}: unknown fields: {', '.join(sorted(unknown))}")
    return {"sequence": sequence, "guest_frame": guest_frame,
            "input_flags": flags,
            **({"camera": {"x": camera["x"], "y": camera["y"]}}
               if camera is not None else {})}


def validate_input_rows(rows: Iterable[dict[str, Any]]) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []
    previous = 0
    for index, row in enumerate(rows, 1):
        normalized = validate_input_row(row, label=f"input row {index}")
        if normalized["sequence"] <= previous:
            raise ToolError("input rows must have strictly increasing sequence")
        previous = normalized["sequence"]
        result.append(normalized)
    return result


def load_input_jsonl(path: Path) -> list[dict[str, Any]]:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise ToolError(f"{path}: cannot read input stream: {exc}") from exc
    rows: list[dict[str, Any]] = []
    for index, line in enumerate(lines, 1):
        if not line.strip():
            raise ToolError(f"{path}: blank line at {index}")
        try:
            rows.append(json.loads(line))
        except json.JSONDecodeError as exc:
            raise ToolError(f"{path}: invalid JSON at line {index}: {exc}") from exc
    return validate_input_rows(rows)


def save_input_jsonl(path: Path, rows: Iterable[dict[str, Any]]) -> None:
    normalized = validate_input_rows(rows)
    if not normalized:
        raise ToolError("input stream is empty")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(json.dumps(row, sort_keys=True, separators=(",", ":")) + "\n"
                            for row in normalized), encoding="utf-8")


def new_run_manifest(name: str, *, profile: str,
                     provenance: dict[str, Any] | None = None) -> dict[str, Any]:
    if not name:
        raise ToolError("run name must be non-empty")
    if profile not in PROFILES:
        raise ToolError(f"unknown parity profile {profile!r}")
    return {"schema": RUN_SCHEMA, "format_version": 3, "name": name,
            "profile": profile, "provenance": dict(provenance or {}), "files": {}}


def validate_manifest(manifest: dict[str, Any]) -> dict[str, Any]:
    if not isinstance(manifest, dict) or manifest.get("schema") != RUN_SCHEMA:
        raise ToolError(f"manifest must use {RUN_SCHEMA}")
    if manifest.get("format_version") != 3:
        raise ToolError("manifest format_version must be 3")
    if not isinstance(manifest.get("name"), str) or not manifest["name"]:
        raise ToolError("manifest name must be non-empty")
    if manifest.get("profile") not in PROFILES:
        raise ToolError("manifest has invalid parity profile")
    if not isinstance(manifest.get("provenance"), dict):
        raise ToolError("manifest provenance must be an object")
    if not isinstance(manifest.get("files"), dict):
        raise ToolError("manifest files must be an object")
    unknown = set(manifest).difference(
        {"schema", "format_version", "name", "profile", "provenance", "files"})
    if unknown:
        raise ToolError(f"manifest has unknown fields: {', '.join(sorted(unknown))}")
    return manifest


def load_manifest(path: Path) -> dict[str, Any]:
    try:
        return validate_manifest(read_json(path))
    except ToolError as exc:
        raise ToolError(f"{path}: {exc}") from exc


def save_manifest(path: Path, manifest: dict[str, Any]) -> None:
    validate_manifest(manifest)
    write_json(path, manifest)


def _fingerprints(directory: Path) -> dict[str, Any]:
    """Fingerprint parity inputs, never regenerable verification reports."""
    return {name: file_fingerprint(directory / name)
            for name in RUN_FILES if (directory / name).is_file()}


def stage_run_files(directory: Path, *, name: str, profile: str,
                    input_rows: Iterable[dict[str, Any]], expected_state: Path,
                    actual_state: Path | None = None,
                    provenance: dict[str, Any] | None = None) -> dict[str, Any]:
    if directory.exists() and any(directory.iterdir()):
        raise ToolError(f"run directory is not empty: {directory}")
    directory.mkdir(parents=True, exist_ok=True)
    save_input_jsonl(directory / "input.jsonl", input_rows)
    for target, source in (("expected-state.jsonl", expected_state),
                           ("actual-state.jsonl", actual_state)):
        if source is not None:
            load_state_jsonl(source)
            shutil.copyfile(source, directory / target)
    manifest = new_run_manifest(name, profile=profile, provenance=provenance)
    manifest["files"] = _fingerprints(directory)
    save_manifest(directory / "manifest.json", manifest)
    return manifest


def validate_run_directory(directory: Path) -> dict[str, Any]:
    manifest = load_manifest(directory / "manifest.json")
    for name, fingerprint in manifest["files"].items():
        if name not in RUN_FILES or not isinstance(fingerprint, dict):
            raise ToolError(f"manifest file entry is invalid: {name}")
        path = directory / name
        if not path.is_file():
            raise ToolError(f"run is missing canonical file: {name}")
        if file_fingerprint(path)["sha256"] != fingerprint.get("sha256"):
            raise ToolError(f"run file digest mismatch: {name}")
    for required in ("input.jsonl", "expected-state.jsonl"):
        if required not in manifest["files"]:
            raise ToolError(f"manifest is missing required file: {required}")
    load_input_jsonl(directory / "input.jsonl")
    load_state_jsonl(directory / "expected-state.jsonl")
    if "actual-state.jsonl" in manifest["files"]:
        load_state_jsonl(directory / "actual-state.jsonl")
    return manifest


def verify_run_directory(directory: Path) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    manifest = validate_run_directory(directory)
    actual = directory / "actual-state.jsonl"
    if not actual.is_file():
        raise ToolError("run parity requires actual-state.jsonl")
    mismatches, coverage = compare_state(
        directory / "expected-state.jsonl", actual, manifest["profile"])
    write_json(directory / "parity.json", {
        "schema": "quiky.recorded-run-parity-v2", "run": manifest["name"],
        "profile": manifest["profile"],
        "status": "pass" if not mismatches else "fail", "mismatches": mismatches})
    write_json(directory / "coverage.json", {
        "schema": "quiky.recorded-run-coverage-v2", "run": manifest["name"],
        "profile": manifest["profile"], "items": coverage})
    manifest["files"] = _fingerprints(directory)
    save_manifest(directory / "manifest.json", manifest)
    return mismatches, coverage


def install_actual_state(directory: Path, state: Path) -> None:
    """Install a freshly replayed canonical state and refresh its digest."""
    manifest = validate_run_directory(directory)
    load_state_jsonl(state)
    shutil.copyfile(state, directory / "actual-state.jsonl")
    manifest["files"] = _fingerprints(directory)
    save_manifest(directory / "manifest.json", manifest)
