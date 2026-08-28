"""Historical trace decoding for the recorded-run import boundary.

Nothing in replay or verification imports this module. It translates the DOS
capture generations and the pre-canonical native session envelope into one
stable field vocabulary before the result is written as parity-state JSONL.
"""

from __future__ import annotations

from typing import Any

from .trace import NormalizedSample, TraceError, load_trace


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


def _ordered_array(value: Any) -> list[Any] | None:
    if isinstance(value, list):
        return value
    if not isinstance(value, dict):
        return None
    indexed: list[tuple[int, Any]] = []
    for key, item in value.items():
        try:
            indexed.append((int(key), item))
        except (TypeError, ValueError):
            return None
    return [item for _, item in sorted(indexed)]


def probes(sample: NormalizedSample) -> list[Any] | None:
    raw, callback = sample.raw, sample.callback
    property_events = raw.get("map_properties")
    if isinstance(property_events, list):
        result: list[Any] = []
        for event in property_events:
            if (isinstance(event, dict) and
                    event.get("scope") == "outside_player_callback"):
                continue
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
                quadrant = ((0x02 if x & 8 else 0x01) if y & 8 else
                            (0x04 if x & 8 else 0x08))
            else:
                quadrant = event.get("quadrant_flag_mask")
            helper = event.get("helper_offset")
            if helper in (0x1C6E, 0x1C92):
                occupied = event.get("raw_map_bit_set")
            elif isinstance(descriptor, int) and isinstance(quadrant, int):
                occupied = bool((descriptor & 0x000F) and (descriptor & quadrant))
            else:
                occupied = event.get("descriptor_flag_set")
            result.append({"x": x, "y": y,
                           "map_word": lookup.get("map_word", lookup.get(
                               "cell_word", lookup.get("raw_cell_word"))),
                           "tile_id": lookup.get("tile_id"),
                           "descriptor_word": descriptor,
                           "quadrant_mask": quadrant, "occupied": occupied})
        return result
    values = callback.get("collisions") if callback else None
    if values is None:
        values = raw.get("collisions", raw.get("collision_probes"))
    if not isinstance(values, list):
        return None
    result = []
    for value in values:
        if not isinstance(value, dict):
            result.append(value)
            continue
        map_property = value.get("map_property")
        lookup = map_property.get("map_lookup") if isinstance(map_property, dict) else None
        source = lookup if isinstance(lookup, dict) else value
        result.append({
            "x": source.get("x"), "y": source.get("y"),
            "map_word": source.get("map_word", source.get(
                "cell_word", source.get("raw_cell_word"))),
            "tile_id": source.get("tile_id"),
            "descriptor_word": (map_property.get("descriptor_word")
                                if isinstance(map_property, dict)
                                else value.get("descriptor_word")),
            "quadrant_mask": value.get("quadrant_mask"),
            "occupied": value.get("occupied"),
        })
    return result


def global_writes(sample: NormalizedSample) -> list[Any] | None:
    value = sample.callback.get("global_writes") if sample.callback else None
    if value is None:
        value = sample.raw.get("global_writes")
    if not isinstance(value, list):
        return None
    result = []
    for item in value:
        if not isinstance(item, dict):
            result.append(item)
            continue
        normalized = dict(item)
        field = normalized.pop("field", None)
        if field in GLOBAL_FIELD_MAP and "offset" not in normalized:
            normalized["offset"], normalized["width"] = GLOBAL_FIELD_MAP[field]
        result.append(normalized)
    return result


def effects(sample: NormalizedSample) -> list[Any] | None:
    for source in (sample.callback, sample.raw):
        if source is None:
            continue
        for key in ("effects", "effect_dispatches"):
            value = _ordered_array(source.get(key))
            if value is not None:
                return value
    return None


def factory_objects(sample: NormalizedSample) -> list[Any] | None:
    event = sample.raw.get("factory_event")
    if not isinstance(event, dict) and sample.callback:
        event = sample.callback.get("factory_event")
    if not isinstance(event, dict):
        return None
    created = _ordered_array(event.get("created_objects"))
    if created is None:
        return None
    return [{key: obj.get(key) for key in (
        "offset", "callback", "kind", "phase", "sprite_slot", "position")}
        if isinstance(obj, dict) else obj for obj in created]


def _callback_offsets(items: Any) -> list[int]:
    if not isinstance(items, list):
        return []
    result = []
    for item in items:
        if not isinstance(item, dict):
            result.append(-1)
            continue
        value = item.get("phase_callback_offset", item.get("callback"))
        if isinstance(value, dict):
            value = value.get("offset")
        result.append(value if isinstance(value, int) else -1)
    return result


def scheduler_callbacks(sample: NormalizedSample) -> list[int] | None:
    raw = sample.raw
    if isinstance(raw.get("scheduler_callbacks"), list):
        return _callback_offsets(raw["scheduler_callbacks"])
    pool = raw.get("pool")
    if isinstance(pool, dict) and isinstance(pool.get("objects"), list):
        result = []
        for item in sorted((item for item in pool["objects"]
                            if isinstance(item, dict)),
                           key=lambda item: item.get("index", 0)):
            value = item.get("callback")
            if value not in (None, 0, 0xFFFF, 16376):
                result.append(value)
        if result:
            return result
    scheduler = raw.get("scheduler")
    if isinstance(scheduler, dict) and isinstance(scheduler.get("entries"), list):
        return _callback_offsets(scheduler["entries"])
    return None


def active_objects(sample: NormalizedSample) -> list[dict[str, Any]] | None:
    raw = sample.raw
    if isinstance(raw.get("entities"), list):
        result = []
        for item in raw["entities"]:
            if not isinstance(item, dict):
                result.append({"value": item})
                continue
            callback = item.get("callback")
            offset = callback.get("offset") if isinstance(callback, dict) else callback
            if offset in (None, 0, 16376, 0xFFFF):
                continue
            record = {"callback": offset, "x": item.get("x"),
                      "y": item.get("y"), "sprite_slot": item.get("sprite_slot")}
            if offset == 0x47E7:
                for source, target in (("ambient_velocity_y_fixed", "velocity_y_fixed"),
                                       ("ambient_animation_delay", "animation_delay"),
                                       ("ambient_animation_cursor", "animation_cursor")):
                    if source in item:
                        record[target] = item[source]
            result.append(record)
        return sorted(result, key=lambda item: (
            item.get("callback", -1), item.get("x", -1),
            item.get("y", -1), item.get("sprite_slot", -1)))
    pool = raw.get("pool")
    if not isinstance(pool, dict) or not isinstance(pool.get("objects"), list):
        return None
    result = []
    for item in pool["objects"]:
        if not isinstance(item, dict):
            result.append({"value": item})
            continue
        callback = item.get("callback")
        if callback == 16376:
            continue
        position = item.get("position")
        record = {"callback": callback,
                  "x": position.get("x") if isinstance(position, dict) else None,
                  "y": position.get("y") if isinstance(position, dict) else None,
                  "sprite_slot": item.get("sprite_slot")}
        if callback == 0x47E7:
            for key in ("velocity_y_fixed", "animation_delay", "animation_cursor"):
                if key in item:
                    record[key] = item[key]
        result.append(record)
    return sorted(result, key=lambda item: (
        item.get("callback", -1), item.get("x", -1),
        item.get("y", -1), item.get("sprite_slot", -1)))


def _record(sample: NormalizedSample) -> str | None:
    if sample.post_record_hex is not None:
        return sample.post_record_hex
    value = sample.raw.get("player_record_hex")
    return value if isinstance(value, str) else None


def _maps(sample: NormalizedSample) -> list[dict[str, Any]]:
    callback = sample.callback
    return [source for source in (
        sample.raw.get("globals"), sample.raw.get("gameplay_state"),
        sample.raw.get("global_state"),
        callback.get("post_globals") if callback else None,
        callback.get("pre_globals") if callback else None,
    ) if isinstance(source, dict)]


def _value(sample: NormalizedSample, *keys: str) -> Any:
    for source in _maps(sample):
        for key in keys:
            if key in source and isinstance(source[key], (int, str, bool)):
                return source[key]
    return None


def _mode(sample: NormalizedSample) -> int | None:
    if sample.callback:
        for name in ("post_object", "object"):
            obj = sample.callback.get(name)
            if isinstance(obj, dict):
                value = obj.get("player_byte_0x37", obj.get("mode"))
                if isinstance(value, int):
                    return value & 0xff
    record = _record(sample)
    if record is not None:
        try:
            data = bytes.fromhex(record)
        except ValueError:
            data = b""
        if len(data) > 0x37:
            return data[0x37]
    value = _value(sample, "mode", "transition_mode")
    return value & 0xff if isinstance(value, int) else None


def _position(sample: NormalizedSample) -> tuple[int, int] | None:
    if sample.callback:
        for name in ("post_object", "object"):
            obj = sample.callback.get(name)
            position = obj.get("position") if isinstance(obj, dict) else None
            if (isinstance(position, dict) and isinstance(position.get("x"), int)
                    and isinstance(position.get("y"), int)):
                return position["x"], position["y"]
    record = _record(sample)
    if record is not None:
        try:
            data = bytes.fromhex(record)
        except ValueError:
            data = b""
        if len(data) >= 0x0A:
            x = int.from_bytes(data[2:6], "little", signed=True)
            y = int.from_bytes(data[6:10], "little", signed=True)
            return x >> 16, y >> 16
    position = sample.raw.get("position")
    if isinstance(position, dict) and isinstance(position.get("x"), int) and isinstance(position.get("y"), int):
        return position["x"], position["y"]
    x, y = _value(sample, "terminal_x_8828", "respawn_x", "x"), _value(
        sample, "terminal_y_882a", "respawn_y", "y")
    return (x, y) if isinstance(x, int) and isinstance(y, int) else None


def lifecycle(sample: NormalizedSample) -> dict[str, Any]:
    health = _value(sample, "current_health_8822", "dispatch_health_8822", "health")
    lives = _value(sample, "lives_880a", "dispatch_lives_880a", "lives")
    gate = _value(sample, "transition_gate_89ea", "transition_mode",
                  "player_control_word", "recovery_gate")
    if isinstance(gate, int):
        gate &= 0xffff
        gate = gate - 0x10000 if gate & 0x8000 else gate
    return {"health": health if isinstance(health, int) else None,
            "lives": lives if isinstance(lives, int) else None,
            "gate": gate if isinstance(gate, int) else None,
            "mode": _mode(sample), "position": _position(sample)}
