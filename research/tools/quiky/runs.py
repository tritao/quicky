"""Canonical recorded-run artifact primitives.

Raw DOS/native traces remain immutable evidence.  A recorded run is the
portable boundary consumed by replay and parity: input is explicit JSONL and
all derived files are named in the manifest with content digests.
"""

from __future__ import annotations

import json
import shutil
from pathlib import Path
from typing import Any, Iterable

from .common import ToolError, file_fingerprint, read_json, write_json


RUN_SCHEMA = "quiky.recorded-run-v2"
RUN_FILES = (
    "input.jsonl",
    "dos-state.json",
    "native-state.json",
    "parity.json",
    "coverage.json",
)


def _trace_ref(path: Path) -> dict[str, Any]:
    if not path.is_file():
        raise ToolError(f"trace does not exist: {path}")
    return file_fingerprint(path)


def new_run_manifest(
    name: str,
    *,
    input_stream: Iterable[dict[str, Any]] | None = None,
    dos_trace: Path | None = None,
    native_trace: Path | None = None,
    artifacts: Iterable[Path] = (),
    provenance: dict[str, Any] | None = None,
) -> dict[str, Any]:
    """Create a deterministic manifest shell without modifying evidence."""

    traces: dict[str, Any] = {}
    if dos_trace is not None:
        traces["dos"] = _trace_ref(dos_trace)
    if native_trace is not None:
        traces["native"] = _trace_ref(native_trace)
    return {
        "schema": RUN_SCHEMA,
        "format_version": 2,
        "name": name,
        "provenance": dict(provenance or {}),
        "input_stream": list(input_stream or ()),
        "traces": traces,
        "artifacts": [_trace_ref(path) for path in artifacts],
        "segments": [],
        "checkpoints": [],
        "parity": None,
        "quality": None,
    }


def validate_input_row(value: Any, *, label: str = "input row") -> dict[str, Any]:
    """Validate one deterministic replay input row.

    Camera is optional for setup-only traces, but when present it must be a
    complete integer pair.  Rejecting extra shape variants here prevents the
    engine frontend from silently inventing camera or input state.
    """

    if not isinstance(value, dict):
        raise ToolError(f"{label}: expected object")
    sequence = value.get("sequence")
    if not isinstance(sequence, int) or sequence < 1:
        raise ToolError(f"{label}: sequence must be a positive integer")
    guest_frame = value.get("guest_frame", sequence)
    if not isinstance(guest_frame, int) or guest_frame < 0:
        raise ToolError(f"{label}: guest_frame must be a non-negative integer")
    flags = value.get("input_flags")
    if not isinstance(flags, int) or not 0 <= flags <= 0xffff:
        raise ToolError(f"{label}: input_flags must be a uint16")
    camera = value.get("camera")
    if camera is not None:
        if (not isinstance(camera, dict) or
                not isinstance(camera.get("x"), int) or
                not isinstance(camera.get("y"), int)):
            raise ToolError(f"{label}: camera must contain integer x and y")
    return {
        "sequence": sequence,
        "guest_frame": guest_frame,
        "input_flags": flags,
        **({"camera": {"x": camera["x"], "y": camera["y"]}}
           if camera is not None else {}),
    }


def validate_input_rows(rows: Iterable[dict[str, Any]]) -> list[dict[str, Any]]:
    """Validate and normalize a complete input stream, fail-closed."""

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
    """Read the canonical replay stream, rejecting malformed or blank rows."""

    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise ToolError(f"{path}: cannot read input stream: {exc}") from exc
    rows: list[dict[str, Any]] = []
    for index, line in enumerate(lines, 1):
        if not line.strip():
            raise ToolError(f"{path}: blank line at {index}")
        try:
            value = json.loads(line)
        except json.JSONDecodeError as exc:
            raise ToolError(f"{path}: invalid JSON at line {index}: {exc}") from exc
        rows.append(value)
    return validate_input_rows(rows)


def save_input_jsonl(path: Path, rows: Iterable[dict[str, Any]]) -> None:
    """Write a normalized, newline-terminated canonical replay stream."""

    normalized = validate_input_rows(rows)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        "".join(json.dumps(row, sort_keys=True, separators=(",", ":")) + "\n"
                for row in normalized),
        encoding="utf-8",
    )


def validate_run_directory(directory: Path) -> dict[str, Any]:
    """Validate a named run without mutating it.

    The manifest may reference source evidence outside the run directory, but
    canonical files listed in ``files`` are always checked relative to the
    directory.  This gives CI a single fail-closed check before replay.
    """

    manifest_path = directory / "manifest.json"
    manifest = load_manifest(manifest_path)
    files = manifest.get("files", {})
    if not isinstance(files, dict):
        raise ToolError("manifest files must be an object")
    for name, fingerprint in files.items():
        if name not in RUN_FILES:
            raise ToolError(f"manifest names unknown run file {name!r}")
        path = directory / name
        if not path.is_file():
            raise ToolError(f"run is missing canonical file: {name}")
        if not isinstance(fingerprint, dict):
            raise ToolError(f"manifest fingerprint for {name} is invalid")
        actual = file_fingerprint(path)
        if actual["sha256"] != fingerprint.get("sha256"):
            raise ToolError(f"run file digest mismatch: {name}")
    input_path = directory / "input.jsonl"
    if input_path.is_file():
        validate_input_rows(load_input_jsonl(input_path))
    return manifest


def stage_run_files(directory: Path, *, input_rows: Iterable[dict[str, Any]],
                    dos_state: Path | None = None,
                    native_state: Path | None = None,
                    parity: Path | None = None,
                    coverage: Path | None = None,
                    name: str = "recorded-run") -> dict[str, Any]:
    """Create a named run directory from already-produced state artifacts."""

    directory.mkdir(parents=True, exist_ok=True)
    save_input_jsonl(directory / "input.jsonl", input_rows)
    sources = {
        "dos-state.json": dos_state,
        "native-state.json": native_state,
        "parity.json": parity,
        "coverage.json": coverage,
    }
    for target, source in sources.items():
        if source is not None:
            if not source.is_file():
                raise ToolError(f"run source does not exist: {source}")
            shutil.copyfile(source, directory / target)
    files = {
        filename: file_fingerprint(directory / filename)
        for filename in RUN_FILES if (directory / filename).is_file()
    }
    manifest = new_run_manifest(name, input_stream=load_input_jsonl(
        directory / "input.jsonl"))
    manifest["files"] = files
    save_manifest(directory / "manifest.json", manifest)
    return manifest


def verify_run_directory(directory: Path) -> tuple[list[dict[str, Any]],
                                                    list[dict[str, Any]]]:
    """Compare the canonical DOS/native states in a named run.

    The comparator is imported lazily so the run-format module remains usable
    by capture tools without importing every parity implementation.
    """

    manifest = validate_run_directory(directory)
    dos_path = directory / "dos-state.json"
    native_path = directory / "native-state.json"
    if not dos_path.is_file() or not native_path.is_file():
        raise ToolError("run parity requires dos-state.json and native-state.json")
    from .parity import compare_session

    mismatches, coverage = compare_session(dos_path, native_path)
    parity = {
        "schema": "quiky.recorded-run-parity-v1",
        "run": manifest["name"],
        "status": "pass" if not mismatches else "fail",
        "mismatches": mismatches,
        "coverage_count": len(coverage),
    }
    coverage_payload = {
        "schema": "quiky.recorded-run-coverage-v1",
        "run": manifest["name"],
        "items": coverage,
    }
    write_json(directory / "parity.json", parity)
    write_json(directory / "coverage.json", coverage_payload)
    refreshed = dict(manifest)
    refreshed["parity"] = parity
    refreshed["quality"] = {"coverage_count": len(coverage)}
    refreshed["files"] = {
        filename: file_fingerprint(directory / filename)
        for filename in RUN_FILES if (directory / filename).is_file()
    }
    save_manifest(directory / "manifest.json", refreshed)
    return mismatches, coverage


def load_manifest(path: Path) -> dict[str, Any]:
    value = read_json(path)
    try:
        return validate_manifest(value)
    except ToolError as exc:
        raise ToolError(f"{path}: {exc}") from exc


def validate_manifest(manifest: dict[str, Any]) -> dict[str, Any]:
    """Validate the in-memory v2 manifest and its required shape."""

    if not isinstance(manifest, dict) or manifest.get("schema") != RUN_SCHEMA:
        raise ToolError(f"manifest must use {RUN_SCHEMA}")
    if manifest.get("format_version") != 2:
        raise ToolError("manifest format_version must be 2")
    if not isinstance(manifest.get("name"), str) or not manifest["name"]:
        raise ToolError("manifest name must be non-empty")
    input_stream = manifest.get("input_stream")
    if not isinstance(input_stream, list):
        raise ToolError("manifest input_stream must be a list")
    validate_input_rows(input_stream)
    for key in ("traces", "artifacts", "segments", "checkpoints"):
        if key not in manifest:
            raise ToolError(f"manifest missing {key}")
    return manifest


def save_manifest(path: Path, manifest: dict[str, Any]) -> None:
    validate_manifest(manifest)
    write_json(path, manifest)
