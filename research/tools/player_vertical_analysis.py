#!/usr/bin/env python3
"""Normalize and validate the recovered QUIKY player vertical path.

The matrix runner owns capture and remains intentionally frozen.  This module
is the vertical-specific analysis layer: it accepts the runner's raw or
normalized traces (and the archival WIP trace shape), applies the recovered
fixed-point equations, and emits compact validation evidence.
"""

from __future__ import annotations

import argparse
import copy
import json
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence


SCHEMA = "quiky-player-vertical-analysis-v1"
VALIDATION_SCHEMA = "quiky-player-vertical-validation-v1"

JUMP_IMPULSE = -0x4A000
RELEASE_FLOOR = -0x20000
ASCENT_ACCELERATION = 0x2000
GRAVITY = 0x2800
TERMINAL_VELOCITY = 0x40000


def _signed32(value: int) -> int:
    value &= 0xFFFFFFFF
    return value - 0x100000000 if value & 0x80000000 else value


def predict_ascent_velocity(old_velocity: int, action_word: int = 0,
                            contact_scratch: int = 0) -> int:
    """Return the raw next velocity for the reachable negative-mode path."""
    value = old_velocity + ASCENT_ACCELERATION
    if contact_scratch == 0 and (action_word & 0x22) == 0 and value < RELEASE_FLOOR:
        value = RELEASE_FLOOR
    return _signed32(value)


def predict_descent_velocity(old_velocity: int) -> int:
    """Return the raw next velocity for positive mode, including its cap."""
    return _signed32(min(old_velocity + GRAVITY, TERMINAL_VELOCITY))


def predict_position(old_position: int, next_velocity: int) -> int:
    """The callback adds the newly updated velocity to the 16.16 Y dword."""
    return _signed32(old_position + next_velocity)


def _as_list(value: Any) -> list[Any]:
    if isinstance(value, list):
        return value
    if isinstance(value, Mapping):
        try:
            return [value[key] for key in sorted(value, key=lambda item: int(item))]
        except (TypeError, ValueError) as exc:
            raise ValueError("numeric-key object expected for trace array") from exc
    return []


def _copy_position(obj: Mapping[str, Any]) -> dict[str, Any]:
    position = obj.get("position")
    if isinstance(position, Mapping):
        result = copy.deepcopy(dict(position))
        if "y_fixed_signed" not in result and "y_fixed" in result:
            result["y_fixed_signed"] = _signed32(int(result["y_fixed"]))
        if "x_fixed_signed" not in result and "x_fixed" in result:
            result["x_fixed_signed"] = _signed32(int(result["x_fixed"]))
        return result
    result = {}
    for name in ("x", "y", "x_fixed", "y_fixed", "x_fixed_signed", "y_fixed_signed"):
        if name in obj:
            result[name] = copy.deepcopy(obj[name])
    if "y_fixed_signed" not in result and "y_fixed" in result:
        result["y_fixed_signed"] = _signed32(int(result["y_fixed"]))
    if "x_fixed_signed" not in result and "x_fixed" in result:
        result["x_fixed_signed"] = _signed32(int(result["x_fixed"]))
    return result


def _signed_velocity(obj: Mapping[str, Any]) -> int:
    for name in ("velocity_y_fixed_signed", "velocity_y_fixed"):
        if name in obj:
            return _signed32(int(obj[name]))
    return 0


def _copy_velocity(obj: Mapping[str, Any]) -> dict[str, Any]:
    result = {
        "velocity_y_fixed_signed": _signed_velocity(obj),
    }
    if "velocity_y_fixed" in obj:
        result["velocity_y_fixed"] = int(obj["velocity_y_fixed"]) & 0xFFFFFFFF
    else:
        result["velocity_y_fixed"] = result["velocity_y_fixed_signed"] & 0xFFFFFFFF
    for name in ("velocity_x_fixed", "velocity_x_fixed_signed"):
        if name in obj:
            result[name] = copy.deepcopy(obj[name])
    return result


ACTION_FIELDS = (
    "action_word", "player_byte_0x36", "player_byte_0x37", "player_byte_0x38",
    "player_byte_0x39", "player_byte_0x3a", "player_byte_0x3b", "player_word_0x3e",
    "vertical_step", "vertical_step_signed", "state_hex",
)


def _copy_action(obj: Mapping[str, Any]) -> dict[str, Any]:
    return {name: copy.deepcopy(obj[name]) for name in ACTION_FIELDS if name in obj}


def _object_from_callback(callback: Mapping[str, Any], name: str) -> Mapping[str, Any] | None:
    value = callback.get(name)
    if isinstance(value, Mapping):
        return value
    if name == "pre_object":
        value = callback.get("object")
        if isinstance(value, Mapping):
            return value
    return None


def _normalize_sample(sample: Mapping[str, Any], original_index: int) -> dict[str, Any]:
    callback = sample.get("player_callback")
    if not isinstance(callback, Mapping):
        callback = {}
    before = _object_from_callback(callback, "pre_object") or {}
    after = _object_from_callback(callback, "post_object") or before
    frame = sample.get("frame_index", sample.get("frame", original_index))
    sequence = sample.get("sequence", original_index + 1)
    return {
        "sequence": int(sequence),
        "frame_index": int(frame),
        "position": {"before": _copy_position(before), "after": _copy_position(after)},
        "velocity": {"before": _copy_velocity(before), "after": _copy_velocity(after)},
        "action_state": {"before": _copy_action(before), "after": _copy_action(after)},
        "callback_writes": copy.deepcopy(callback.get("writes", sample.get("writes", []))),
        "collision_events": copy.deepcopy(sample.get("collision_events", [])),
        "descriptor_properties": copy.deepcopy(sample.get("descriptor_properties", [])),
        "map_cells": copy.deepcopy(sample.get("map_lookups", sample.get("map_cells", []))),
        "mutation_ledger": copy.deepcopy(sample.get("mutation_ledger", {})),
        "execute_watches": copy.deepcopy(sample.get("execute_watches", sample.get("related_breakpoints", {}))),
    }


def normalize_trace(payload: Mapping[str, Any]) -> dict[str, Any]:
    """Return the compact vertical timeline for a raw/current/WIP trace."""
    if not isinstance(payload, Mapping):
        raise ValueError("trace must be a JSON object")
    if isinstance(payload.get("frames"), list):
        frames = copy.deepcopy(payload["frames"])
    else:
        samples: Any = payload.get("samples")
        if samples is None and isinstance(payload.get("trace"), Mapping):
            samples = payload["trace"].get("samples")
        if samples is None and isinstance(payload.get("events"), list) and payload["events"]:
            samples = payload["events"][0].get("samples", [])
        frames = [_normalize_sample(sample, index) for index, sample in enumerate(_as_list(samples))
                  if isinstance(sample, Mapping)]
    frames.sort(key=lambda item: (int(item.get("frame_index", 0)), int(item.get("sequence", 0))))
    return {
        "schema": SCHEMA,
        "source_schema": payload.get("schema", payload.get("trace_schema_version")),
        "frames": frames,
    }


def _state(frame: Mapping[str, Any], side: str) -> Mapping[str, Any]:
    value = frame.get("action_state", {}).get(side, {})
    return value if isinstance(value, Mapping) else {}


def _position_y(frame: Mapping[str, Any], side: str) -> int:
    value = frame.get("position", {}).get(side, {})
    if not isinstance(value, Mapping):
        return 0
    return int(value.get("y_fixed_signed", _signed32(int(value.get("y_fixed", 0)))))


def _velocity_y(frame: Mapping[str, Any], side: str) -> int:
    value = frame.get("velocity", {}).get(side, {})
    if not isinstance(value, Mapping):
        return 0
    return _signed32(int(value.get("velocity_y_fixed_signed", value.get("velocity_y_fixed", 0))))


def _mode(frame: Mapping[str, Any], side: str) -> int:
    value = _state(frame, side).get("player_byte_0x37", 0)
    value = int(value) & 0xFF
    return value - 0x100 if value & 0x80 else value


def _action(frame: Mapping[str, Any], side: str) -> int:
    return int(_state(frame, side).get("action_word", 0)) & 0xFFFF


def _comparison(kind: str, frame: Mapping[str, Any], expected: int, actual: int,
                field: str) -> dict[str, Any]:
    return {
        "kind": kind,
        "sequence": frame.get("sequence"),
        "frame_index": frame.get("frame_index"),
        "field": field,
        "expected": expected,
        "actual": actual,
    }


def validate_timeline(timeline: Mapping[str, Any]) -> dict[str, Any]:
    """Validate exact free-space vertical transitions in one compact trace."""
    frames = timeline.get("frames", []) if isinstance(timeline, Mapping) else []
    checks = 0
    mismatches: list[dict[str, Any]] = []
    skipped: list[dict[str, Any]] = []
    apex: list[dict[str, Any]] = []
    for previous, current in zip(frames, frames[1:]):
        old_mode = _mode(previous, "after")
        new_mode = _mode(current, "after")
        old_y = _position_y(previous, "after")
        old_vy = _velocity_y(previous, "after")
        new_y = _position_y(current, "after")
        new_vy = _velocity_y(current, "after")
        if old_mode == -1 and new_mode == -1:
            expected_vy = predict_ascent_velocity(old_vy, _action(current, "after"))
            checks += 1
            if new_vy != expected_vy:
                mismatches.append(_comparison("ascent_velocity", current, expected_vy, new_vy, "vy"))
            expected_y = predict_position(old_y, expected_vy)
            checks += 1
            if new_y != expected_y:
                mismatches.append(_comparison("ascent_position", current, expected_y, new_y, "y"))
        elif old_mode == -1 and new_mode > 0:
            checks += 2
            if new_vy != 0:
                mismatches.append(_comparison("apex_velocity", current, 0, new_vy, "vy"))
            if new_y != old_y:
                mismatches.append(_comparison("apex_position", current, old_y, new_y, "y"))
            apex.append({"sequence": current.get("sequence"), "frame_index": current.get("frame_index"),
                         "y_fixed": new_y, "velocity_y_fixed": new_vy, "mode": new_mode})
        elif old_mode == 1 and new_mode == 1:
            expected_vy = predict_descent_velocity(old_vy)
            checks += 1
            if new_vy != expected_vy:
                mismatches.append(_comparison("descent_velocity", current, expected_vy, new_vy, "vy"))
            expected_y = predict_position(old_y, expected_vy)
            checks += 1
            if new_y != expected_y:
                mismatches.append(_comparison("descent_position", current, expected_y, new_y, "y"))
        else:
            skipped.append({"sequence": current.get("sequence"), "frame_index": current.get("frame_index"),
                            "from_mode": old_mode, "to_mode": new_mode,
                            "reason": "collision_or_non-free-space_transition"})
    return {
        "schema": SCHEMA,
        "checked_values": checks,
        "mismatches": len(mismatches),
        "first_mismatch": mismatches[0] if mismatches else None,
        "apex_transitions": apex,
        "skipped_transitions": skipped,
        "status": "pass" if not mismatches else "fail",
    }


def _trace_paths(values: Sequence[str]) -> list[Path]:
    result: list[Path] = []
    for value in values:
        path = Path(value)
        if path.is_dir():
            result.extend(sorted(path.rglob("normalized.json")))
        elif path.is_file():
            result.append(path)
        else:
            raise ValueError(f"trace path does not exist: {path}")
    return result


def build_validation(derivation: Sequence[Path], held_out: Sequence[Path]) -> dict[str, Any]:
    records: list[dict[str, Any]] = []
    for role, paths in (("derivation", derivation), ("held-out", held_out)):
        for path in paths:
            payload = json.loads(path.read_text(encoding="utf-8"))
            timeline = normalize_trace(payload)
            result = validate_timeline(timeline)
            records.append({"role": role, "path": str(path), **result})
    checked = sum(int(item["checked_values"]) for item in records)
    mismatches = sum(int(item["mismatches"]) for item in records)
    return {
        "schema": VALIDATION_SCHEMA,
        "formula": {
            "format": "signed 32-bit 16.16",
            "jump_impulse": JUMP_IMPULSE,
            "release_floor": RELEASE_FLOOR,
            "ascent_acceleration": ASCENT_ACCELERATION,
            "gravity": GRAVITY,
            "terminal_velocity": TERMINAL_VELOCITY,
            "position_uses": "newly updated vertical velocity",
            "static_ranges": ["01F7:41E8-427E", "01F7:4323-436A"],
        },
        "derivation_traces": [item for item in records if item["role"] == "derivation"],
        "held_out_traces": [item for item in records if item["role"] == "held-out"],
        "derivation_trace_count": len(derivation),
        "held_out_trace_count": len(held_out),
        "checked_values": checked,
        "mismatches": mismatches,
        "first_mismatch": next((item["first_mismatch"] for item in records if item["first_mismatch"]), None),
        "status": "pass" if mismatches == 0 else "fail",
    }


def _main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    normalize = subparsers.add_parser("normalize")
    normalize.add_argument("input", type=Path)
    normalize.add_argument("--output", required=True, type=Path)
    validate = subparsers.add_parser("validate")
    validate.add_argument("--derivation", action="append", default=[])
    validate.add_argument("--held-out", action="append", default=[])
    validate.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if args.command == "normalize":
        payload = json.loads(args.input.read_text(encoding="utf-8"))
        result = normalize_trace(payload)
    else:
        result = build_validation(_trace_paths(args.derivation), _trace_paths(args.held_out))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"status": result.get("status", "ok"), "output": str(args.output)}))
    return 0 if result.get("status", "pass") == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(_main())
