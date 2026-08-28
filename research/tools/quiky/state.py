"""Canonical parity state rows and strict comparisons.

Trace-shaped JSON is accepted by the importer, never by replay verification.
Recorded runs contain this module's JSONL format on both sides of the parity
boundary, so comparison has no aliases, envelope detection, or source-specific
rules.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Iterable

from .common import ToolError


STATE_SCHEMA = "quiky.parity-state-v1"
PROFILES = ("exact", "lifecycle")
CHECKPOINTS = (
    "terminal_damage", "death_hold", "recovery_gate",
    "recovered_callback", "respawn",
)


def _validate_hex(value: Any, label: str) -> str:
    if not isinstance(value, str) or len(value) != 0x78 * 2:
        raise ToolError(f"{label}: expected a complete 0x78-byte record")
    try:
        bytes.fromhex(value)
    except ValueError as exc:
        raise ToolError(f"{label}: record is not hexadecimal") from exc
    return value.lower()


def validate_row(value: Any, *, label: str = "state row") -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ToolError(f"{label}: expected object")
    if value.get("schema") != STATE_SCHEMA:
        raise ToolError(f"{label}: schema must be {STATE_SCHEMA}")
    sequence = value.get("sequence")
    if not isinstance(sequence, int) or sequence < 1:
        raise ToolError(f"{label}: sequence must be a positive integer")
    result = dict(value)
    for key in ("pre_record", "post_record"):
        if key in result:
            result[key] = _validate_hex(result[key], f"{label}.{key}")
    if "input_flags" in result and (
            not isinstance(result["input_flags"], int) or
            not 0 <= result["input_flags"] <= 0xffff):
        raise ToolError(f"{label}.input_flags must be a uint16")
    camera = result.get("camera")
    if camera is not None and (not isinstance(camera, dict) or
                               not isinstance(camera.get("x"), int) or
                               not isinstance(camera.get("y"), int)):
        raise ToolError(f"{label}.camera must contain integer x and y")
    checkpoint = result.get("checkpoint")
    if checkpoint is not None and checkpoint not in CHECKPOINTS:
        raise ToolError(f"{label}: unknown checkpoint {checkpoint!r}")
    allowed = {
        "schema", "sequence", "checkpoint", "pre_record", "post_record",
        "input_flags", "camera", "probes", "global_writes", "effects",
        "factory_objects", "scheduler_callbacks", "active_objects",
        "lifecycle",
    }
    unknown = set(result).difference(allowed)
    if unknown:
        raise ToolError(f"{label}: unknown fields: {', '.join(sorted(unknown))}")
    return result


def load_state_jsonl(path: Path) -> list[dict[str, Any]]:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise ToolError(f"{path}: cannot read state: {exc}") from exc
    if not lines:
        raise ToolError(f"{path}: state stream is empty")
    rows: list[dict[str, Any]] = []
    previous = 0
    checkpoints: set[str] = set()
    for index, line in enumerate(lines, 1):
        if not line.strip():
            raise ToolError(f"{path}: blank line at {index}")
        try:
            value = json.loads(line)
        except json.JSONDecodeError as exc:
            raise ToolError(f"{path}: invalid JSON at line {index}: {exc}") from exc
        row = validate_row(value, label=f"{path}:{index}")
        if row["sequence"] <= previous:
            raise ToolError(f"{path}: sequences must be strictly increasing")
        previous = row["sequence"]
        checkpoint = row.get("checkpoint")
        if checkpoint in checkpoints:
            raise ToolError(f"{path}: duplicate checkpoint {checkpoint}")
        if checkpoint is not None:
            checkpoints.add(checkpoint)
        rows.append(row)
    return rows


def save_state_jsonl(path: Path, rows: Iterable[dict[str, Any]]) -> None:
    normalized = [validate_row(row, label=f"state row {index}")
                  for index, row in enumerate(rows, 1)]
    if not normalized:
        raise ToolError("state stream is empty")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(
        json.dumps(row, sort_keys=True, separators=(",", ":")) + "\n"
        for row in normalized
    ), encoding="utf-8")
    load_state_jsonl(path)


def import_trace(path: Path, profile: str) -> list[dict[str, Any]]:
    """Convert one historical trace at the explicit import boundary."""

    if profile not in PROFILES:
        raise ToolError(f"unknown parity profile {profile!r}")
    # Deliberately local: the historical adapter is not reachable from run
    # validation or comparison.
    from .parity import (
        _checkpoint_state, _effects, _global_writes, _session_probes,
        active_objects, canonical_factory, scheduler_offsets,
    )
    from .trace import TraceError, load_trace

    try:
        samples = list(load_trace(path).samples)
    except TraceError as exc:
        raise ToolError(str(exc)) from exc
    if not samples:
        raise ToolError(f"{path}: trace contains no samples")
    rows: list[dict[str, Any]] = []
    for sample in samples:
        row: dict[str, Any] = {"schema": STATE_SCHEMA,
                               "sequence": sample.sequence}
        values = {
            "pre_record": sample.pre_record_hex,
            "post_record": sample.post_record_hex,
            "input_flags": sample.input_flags,
            "probes": _session_probes(sample),
            "global_writes": _global_writes(sample),
            "effects": _effects(sample),
            "factory_objects": canonical_factory(sample),
            "scheduler_callbacks": scheduler_offsets(sample),
            "active_objects": active_objects(sample),
        }
        if sample.camera is not None:
            values["camera"] = {"x": sample.camera[0], "y": sample.camera[1]}
        for field, value in values.items():
            if value is not None:
                row[field] = value
        lifecycle = _checkpoint_state(sample)
        lifecycle = {key: value for key, value in lifecycle.items()
                     if value is not None and key != "record"}
        if lifecycle:
            if isinstance(lifecycle.get("position"), tuple):
                x, y = lifecycle["position"]
                lifecycle["position"] = {"x": x, "y": y}
            row["lifecycle"] = lifecycle
        rows.append(row)
    if profile == "lifecycle":
        _label_lifecycle(rows)
    return [validate_row(row, label=f"imported sample {row['sequence']}")
            for row in rows]


def import_input(path: Path) -> list[dict[str, Any]]:
    """Extract the explicit replay stream while importing trace evidence."""
    from .trace import TraceError, load_trace
    try:
        trace = load_trace(path)
    except TraceError as exc:
        raise ToolError(str(exc)) from exc
    stream = trace.payload.get("input_stream")
    if stream is None:
        events = trace.payload.get("events")
        if isinstance(events, list) and len(events) == 1 and isinstance(events[0], dict):
            stream = events[0].get("input_stream")
    if isinstance(stream, dict):
        try:
            stream = [stream[key] for key in sorted(stream, key=lambda key: int(key))]
        except (TypeError, ValueError) as exc:
            raise ToolError(f"{path}: input_stream keys must be numeric") from exc
    if stream is not None:
        if not isinstance(stream, list) or any(not isinstance(row, dict) for row in stream):
            raise ToolError(f"{path}: input_stream must be an array of objects")
        return [dict(row) for row in stream]
    rows: list[dict[str, Any]] = []
    for sample in trace.samples:
        if sample.input_flags is None:
            raise ToolError(f"{path}: sample {sample.sequence} has no input flags")
        frame = sample.raw.get("frame_index", sample.sequence)
        row = {"sequence": sample.sequence, "guest_frame": frame,
               "input_flags": sample.input_flags}
        if sample.camera is not None:
            row["camera"] = {"x": sample.camera[0], "y": sample.camera[1]}
        rows.append(row)
    return rows


def _label_lifecycle(rows: list[dict[str, Any]]) -> None:
    previous_health: int | None = None
    terminal_seen = False
    last_negative: dict[str, Any] | None = None
    recovered: dict[str, Any] | None = None
    for row in rows:
        state = row.get("lifecycle", {})
        health, gate, mode = (state.get("health"), state.get("gate"),
                              state.get("mode"))
        if (not terminal_seen and health == 0 and
                isinstance(previous_health, int) and previous_health > 0):
            row["checkpoint"] = "terminal_damage"
            terminal_seen = True
        elif terminal_seen and health == 0 and mode == 0xff and not any(
                item.get("checkpoint") == "death_hold" for item in rows):
            row["checkpoint"] = "death_hold"
        if isinstance(gate, int) and gate < 0:
            last_negative = row
        if isinstance(gate, int) and gate <= -350 and not any(
                item.get("checkpoint") == "recovery_gate" for item in rows):
            row["checkpoint"] = "recovery_gate"
        if health and gate == 0 and mode == 0 and last_negative is not None:
            if not any(item.get("checkpoint") == "recovery_gate" for item in rows):
                last_negative["checkpoint"] = "recovery_gate"
            if not any(item.get("checkpoint") == "recovered_callback"
                       for item in rows):
                row["checkpoint"] = "recovered_callback"
                recovered = row
        elif recovered is not None and row is not recovered and state.get("position"):
            if not any(item.get("checkpoint") == "respawn" for item in rows):
                row["checkpoint"] = "respawn"
        if isinstance(health, int):
            previous_health = health


def compare_state(expected_path: Path, actual_path: Path, profile: str
                  ) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    if profile not in PROFILES:
        raise ToolError(f"unknown parity profile {profile!r}")
    expected = load_state_jsonl(expected_path)
    actual = load_state_jsonl(actual_path)
    key = ((lambda row: row.get("checkpoint")) if profile == "lifecycle"
           else (lambda row: row["sequence"]))
    if profile == "lifecycle":
        expected = [row for row in expected if row.get("checkpoint")]
        actual = [row for row in actual if row.get("checkpoint")]
    left = {key(row): row for row in expected}
    right = {key(row): row for row in actual}
    mismatches: list[dict[str, Any]] = []
    coverage: list[dict[str, Any]] = []
    for identity in sorted(set(left) | set(right), key=str):
        label = "checkpoint" if profile == "lifecycle" else "sequence"
        if identity not in left or identity not in right:
            mismatches.append({label: identity, "field": "row",
                               "expected_present": identity in left,
                               "actual_present": identity in right})
            continue
        a, b = left[identity], right[identity]
        fields = set(a) | set(b)
        fields.difference_update(("schema", "sequence", "checkpoint"))
        for field in sorted(fields):
            if field not in a or field not in b:
                coverage.append({label: identity, "field": field,
                                 "expected_present": field in a,
                                 "actual_present": field in b})
            elif a[field] != b[field]:
                mismatches.append({label: identity, "field": field,
                                   "expected": a[field], "actual": b[field]})
    return mismatches, coverage
