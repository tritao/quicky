#!/usr/bin/env python3
"""Compare recovered object contracts with a DOSBox object-behavior trace.

This is intentionally a contract comparator, not a second game emulator.  It
checks the parts already recovered exactly (descriptor sequencing, type-0x34
gate/action output, and lifecycle signatures) and reports type-0x33 motion as
observations until its MAP-dependent pre-state is modeled.
"""

from __future__ import annotations

import argparse
import json
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any

from object_behavior_trace import normalize_behavior_trace


@dataclass(frozen=True)
class ComparisonIssue:
    sequence: int
    contract: str
    field: str
    expected: Any
    actual: Any
    message: str


def _issue(issues: list[ComparisonIssue], sequence: int, contract: str,
           field: str, expected: Any, actual: Any, message: str) -> None:
    issues.append(ComparisonIssue(
        sequence=sequence,
        contract=contract,
        field=field,
        expected=expected,
        actual=actual,
        message=message,
    ))


def _samples(payload: dict[str, Any]) -> list[dict[str, Any]]:
    if "events" in payload:
        events = payload.get("events") or []
        if not events:
            return []
        return events[0].get("samples", []) or []
    return payload.get("samples", []) or []


def _config(payload: dict[str, Any]) -> dict[str, Any]:
    if "config" in payload:
        return payload["config"]
    return {}


def _signed16(value: int) -> int:
    return value - 0x10000 if value & 0x8000 else value


def _sequence_words(descriptor: dict[str, Any]) -> list[int]:
    words = descriptor.get("sequence_words", [])
    if isinstance(words, dict):
        return [item for _, item in sorted(
            ((int(key), value) for key, value in words.items()),
        )]
    return list(words or [])


def _sequence_value(descriptor: dict[str, Any], address: int) -> int | None:
    base = descriptor.get("sequence_base", 0)
    words = _sequence_words(descriptor)
    delta = (address - base) & 0xffff
    if delta & 1 or delta // 2 >= len(words):
        return None
    return _signed16(words[delta // 2] & 0xffff)


def _resolve_descriptor(descriptor: dict[str, Any]) -> tuple[int, int] | None:
    """Resolve the next 5D60 entry from a pre-callback descriptor state."""
    cursor = (descriptor["sequence_cursor"] + 2) & 0xffff
    value = _sequence_value(descriptor, cursor)
    if value is None:
        return None
    jumps = 0
    while value < 0:
        cursor = (cursor + value * 2) & 0xffff
        value = _sequence_value(descriptor, cursor)
        jumps += 1
        if value is None or jumps > 32:
            return None
    return cursor, value


def compare_descriptors(samples: list[dict[str, Any]],
                        issues: list[ComparisonIssue],
                        config: dict[str, Any] | None = None) -> dict[str, int]:
    config = config or {}
    mode_is_probe_controlled = config.get("probe_descriptor_mode") is not None
    checked = 0
    resolved = 0
    for sample in samples:
        before = (sample.get("object_before") or {}).get("descriptor")
        after = (sample.get("object_after") or {}).get("descriptor")
        if not isinstance(before, dict) or not isinstance(after, dict):
            continue
        callback = sample.get("callback") or {}
        helper_offsets = _related_offsets(sample)
        # A type-0x34 hit reloads the descriptor through an additional 5D38
        # call after 5D60; the single-step descriptor contract does not apply
        # to that frame. Likewise, a gated 9C0C callback never reaches 5D60.
        if 0x5d38 in helper_offsets:
            continue
        if callback.get("offset") == 0x9c0c and 0x5d60 not in helper_offsets:
            continue
        sequence = int(sample.get("sequence", 0))
        if after.get("reload_delay", 0) == 0:
            continue
        checked += 1

        initializing = (before.get("reload_delay", 0) == 0 and
                        before.get("sequence_base", 0) == 0)
        if initializing:
            expected = after.get("reload_delay")
            if after.get("timer") != expected:
                _issue(issues, sequence, "descriptor", "timer",
                       expected, after.get("timer"),
                       "descriptor initializer did not copy its reload delay")
            if after.get("sequence_cursor") != after.get("sequence_base"):
                _issue(issues, sequence, "descriptor", "sequence_cursor",
                       after.get("sequence_base"), after.get("sequence_cursor"),
                       "descriptor initializer did not point at sequence base")
            continue

        for field in ("reload_delay", "sequence_base", "mode"):
            if field == "mode" and mode_is_probe_controlled:
                continue
            if before.get(field) != after.get(field):
                _issue(issues, sequence, "descriptor", field,
                       before.get(field), after.get(field),
                       "descriptor identity changed during 5D60")

        if before.get("timer", 0) > 0:
            expected_timer = before["timer"] - 1
            if after.get("timer") != expected_timer:
                _issue(issues, sequence, "descriptor", "timer",
                       expected_timer, after.get("timer"),
                       "non-expired descriptor timer must decrement by one")
            if after.get("sequence_cursor") != before.get("sequence_cursor"):
                _issue(issues, sequence, "descriptor", "sequence_cursor",
                       before.get("sequence_cursor"), after.get("sequence_cursor"),
                       "cursor moved while descriptor timer was nonzero")
        else:
            resolution = _resolve_descriptor(before)
            if resolution is not None:
                expected_cursor, value = resolution
                resolved += 1
                if after.get("sequence_cursor") != expected_cursor:
                    _issue(issues, sequence, "descriptor", "sequence_cursor",
                           expected_cursor, after.get("sequence_cursor"),
                           "expired descriptor did not follow its sequence/jump word")
                expected_timer = before.get("reload_delay", 0)
                if after.get("timer") != expected_timer:
                    _issue(issues, sequence, "descriptor", "timer",
                           expected_timer, after.get("timer"),
                           "expired descriptor did not reload its timer")
                expected_action = value
                if before.get("mode") == 0xff:
                    expected_action += 0x32
                actual_action = (sample.get("object_after") or {}).get("sprite_slot")
                if actual_action is not None and actual_action != (expected_action & 0xffff):
                    _issue(issues, sequence, "descriptor", "sprite_slot",
                           expected_action & 0xffff, actual_action,
                           "expired descriptor selected the wrong action")

    return {"samples_checked": checked, "expiry_steps_resolved": resolved}


def _related_offsets(sample: dict[str, Any]) -> set[int]:
    callback = sample.get("callback") or {}
    offsets = {
        int(hit["offset"])
        for hit in callback.get("related_hits", []) or []
        if isinstance(hit, dict) and "offset" in hit
    }
    offsets.update(
        int(hit["offset"])
        for hit in callback.get("helper_calls", []) or []
        if isinstance(hit, dict) and "offset" in hit
    )
    return offsets


def _strict_proximity(object_position: dict[str, Any],
                       player_position: dict[str, Any]) -> bool:
    ox, oy = object_position["x"], object_position["y"]
    px, py = player_position["x"], player_position["y"]
    return (ox - 0x19 < px < ox + 0x19 and
            oy - 8 < py < oy)


def compare_type34(payload: dict[str, Any], samples: list[dict[str, Any]],
                   issues: list[ComparisonIssue]) -> dict[str, int]:
    config = _config(payload)
    checked = 0
    hit_frames = 0
    gate_frames = 0
    for sample in samples:
        callback = sample.get("callback") or {}
        if callback.get("offset") != 0x9c0c:
            continue
        before_globals = sample.get("globals_before") or {}
        after_globals = sample.get("globals_after") or {}
        before_object = sample.get("object_before") or {}
        before_player = sample.get("bounds_object_before") or {}
        if "proximity_gate" not in before_globals:
            continue
        checked += 1
        sequence = int(sample.get("sequence", 0))
        gate = int(before_globals["proximity_gate"])
        active = gate < 0x32
        offsets = _related_offsets(sample)
        if active:
            gate_frames += 1
            if 0x9c29 not in offsets and not sample.get("termination", {}).get("visibility_gate_hit"):
                _issue(issues, sequence, "type34", "proximity_helper",
                       True, False,
                       "active DS:85DA gate did not reach 9C29")

        probe_class = config.get("probe_bounds_byte_37")
        probe_position = config.get("probe_position_x") is not None
        can_predict_hit = (probe_class is not None and probe_class > 0 and
                           probe_class < 0x80 and probe_position and
                           isinstance(before_object.get("position"), dict) and
                           isinstance(before_player.get("position"), dict))
        predicted_hit = False
        if can_predict_hit and active:
            predicted_hit = _strict_proximity(
                before_object["position"], before_player["position"])
        action_before = int(before_globals.get("action_word", 0))
        action_after = int(after_globals.get("action_word", 0))
        if predicted_hit:
            hit_frames += 1
            if action_after != 4:
                _issue(issues, sequence, "type34", "action_word",
                       4, action_after,
                       "strict proximity hit did not publish action 4")
            # The action chain is entered only on the 0 -> 4 transition;
            # subsequent frames retain action 4 while only 5D60/39FE run.
            if action_before != 4:
                expected_helpers = {0x1b5d, 0x0fcf}
                missing = sorted(expected_helpers - offsets)
                if missing:
                    _issue(issues, sequence, "type34", "action_chain",
                           sorted(expected_helpers), sorted(offsets),
                           f"proximity hit missed helpers {missing}")
        elif not active and action_after != action_before:
            _issue(issues, sequence, "type34", "action_word",
                   action_before, action_after,
                   "inactive DS:85DA gate changed the action word")

    return {"samples_checked": checked, "active_gate_frames": gate_frames,
            "predicted_hit_frames": hit_frames}


def compare_type33(samples: list[dict[str, Any]],
                   issues: list[ComparisonIssue]) -> dict[str, int]:
    """Check stable type-0x33 facts without pretending MAP state is solved."""
    checked = 0
    descriptor_frames = 0
    movement_observations = 0
    for sample in samples:
        callback = sample.get("callback") or {}
        if callback.get("offset") != 0x882f:
            continue
        before = sample.get("object_before") or {}
        after = sample.get("object_after") or {}
        if not before or not after:
            continue
        checked += 1
        if before.get("descriptor") and after.get("descriptor"):
            descriptor_frames += 1
        before_position = before.get("position") or {}
        after_position = after.get("position") or {}
        if "x_fixed" in before_position and "x_fixed" in after_position:
            movement_observations += 1
        if after.get("update_callback") == 0 and not sample.get("termination", {}).get("visibility_gate_hit"):
            _issue(issues, int(sample.get("sequence", 0)), "type33",
                   "update_callback", "nonzero or visibility cull",
                   after.get("update_callback"),
                   "type-0x33 callback cleared without a recorded camera cull")
    return {"samples_checked": checked, "descriptor_frames": descriptor_frames,
            "movement_observations": movement_observations}


def compare_payload(payload: dict[str, Any], family: str = "auto") -> dict[str, Any]:
    payload = json.loads(json.dumps(payload))
    if "events" in payload and payload.get("events"):
        payload["events"][0] = normalize_behavior_trace(payload["events"][0])
    elif "samples" in payload:
        payload = normalize_behavior_trace(payload)
    samples = _samples(payload)
    entity_type = int(_config(payload).get("entity_type", -1))
    if family == "auto":
        family = {0x33: "type33", 0x34: "type34"}.get(entity_type, "generic")

    issues: list[ComparisonIssue] = []
    descriptor_result = compare_descriptors(samples, issues, _config(payload))
    family_result: dict[str, Any] = {}
    if family == "type33":
        family_result = compare_type33(samples, issues)
    elif family == "type34":
        family_result = compare_type34(payload, samples, issues)

    return {
        "schema": "quiky-object-behavior-comparison-v1",
        "family": family,
        "entity_type": entity_type,
        "sample_count": len(samples),
        "descriptor": descriptor_result,
        "family_result": family_result,
        "issue_count": len(issues),
        "issues": [asdict(issue) for issue in issues],
        "passed": not issues,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("--family", choices=("auto", "generic", "type33", "type34"),
                        default="auto")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    payload = json.loads(args.trace.read_text(encoding="utf-8"))
    result = compare_payload(payload, args.family)
    text = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.write_text(text, encoding="utf-8")
    else:
        print(text, end="")
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
