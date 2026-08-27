"""Recorded-run manifest primitives.

This is intentionally a small artifact boundary for the next parity phases;
it does not capture or replay a run by itself.  Existing raw traces remain the
authoritative evidence and are referenced by path plus digest.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any, Iterable

from .common import ToolError, file_fingerprint, read_json, write_json


RUN_SCHEMA = "quiky.recorded-run-v1"


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


def load_manifest(path: Path) -> dict[str, Any]:
    value = read_json(path)
    if not isinstance(value, dict) or value.get("schema") != RUN_SCHEMA:
        raise ToolError(f"{path}: expected {RUN_SCHEMA}")
    return value


def save_manifest(path: Path, manifest: dict[str, Any]) -> None:
    if manifest.get("schema") != RUN_SCHEMA:
        raise ToolError(f"manifest must use {RUN_SCHEMA}")
    write_json(path, manifest)
