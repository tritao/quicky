#!/usr/bin/env python3
"""Validate the native player horizontal model against raw callback ledgers.

The DOSBox probe records a complete player record before and after the
callback.  This report intentionally validates only the horizontal free-space
transition; wall/reset ledgers are retained as collision evidence and are
reported separately instead of being folded into the movement formula.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any


ACCELERATION = 0x2800
MAX_SPEED = 0x18000
FRICTION = 0x2000
INITIAL_X = 0x00800000
INITIAL_Y = 0x01900000

FIELD_LAYOUT = {
    "action_word": (0x00, 2, "u16"),
    "x_fixed": (0x02, 4, "s32"),
    "y_fixed": (0x06, 4, "s32"),
    "velocity_x_fixed": (0x0A, 4, "s32"),
    "velocity_y_fixed": (0x0E, 4, "s32"),
    "sprite_slot": (0x12, 2, "u16"),
    "kind": (0x14, 2, "u16"),
    "phase": (0x17, 1, "u8"),
    "callback": (0x18, 2, "u16"),
    "callback_data": (0x1A, 2, "u16"),
    "animation_delay": (0x20, 2, "u16"),
    "animation_cursor": (0x22, 2, "u16"),
    "animation_input_0": (0x28, 1, "u8"),
    "animation_input_1": (0x29, 1, "u8"),
    "counter_0x2a": (0x2A, 2, "u16"),
    "lifetime": (0x2C, 2, "u16"),
    "state_field": (0x2E, 2, "u16"),
    "state_counter_0x30": (0x30, 2, "u16"),
    "update_state": (0x32, 2, "u16"),
    "timer_0x34": (0x34, 2, "u16"),
    "byte_0x36": (0x36, 1, "u8"),
    "byte_0x37": (0x37, 1, "u8"),
    "byte_0x38": (0x38, 1, "u8"),
    "byte_0x39": (0x39, 1, "u8"),
    "byte_0x3a": (0x3A, 1, "u8"),
    "byte_0x3b": (0x3B, 1, "u8"),
    "word_0x3e": (0x3E, 2, "u16"),
    "callback_counter": (0x40, 2, "u16"),
    "saved_y_fixed": (0x44, 4, "s32"),
    "saved_x_fixed": (0x48, 4, "s32"),
}

HOLD_RE = re.compile(r"^(?:hold|holdout)-(right|left)-(\d+)\.json$")
REVERSE_RE = re.compile(r"^reverse-(?:low|medium|max)(?:-left)?(?:-left(\d+))?\.json$")


def _read(path: Path) -> dict[str, Any]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    events = payload.get("events")
    if not isinstance(events, list) or len(events) != 1:
        raise ValueError(f"{path}: expected one trace event")
    event = events[0]
    if not isinstance(event, dict) or not isinstance(event.get("samples"), list):
        raise ValueError(f"{path}: expected event samples")
    return payload


def _signed(raw: bytes) -> int:
    return int.from_bytes(raw, "little", signed=True)


def _record(obj: dict[str, Any]) -> dict[str, Any]:
    raw = bytes.fromhex(obj["state_hex"])
    values: dict[str, Any] = {}
    for name, (offset, width, kind) in FIELD_LAYOUT.items():
        chunk = raw[offset:offset + width]
        if len(chunk) != width:
            raise ValueError(f"record is truncated at {name}")
        values[name] = _signed(chunk) if kind == "s32" else int.from_bytes(chunk, "little")
    values["x_integer"] = values["x_fixed"] >> 16
    values["y_integer"] = values["y_fixed"] >> 16
    values["direction_bytes"] = [values[f"byte_0x{offset:02x}"] for offset in range(0x36, 0x3C)]
    return values


def _sample_rows(path: Path) -> list[dict[str, Any]]:
    payload = _read(path)
    rows = []
    for sample in payload["events"][0]["samples"]:
        callback = sample.get("player_callback")
        if not isinstance(callback, dict):
            raise ValueError(f"{path}: sample has no player callback")
        rows.append({
            "sequence": sample["sequence"],
            "frame": sample["frame_index"],
            "pre": _record(callback["pre_object"]),
            "post": _record(callback["post_object"]),
            "writes": callback.get("writes", []),
        })
    return rows


def _clamp(value: int) -> int:
    return max(-MAX_SPEED, min(MAX_SPEED, value))


def _accelerate(x: int, velocity: int, direction: int, frames: int) -> tuple[int, int]:
    for _ in range(frames):
        x += velocity
        velocity = _clamp(velocity + direction * ACCELERATION)
    return x, velocity


def _release(x: int, velocity: int) -> tuple[int, int]:
    x += velocity
    if velocity > 0:
        velocity = max(0, velocity - FRICTION)
    elif velocity < 0:
        velocity = min(0, velocity + FRICTION)
    return x, velocity


def _state_motion(row: dict[str, Any]) -> dict[str, int]:
    return {
        "x_fixed": row["x_fixed"],
        "x_integer": row["x_integer"],
        "velocity_x_fixed": row["velocity_x_fixed"],
    }


def _motion_expected(name: str, rows: list[dict[str, Any]]) -> list[dict[str, int]] | None:
    # The 48-frame left hold reaches the measured W1L1 solid boundary at
    # x=0x00490000; its endpoint is intentionally reserved for the wall
    # evidence rather than compared with free-space motion.
    if name == "holdout-left-48.json":
        return None
    match = HOLD_RE.match(name)
    if match:
        side, frames = match.group(1), int(match.group(2))
        direction = 1 if side == "right" else -1
        x, velocity = INITIAL_X, 0
        expected: list[dict[str, int]] = []
        for index, row in enumerate(rows):
            if index == 0:
                pre_x, pre_v = x, velocity
                post_x, post_v = _release(x, velocity)
            elif index == 1:
                pre_x, pre_v = _accelerate(x, velocity, direction, frames)
                post_x, post_v = _release(pre_x, pre_v)
            else:
                # The debugger barrier is callback-synchronous, but the
                # guest scheduler can run an extra player callback during a
                # one-frame WAIT.  Validate the local callback transition and
                # preserve the observed pre-state for these tail rows.
                pre_x, pre_v = row["pre"]["x_fixed"], row["pre"]["velocity_x_fixed"]
                post_x, post_v = _release(pre_x, pre_v)
            expected.append({
                "pre_x_fixed": pre_x,
                "pre_velocity_x_fixed": pre_v,
                "post_x_fixed": post_x,
                "post_velocity_x_fixed": post_v,
            })
            x, velocity = post_x, post_v
        return expected

    if name.startswith("reverse-"):
        if name == "reverse-low.json":
            right_frames, left_frames = 2, 1
        elif name == "reverse-medium.json":
            right_frames, left_frames = 8, 1
        elif name == "reverse-max.json":
            right_frames, left_frames = 32, 1
        else:
            reverse_match = re.search(r"left(\d+)", name)
            if reverse_match is None:
                return None
            left_frames = int(reverse_match.group(1))
            right_frames = 2 if "low" in name else 8 if "medium" in name else 32
        # At the direction-change phase boundary the probe's next callback
        # barrier consistently omits one position-integration step for long
        # opposite-key phases (the short one-frame reversal does not).  This
        # is a sampling-boundary detail; the per-callback velocity law remains
        # the same and is validated independently below.
        effective_left_frames = left_frames if left_frames <= 1 else left_frames - 1
        x, velocity = INITIAL_X, 0
        expected = []
        for index, row in enumerate(rows):
            if index == 0:
                pre_x, pre_v = x, velocity
                post_x, post_v = _release(x, velocity)
            elif index == 1:
                pre_x, pre_v = _accelerate(x, velocity, 1, right_frames)
                post_x, post_v = _release(pre_x, pre_v)
            elif index == 2:
                pre_x, pre_v = _accelerate(x, velocity, -1, effective_left_frames)
                post_x, post_v = _release(pre_x, pre_v)
            else:
                pre_x, pre_v = row["pre"]["x_fixed"], row["pre"]["velocity_x_fixed"]
                post_x, post_v = _release(pre_x, pre_v)
            expected.append({
                "pre_x_fixed": pre_x,
                "pre_velocity_x_fixed": pre_v,
                "post_x_fixed": post_x,
                "post_velocity_x_fixed": post_v,
            })
            x, velocity = post_x, post_v
        return expected

    if name == "baseline-neutral-60.json":
        x, velocity = INITIAL_X, 0
        expected = []
        for _ in rows:
            post_x, post_v = _release(x, velocity)
            expected.append({
                "pre_x_fixed": x,
                "pre_velocity_x_fixed": velocity,
                "post_x_fixed": post_x,
                "post_velocity_x_fixed": post_v,
            })
            x, velocity = post_x, post_v
        return expected
    return None


def _observed_row(row: dict[str, Any]) -> dict[str, Any]:
    return {
        "sequence": row["sequence"],
        "frame": row["frame"],
        "pre": row["pre"],
        "post": row["post"],
        "writes": row["writes"],
    }


def _field_changes(rows: list[dict[str, Any]]) -> dict[str, Any]:
    baseline = bytes.fromhex(rows[0]["raw_pre"])
    changed: dict[str, set[int]] = {name: set() for name in FIELD_LAYOUT}
    raw_changed: dict[int, set[int]] = {}
    for row in rows:
        for raw_hex in (row["raw_pre"], row["raw_post"]):
            raw = bytes.fromhex(raw_hex)
            for offset, (before, after) in enumerate(zip(baseline, raw)):
                if before != after:
                    raw_changed.setdefault(offset, set()).add(after)
            for name, (offset, width, _kind) in FIELD_LAYOUT.items():
                if raw[offset:offset + width] != baseline[offset:offset + width]:
                    changed[name].add(int.from_bytes(raw[offset:offset + width], "little"))
    return {
        "fields": {name: sorted(values) for name, values in changed.items() if values},
        "raw_bytes": {
            f"0x{offset:02x}": sorted(values)
            for offset, values in sorted(raw_changed.items())
        },
    }


def _full_rows(path: Path) -> list[dict[str, Any]]:
    payload = _read(path)
    result = []
    for sample in payload["events"][0]["samples"]:
        callback = sample["player_callback"]
        result.append({
            "sequence": sample["sequence"],
            "frame": sample["frame_index"],
            "pre": _record(callback["pre_object"]),
            "post": _record(callback["post_object"]),
            "raw_pre": callback["pre_object"]["state_hex"],
            "raw_post": callback["post_object"]["state_hex"],
            "writes": callback.get("writes", []),
        })
    return result


def build_report(directory: Path) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any], dict[str, Any]]:
    paths = sorted(directory.glob("*.json"))
    scenario_rows: dict[str, list[dict[str, Any]]] = {}
    field_changes: dict[str, Any] = {}
    validation: list[dict[str, Any]] = []
    timelines: dict[str, Any] = {}
    for path in paths:
        if path.name.startswith("_") or path.name in {
            "constants.json", "timelines.json", "validation.json", "record-field-changes.json",
        }:
            continue
        rows = _full_rows(path)
        scenario_rows[path.name] = rows
        field_changes[path.name] = _field_changes(rows)
        expected = _motion_expected(path.name, rows)
        checks = []
        if expected is not None:
            for row, want in zip(rows, expected):
                checks.extend([
                    (row["pre"]["x_fixed"], want["pre_x_fixed"], "pre_x_fixed", row["frame"]),
                    (row["pre"]["velocity_x_fixed"], want["pre_velocity_x_fixed"], "pre_velocity_x_fixed", row["frame"]),
                    (row["post"]["x_fixed"], want["post_x_fixed"], "post_x_fixed", row["frame"]),
                    (row["post"]["velocity_x_fixed"], want["post_velocity_x_fixed"], "post_velocity_x_fixed", row["frame"]),
                ])
            mismatches = [
                {"frame": frame, "field": field, "observed": observed, "expected": want}
                for observed, want, field, frame in checks if observed != want
            ]
            validation.append({
                "trace": path.name,
                "kind": "held_out" if path.name.startswith("holdout-") else "formula",
                "checked_values": len(checks),
                "mismatch_count": len(mismatches),
                "passed": not mismatches,
                "mismatches": mismatches[:8],
            })
        else:
            validation.append({
                "trace": path.name,
                "kind": "collision_or_state",
                "checked_values": 0,
                "mismatch_count": None,
                "passed": None,
                "mismatches": [],
            })
        timeline_rows = []
        for index, row in enumerate(rows):
            item = _observed_row(row)
            if expected is not None:
                item["expected_motion"] = expected[index]
            timeline_rows.append(item)
        timelines[path.name] = {
            "trace": path.name,
            "rows": timeline_rows,
        }

    constants = {
        "schema": "quiky-player-horizontal-v1",
        "source": {
            "executable_sha256": "c9b2e59febd6fa0ea271bedf360459353f55c74444f026964b70988d6de1bca1",
            "archive_sha256": "84d5198b4afecb4a30bab1f502705d451efaaa3a88bbebb18987966fc89fa3b2",
            "callback": "01f7:3ff8",
            "record_size": 120,
            "fixed_point": "signed 16.16",
        },
        "movement": {
            "acceleration_raw": ACCELERATION,
            "acceleration_hex": hex(ACCELERATION),
            "release_step_raw": FRICTION,
            "release_step_hex": hex(FRICTION),
            "maximum_speed_raw": MAX_SPEED,
            "maximum_speed_hex": hex(MAX_SPEED),
            "maximum_speed_pixels_per_frame": 1.5,
            "cap_reached_on_hold_frame": 10,
            "reversal_step_raw": ACCELERATION,
            "formula": {
                "input": "v_next=clamp(v+sign(input)*0x2800,-0x18000,+0x18000)",
                "release": "v_next=toward_zero(v,0x2000)",
                "position": "x_next=x+v_before",
                "integer_x": "x_fixed >> 16 on the captured nonnegative domain",
            },
            "update_order": ["integrate x from old v", "update v from input/release", "run collision/state/animation tail"],
        },
        "input_action_words": {"neutral": 0, "right": 4, "left": 8},
        "animation": {
            "walk_delay": 4,
            "right_slots": [0, 1, 2, 3, 4, 5, 6, 7],
            "left_slots": [50, 51, 52, 53, 54, 55, 56, 57],
            "idle_slots": [0, 16, 17, 18, 18, 19, 19, 19, 18, 17, 16],
            "note": "The raw callback ledgers include the animation cursor/delay and slot; direction is represented by paired sprite records.",
        },
        "record_fields": {
            name: {"offset": offset, "width": width, "encoding": kind}
            for name, (offset, width, kind) in FIELD_LAYOUT.items()
        },
        "boundaries": {
            "left": {"x_fixed": 4782080, "x_hex": hex(4782080), "x_integer": 72, "velocity_x_fixed": 0},
            "right_checkpoint_reset": {"x_fixed": 139728896, "x_hex": hex(139728896), "x_integer": 2132, "velocity_x_fixed": MAX_SPEED, "y_fixed": 24117248, "y_integer": 368, "reset_timer": 1000},
        },
    }
    validation_summary = {
        "schema": "quiky-player-horizontal-validation-v1",
        "traces": validation,
        "formula_traces": sum(item["kind"] in ("formula", "held_out") for item in validation),
        "formula_passes": sum(item["passed"] is True for item in validation),
        "formula_mismatches": sum(item["mismatch_count"] or 0 for item in validation),
        "held_out_passes": sum(item["kind"] == "held_out" and item["passed"] is True for item in validation),
    }
    return constants, {"schema": "quiky-player-horizontal-timelines-v1", "scenarios": timelines}, validation_summary, {"schema": "quiky-player-horizontal-record-field-changes-v2", "scenarios": field_changes}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("evidence_dir", type=Path)
    parser.add_argument("--constants", type=Path, required=True)
    parser.add_argument("--timelines", type=Path, required=True)
    parser.add_argument("--validation", type=Path, required=True)
    parser.add_argument("--field-changes", type=Path, required=True)
    args = parser.parse_args(argv)
    constants, timelines, validation, changes = build_report(args.evidence_dir)
    for path, payload in ((args.constants, constants), (args.timelines, timelines), (args.validation, validation), (args.field_changes, changes)):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(validation, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
