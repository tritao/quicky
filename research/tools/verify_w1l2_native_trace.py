#!/usr/bin/env python3
"""Fail-closed audit of the native W1L2 BUMP/WURM2 scheduler trace.

This checks only facts published by ``quiky-w1l1-trace``.  It is deliberately
not a DOS comparator: the DOS side has no captured W1L2 session record yet,
and descriptor contents/707B target rows remain explicit runtime inputs.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


PLAYER_CALLBACK = 0x3FF8
BUMP_CALLBACK = 0x9C0C
WURM2_CALLBACK = 0x6DC4


class TraceError(RuntimeError):
    pass


def _callback_offset(value: Any) -> int | None:
    if not isinstance(value, dict):
        return None
    if isinstance(value.get("offset"), int):
        return value["offset"]
    callback = value.get("callback")
    if isinstance(callback, dict):
        value = callback.get("offset")
    if isinstance(value, int):
        return value
    return None


def _entity_map(sample: dict[str, Any]) -> dict[int, dict[str, Any]]:
    entities = sample.get("entities")
    if not isinstance(entities, list):
        raise TraceError("sample has no entities[]")
    result: dict[int, dict[str, Any]] = {}
    for entity in entities:
        if not isinstance(entity, dict) or not isinstance(entity.get("id"), int):
            raise TraceError("entity entry lacks integer id")
        result[entity["id"]] = entity
    return result


def _signed32(value: int) -> int:
    value &= 0xFFFFFFFF
    return value - 0x100000000 if value & 0x80000000 else value


def _clamp(value: int, limit: int = 0x15000) -> int:
    return max(-limit, min(limit, value))


def _expected_wurm2_delta(old: dict[str, Any], new: dict[str, Any]) -> int:
    """Return the integrated velocity for one recovered 6DC4 callback.

    The callback can rewrite +0x0A after the position integration when its
    timer rolls over, so this must use the pre-state and not simply compare
    the position delta with the post-state velocity.
    """
    old_state = int(old["enemy_state"])
    old_velocity = int(old["velocity_x_fixed"])
    orientation = int(old["enemy_orientation"])
    patrol = int(old["enemy_patrol_direction"])
    map_value = int(new["map_blocked"])

    if old_state < 1:
        # Native map_blocked is the normalized form of raw object+0x2F.
        # The DOS 6DC4 listing uses JLE at 6E31-6E36: zero/nonpositive takes
        # 6F16 ordinary integration, while a positive latch takes 6E3A.
        if map_value <= 0:
            return old_velocity
        if patrol < 0:
            return _clamp(old_velocity - (orientation << 12))
        return _clamp(old_velocity + (orientation << 10))

    if map_value > 0:
        return 0
    if old_state == 2:
        if int(old["enemy_transition_timer"]) + 1 <= 0x4B:
            return 0
        return _clamp(old_velocity + (orientation << 11))
    if old_state != 3:
        return _clamp(old_velocity - (orientation << 11))
    return _clamp(old_velocity + (orientation << 11))


def verify(payload: dict[str, Any]) -> dict[str, int]:
    samples = payload.get("samples")
    if not isinstance(samples, list) or not samples:
        raise TraceError("trace has no samples[]")

    previous: dict[int, dict[str, Any]] = {}
    checked_entities = 0
    checked_motion = 0
    checked_bump = 0
    for sample_index, sample in enumerate(samples):
        if not isinstance(sample, dict):
            raise TraceError(f"sample {sample_index} is not an object")

        dependency = sample.get("player_dependency_order")
        if not isinstance(dependency, list):
            raise TraceError(f"sample {sample_index} has no player dependency order")
        phases = [item.get("phase") for item in dependency
                  if isinstance(item, dict)]
        callbacks = [_callback_offset(item) for item in dependency]
        player_indices = [index for index, offset in enumerate(callbacks)
                          if offset == PLAYER_CALLBACK]
        if len(player_indices) != 1:
            raise TraceError(f"sample {sample_index}: expected one player callback")
        player_index = player_indices[0]
        if any(phase != 1 for phase in phases[:player_index]) or phases[player_index] != 2:
            raise TraceError(
                f"sample {sample_index}: gameplay callbacks are not phase-1 before player phase-2")

        entities = _entity_map(sample)
        for entity_id, entity in entities.items():
            entity_type = entity.get("type")
            if entity_type not in (0x01, 0x02, 0x34):
                continue
            checked_entities += 1
            required = {
                "x_fixed", "y_fixed", "velocity_x_fixed", "velocity_y_fixed",
                "enemy_state", "enemy_orientation", "enemy_patrol_direction",
                "enemy_timer", "map_blocked", "enemy_animation_delay",
                "enemy_animation_sequence", "bump_animation_delay20",
                "bump_animation_cursor24",
            }
            missing = sorted(required - set(entity))
            if missing:
                raise TraceError(
                    f"sample {sample_index}, entity {entity_id}: missing {missing}")
            callback = _callback_offset(entity.get("callback"))
            expected = BUMP_CALLBACK if entity_type == 0x34 else WURM2_CALLBACK
            if callback != expected:
                raise TraceError(
                    f"sample {sample_index}, entity {entity_id}: callback "
                    f"0x{callback or 0:04x} != 0x{expected:04x}")

            old = previous.get(entity_id)
            if old is not None and entity_type in (0x01, 0x02):
                old_x = _signed32(int(old["x_fixed"]))
                new_x = _signed32(int(entity["x_fixed"]))
                delta = (new_x - old_x) & 0xFFFFFFFF
                expected_delta = _expected_wurm2_delta(old, entity) & 0xFFFFFFFF
                if delta != expected_delta:
                    raise TraceError(
                        f"sample {sample_index}, entity {entity_id}: WURM2 "
                        f"position delta 0x{delta:08x} != integrated velocity "
                        f"0x{expected_delta:08x}")
                checked_motion += 1

            if entity_type == 0x34:
                delay = int(entity["bump_animation_delay20"])
                cursor = int(entity["bump_animation_cursor24"])
                if not 0 <= delay <= 6 or not 0 <= cursor <= 3:
                    raise TraceError(
                        f"sample {sample_index}, entity {entity_id}: invalid BUMP timer/cursor")
                if old is not None:
                    old_delay = int(old["bump_animation_delay20"])
                    old_cursor = int(old["bump_animation_cursor24"])
                    if old_delay > 0:
                        if delay != old_delay - 1 or cursor != old_cursor:
                            raise TraceError(
                                f"sample {sample_index}, entity {entity_id}: "
                                "BUMP timer/cursor did not follow decrement path")
                    elif (delay, cursor) != (6, (old_cursor + 1) & 3):
                        raise TraceError(
                            f"sample {sample_index}, entity {entity_id}: "
                            "BUMP timer/cursor did not follow 5D60 cycle")
                checked_bump += 1
            previous[entity_id] = entity

    return {
        "samples_checked": len(samples),
        "entities_checked": checked_entities,
        "wurm2_motion_steps": checked_motion,
        "bump_steps": checked_bump,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    args = parser.parse_args()
    try:
        payload = json.loads(args.trace.read_text(encoding="utf-8"))
        result = verify(payload)
    except (OSError, json.JSONDecodeError, TraceError) as exc:
        parser.error(str(exc))
    print("OK: W1L2 native scheduler trace matches closed BUMP/WURM2 contracts")
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
