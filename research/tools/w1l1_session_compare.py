#!/usr/bin/env python3
"""Compare a native W1L1 session trace with a DOS callback/session trace.

The comparator keeps the comparison at recovered boundaries: complete player
records, normalized input, camera anchor, callback probes when the DOS capture
contains them, callback globals/effects when published, scheduler callback
order, and active non-player object positions. Fields absent from the DOS
capture are reported as coverage gaps rather than silently treated as equal.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


class SessionTraceError(Exception):
    pass


def load_payload(path: Path) -> dict[str, Any]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise SessionTraceError(f"{path}: cannot read JSON: {exc}") from exc
    if not isinstance(payload, dict):
        raise SessionTraceError(f"{path}: expected a JSON object")
    if not isinstance(payload.get("samples"), list) and not isinstance(
            payload.get("events"), list):
        raise SessionTraceError(f"{path}: expected samples[] or events[]")
    return payload


def expected_samples(payload: dict[str, Any]) -> list[dict[str, Any]]:
    # DOS resource traces wrap the event in events[0]. Native session traces
    # publish samples directly at the top level.
    events = payload.get("events")
    if isinstance(events, list):
        if len(events) != 1 or not isinstance(events[0], dict):
            raise SessionTraceError("DOS trace must contain one event object")
        event = events[0]
        if not isinstance(event.get("samples"), list):
            raise SessionTraceError("DOS event has no samples[]")
        return event["samples"]
    return payload["samples"]


def sample_map(samples: list[Any], label: str) -> dict[int, dict[str, Any]]:
    result: dict[int, dict[str, Any]] = {}
    for sample in samples:
        if not isinstance(sample, dict) or not isinstance(sample.get("sequence"), int):
            raise SessionTraceError(f"{label}: each sample needs integer sequence")
        sequence = sample["sequence"]
        if sequence in result:
            raise SessionTraceError(f"{label}: duplicate sequence {sequence}")
        result[sequence] = sample
    return result


def callback(sample: dict[str, Any]) -> dict[str, Any]:
    value = sample.get("player_callback")
    return value if isinstance(value, dict) else {}


def record(sample: dict[str, Any], which: str) -> str | None:
    cb = callback(sample)
    if "_record_hex" in sample:
        value = sample.get("_record_hex")
        return value if isinstance(value, str) else None
    object_key = "pre_object" if which == "pre" else "post_object"
    obj = cb.get(object_key)
    if isinstance(obj, dict) and isinstance(obj.get("state_hex"), str):
        return obj["state_hex"]
    direct = cb.get(f"{which}_record_hex")
    if isinstance(direct, str):
        return direct
    direct = sample.get(f"{which}_record_hex")
    return direct if isinstance(direct, str) else None


def valid_record(value: str | None, label: str) -> str:
    if value is None:
        raise SessionTraceError(f"{label}: missing player record")
    normalized = value.lower()
    if len(normalized) != 0x78 * 2:
        raise SessionTraceError(f"{label}: record is not exactly 0x78 bytes")
    try:
        bytes.fromhex(normalized)
    except ValueError as exc:
        raise SessionTraceError(f"{label}: record is not hexadecimal") from exc
    return normalized


def input_flags(sample: dict[str, Any]) -> int | None:
    cb = callback(sample)
    value = cb.get("input_flags", sample.get("input_flags"))
    if isinstance(value, int):
        return value & 0xffff
    globals_value = sample.get("globals")
    if isinstance(globals_value, dict):
        keyboard = globals_value.get("keyboard_action_flags")
        auxiliary = globals_value.get("input_action_flags")
        if isinstance(keyboard, int) and isinstance(auxiliary, int):
            return (keyboard | auxiliary) & 0xffff
        if isinstance(auxiliary, int):
            return auxiliary & 0xffff
        if isinstance(keyboard, int):
            return keyboard & 0xffff
    return None


def camera(sample: dict[str, Any]) -> tuple[int, int] | None:
    value = sample.get("camera")
    if isinstance(value, dict) and isinstance(value.get("x"), int) and isinstance(value.get("y"), int):
        return value["x"], value["y"]
    globals_value = sample.get("globals")
    if isinstance(globals_value, dict):
        x, y = globals_value.get("camera_x"), globals_value.get("camera_y")
        if isinstance(x, int) and isinstance(y, int):
            return x, y
    return None


def probes(sample: dict[str, Any]) -> list[Any] | None:
    cb = callback(sample)
    values = cb.get("collisions") if cb else None
    if values is None:
        values = sample.get("collisions")
    if values is None:
        values = sample.get("collision_probes")
    if not isinstance(values, list):
        # DOS property captures use map_properties rather than collisions.
        values = sample.get("map_properties")
    if not isinstance(values, list):
        return None
    result = []
    for value in values:
        if not isinstance(value, dict):
            result.append(value)
            continue
        lookup = value.get("map_lookup")
        if not isinstance(lookup, dict):
            lookup = value
        result.append({
            "x": lookup.get("x", value.get("x")),
            "y": lookup.get("y", value.get("y")),
            "map_word": lookup.get("map_word", lookup.get("cell_word",
                                                               lookup.get("raw_cell_word"))),
            "tile_id": lookup.get("tile_id"),
            "descriptor_word": value.get("descriptor_word",
                                          lookup.get("descriptor_word")),
            "quadrant_mask": value.get("quadrant_mask",
                                        value.get("quadrant_flag_mask")),
            "occupied": value.get("occupied", value.get("descriptor_flag_set")),
        })
    return result


def global_writes(sample: dict[str, Any]) -> list[Any] | None:
    cb = callback(sample)
    value = cb.get("global_writes") if cb else None
    if value is None:
        value = sample.get("global_writes")
    return value if isinstance(value, list) else None


def effects(sample: dict[str, Any]) -> list[Any] | None:
    cb = callback(sample)
    value = cb.get("effects") if cb else None
    if value is None:
        value = sample.get("effects")
    return value if isinstance(value, list) else None


def callback_offsets(items: Any) -> list[int]:
    if not isinstance(items, list):
        return []
    result = []
    for item in items:
        if not isinstance(item, dict):
            result.append(-1)
            continue
        callback_value = item.get("callback")
        if isinstance(callback_value, dict):
            value = callback_value.get("offset")
        else:
            value = callback_value
        result.append(value if isinstance(value, int) else -1)
    return result


def scheduler_offsets(sample: dict[str, Any]) -> list[int] | None:
    value = sample.get("scheduler_callbacks")
    if isinstance(value, list):
        return callback_offsets(value)
    scheduler = sample.get("scheduler")
    pool = sample.get("pool")
    if isinstance(pool, dict) and isinstance(pool.get("objects"), list):
        # The DOS scheduler dump includes stale terminators and released
        # records after the live pool prefix. The active pool records are the
        # mechanically auditable callback order for this capture.
        result = []
        for item in sorted(
                (item for item in pool["objects"] if isinstance(item, dict)),
                key=lambda item: item.get("index", 0)):
            value = item.get("callback")
            if value in (None, 0, 0xffff, 16376):
                continue
            result.append(value)
        if result:
            return result
    if isinstance(scheduler, dict) and isinstance(scheduler.get("entries"), list):
        return callback_offsets(scheduler["entries"])
    return None


def active_objects(sample: dict[str, Any]) -> list[tuple[Any, ...]] | None:
    if isinstance(sample.get("entities"), list):
        values = sample["entities"]
        result = []
        for item in values:
            if not isinstance(item, dict):
                result.append((item,))
                continue
            cb = item.get("callback")
            offset = cb.get("offset") if isinstance(cb, dict) else cb
            if offset in (None, 0, 16376, 0xffff):
                continue
            result.append((offset, item.get("x"), item.get("y"),
                           item.get("sprite_slot")))
        return sorted(result, key=repr)
    pool = sample.get("pool")
    if not isinstance(pool, dict) or not isinstance(pool.get("objects"), list):
        return None
    result = []
    for item in pool["objects"]:
        if not isinstance(item, dict):
            result.append((item,))
            continue
        callback_value = item.get("callback")
        # The native callback trace's player object is represented separately
        # by player_callback, not by LevelSession.entities[].
        if callback_value == 16376:
            continue
        position = item.get("position")
        result.append((callback_value,
                       position.get("x") if isinstance(position, dict) else None,
                       position.get("y") if isinstance(position, dict) else None,
                       item.get("sprite_slot")))
    return sorted(result, key=repr)


def compare(original: Path, candidate: Path) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    left = sample_map(expected_samples(load_payload(original)), "original")
    right = sample_map(expected_samples(load_payload(candidate)), "candidate")
    mismatches: list[dict[str, Any]] = []
    coverage: list[dict[str, Any]] = []
    for sequence in sorted(set(left) | set(right)):
        if sequence not in left or sequence not in right:
            mismatches.append({"sequence": sequence, "field": "sample",
                               "original": sequence in left,
                               "candidate": sequence in right})
            continue
        expected, actual = left[sequence], right[sequence]
        for which in ("pre", "post"):
            try:
                expected_record = valid_record(record(expected, which),
                                               f"original sample {sequence} {which}")
                actual_record = valid_record(record(actual, which),
                                             f"candidate sample {sequence} {which}")
            except SessionTraceError as exc:
                mismatches.append({"sequence": sequence,
                                   "field": f"{which}_record",
                                   "error": str(exc)})
                continue
            if expected_record != actual_record:
                mismatches.append({"sequence": sequence,
                                   "field": f"{which}_record",
                                   "original": expected_record,
                                   "candidate": actual_record})

        fields = (("input_flags", input_flags(expected), input_flags(actual)),
                  ("camera", camera(expected), camera(actual)),
                  ("scheduler_callbacks", scheduler_offsets(expected),
                   scheduler_offsets(actual)),
                  ("active_objects", active_objects(expected),
                   active_objects(actual)))
        for field, expected_value, actual_value in fields:
            if expected_value is None or actual_value is None:
                coverage.append({"sequence": sequence, "field": field,
                                 "original_present": expected_value is not None,
                                 "candidate_present": actual_value is not None})
            elif expected_value != actual_value:
                mismatches.append({"sequence": sequence, "field": field,
                                   "original": expected_value,
                                   "candidate": actual_value})

        for field, normalizer in (("probes", probes),
                                  ("global_writes", global_writes),
                                  ("effects", effects)):
            expected_value = normalizer(expected)
            actual_value = normalizer(actual)
            if expected_value is None or actual_value is None:
                coverage.append({"sequence": sequence, "field": field,
                                 "original_present": expected_value is not None,
                                 "candidate_present": actual_value is not None})
            elif expected_value != actual_value:
                mismatches.append({"sequence": sequence, "field": field,
                                   "original": expected_value,
                                   "candidate": actual_value})
    return mismatches, coverage


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--max-report", type=int, default=8)
    args = parser.parse_args(argv)
    try:
        mismatches, coverage = compare(args.original, args.candidate)
    except SessionTraceError as exc:
        print(f"w1l1-session: {exc}", file=sys.stderr)
        return 2
    if mismatches:
        print(f"MISMATCH fields={len(mismatches)} coverage_gaps={len(coverage)}")
        for item in mismatches[:args.max_report]:
            print(json.dumps(item, sort_keys=True))
        if coverage:
            print("COVERAGE GAPS")
            for item in coverage[:args.max_report]:
                print(json.dumps(item, sort_keys=True))
        return 1
    print(f"OK: W1L1 session parity fields; coverage_gaps={len(coverage)}")
    for item in coverage[:args.max_report]:
        print(json.dumps(item, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
