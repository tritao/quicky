#!/usr/bin/env python3
"""Fail-closed callback parity comparison for original and C++ trace JSON.

The candidate adapter intentionally uses the same small interchange fields as
the DOSBox player tracer. A C++ trace must provide, for each sample, either a
``player_callback`` object or ``pre_record_hex``/``post_record_hex`` and must
provide the complete ordered collision-probe, callback-global, and known
effect fields. Missing data is a mismatch; position-only comparisons are not
accepted.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


class ParityError(Exception):
    pass


GLOBAL_FIELD_MAP = {
    "horizontal_timer": (0x4FEE, 2),
    "horizontal_accumulator": (0x4FE2, 4),
    "horizontal_aux": (0x4FE8, 4),
    "horizontal_branch_counter": (0x4FEC, 2),
    "camera_x": (0x81C0, 2),
    "camera_y": (0x81C4, 2),
    "player_vertical_adjust": (0x8812, 4),
    "horizontal_result_byte": (0x4FF0, 1),
}


def load_payload(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ParityError(f"{path}: cannot read JSON: {exc}") from exc
    if not isinstance(value, dict):
        raise ParityError(f"{path}: top-level value must be an object")
    events = value.get("events")
    if isinstance(events, list):
        if len(events) != 1 or not isinstance(events[0], dict):
            raise ParityError(f"{path}: events must contain one object")
        value = events[0]
    if not isinstance(value.get("samples"), list):
        raise ParityError(f"{path}: no samples array")
    return value


def sample_map(payload: dict[str, Any], label: str) -> dict[int, dict[str, Any]]:
    result: dict[int, dict[str, Any]] = {}
    for sample in payload["samples"]:
        if not isinstance(sample, dict) or not isinstance(sample.get("sequence"), int):
            raise ParityError(f"{label}: every sample needs integer sequence")
        sequence = sample["sequence"]
        if sequence in result:
            raise ParityError(f"{label}: duplicate sample sequence {sequence}")
        result[sequence] = sample
    return result


def callback(sample: dict[str, Any]) -> dict[str, Any] | None:
    value = sample.get("player_callback")
    return value if isinstance(value, dict) else None


def record_hex(sample: dict[str, Any], which: str) -> str | None:
    cb = callback(sample)
    if cb is not None:
        obj = cb.get("pre_object" if which == "pre" else "post_object")
        if isinstance(obj, dict) and isinstance(obj.get("state_hex"), str):
            return obj["state_hex"]
        direct = cb.get("pre_record_hex" if which == "pre" else "post_record_hex")
        if isinstance(direct, str):
            return direct
    direct = sample.get("pre_record_hex" if which == "pre" else "post_record_hex")
    return direct if isinstance(direct, str) else None


def validate_record(value: str | None, label: str) -> str:
    if value is None:
        raise ParityError(f"{label}: missing complete player record")
    normalized = value.lower()
    if len(normalized) != 0x78 * 2:
        raise ParityError(f"{label}: player record is not exactly 0x78 bytes")
    try:
        bytes.fromhex(normalized)
    except ValueError as exc:
        raise ParityError(f"{label}: player record is not hexadecimal") from exc
    return normalized


def canonical_probes(sample: dict[str, Any]) -> list[Any] | None:
    events = sample.get("collisions")
    if events is None:
        events = sample.get("collision_probes")
    if not isinstance(events, list):
        return None
    result: list[Any] = []
    for event in events:
        if not isinstance(event, dict):
            result.append(event)
            continue
        map_property = event.get("map_property")
        lookup = (map_property.get("map_lookup")
                  if isinstance(map_property, dict) else None)
        source = lookup if isinstance(lookup, dict) else event
        result.append({
            # These are the fields both the DOS normalizer and C++ emitter
            # can publish without interpreting the helper's carry/flags.
            "x": source.get("x"),
            "y": source.get("y"),
            "cell_word": source.get("cell_word", source.get("raw_cell_word")),
            "tile_id": source.get("tile_id"),
            "descriptor_word": (map_property.get("descriptor_word")
                                 if isinstance(map_property, dict)
                                 else event.get("descriptor_word")),
            "quadrant_mask": event.get("quadrant_mask"),
            "occupied": event.get("occupied"),
        })
    return result


def canonical_global_write(value: Any) -> Any:
    if not isinstance(value, dict):
        return value
    result = dict(value)
    field = result.get("field")
    if field in GLOBAL_FIELD_MAP and "offset" not in result:
        result["offset"], result["width"] = GLOBAL_FIELD_MAP[field]
    result.pop("field", None)
    return result


def canonical_globals(sample: dict[str, Any]) -> Any:
    cb = callback(sample)
    if cb is not None and isinstance(cb.get("global_writes"), list):
        return [canonical_global_write(value) for value in cb["global_writes"]]
    if isinstance(sample.get("global_writes"), list):
        return [canonical_global_write(value) for value in sample["global_writes"]]
    return []


def canonical_factory(sample: dict[str, Any]) -> Any:
    event = sample.get("factory_event")
    if not isinstance(event, dict):
        return []
    created = event.get("created_objects")
    if not isinstance(created, list):
        return []
    selected = []
    for obj in created:
        if not isinstance(obj, dict):
            selected.append(obj)
            continue
        position = obj.get("position")
        selected.append({
            "offset": obj.get("offset"),
            "callback": obj.get("callback"),
            "kind": obj.get("kind"),
            "phase": obj.get("phase"),
            "sprite_slot": obj.get("sprite_slot"),
            "position": position,
        })
    return selected


def canonical_effects(sample: dict[str, Any]) -> Any:
    cb = callback(sample)
    if cb is not None and isinstance(cb.get("effect_dispatches"), list):
        return cb["effect_dispatches"]
    for key in ("effects", "effect_dispatches"):
        if isinstance(sample.get(key), list):
            return sample[key]
    return []


def canonical_input(sample: dict[str, Any]) -> Any:
    cb = callback(sample)
    if cb is not None and isinstance(cb.get("input_flags"), int):
        return cb["input_flags"]
    globals_value = sample.get("globals")
    if isinstance(globals_value, dict) and isinstance(
            globals_value.get("input_action_flags"), int):
        return globals_value["input_action_flags"]
    return None


def compare(original: Path, candidate: Path) -> list[dict[str, Any]]:
    left = sample_map(load_payload(original), "original")
    right = sample_map(load_payload(candidate), "candidate")
    mismatches: list[dict[str, Any]] = []
    for sequence in sorted(set(left) | set(right)):
        if sequence not in left or sequence not in right:
            mismatches.append({"sequence": sequence, "field": "sample", "original": sequence in left, "candidate": sequence in right})
            continue
        a, b = left[sequence], right[sequence]
        for which in ("pre", "post"):
            try:
                av = validate_record(record_hex(a, which), f"original sample {sequence} {which}")
                bv = validate_record(record_hex(b, which), f"candidate sample {sequence} {which}")
            except ParityError as exc:
                mismatches.append({"sequence": sequence, "field": f"{which}_record", "error": str(exc)})
                continue
            if av != bv:
                mismatches.append({"sequence": sequence, "field": f"{which}_record", "original": av, "candidate": bv})
        fields = (("input_flags", canonical_input(a), canonical_input(b)),
                  ("probes", canonical_probes(a), canonical_probes(b)),
                  ("global_writes", canonical_globals(a), canonical_globals(b)),
                  ("factory_objects", canonical_factory(a), canonical_factory(b)),
                  ("effects", canonical_effects(a), canonical_effects(b)))
        for field, av, bv in fields:
            if av is None or bv is None:
                mismatches.append({"sequence": sequence, "field": field, "error": "missing required parity data"})
            elif av != bv:
                mismatches.append({"sequence": sequence, "field": field, "original": av, "candidate": bv})
    return mismatches


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--max-report", type=int, default=8)
    args = parser.parse_args(argv)
    try:
        mismatches = compare(args.original, args.candidate)
    except ParityError as exc:
        print(f"player-parity: {exc}", file=sys.stderr)
        return 2
    if mismatches:
        print(f"MISMATCH callbacks={len(mismatches)}")
        for item in mismatches[:args.max_report]:
            print(json.dumps(item, sort_keys=True))
        return 1
    print("OK: player callback parity")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
