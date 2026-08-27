#!/usr/bin/env python3
"""Verify the statically closed portion of a natural DOS WURM2 trace."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


CALLBACK = 0x6DC4


class TraceError(RuntimeError):
    pass


def signed8(value: int) -> int:
    return value - 0x100 if value & 0x80 else value


def signed32(value: int) -> int:
    value &= 0xFFFFFFFF
    return value - 0x100000000 if value & 0x80000000 else value


def u16(raw: bytes, offset: int) -> int:
    return int.from_bytes(raw[offset:offset + 2], "little")


def i32(raw: bytes, offset: int) -> int:
    return signed32(int.from_bytes(raw[offset:offset + 4], "little"))


def clamp(value: int) -> int:
    return max(-0x15000, min(0x15000, value))


def samples(payload: dict[str, Any]) -> list[dict[str, Any]]:
    events = payload.get("events")
    if not isinstance(events, list) or len(events) != 1:
        raise TraceError("trace must contain one events[] entry")
    value = events[0].get("samples")
    if not isinstance(value, list):
        raise TraceError("trace event has no samples[]")
    return [item for item in value if isinstance(item, dict)]


def verify(payload: dict[str, Any]) -> dict[str, int]:
    callbacks = []
    for sample in samples(payload):
        callback = sample.get("player_callback")
        if isinstance(callback, dict) and callback.get("callback_offset") == CALLBACK:
            callbacks.append((sample, callback))
    if not callbacks:
        raise TraceError("trace contains no natural 01F7:6DC4 callback samples")

    checked_motion = 0
    checked_animation = 0
    for sample, callback in callbacks:
        before = callback.get("pre_object")
        after = callback.get("post_object")
        if not isinstance(before, dict) or not isinstance(after, dict):
            raise TraceError(f"sample {sample.get('sequence')}: missing object records")
        try:
            before_raw = bytes.fromhex(before["state_hex"])
            after_raw = bytes.fromhex(after["state_hex"])
        except (KeyError, ValueError) as exc:
            raise TraceError(f"sample {sample.get('sequence')}: invalid object record") from exc
        if len(before_raw) != 0x78 or len(after_raw) != 0x78:
            raise TraceError(f"sample {sample.get('sequence')}: object record is not 0x78 bytes")
        if u16(before_raw, 0x18) != CALLBACK or u16(after_raw, 0x18) != CALLBACK:
            raise TraceError(f"sample {sample.get('sequence')}: callback word changed")

        state = signed8(before_raw[0x32])
        map_value = signed8(before_raw[0x2f])
        old_velocity = i32(before_raw, 0x0a)
        orientation = signed8(before_raw[0x29])
        patrol = signed8(before_raw[0x2c])
        if state < 1:
            # 6E31-6E36 uses JLE to enter 6F16.  The raw DOS latch is
            # therefore nonpositive for the ordinary/no-contact path (the
            # initializer publishes 0xFF), while positive means the
            # descriptor-contact movement path.  Native code stores this as
            # the opposite boolean spelling: mapBlocked=false is raw <= 0.
            if map_value <= 0:
                expected_velocity = old_velocity
            elif patrol < 0:
                expected_velocity = clamp(old_velocity - (orientation << 12))
            else:
                expected_velocity = clamp(old_velocity + (orientation << 10))
            expected_x = (i32(before_raw, 0x02) + expected_velocity) & 0xFFFFFFFF
            actual_x = i32(after_raw, 0x02) & 0xFFFFFFFF
            if actual_x != expected_x:
                raise TraceError(
                    f"sample {sample.get('sequence')}: 6DC4 X integration "
                    f"0x{actual_x:08x} != 0x{expected_x:08x}")
            checked_motion += 1

        old_delay = u16(before_raw, 0x20)
        new_delay = u16(after_raw, 0x20)
        old_cursor = u16(before_raw, 0x24)
        new_cursor = u16(after_raw, 0x24)
        if old_delay > 0:
            if new_delay != old_delay - 1 or new_cursor != old_cursor:
                raise TraceError(
                    f"sample {sample.get('sequence')}: 5D60 non-expired descriptor step diverged")
        else:
            if new_delay != u16(after_raw, 0x1e):
                raise TraceError(
                    f"sample {sample.get('sequence')}: 5D60 did not reload descriptor delay")
            if new_cursor == old_cursor:
                raise TraceError(
                    f"sample {sample.get('sequence')}: 5D60 expired without cursor advance")
        checked_animation += 1

    target_count = []
    for sample, _ in callbacks:
        globals_value = sample.get("globals")
        if isinstance(globals_value, dict):
            value = globals_value.get("target_active_count")
            if isinstance(value, int):
                target_count.append(value)
    return {
        "callbacks_checked": len(callbacks),
        "motion_steps_checked": checked_motion,
        "animation_steps_checked": checked_animation,
        "target_active_count_observations": len(target_count),
        "target_loop_active_observations": sum(value != 0 for value in target_count),
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
    print("OK: natural DOS WURM2 callback matches static 6DC4 contract")
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
