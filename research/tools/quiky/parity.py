"""Shared parity comparisons built on the normalized trace adapters."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Iterable

from .trace import NormalizedSample, TraceError, extract_samples, load_trace


class ParityError(Exception):
    """Raised when a player callback trace cannot be compared safely."""


class SessionTraceError(Exception):
    """Raised when a session trace cannot be compared safely."""


PLAYER_PARITY_FIELDS = (
    "input_flags",
    "probes",
    "global_writes",
    "factory_objects",
    "effects",
)


GLOBAL_FIELD_MAP = {
    "dispatch_previous_word_60da": (0x60DA, 2),
    "dispatch_aux_4ff2": (0x4FF2, 4),
    "dispatch_aux_4ff8": (0x4FF8, 2),
    "dispatch_aux_4ffa": (0x4FFA, 2),
    "horizontal_timer": (0x4FEE, 2),
    "horizontal_accumulator": (0x4FE2, 4),
    "horizontal_aux": (0x4FE8, 4),
    "horizontal_branch_counter": (0x4FEC, 2),
    "pending_event": (0x612E, 2),
    "camera_x": (0x81C0, 2),
    "camera_y": (0x81C4, 2),
    "player_vertical_adjust": (0x8812, 4),
    "horizontal_result_byte": (0x4FF0, 1),
}


def _raw(sample: NormalizedSample | dict[str, Any]) -> dict[str, Any]:
    return sample.raw if isinstance(sample, NormalizedSample) else sample


def _callback(sample: NormalizedSample | dict[str, Any]) -> dict[str, Any] | None:
    if isinstance(sample, NormalizedSample):
        return sample.callback
    value = sample.get("player_callback")
    return value if isinstance(value, dict) else None


def _ordered_array(value: Any) -> list[Any] | None:
    """Accept JSON arrays and Lua's numeric-key table representation."""

    if isinstance(value, list):
        return value
    if not isinstance(value, dict):
        return None
    indexed: list[tuple[int, Any]] = []
    for key, item in value.items():
        try:
            index = int(key)
        except (TypeError, ValueError):
            return None
        indexed.append((index, item))
    return [item for _, item in sorted(indexed)]


def _record_hex(sample: NormalizedSample | dict[str, Any], which: str) -> str | None:
    if isinstance(sample, NormalizedSample):
        return sample.pre_record_hex if which == "pre" else sample.post_record_hex
    callback = _callback(sample)
    if callback is not None:
        obj = callback.get("pre_object" if which == "pre" else "post_object")
        if isinstance(obj, dict) and isinstance(obj.get("state_hex"), str):
            return obj["state_hex"]
        direct = callback.get(f"{which}_record_hex")
        if isinstance(direct, str):
            return direct
    direct = sample.get(f"{which}_record_hex")
    return direct if isinstance(direct, str) else None


def validate_record(value: str | None, label: str, *, error_type: type[Exception] = ParityError) -> str:
    if value is None:
        missing = ("missing player record" if error_type is SessionTraceError
                   else "missing complete player record")
        raise error_type(f"{label}: {missing}")
    normalized = value.lower()
    if len(normalized) != 0x78 * 2:
        kind = "record" if error_type is SessionTraceError else "player record"
        raise error_type(f"{label}: {kind} is not exactly 0x78 bytes")
    try:
        bytes.fromhex(normalized)
    except ValueError as exc:
        kind = "record" if error_type is SessionTraceError else "player record"
        raise error_type(f"{label}: {kind} is not hexadecimal") from exc
    return normalized


def canonical_probes(sample: NormalizedSample | dict[str, Any]) -> list[Any] | None:
    """Canonicalize the callback/property probe variants used by player parity."""

    raw = _raw(sample)
    property_events = raw.get("map_properties")
    if isinstance(property_events, list):
        # New player captures explicitly tag the one property helper that can
        # fire before the callback barrier. Older traces have no scope field
        # and retain their historical behavior.
        property_events = [
            event for event in property_events
            if not (isinstance(event, dict) and
                    event.get("scope") == "outside_player_callback")
        ]
        result: list[Any] = []
        for event in property_events:
            if not isinstance(event, dict):
                result.append(event)
                continue
            lookup = event.get("map_lookup")
            if not isinstance(lookup, dict):
                lookup = event
            coordinates = event.get("coordinates")
            if not isinstance(coordinates, dict):
                coordinates = {}
            x = lookup.get("x", coordinates.get("x"))
            y = lookup.get("y", coordinates.get("y"))
            descriptor = event.get("descriptor_word")
            if isinstance(x, int) and isinstance(y, int):
                x_bit = (x & 0x0008) != 0
                y_bit = (y & 0x0008) != 0
                quadrant = ((0x02 if x_bit else 0x01) if y_bit else
                            (0x04 if x_bit else 0x08))
            else:
                quadrant = event.get("quadrant_flag_mask")
            helper = event.get("helper_offset")
            if helper in (0x1C6E, 0x1C92):
                occupied = event.get("raw_map_bit_set")
            elif isinstance(descriptor, int) and isinstance(quadrant, int):
                occupied = bool((descriptor & 0x000F) and
                                (descriptor & quadrant))
            else:
                occupied = event.get("descriptor_flag_set")
            result.append({
                "x": x,
                "y": y,
                "cell_word": lookup.get("cell_word",
                                         lookup.get("raw_cell_word")),
                "tile_id": lookup.get("tile_id"),
                "descriptor_word": descriptor,
                "quadrant_mask": quadrant,
                "occupied": occupied,
            })
        return result

    events = raw.get("collisions")
    if events is None:
        events = raw.get("collision_probes")
    if not isinstance(events, list):
        return None
    result = []
    for event in events:
        if not isinstance(event, dict):
            result.append(event)
            continue
        map_property = event.get("map_property")
        lookup = (map_property.get("map_lookup")
                  if isinstance(map_property, dict) else None)
        source = lookup if isinstance(lookup, dict) else event
        result.append({
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


def canonical_globals(sample: NormalizedSample | dict[str, Any]) -> Any:
    raw = _raw(sample)
    callback = _callback(sample)
    if callback is not None and isinstance(callback.get("global_writes"), list):
        return [canonical_global_write(value) for value in callback["global_writes"]]
    if isinstance(raw.get("global_writes"), list):
        return [canonical_global_write(value) for value in raw["global_writes"]]
    return None


def canonical_factory(sample: NormalizedSample | dict[str, Any]) -> Any:
    raw = _raw(sample)
    event = raw.get("factory_event")
    if not isinstance(event, dict):
        callback = _callback(sample)
        event = callback.get("factory_event") if callback is not None else None
    if not isinstance(event, dict):
        return None
    created = _ordered_array(event.get("created_objects"))
    if created is None:
        return None
    selected = []
    for obj in created:
        if not isinstance(obj, dict):
            selected.append(obj)
            continue
        selected.append({
            "offset": obj.get("offset"),
            "callback": obj.get("callback"),
            "kind": obj.get("kind"),
            "phase": obj.get("phase"),
            "sprite_slot": obj.get("sprite_slot"),
            "position": obj.get("position"),
        })
    return selected


def canonical_effects(sample: NormalizedSample | dict[str, Any]) -> Any:
    raw = _raw(sample)
    callback = _callback(sample)
    if callback is not None:
        for key in ("effects", "effect_dispatches"):
            value = _ordered_array(callback.get(key))
            if value is not None:
                return value
    for key in ("effects", "effect_dispatches"):
        value = _ordered_array(raw.get(key))
        if value is not None:
            return value
    return None


def _record_coverage(value: str | None) -> str:
    if value is None:
        return "missing"
    try:
        validate_record(value, "player parity coverage")
    except ParityError:
        return "invalid"
    return "present"


def _field_coverage(values: Iterable[tuple[int, Any]]) -> dict[str, Any]:
    present = 0
    missing: list[int] = []
    for sequence, value in values:
        if value is None:
            missing.append(sequence)
        else:
            present += 1
    return {"present": present, "missing": missing}


def player_parity_coverage(path: Path) -> dict[str, Any]:
    """Report which comparable player fields a trace actually publishes."""

    try:
        trace = load_trace(path)
    except TraceError as exc:
        raise ParityError(str(exc)) from exc

    samples = trace.by_sequence
    record_fields: dict[str, dict[str, Any]] = {}
    for which in ("pre", "post"):
        statuses = {"present": 0, "missing": [], "invalid": []}
        for sequence, sample in sorted(samples.items()):
            status = _record_coverage(_record_hex(sample, which))
            if status == "present":
                statuses["present"] += 1
            else:
                statuses[status].append(sequence)
        record_fields[f"{which}_record"] = statuses

    comparable = {
        "input_flags": canonical_input,
        "probes": canonical_probes,
        "global_writes": canonical_globals,
        "factory_objects": canonical_factory,
        "effects": canonical_effects,
    }
    fields: dict[str, Any] = dict(record_fields)
    for field, getter in comparable.items():
        fields[field] = _field_coverage(
            (sequence, getter(sample))
            for sequence, sample in sorted(samples.items())
        )
    return {
        "schema": "quiky.player-parity-coverage.v1",
        "trace": str(path),
        "samples": len(samples),
        "fields": fields,
    }


def canonical_input(sample: NormalizedSample | dict[str, Any]) -> Any:
    raw = _raw(sample)
    callback = _callback(sample)
    if callback is not None and isinstance(callback.get("input_flags"), int):
        return callback["input_flags"]
    globals_value = raw.get("globals")
    if isinstance(globals_value, dict):
        keyboard = globals_value.get("keyboard_action_flags")
        auxiliary = globals_value.get("input_action_flags")
        if isinstance(keyboard, int) and isinstance(auxiliary, int):
            return (keyboard | auxiliary) & 0xFFFF
        if isinstance(auxiliary, int):
            return auxiliary
        if isinstance(keyboard, int):
            return keyboard
    return None


def compare_player(original: Path, candidate: Path,
                   require_complete: bool = False,
                   required_fields: Iterable[str] | None = None
                   ) -> list[dict[str, Any]]:
    try:
        left = load_trace(original).by_sequence
        right = load_trace(candidate).by_sequence
    except TraceError as exc:
        raise ParityError(str(exc)) from exc
    required = set(required_fields or ())
    if require_complete:
        required.update(PLAYER_PARITY_FIELDS)
    unknown_required = required.difference(PLAYER_PARITY_FIELDS)
    if unknown_required:
        raise ParityError(
            "unknown player parity field(s): "
            + ", ".join(sorted(unknown_required))
        )
    mismatches: list[dict[str, Any]] = []
    for sequence in sorted(set(left) | set(right)):
        if sequence not in left or sequence not in right:
            mismatches.append({"sequence": sequence, "field": "sample",
                               "original": sequence in left,
                               "candidate": sequence in right})
            continue
        a, b = left[sequence], right[sequence]
        for which in ("pre", "post"):
            try:
                av = validate_record(_record_hex(a, which),
                                     f"original sample {sequence} {which}")
                bv = validate_record(_record_hex(b, which),
                                     f"candidate sample {sequence} {which}")
            except ParityError as exc:
                mismatches.append({"sequence": sequence,
                                   "field": f"{which}_record",
                                   "error": str(exc)})
                continue
            if av != bv:
                mismatches.append({"sequence": sequence,
                                   "field": f"{which}_record",
                                   "original": av, "candidate": bv})
        fields = (("input_flags", canonical_input(a), canonical_input(b)),
                  ("probes", canonical_probes(a), canonical_probes(b)),
                  ("global_writes", canonical_globals(a), canonical_globals(b)),
                  ("factory_objects", canonical_factory(a), canonical_factory(b)),
                  ("effects", canonical_effects(a), canonical_effects(b)))
        for field, av, bv in fields:
            if field not in required:
                if av is None:
                    av = []
                if bv is None:
                    bv = []
            if av is None or bv is None:
                mismatches.append({"sequence": sequence, "field": field,
                                   "error": "missing required parity data"})
            elif av != bv:
                mismatches.append({"sequence": sequence, "field": field,
                                   "original": av, "candidate": bv})
    return mismatches


def _session_record(sample: NormalizedSample, which: str) -> str | None:
    return sample.pre_record_hex if which == "pre" else sample.post_record_hex


def _session_input(sample: NormalizedSample) -> int | None:
    return sample.input_flags


def _session_camera(sample: NormalizedSample) -> tuple[int, int] | None:
    return sample.camera


def _session_probes(sample: NormalizedSample) -> list[Any] | None:
    raw = sample.raw
    callback = sample.callback
    values = callback.get("collisions") if callback else None
    if values is None:
        values = raw.get("collisions")
    if values is None:
        values = raw.get("collision_probes")
    if not isinstance(values, list):
        values = raw.get("map_properties")
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


def _global_writes(sample: NormalizedSample) -> list[Any] | None:
    raw = sample.raw
    callback = sample.callback
    value = callback.get("global_writes") if callback else None
    if value is None:
        value = raw.get("global_writes")
    if not isinstance(value, list):
        return None
    names = {0x4FEE: "horizontal_timer", 0x4FF8: "dispatch_aux_4ff8"}
    result = []
    for item in value:
        if not isinstance(item, dict):
            result.append(item)
            continue
        address = item.get("offset")
        if isinstance(address, int) and address in names:
            result.append({"after": item.get("after"),
                           "before": item.get("before"),
                           "field": names[address]})
        else:
            result.append(item)
    return result


def _effects(sample: NormalizedSample) -> list[Any] | None:
    raw = sample.raw
    callback = sample.callback
    value = callback.get("effects") if callback else None
    if value is None:
        value = raw.get("effects")
    return value if isinstance(value, list) else None


def callback_offsets(items: Any) -> list[int]:
    if not isinstance(items, list):
        return []
    result = []
    for item in items:
        if not isinstance(item, dict):
            result.append(-1)
            continue
        callback_value = item.get("phase_callback_offset")
        if callback_value is None:
            callback_value = item.get("callback")
        if isinstance(callback_value, dict):
            value = callback_value.get("offset")
        else:
            value = callback_value
        result.append(value if isinstance(value, int) else -1)
    return result


def scheduler_offsets(sample: NormalizedSample) -> list[int] | None:
    raw = sample.raw
    value = raw.get("scheduler_callbacks")
    if isinstance(value, list):
        return callback_offsets(value)
    scheduler = raw.get("scheduler")
    pool = raw.get("pool")
    if isinstance(pool, dict) and isinstance(pool.get("objects"), list):
        result = []
        for item in sorted(
                (item for item in pool["objects"] if isinstance(item, dict)),
                key=lambda item: item.get("index", 0)):
            value = item.get("callback")
            if value in (None, 0, 0xFFFF, 16376):
                continue
            result.append(value)
        if result:
            return result
    if isinstance(scheduler, dict) and isinstance(scheduler.get("entries"), list):
        return callback_offsets(scheduler["entries"])
    return None


def active_objects(sample: NormalizedSample) -> list[dict[str, Any]] | None:
    raw = sample.raw
    if isinstance(raw.get("entities"), list):
        result = []
        for item in raw["entities"]:
            if not isinstance(item, dict):
                result.append({"value": item})
                continue
            callback_value = item.get("callback")
            offset = (callback_value.get("offset")
                      if isinstance(callback_value, dict) else callback_value)
            if offset in (None, 0, 16376, 0xFFFF):
                continue
            record = {"callback": offset, "x": item.get("x"),
                      "y": item.get("y"), "sprite_slot": item.get("sprite_slot")}
            if offset == 0x47E7:
                for source, target in (
                        ("ambient_velocity_y_fixed", "velocity_y_fixed"),
                        ("ambient_animation_delay", "animation_delay"),
                        ("ambient_animation_cursor", "animation_cursor")):
                    if source in item:
                        record[target] = item[source]
            result.append(record)
        return sorted(result, key=repr)
    pool = raw.get("pool")
    if not isinstance(pool, dict) or not isinstance(pool.get("objects"), list):
        return None
    result = []
    for item in pool["objects"]:
        if not isinstance(item, dict):
            result.append({"value": item})
            continue
        callback_value = item.get("callback")
        if callback_value == 16376:
            continue
        position = item.get("position")
        record = {"callback": callback_value,
                  "x": position.get("x") if isinstance(position, dict) else None,
                  "y": position.get("y") if isinstance(position, dict) else None,
                  "sprite_slot": item.get("sprite_slot")}
        if callback_value == 0x47E7:
            for key in ("velocity_y_fixed", "animation_delay", "animation_cursor"):
                if key in item:
                    record[key] = item[key]
        result.append(record)
    return sorted(result, key=repr)


def compare_active_objects(expected: list[dict[str, Any]],
                           actual: list[dict[str, Any]],
                           sequence: int) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    mismatches: list[dict[str, Any]] = []
    coverage: list[dict[str, Any]] = []
    if len(expected) != len(actual):
        return ([{"sequence": sequence, "field": "active_objects",
                  "original": expected, "candidate": actual}], coverage)
    identity = ("callback", "x", "y", "sprite_slot")
    for expected_item, actual_item in zip(expected, actual):
        if tuple(expected_item.get(key) for key in identity) != tuple(
                actual_item.get(key) for key in identity):
            mismatches.append({"sequence": sequence, "field": "active_objects",
                               "original": expected, "candidate": actual})
            return mismatches, coverage
        for key in sorted(set(expected_item) | set(actual_item)):
            if key in identity or key == "value":
                continue
            expected_present = key in expected_item
            actual_present = key in actual_item
            if not expected_present or not actual_present:
                coverage.append({"sequence": sequence,
                                 "field": f"active_objects.{key}",
                                 "original_present": expected_present,
                                 "candidate_present": actual_present})
            elif expected_item[key] != actual_item[key]:
                mismatches.append({"sequence": sequence,
                                   "field": f"active_objects.{key}",
                                   "original": expected_item[key],
                                   "candidate": actual_item[key]})
    return mismatches, coverage


def compare_session(original: Path, candidate: Path,
                    require_complete: bool = False) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    try:
        left = load_trace(original).by_sequence
        right = load_trace(candidate).by_sequence
    except TraceError as exc:
        raise SessionTraceError(str(exc)) from exc
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
                expected_record = validate_record(
                    _session_record(expected, which),
                    f"original sample {sequence} {which}",
                    error_type=SessionTraceError,
                )
                actual_record = validate_record(
                    _session_record(actual, which),
                    f"candidate sample {sequence} {which}",
                    error_type=SessionTraceError,
                )
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

        fields = (("input_flags", _session_input(expected), _session_input(actual)),
                  ("camera", _session_camera(expected), _session_camera(actual)),
                  ("scheduler_callbacks", scheduler_offsets(expected),
                   scheduler_offsets(actual)))
        for field, expected_value, actual_value in fields:
            if expected_value is None or actual_value is None:
                coverage.append({"sequence": sequence, "field": field,
                                 "original_present": expected_value is not None,
                                 "candidate_present": actual_value is not None})
            elif expected_value != actual_value:
                mismatches.append({"sequence": sequence, "field": field,
                                   "original": expected_value,
                                   "candidate": actual_value})

        expected_objects = active_objects(expected)
        actual_objects = active_objects(actual)
        if expected_objects is None or actual_objects is None:
            coverage.append({"sequence": sequence, "field": "active_objects",
                             "original_present": expected_objects is not None,
                             "candidate_present": actual_objects is not None})
        else:
            object_mismatches, object_coverage = compare_active_objects(
                expected_objects, actual_objects, sequence)
            mismatches.extend(object_mismatches)
            coverage.extend(object_coverage)

        for field, normalizer in (("probes", _session_probes),
                                  ("global_writes", _global_writes),
                                  ("effects", _effects)):
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
    if require_complete and not mismatches and coverage:
        # Keep the existing return contract.  The CLI decides whether a
        # coverage-only result is an incomplete acceptance; callers can still
        # inspect both lists without a second mode-specific exception.
        pass
    return mismatches, coverage


# These are deliberately named lifecycle barriers, rather than guessed frame
# numbers.  DOS captures commonly sample only every N callbacks while the
# native frontend can publish every input row; checkpoint comparison therefore
# compares the first sample satisfying each barrier independently.
SESSION_CHECKPOINTS = (
    "terminal_damage",
    "death_hold",
    "recovery_gate",
    "recovered_callback",
    "respawn",
)


def _checkpoint_record(sample: NormalizedSample) -> str | None:
    """Return the post-callback record across DOS and native envelopes."""

    value = _record_hex(sample, "post")
    if value is not None:
        return value
    raw = sample.raw
    value = raw.get("player_record_hex")
    return value if isinstance(value, str) else None


def _checkpoint_maps(sample: NormalizedSample) -> list[dict[str, Any]]:
    raw = sample.raw
    callback = sample.callback
    values: list[dict[str, Any]] = []
    for source in (
        raw.get("globals"),
        raw.get("gameplay_state"),
        raw.get("global_state"),
        callback.get("post_globals") if callback else None,
        callback.get("pre_globals") if callback else None,
    ):
        if isinstance(source, dict):
            values.append(source)
    return values


def _checkpoint_value(sample: NormalizedSample,
                      *keys: str) -> Any:
    for source in _checkpoint_maps(sample):
        for key in keys:
            if key in source and isinstance(source[key], (int, str, bool)):
                return source[key]
    return None


def _checkpoint_mode(sample: NormalizedSample) -> int | None:
    callback = sample.callback
    if callback is not None:
        for name in ("post_object", "object"):
            obj = callback.get(name)
            if isinstance(obj, dict):
                for key in ("player_byte_0x37", "mode"):
                    value = obj.get(key)
                    if isinstance(value, int):
                        return value & 0xff
    record = _checkpoint_record(sample)
    if record is not None:
        try:
            data = bytes.fromhex(record)
        except ValueError:
            return None
        if len(data) > 0x37:
            return data[0x37]
    value = _checkpoint_value(sample, "mode", "transition_mode")
    return value & 0xff if isinstance(value, int) else None


def _checkpoint_position(sample: NormalizedSample) -> tuple[int, int] | None:
    callback = sample.callback
    if callback is not None:
        for name in ("post_object", "object"):
            obj = callback.get(name)
            position = obj.get("position") if isinstance(obj, dict) else None
            if (isinstance(position, dict) and
                    isinstance(position.get("x"), int) and
                    isinstance(position.get("y"), int)):
                return position["x"], position["y"]
    record = _checkpoint_record(sample)
    if record is not None:
        try:
            data = bytes.fromhex(record)
        except ValueError:
            data = b""
        # PlayerRecord stores the signed 16.16 positions at +0x02/+0x06.
        if len(data) >= 0x0a:
            x_raw = int.from_bytes(data[0x02:0x06], "little", signed=False)
            y_raw = int.from_bytes(data[0x06:0x0a], "little", signed=False)
            x = x_raw - (1 << 32) if x_raw & (1 << 31) else x_raw
            y = y_raw - (1 << 32) if y_raw & (1 << 31) else y_raw
            return x >> 16, y >> 16
    raw = sample.raw
    position = raw.get("position")
    if (isinstance(position, dict) and isinstance(position.get("x"), int) and
            isinstance(position.get("y"), int)):
        return position["x"], position["y"]
    x = _checkpoint_value(sample, "terminal_x_8828", "respawn_x", "x")
    y = _checkpoint_value(sample, "terminal_y_882a", "respawn_y", "y")
    if isinstance(x, int) and isinstance(y, int):
        return x, y
    return None


def _signed_word(value: Any) -> int | None:
    if not isinstance(value, int):
        return None
    value &= 0xffff
    return value - 0x10000 if value & 0x8000 else value


def _checkpoint_state(sample: NormalizedSample) -> dict[str, Any]:
    health = _checkpoint_value(
        sample, "current_health_8822", "dispatch_health_8822", "health")
    lives = _checkpoint_value(
        sample, "lives_880a", "dispatch_lives_880a", "lives")
    gate = _checkpoint_value(
        sample, "transition_gate_89ea", "transition_mode",
        "player_control_word", "recovery_gate")
    return {
        "record": _checkpoint_record(sample),
        "health": health if isinstance(health, int) else None,
        "lives": lives if isinstance(lives, int) else None,
        "gate": _signed_word(gate),
        "mode": _checkpoint_mode(sample),
        "position": _checkpoint_position(sample),
    }


def _checkpoint_samples(path: Path) -> dict[str, tuple[NormalizedSample, dict[str, Any]]]:
    try:
        samples = list(load_trace(path).samples)
    except TraceError as exc:
        raise SessionTraceError(str(exc)) from exc
    result: dict[str, tuple[NormalizedSample, dict[str, Any]]] = {}
    previous_health: int | None = None
    terminal_seen = False
    pending_negative_gate: tuple[NormalizedSample, dict[str, Any]] | None = None
    for sample in samples:
        state = _checkpoint_state(sample)
        health = state["health"]
        mode = state["mode"]
        gate = state["gate"]
        if (not terminal_seen and isinstance(health, int) and health == 0 and
                isinstance(previous_health, int) and previous_health > 0):
            result["terminal_damage"] = (sample, state)
            terminal_seen = True
        if (terminal_seen and "death_hold" not in result and health == 0 and
                mode == 0xff):
            result["death_hold"] = (sample, state)
        if (gate is not None and gate <= -350):
            result.setdefault("recovery_gate", (sample, state))
            state["gate_crossed"] = True
        if isinstance(gate, int) and gate < 0:
            pending_negative_gate = (sample, state)
        if (isinstance(health, int) and health > 0 and gate == 0 and
                mode == 0 and pending_negative_gate is not None):
            # Sparse callback traces can step from (for example) -346 to the
            # first restored callback, skipping the literal -350 observation.
            # Keep the last negative gate as the measured threshold-crossing
            # sample; never synthesize a -350 value.
            if "recovery_gate" not in result:
                gate_sample, gate_state = pending_negative_gate
                gate_state["gate_crossed"] = True
                result["recovery_gate"] = (gate_sample, gate_state)
            if "recovered_callback" not in result:
                result["recovered_callback"] = (sample, state)
        if ("recovered_callback" in result and "respawn" not in result and
                result["recovered_callback"][0] is not sample and
                state["position"] is not None):
            result["respawn"] = (sample, state)
        if isinstance(health, int):
            previous_health = health
    return result


def _checkpoint_required(label: str) -> tuple[str, ...]:
    if label == "terminal_damage":
        return ("health",)
    if label == "death_hold":
        return ("health", "mode")
    if label == "recovery_gate":
        return ("gate",)
    if label == "recovered_callback":
        return ("health", "gate", "mode")
    if label == "respawn":
        return ("position",)
    return ()


def compare_session_checkpoints(
    original: Path, candidate: Path,
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    """Compare lifecycle barriers when trace sample cadence differs.

    This is intentionally fail-closed: a missing barrier or required field is
    a mismatch, while optional scheduler/object publications remain coverage
    items until both traces publish them at the same barrier.
    """

    left = _checkpoint_samples(original)
    right = _checkpoint_samples(candidate)
    mismatches: list[dict[str, Any]] = []
    coverage: list[dict[str, Any]] = []
    for label in SESSION_CHECKPOINTS:
        expected = left.get(label)
        actual = right.get(label)
        if expected is None or actual is None:
            mismatches.append({
                "checkpoint": label,
                "field": "checkpoint",
                "original_present": expected is not None,
                "candidate_present": actual is not None,
            })
            continue
        expected_sample, expected_state = expected
        actual_sample, actual_state = actual
        for field in _checkpoint_required(label):
            expected_value = expected_state.get(field)
            actual_value = actual_state.get(field)
            if expected_value is None or actual_value is None:
                mismatches.append({
                    "checkpoint": label,
                    "sequence": expected_sample.sequence,
                    "field": field,
                    "error": "missing required lifecycle data",
                    "original_present": expected_value is not None,
                    "candidate_present": actual_value is not None,
                })
            elif (label == "recovery_gate" and field == "gate"):
                expected_crossed = bool(expected_state.get("gate_crossed"))
                actual_crossed = bool(actual_state.get("gate_crossed"))
                if not expected_crossed or not actual_crossed:
                    mismatches.append({
                        "checkpoint": label,
                        "sequence": expected_sample.sequence,
                        "field": field,
                        "error": "recovery threshold was not crossed",
                        "original": expected_value,
                        "candidate": actual_value,
                    })
            elif expected_value != actual_value:
                mismatches.append({
                    "checkpoint": label,
                    "sequence": expected_sample.sequence,
                    "field": field,
                    "original": expected_value,
                    "candidate": actual_value,
                })
        for field in ("record", "lives"):
            expected_value = expected_state.get(field)
            actual_value = actual_state.get(field)
            if expected_value is None or actual_value is None:
                coverage.append({
                    "checkpoint": label,
                    "field": field,
                    "original_present": expected_value is not None,
                    "candidate_present": actual_value is not None,
                })
            elif expected_value != actual_value:
                mismatches.append({
                    "checkpoint": label,
                    "sequence": expected_sample.sequence,
                    "field": field,
                    "original": expected_value,
                    "candidate": actual_value,
                })
        for field, getter in (("scheduler_callbacks", scheduler_offsets),
                              ("active_objects", active_objects)):
            expected_value = getter(expected_sample)
            actual_value = getter(actual_sample)
            if expected_value is None or actual_value is None:
                coverage.append({
                    "checkpoint": label,
                    "field": field,
                    "original_present": expected_value is not None,
                    "candidate_present": actual_value is not None,
                })
            elif expected_value != actual_value:
                mismatches.append({
                    "checkpoint": label,
                    "sequence": expected_sample.sequence,
                    "field": field,
                    "original": expected_value,
                    "candidate": actual_value,
                })
    return mismatches, coverage


def player_payload(path: Path) -> dict[str, Any]:
    """Compatibility helper returning the original JSON object."""

    return load_trace(path).payload


def session_samples(payload: dict[str, Any]) -> list[dict[str, Any]]:
    """Compatibility helper for scripts that already have decoded JSON."""

    try:
        return extract_samples(payload)[0]
    except TraceError as exc:
        raise SessionTraceError(str(exc)) from exc
