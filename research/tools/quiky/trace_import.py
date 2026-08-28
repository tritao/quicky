"""Strict conversion from the current DOS parity capture schema."""

from __future__ import annotations

from pathlib import Path
from typing import Any

from .common import ToolError, read_json

CAPTURE_SCHEMA = "quiky-player-dos-parity-v1"

GLOBAL_WRITES = {
    "dispatch_previous_word_60da": (0x60DA, 2), "dispatch_aux_4ff2": (0x4FF2, 4),
    "dispatch_aux_4ff8": (0x4FF8, 2), "dispatch_aux_4ffa": (0x4FFA, 2),
    "horizontal_timer": (0x4FEE, 2), "horizontal_accumulator": (0x4FE2, 4),
    "horizontal_aux": (0x4FE8, 4), "horizontal_branch_counter": (0x4FEC, 2),
    "pending_event": (0x612E, 2), "camera_x": (0x81C0, 2),
    "camera_y": (0x81C4, 2), "player_vertical_adjust": (0x8812, 4),
    "horizontal_result_byte": (0x4FF0, 1),
    "timer_clear": (0x8810, 2),
}

# The diagnostic callback snapshot also observes the keyboard latch. It is
# input plumbing (and is already represented by input_flags), not a native
# replay-side state write. Keep it in raw capture evidence, but do not turn it
# into a canonical write that native replay cannot publish.
OBSERVATION_ONLY_WRITES = {"keyboard_action_flags"}


def load_capture(path: Path) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    payload = read_json(path)
    if not isinstance(payload, dict) or payload.get("schema") != CAPTURE_SCHEMA:
        raise ToolError(f"{path}: capture must use {CAPTURE_SCHEMA}")
    unknown = set(payload).difference({"schema", "source_trace", "trace_kind", "events"})
    if unknown:
        raise ToolError(f"{path}: unknown capture fields: {', '.join(sorted(unknown))}")
    events = payload.get("events")
    if not isinstance(events, list) or len(events) != 1 or not isinstance(events[0], dict):
        raise ToolError(f"{path}: events must contain exactly one object")
    event = events[0]
    unknown = set(event).difference({"samples", "input_stream"})
    if unknown:
        raise ToolError(f"{path}: unknown event fields: {', '.join(sorted(unknown))}")
    samples = event.get("samples")
    if not isinstance(samples, list) or not samples or any(
            not isinstance(sample, dict) for sample in samples):
        raise ToolError(f"{path}: samples must be a non-empty array of objects")
    previous = 0
    for sample in samples:
        sequence = sample.get("sequence")
        if not isinstance(sequence, int) or sequence <= previous:
            raise ToolError(f"{path}: sample sequences must be strictly increasing")
        if not isinstance(sample.get("globals"), dict):
            raise ToolError(f"{path}: sample {sequence} requires globals")
        if not isinstance(sample.get("player_callback"), dict):
            raise ToolError(f"{path}: sample {sequence} requires player_callback")
        previous = sequence
    return payload, samples


def records(sample: dict[str, Any]) -> tuple[str, str]:
    callback = sample["player_callback"]
    try:
        pre = callback["pre_object"]["state_hex"]
        post = callback["post_object"]["state_hex"]
    except (KeyError, TypeError) as exc:
        raise ToolError(f"sample {sample['sequence']}: callback records are missing") from exc
    if not isinstance(pre, str) or not isinstance(post, str):
        raise ToolError(f"sample {sample['sequence']}: callback records must be hex strings")
    return pre, post


def input_flags(sample: dict[str, Any]) -> int:
    globals_ = sample["globals"]
    keyboard, auxiliary = (globals_.get("keyboard_action_flags"),
                           globals_.get("input_action_flags"))
    if not isinstance(keyboard, int) or not isinstance(auxiliary, int):
        raise ToolError(f"sample {sample['sequence']}: input action flags are missing")
    return (keyboard | auxiliary) & 0xFFFF


def camera(sample: dict[str, Any]) -> tuple[int, int]:
    globals_ = sample["globals"]
    x, y = globals_.get("camera_x"), globals_.get("camera_y")
    if not isinstance(x, int) or not isinstance(y, int):
        raise ToolError(f"sample {sample['sequence']}: camera is missing")
    return x, y


def probes(sample: dict[str, Any]) -> list[dict[str, Any]] | None:
    events = sample.get("map_properties")
    if events is None:
        return None
    if not isinstance(events, list):
        raise ToolError(f"sample {sample['sequence']}: map_properties must be an array")
    result = []
    for event in events:
        if not isinstance(event, dict):
            raise ToolError(f"sample {sample['sequence']}: invalid map property")
        if event.get("scope") == "outside_player_callback":
            continue
        lookup = event.get("map_lookup")
        if not isinstance(lookup, dict):
            raise ToolError(f"sample {sample['sequence']}: map property requires map_lookup")
        x, y, descriptor = lookup.get("x"), lookup.get("y"), event.get("descriptor_word")
        if not isinstance(x, int) or not isinstance(y, int) or not isinstance(descriptor, int):
            raise ToolError(f"sample {sample['sequence']}: map property is incomplete")
        quadrant = (0x02 if x & 8 else 0x01) if y & 8 else (0x04 if x & 8 else 0x08)
        occupied = (event.get("raw_map_bit_set")
                    if event.get("helper_offset") in (0x1C6E, 0x1C92)
                    else bool((descriptor & 0x000F) and (descriptor & quadrant)))
        result.append({"x": x, "y": y, "map_word": lookup.get("cell_word"),
                       "tile_id": lookup.get("tile_id"),
                       "descriptor_word": descriptor, "quadrant_mask": quadrant,
                       "occupied": occupied})
    return result


def global_writes(sample: dict[str, Any]) -> list[dict[str, Any]] | None:
    values = sample["player_callback"].get("global_writes")
    if values is None:
        return None
    if not isinstance(values, list) or any(not isinstance(item, dict) for item in values):
        raise ToolError(f"sample {sample['sequence']}: global_writes must be objects")
    result = []
    for item in values:
        normalized = dict(item)
        field = normalized.pop("field", None)
        if field in OBSERVATION_ONLY_WRITES:
            continue
        if field not in GLOBAL_WRITES:
            raise ToolError(f"sample {sample['sequence']}: unknown global write field {field!r}")
        normalized["offset"], normalized["width"] = GLOBAL_WRITES[field]
        result.append(normalized)
    return result


def effects(sample: dict[str, Any]) -> list[Any] | None:
    value = sample["player_callback"].get("effects")
    if value is not None and not isinstance(value, list):
        raise ToolError(f"sample {sample['sequence']}: effects must be an array")
    return value


def scheduler_callbacks(sample: dict[str, Any]) -> list[int] | None:
    values = sample.get("scheduler_callbacks")
    if values is None:
        return None
    try:
        return [item["callback"]["offset"] for item in values]
    except (KeyError, TypeError) as exc:
        raise ToolError(f"sample {sample['sequence']}: scheduler callback is invalid") from exc


def active_objects(sample: dict[str, Any]) -> list[dict[str, Any]] | None:
    values = sample.get("entities")
    if values is None:
        return None
    if not isinstance(values, list):
        raise ToolError(f"sample {sample['sequence']}: entities must be an array")
    result = []
    for item in values:
        if not isinstance(item, dict) or not isinstance(item.get("callback"), dict):
            raise ToolError(f"sample {sample['sequence']}: entity is invalid")
        offset = item["callback"].get("offset")
        if offset in (None, 0, 0x3FF8, 0xFFFF):
            continue
        result.append({"callback": offset, "x": item.get("x"), "y": item.get("y"),
                       "sprite_slot": item.get("sprite_slot")})
    return sorted(result, key=lambda item: (item["callback"], item["x"], item["y"]))


def lifecycle(sample: dict[str, Any], post_record: str) -> dict[str, Any]:
    globals_ = sample["globals"]
    data = bytes.fromhex(post_record)
    position = ((int.from_bytes(data[2:6], "little", signed=True) >> 16,
                 int.from_bytes(data[6:10], "little", signed=True) >> 16)
                if len(data) >= 10 else None)
    gate = globals_.get("transition_gate_89ea")
    if isinstance(gate, int):
        gate = (gate & 0xFFFF) - (0x10000 if gate & 0x8000 else 0)
    return {"health": globals_.get("current_health_8822"),
            "lives": globals_.get("lives_880a"), "gate": gate,
            "mode": data[0x37] if len(data) > 0x37 else None,
            "position": position}
