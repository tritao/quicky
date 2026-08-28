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


STATE_SCHEMA = "quiky.parity-state-v2"
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
    checkpoints = result.get("checkpoints")
    if checkpoints is not None:
        if (not isinstance(checkpoints, list) or
                any(item not in CHECKPOINTS for item in checkpoints)):
            raise ToolError(f"{label}.checkpoints contains an unknown checkpoint")
        if len(set(checkpoints)) != len(checkpoints):
            raise ToolError(f"{label}.checkpoints contains duplicates")
    allowed = {
        "schema", "sequence", "checkpoints", "pre_record", "post_record",
        "input_flags", "camera", "probes", "global_writes", "effects",
        "factory_objects", "scheduler_callbacks", "active_objects",
        "lifecycle",
    }
    unknown = set(result).difference(allowed)
    if unknown:
        raise ToolError(f"{label}: unknown fields: {', '.join(sorted(unknown))}")
    for field in ("probes", "global_writes", "effects", "factory_objects",
                  "scheduler_callbacks", "active_objects"):
        if field in result and not isinstance(result[field], list):
            raise ToolError(f"{label}.{field} must be an array")
    for index, item in enumerate(result.get("probes", [])):
        _validate_object(item, {"x", "y", "map_word", "tile_id",
                                "descriptor_word", "quadrant_mask", "occupied"},
                         f"{label}.probes[{index}]")
        for key in ("x", "y", "map_word", "tile_id", "descriptor_word",
                    "quadrant_mask"):
            if item[key] is not None and not isinstance(item[key], int):
                raise ToolError(f"{label}.probes[{index}].{key} must be integer or null")
        if item["occupied"] is not None and not isinstance(item["occupied"], bool):
            raise ToolError(f"{label}.probes[{index}].occupied must be boolean or null")
    for index, item in enumerate(result.get("global_writes", [])):
        _validate_object(item, {"offset", "width", "before", "after"},
                         f"{label}.global_writes[{index}]")
        if any(not isinstance(item[key], int) for key in item):
            raise ToolError(f"{label}.global_writes[{index}] values must be integers")
        if item["width"] not in (1, 2, 4):
            raise ToolError(f"{label}.global_writes[{index}].width is invalid")
    for index, item in enumerate(result.get("effects", [])):
        _validate_object(item, {"address", "code"}, f"{label}.effects[{index}]")
        if any(not isinstance(item[key], int) for key in item):
            raise ToolError(f"{label}.effects[{index}] values must be integers")
    for index, item in enumerate(result.get("factory_objects", [])):
        _validate_object(item, {"offset", "callback", "kind", "phase",
                                "sprite_slot", "position"},
                         f"{label}.factory_objects[{index}]")
    if any(not isinstance(item, int)
           for item in result.get("scheduler_callbacks", [])):
        raise ToolError(f"{label}.scheduler_callbacks must contain integers")
    for index, item in enumerate(result.get("active_objects", [])):
        if not isinstance(item, dict):
            raise ToolError(f"{label}.active_objects[{index}] must be an object")
        required = {"callback", "x", "y", "sprite_slot"}
        optional = {"velocity_y_fixed", "animation_delay", "animation_cursor"}
        if not required.issubset(item) or set(item).difference(required | optional):
            raise ToolError(f"{label}.active_objects[{index}] has invalid fields")
        if any(value is not None and not isinstance(value, int)
               for value in item.values()):
            raise ToolError(f"{label}.active_objects[{index}] values must be integers or null")
    lifecycle = result.get("lifecycle")
    if lifecycle is not None:
        if not isinstance(lifecycle, dict):
            raise ToolError(f"{label}.lifecycle must be an object")
        unknown_lifecycle = set(lifecycle).difference(
            {"health", "lives", "gate", "mode", "position"})
        if unknown_lifecycle:
            raise ToolError(f"{label}.lifecycle has unknown fields")
        for key in ("health", "lives", "gate", "mode"):
            if key in lifecycle and not isinstance(lifecycle[key], int):
                raise ToolError(f"{label}.lifecycle.{key} must be an integer")
        position = lifecycle.get("position")
        if position is not None and (not isinstance(position, dict) or
                                     set(position) != {"x", "y"} or
                                     not isinstance(position["x"], int) or
                                     not isinstance(position["y"], int)):
            raise ToolError(f"{label}.lifecycle.position must contain integer x and y")
    return result


def _validate_object(value: Any, fields: set[str], label: str) -> None:
    if not isinstance(value, dict) or set(value) != fields:
        raise ToolError(f"{label} must contain exactly {', '.join(sorted(fields))}")


def load_state_jsonl(path: Path) -> list[dict[str, Any]]:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise ToolError(f"{path}: cannot read state: {exc}") from exc
    if not lines:
        raise ToolError(f"{path}: state stream is empty")
    rows: list[dict[str, Any]] = []
    previous = 0
    published_checkpoints: set[str] = set()
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
        for checkpoint in row.get("checkpoints", []):
            if checkpoint in published_checkpoints:
                raise ToolError(f"{path}: duplicate checkpoint {checkpoint}")
            published_checkpoints.add(checkpoint)
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
    from .trace_import import (
        active_objects, effects, factory_objects, global_writes, lifecycle,
        probes, scheduler_callbacks, TraceError, load_trace,
    )

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
            "probes": probes(sample),
            "global_writes": global_writes(sample),
            "effects": effects(sample),
            "factory_objects": factory_objects(sample),
            "scheduler_callbacks": scheduler_callbacks(sample),
            "active_objects": active_objects(sample),
        }
        if sample.camera is not None:
            values["camera"] = {"x": sample.camera[0], "y": sample.camera[1]}
        for field, value in values.items():
            if value is not None:
                row[field] = value
        lifecycle_state = {key: value for key, value in lifecycle(sample).items()
                           if value is not None}
        if lifecycle_state:
            if isinstance(lifecycle_state.get("position"), tuple):
                x, y = lifecycle_state["position"]
                lifecycle_state["position"] = {"x": x, "y": y}
            row["lifecycle"] = lifecycle_state
        rows.append(row)
    if profile == "lifecycle":
        label_lifecycle(rows)
    return [validate_row(row, label=f"imported sample {row['sequence']}")
            for row in rows]


def import_input(path: Path) -> list[dict[str, Any]]:
    """Extract the explicit replay stream while importing trace evidence."""
    from .trace_import import TraceError, load_trace
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


def label_lifecycle(rows: list[dict[str, Any]]) -> None:
    def publish(row: dict[str, Any], checkpoint: str) -> None:
        values = row.setdefault("checkpoints", [])
        if checkpoint not in values:
            values.append(checkpoint)

    def published(checkpoint: str) -> bool:
        return any(checkpoint in item.get("checkpoints", []) for item in rows)

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
            publish(row, "terminal_damage")
            terminal_seen = True
        if terminal_seen and health == 0 and mode == 0xff and not any(
                "death_hold" in item.get("checkpoints", []) for item in rows):
            publish(row, "death_hold")
        if isinstance(gate, int) and gate < 0:
            last_negative = row
        if isinstance(gate, int) and gate <= -350 and not any(
                "recovery_gate" in item.get("checkpoints", []) for item in rows):
            publish(row, "recovery_gate")
        if health and gate == 0 and mode == 0 and last_negative is not None:
            if not published("recovery_gate"):
                publish(last_negative, "recovery_gate")
            if not published("recovered_callback"):
                publish(row, "recovered_callback")
                recovered = row
        elif recovered is not None and row is not recovered and state.get("position"):
            if not published("respawn"):
                publish(row, "respawn")
        if isinstance(health, int):
            previous_health = health


def compare_state(expected_path: Path, actual_path: Path, profile: str
                  ) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    if profile not in PROFILES:
        raise ToolError(f"unknown parity profile {profile!r}")
    expected = load_state_jsonl(expected_path)
    actual = load_state_jsonl(actual_path)
    if profile == "lifecycle":
        left = {checkpoint: row for row in expected
                for checkpoint in row.get("checkpoints", [])}
        right = {checkpoint: row for row in actual
                 for checkpoint in row.get("checkpoints", [])}
        identities: list[Any] = list(CHECKPOINTS)
    else:
        left = {row["sequence"]: row for row in expected}
        right = {row["sequence"]: row for row in actual}
        identities = sorted(set(left) | set(right))
    mismatches: list[dict[str, Any]] = []
    coverage: list[dict[str, Any]] = []
    for identity in identities:
        label = "checkpoint" if profile == "lifecycle" else "sequence"
        if identity not in left or identity not in right:
            mismatches.append({label: identity, "field": "row",
                               "expected_present": identity in left,
                               "actual_present": identity in right})
            continue
        a, b = left[identity], right[identity]
        fields = set(a) | set(b)
        fields.difference_update(("schema", "sequence", "checkpoints"))
        for field in sorted(fields):
            if field not in a or field not in b:
                coverage.append({label: identity, "field": field,
                                 "expected_present": field in a,
                                 "actual_present": field in b})
            elif field == "lifecycle":
                _compare_mapping(a[field], b[field], label, identity, field,
                                 mismatches, coverage)
            elif field == "active_objects":
                _compare_active_objects(a[field], b[field], label, identity,
                                        mismatches, coverage)
            elif a[field] != b[field]:
                mismatches.append({label: identity, "field": field,
                                   "expected": a[field], "actual": b[field]})
    return mismatches, coverage


def _compare_mapping(expected: dict[str, Any], actual: dict[str, Any],
                     identity_name: str, identity: Any, field: str,
                     mismatches: list[dict[str, Any]],
                     coverage: list[dict[str, Any]]) -> None:
    for key in sorted(set(expected) | set(actual)):
        name = f"{field}.{key}"
        if key not in expected or key not in actual:
            coverage.append({identity_name: identity, "field": name,
                             "expected_present": key in expected,
                             "actual_present": key in actual})
        elif expected[key] != actual[key]:
            mismatches.append({identity_name: identity, "field": name,
                               "expected": expected[key], "actual": actual[key]})


def _compare_active_objects(expected: list[dict[str, Any]],
                            actual: list[dict[str, Any]],
                            identity_name: str, identity: Any,
                            mismatches: list[dict[str, Any]],
                            coverage: list[dict[str, Any]]) -> None:
    keys = ("callback", "x", "y", "sprite_slot")
    if (len(expected) != len(actual) or
            [tuple(item.get(key) for key in keys) for item in expected] !=
            [tuple(item.get(key) for key in keys) for item in actual]):
        mismatches.append({identity_name: identity, "field": "active_objects",
                           "expected": expected, "actual": actual})
        return
    for index, (left, right) in enumerate(zip(expected, actual)):
        for key in sorted((set(left) | set(right)).difference(keys)):
            field = f"active_objects[{index}].{key}"
            if key not in left or key not in right:
                coverage.append({identity_name: identity, "field": field,
                                 "expected_present": key in left,
                                 "actual_present": key in right})
            elif left[key] != right[key]:
                mismatches.append({identity_name: identity, "field": field,
                                   "expected": left[key], "actual": right[key]})
