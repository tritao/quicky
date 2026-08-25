#!/usr/bin/env python3
"""Run declarative player-trace experiments and compare their timelines.

This module is deliberately a layer above :mod:`quikytrace`.  The existing
tracer remains the single implementation of the DOSBox/debugger interaction;
this file only supplies experiment descriptions, artifact management,
normalization, assertions, and comparisons.
"""

from __future__ import annotations

import argparse
import copy
import json
import os
import re
import secrets
import subprocess
import sys
import time
from contextlib import contextmanager
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Iterator, Mapping, Sequence

from quikytrace import (
    ApiClient,
    ExecuteWatch,
    InputPhase,
    MemoryPatch,
    ObjectFocus,
    PlayerTraceConfig,
    TraceError,
    capture_failure,
    discover_token,
    normalize_player_trace,
    parse_execute_watch,
    parse_input_phase,
    parse_memory_patch,
    parse_object_focus,
    sha256,
    trace_player_lua,
)


MATRIX_SCHEMA_VERSION = 1
RAW_SCHEMA = "quiky-player-trace-matrix-raw-v1"
NORMALIZED_SCHEMA = "quiky-player-trace-matrix-normalized-v1"
COMPARISON_SCHEMA = "quiky-player-trace-matrix-comparison-v1"
MANIFEST_SCHEMA = "quiky-player-trace-matrix-manifest-v1"


class MatrixError(Exception):
    """Raised for invalid experiment files or matrix-runner failures."""


class AssertionFailure(MatrixError):
    """Raised when an experiment's expected output does not match its trace."""

    def __init__(self, failures: Sequence[Mapping[str, Any]]):
        self.failures = [dict(item) for item in failures]
        super().__init__("; ".join(str(item.get("message", item)) for item in self.failures))


def _integer(value: Any, field_name: str) -> int:
    if isinstance(value, bool):
        raise MatrixError(f"{field_name} must be an integer")
    try:
        return int(value, 0) if isinstance(value, str) else int(value)
    except (TypeError, ValueError) as exc:
        raise MatrixError(f"{field_name} must be an integer") from exc


def _positive(value: Any, field_name: str) -> int:
    result = _integer(value, field_name)
    if result < 1:
        raise MatrixError(f"{field_name} must be positive")
    return result


def _nonnegative(value: Any, field_name: str) -> int:
    result = _integer(value, field_name)
    if result < 0:
        raise MatrixError(f"{field_name} cannot be negative")
    return result


def _parse_input_phase_value(value: Any) -> InputPhase:
    if isinstance(value, str):
        try:
            return parse_input_phase(value)
        except argparse.ArgumentTypeError as exc:
            raise MatrixError(str(exc)) from exc
    if not isinstance(value, Mapping):
        raise MatrixError("input phase must be KEY:FRAMES or an object")
    keys = value.get("keys", ())
    if isinstance(keys, str):
        keys = () if keys.upper() == "WAIT" else tuple(keys.split("+"))
    try:
        parsed_keys = tuple(str(key) for key in keys)
    except TypeError as exc:
        raise MatrixError("input phase keys must be an array") from exc
    if len(parsed_keys) > 3 or any(not key for key in parsed_keys):
        raise MatrixError("input phase accepts at most three non-empty keys")
    return InputPhase(parsed_keys, _nonnegative(value.get("frames", 0), "phase frames"))


def _parse_execute_watch_value(value: Any) -> ExecuteWatch:
    if isinstance(value, str):
        try:
            return parse_execute_watch(value)
        except argparse.ArgumentTypeError as exc:
            raise MatrixError(str(exc)) from exc
    if not isinstance(value, Mapping):
        raise MatrixError("execute watch must be SEGMENT:OFFSET or an object")
    segment = _integer(value.get("segment"), "execute watch segment")
    offset = _integer(value.get("offset"), "execute watch offset")
    if not 0 <= segment <= 0xFFFF or not 0 <= offset <= 0xFFFFFFFF:
        raise MatrixError("execute watch address is out of range")
    return ExecuteWatch(segment, offset)


def _parse_patch_value(value: Any) -> MemoryPatch:
    if isinstance(value, str):
        try:
            return parse_memory_patch(value)
        except argparse.ArgumentTypeError as exc:
            raise MatrixError(str(exc)) from exc
    if not isinstance(value, Mapping):
        raise MatrixError("patch must be TARGET=VALUE or an object")
    space = str(value.get("space", ""))
    width_value = value.get("width", 1)
    widths = {"u8": 1, "u16": 2, "u32": 4}
    width_key = str(width_value).lower()
    width = widths[width_key] if width_key in widths else _integer(width_value, "patch width")
    if width not in widths.values():
        raise MatrixError("patch width must be 1, 2, 4, u8, u16, or u32")
    offset = _integer(value.get("offset", 0), "patch offset")
    patch_value = _integer(value.get("value"), "patch value")
    selector = value.get("selector")
    map_x = value.get("map_x", value.get("x"))
    map_y = value.get("map_y", value.get("y"))
    selector_int = None if selector is None else _integer(selector, "patch selector")
    map_x_int = None if map_x is None else _integer(map_x, "patch map x")
    map_y_int = None if map_y is None else _integer(map_y, "patch map y")
    if space not in ("ds", "player", "selector", "map"):
        raise MatrixError("patch space must be ds, player, selector, or map")
    if not 0 <= offset <= 0xFFFF or (selector_int is not None and not 0 <= selector_int <= 0xFFFF):
        raise MatrixError("patch offset or selector is out of range")
    if space == "map" and (map_x_int is None or map_y_int is None):
        raise MatrixError("MAP patches require map_x and map_y")
    if space == "map" and width != 2:
        raise MatrixError("MAP patches require width u16/2")
    if map_x_int is not None and map_x_int < 0 or map_y_int is not None and map_y_int < 0:
        raise MatrixError("patch map coordinates cannot be negative")
    if not 0 <= patch_value < 1 << (width * 8):
        raise MatrixError("patch value does not fit its width")
    return MemoryPatch(space, offset, width, patch_value, selector_int, map_x_int, map_y_int)


@dataclass(frozen=True)
class FocusSpec:
    """Declarative mapping to one of the stable tracer's focus modes."""

    kind: str = "player"
    callback_offset: int = 0x3FF8
    object_offset: int | None = None
    capture_record: bool = False

    VALID_KINDS = frozenset({
        "player", "callback", "object", "map", "collision", "property",
        "branch", "factory",
    })

    @classmethod
    def from_value(cls, value: Any) -> "FocusSpec":
        if value is None:
            return cls()
        if isinstance(value, ObjectFocus):
            return cls("object", value.callback_offset, value.object_offset, True)
        if isinstance(value, str):
            if value in cls.VALID_KINDS:
                return cls(kind=value)
            try:
                focus = parse_object_focus(value)
            except argparse.ArgumentTypeError as exc:
                raise MatrixError(str(exc)) from exc
            return cls("object", focus.callback_offset, focus.object_offset, True)
        if not isinstance(value, Mapping):
            raise MatrixError("focus must be player/callback/object or an object")
        kind = str(value.get("kind", value.get("type", "player"))).lower()
        nested = value.get("object_focus")
        if nested is not None:
            nested_focus = cls.from_value(nested)
            callback_offset = nested_focus.callback_offset
            object_offset = nested_focus.object_offset
            kind = "object"
        else:
            callback_offset = _integer(
                value.get("callback_offset", value.get("callback", 0x3FF8)),
                "focus callback offset",
            )
            object_value = value.get("object_offset", value.get("object"))
            object_offset = None if object_value is None else _integer(object_value, "focus object offset")
        if kind not in cls.VALID_KINDS:
            raise MatrixError(f"unsupported focus kind: {kind}")
        if not 0 <= callback_offset <= 0xFFFF:
            raise MatrixError("focus callback offset is out of range")
        if object_offset is not None and not 0 <= object_offset <= 0xFFFF:
            raise MatrixError("focus object offset is out of range")
        capture = bool(value.get("capture_record", value.get("capture_player_record", False)))
        if kind == "object":
            capture = True
        return cls(kind, callback_offset, object_offset, capture)

    def to_dict(self) -> dict[str, Any]:
        result: dict[str, Any] = {"kind": self.kind}
        if self.kind in ("callback", "object"):
            result["callback_offset"] = self.callback_offset
        if self.object_offset is not None:
            result["object_offset"] = self.object_offset
        if self.capture_record:
            result["capture_record"] = True
        return result


@dataclass(frozen=True)
class Experiment:
    """The compact, JSON-serializable description of one trace run."""

    name: str
    category: str
    level: str | None
    sample_count: int
    input_phases: tuple[InputPhase, ...] = ()
    execute_watches: tuple[ExecuteWatch, ...] = ()
    focus: FocusSpec = field(default_factory=FocusSpec)
    patches: tuple[MemoryPatch, ...] = ()
    expected_outputs: Mapping[str, Any] = field(default_factory=dict)
    frames_between: int = 30
    trace_options: Mapping[str, Any] = field(default_factory=dict)

    @classmethod
    def from_dict(cls, value: Mapping[str, Any]) -> "Experiment":
        if not isinstance(value, Mapping):
            raise MatrixError("experiment must be an object")
        name = str(value.get("name", "")).strip()
        if not name:
            raise MatrixError("experiment name is required")
        category = str(value.get("category", "uncategorized")).strip() or "uncategorized"
        level = value.get("level", value.get("select_level"))
        if level is not None:
            level = str(level).upper()
            if not re.fullmatch(r"W[1-5]L[1-4]", level):
                raise MatrixError(f"invalid level selector: {level}")
        sample_count = _positive(value.get("sample_count", value.get("samples", 0)), "sample_count")
        phases = tuple(_parse_input_phase_value(item) for item in value.get("input_phases", ()))
        watches = tuple(_parse_execute_watch_value(item) for item in value.get("execute_watches", ()))
        focus_value = value.get("focus", value.get("object_player_focus"))
        if focus_value is None and "object_focus" in value:
            focus_value = {"kind": "object", "object_focus": value["object_focus"]}
        focus = FocusSpec.from_value(focus_value)
        patches = tuple(_parse_patch_value(item) for item in value.get("patches", value.get("reversible_patches", ())))
        expected = value.get("expected_outputs", value.get("expected", {})) or {}
        if not isinstance(expected, Mapping):
            raise MatrixError("expected_outputs must be an object")
        frames_between = _nonnegative(value.get("frames_between", 30), "frames_between")
        options = value.get("trace_options", value.get("options", {})) or {}
        if not isinstance(options, Mapping):
            raise MatrixError("trace_options must be an object")
        return cls(
            name=name,
            category=category,
            level=level,
            sample_count=sample_count,
            input_phases=phases,
            execute_watches=watches,
            focus=focus,
            patches=patches,
            expected_outputs=dict(expected),
            frames_between=frames_between,
            trace_options=dict(options),
        )

    def to_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "category": self.category,
            "level": self.level,
            "sample_count": self.sample_count,
            "input_phases": [
                {"keys": list(phase.keys), "frames": phase.frames}
                for phase in self.input_phases
            ],
            "execute_watches": [
                {"segment": watch.segment, "offset": watch.offset}
                for watch in self.execute_watches
            ],
            "focus": self.focus.to_dict(),
            "patches": [
                {
                    "space": patch.space,
                    "selector": patch.selector,
                    "offset": patch.offset,
                    "width": patch.width,
                    "value": patch.value,
                    "map_x": patch.map_x,
                    "map_y": patch.map_y,
                }
                for patch in self.patches
            ],
            "expected_outputs": copy.deepcopy(dict(self.expected_outputs)),
            "frames_between": self.frames_between,
            "trace_options": copy.deepcopy(dict(self.trace_options)),
        }


@dataclass(frozen=True)
class ExperimentCatalog:
    experiments: Mapping[str, Experiment]
    suites: Mapping[str, tuple[str, ...]]

    def select(self, *, names: Sequence[str] = (), suite: str | None = None) -> list[Experiment]:
        selected: list[str] = []
        if suite is not None:
            if suite not in self.suites:
                raise MatrixError(f"unknown suite: {suite}")
            selected.extend(self.suites[suite])
        selected.extend(names)
        if not selected:
            raise MatrixError("select an experiment with --experiment or a suite with --suite")
        result: list[Experiment] = []
        for name in selected:
            if name not in self.experiments:
                raise MatrixError(f"unknown experiment: {name}")
            result.append(self.experiments[name])
        return result


def load_catalog(path: Path) -> ExperimentCatalog:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise MatrixError(f"cannot read experiment file {path}: {exc}") from exc
    if isinstance(payload, list):
        raw_experiments: Any = payload
        raw_suites: Any = {}
    elif isinstance(payload, Mapping):
        raw_experiments = payload.get("experiments", payload)
        raw_suites = payload.get("suites", {})
    else:
        raise MatrixError("experiment file must contain an object or array")
    if isinstance(raw_experiments, Mapping):
        raw_experiments = [dict(item, name=name) for name, item in raw_experiments.items()]
    if not isinstance(raw_experiments, list):
        raise MatrixError("experiments must be an array or object")
    experiments: dict[str, Experiment] = {}
    for item in raw_experiments:
        experiment = Experiment.from_dict(item)
        if experiment.name in experiments:
            raise MatrixError(f"duplicate experiment: {experiment.name}")
        experiments[experiment.name] = experiment
    suites: dict[str, tuple[str, ...]] = {}
    if isinstance(raw_suites, Mapping):
        suite_items = raw_suites.items()
    elif isinstance(raw_suites, list):
        suite_items = ((str(item["name"]), item.get("experiments", ())) for item in raw_suites)
    else:
        raise MatrixError("suites must be an object or array")
    for suite_name, members in suite_items:
        if not isinstance(members, Sequence) or isinstance(members, str):
            raise MatrixError(f"suite {suite_name} must contain experiment names")
        names = tuple(str(member) for member in members)
        missing = [name for name in names if name not in experiments]
        if missing:
            raise MatrixError(f"suite {suite_name} references unknown experiments: {', '.join(missing)}")
        suites[str(suite_name)] = names
    return ExperimentCatalog(experiments, suites)


def build_player_config(
    experiment: Experiment,
    startup_recording: Path,
    *,
    timeout: float = 30.0,
    poll_interval: float = 0.05,
) -> PlayerTraceConfig:
    """Translate one matrix experiment to the stable tracer's config object."""
    focus = experiment.focus
    options = dict(experiment.trace_options)
    callback_focus = focus.kind in ("callback", "object")
    object_focus = None
    if focus.kind == "object":
        object_focus = ObjectFocus(focus.callback_offset, focus.object_offset)
    option_values: dict[str, Any] = {
        "collision_event_limit": _positive(options.get("collision_event_limit", 96), "collision_event_limit"),
        "collision_repeat_limit": _positive(options.get("collision_repeat_limit", 3), "collision_repeat_limit"),
        "selector_frames": _positive(options.get("selector_frames", 60), "selector_frames"),
        "descriptor_count": _positive(options.get("descriptor_count", 512), "descriptor_count"),
        "map_width": _positive(options.get("map_width", 270), "map_width"),
        "map_height": _positive(options.get("map_height", 30), "map_height"),
    }
    allowed = {
        "property_helper_offset", "collision_patch_side", "branch_patch_tile",
        "collision_patch_tile", "branch_patch_flags", "probe_spawn_emitter",
        "probe_release_emitter", "transition_focus", "transition_steps",
        "transition_hold_events", "transition_force_player_fall",
        "transition_probe_frames", "transition_probe_tail_frames",
        "transition_probe_tail_camera_x", "transition_warmup_frames",
    }
    for key in allowed:
        if key in options:
            option_values[key] = options[key]
    for key in (
        "property_helper_offset", "branch_patch_tile", "collision_patch_tile",
        "branch_patch_flags", "transition_steps", "transition_hold_events",
        "transition_probe_frames", "transition_probe_tail_frames",
        "transition_probe_tail_camera_x", "transition_warmup_frames",
    ):
        if key in option_values and option_values[key] is not None:
            option_values[key] = _integer(option_values[key], key)
    if option_values.get("property_helper_offset") not in (None, 0x5C27, 0x5CC3):
        raise MatrixError("property_helper_offset must be 0x5c27 or 0x5cc3")
    for key, maximum in (("branch_patch_tile", 0x1FF), ("collision_patch_tile", 0x1FF), ("branch_patch_flags", 0xFFFF)):
        if key in option_values and option_values[key] is not None and not 0 <= option_values[key] <= maximum:
            raise MatrixError(f"{key} is out of range")
    if "collision_patch_side" in option_values and option_values["collision_patch_side"] not in ("left", "right", "both"):
        raise MatrixError("collision_patch_side must be left, right, or both")
    return PlayerTraceConfig(
        startup_recording=startup_recording,
        timeout=timeout,
        poll_interval=poll_interval,
        samples=experiment.sample_count,
        frames_between=experiment.frames_between,
        focus_callback=callback_focus,
        focus_callback_offset=focus.callback_offset,
        map_focus=focus.kind == "map",
        collision_focus=focus.kind == "collision",
        property_focus=focus.kind == "property",
        property_helper_offset=option_values.get("property_helper_offset"),
        branch_focus=focus.kind == "branch",
        branch_patch_tile=option_values.get("branch_patch_tile"),
        collision_patch_tile=option_values.get("collision_patch_tile"),
        collision_patch_side=option_values.get("collision_patch_side", "left"),
        branch_patch_flags=option_values.get("branch_patch_flags"),
        descriptor_census=bool(options.get("descriptor_census", False)),
        descriptor_count=option_values["descriptor_count"],
        map_width=option_values["map_width"],
        map_height=option_values["map_height"],
        probe_spawn_emitter=bool(option_values.get("probe_spawn_emitter", False)),
        probe_release_emitter=bool(option_values.get("probe_release_emitter", False)),
        capture_player_record=focus.capture_record or bool(options.get("capture_player_record", False)),
        collision_event_limit=option_values["collision_event_limit"],
        collision_repeat_limit=option_values["collision_repeat_limit"],
        transition_focus=bool(option_values.get("transition_focus", False)),
        transition_steps=_positive(option_values.get("transition_steps", 48), "transition_steps"),
        transition_hold_events=_nonnegative(option_values.get("transition_hold_events", 48), "transition_hold_events"),
        transition_force_player_fall=bool(option_values.get("transition_force_player_fall", False)),
        transition_probe_frames=_nonnegative(option_values.get("transition_probe_frames", 0), "transition_probe_frames"),
        transition_probe_tail_frames=_nonnegative(option_values.get("transition_probe_tail_frames", 0), "transition_probe_tail_frames"),
        transition_probe_tail_camera_x=_nonnegative(option_values.get("transition_probe_tail_camera_x", 0), "transition_probe_tail_camera_x"),
        transition_warmup_frames=_nonnegative(option_values.get("transition_warmup_frames", 0), "transition_warmup_frames"),
        select_level=experiment.level,
        selector_frames=option_values["selector_frames"],
        patches=experiment.patches,
        input_phases=experiment.input_phases,
        execute_watches=experiment.execute_watches,
        object_focus=object_focus,
        factory_focus=focus.kind == "factory",
    )


def _array(value: Any) -> list[Any]:
    if value is None:
        return []
    if isinstance(value, list):
        return value
    if isinstance(value, Mapping):
        try:
            return [value[key] for key in sorted(value, key=lambda item: int(item))]
        except (TypeError, ValueError) as exc:
            raise MatrixError("trace array has non-numeric object keys") from exc
    raise MatrixError("trace array is neither a list nor a numeric-key object")


def normalize_frame_timeline(raw_trace: Mapping[str, Any]) -> list[dict[str, Any]]:
    """Return deep-copied samples in deterministic frame/sequence order."""
    if not isinstance(raw_trace, Mapping):
        raise MatrixError("raw trace must be an object")
    trace = copy.deepcopy(dict(raw_trace))
    # Reuse the stable tracer's Lua-array normalization, but never hand it the
    # caller's object: it intentionally normalizes in place.
    normalized = normalize_player_trace(trace)
    samples = _array(normalized.get("samples", []))
    indexed: list[tuple[int, dict[str, Any]]] = []
    for original_index, sample in enumerate(samples):
        if not isinstance(sample, dict):
            raise MatrixError("trace sample is not an object")
        item = copy.deepcopy(sample)
        frame = item.get("frame_index", item.get("frame", original_index))
        sequence = item.get("sequence", original_index + 1)
        item["frame_index"] = _integer(frame, "sample frame_index")
        item["sequence"] = _integer(sequence, "sample sequence")
        indexed.append((original_index, item))
    indexed.sort(key=lambda item: (item[1]["frame_index"], item[1]["sequence"], item[0]))
    return [item for _, item in indexed]


def _copy_fields(value: Any, fields: Sequence[str]) -> dict[str, Any]:
    if not isinstance(value, Mapping):
        return {}
    return {field_name: copy.deepcopy(value[field_name]) for field_name in fields if field_name in value}


POSITION_FIELDS = ("x", "y", "x_fixed", "y_fixed", "x_fixed_signed", "y_fixed_signed")
VELOCITY_FIELDS = (
    "velocity_x_fixed", "velocity_y_fixed", "velocity_x_fixed_signed",
    "velocity_y_fixed_signed",
)
ACTION_FIELDS = (
    "action_word", "kind", "phase", "callback", "callback_data", "sprite_slot",
    "lifetime", "state_field", "state_field_signed", "vertical_step",
    "vertical_step_signed", "target_cursor", "update_state", "player_byte_0x36",
    "player_byte_0x37", "player_byte_0x38", "player_byte_0x39", "player_byte_0x3a",
    "player_byte_0x3b", "player_word_0x3e",
)


def _focused_objects(sample: Mapping[str, Any]) -> tuple[dict[str, Any] | None, dict[str, Any] | None]:
    callback = sample.get("player_callback")
    if isinstance(callback, Mapping):
        before = callback.get("pre_object", callback.get("object"))
        after = callback.get("post_object")
        return (
            copy.deepcopy(before) if isinstance(before, Mapping) else None,
            copy.deepcopy(after) if isinstance(after, Mapping) else None,
        )
    globals_value = sample.get("globals")
    target_offset = globals_value.get("player_object_offset") if isinstance(globals_value, Mapping) else None
    pool = sample.get("pool")
    objects = pool.get("objects", []) if isinstance(pool, Mapping) else []
    objects = _array(objects)
    selected = next((item for item in objects if isinstance(item, Mapping) and item.get("offset") == target_offset), None)
    if selected is None and len(objects) == 1 and isinstance(objects[0], Mapping):
        selected = objects[0]
    return (copy.deepcopy(selected) if isinstance(selected, Mapping) else None, None)


def _object_fields(value: Mapping[str, Any] | None) -> dict[str, Any]:
    if value is None:
        return {}
    return {
        "selector": value.get("selector"),
        "offset": value.get("offset"),
        "position": _copy_fields(value.get("position"), POSITION_FIELDS),
        "velocity": _copy_fields(value, VELOCITY_FIELDS),
        "action_state": _copy_fields(value, ACTION_FIELDS),
    }


def _map_fields(value: Any) -> dict[str, Any]:
    if not isinstance(value, Mapping):
        return {}
    fields = (
        "event_index", "frame_index", "helper_offset", "x", "y", "world_x", "world_y",
        "map_selector", "map_base", "row_stride", "cell_offset", "cell_word",
        "raw_cell_word", "tile_id", "property", "map_property_field", "descriptor",
        "descriptor_word", "descriptor_low_nibble", "descriptor_flag_set",
        "quadrant_flag_mask", "quadrant_bits", "descriptor_offset", "descriptor_tile_id",
    )
    result = _copy_fields(value, fields)
    if isinstance(value.get("map_lookup"), Mapping):
        result["map_lookup"] = _map_fields(value["map_lookup"])
    if isinstance(value.get("patch"), Mapping):
        result["patch"] = copy.deepcopy(dict(value["patch"]))
    return result


def _unique_dicts(items: Sequence[Mapping[str, Any]]) -> list[dict[str, Any]]:
    seen: set[str] = set()
    result: list[dict[str, Any]] = []
    for item in items:
        key = json.dumps(item, sort_keys=True, separators=(",", ":"), default=str)
        if key not in seen:
            seen.add(key)
            result.append(dict(item))
    return result


def extract_trace_fields(samples: Sequence[Mapping[str, Any]]) -> list[dict[str, Any]]:
    """Extract stable, comparison-friendly fields from normalized samples."""
    frames: list[dict[str, Any]] = []
    for sample in samples:
        before, after = _focused_objects(sample)
        callback = sample.get("player_callback")
        callback = callback if isinstance(callback, Mapping) else {}
        before_fields = _object_fields(before)
        after_fields = _object_fields(after)
        frame: dict[str, Any] = {
            "sequence": sample.get("sequence"),
            "frame_index": sample.get("frame_index"),
            "breakpoint": copy.deepcopy(sample.get("breakpoint")),
            "breakpoint_owners": copy.deepcopy(sample.get("breakpoint_owners", [])),
            "position": {
                "before": before_fields.get("position", {}),
                "after": after_fields.get("position", {}),
            },
            "velocity": {
                "before": before_fields.get("velocity", {}),
                "after": after_fields.get("velocity", {}),
            },
            "action_state": {
                "before": before_fields.get("action_state", {}),
                "after": after_fields.get("action_state", {}),
            },
            "callback": {
                "offset": callback.get("callback_offset", (sample.get("breakpoint") or {}).get("offset")),
                "breakpoint": copy.deepcopy(callback.get("breakpoint")),
                "record_size": callback.get("record_size"),
                "return_expected": copy.deepcopy(callback.get("return_expected")),
                "return_actual": copy.deepcopy(callback.get("return_actual")),
            },
            "callback_writes": {
                "record": copy.deepcopy(callback.get("writes", [])),
                "globals": copy.deepcopy(callback.get("global_writes", [])),
            },
            "globals": {
                "sample": copy.deepcopy(sample.get("globals", {})),
                "before": copy.deepcopy(callback.get("pre_globals", {})),
                "after": copy.deepcopy(callback.get("post_globals", {})),
            },
            "collision_events": [
                _map_fields(item) for item in _array(sample.get("collisions", []))
                if isinstance(item, Mapping)
            ],
            "map_cells": [
                _map_fields(item) for item in _array(sample.get("map_lookups", []))
                if isinstance(item, Mapping)
            ],
            "descriptor_properties": [
                _map_fields(item) for item in _array(sample.get("map_properties", []))
                if isinstance(item, Mapping)
            ],
        }
        if isinstance(sample.get("map_lookup"), Mapping):
            frame["map_cells"].append(_map_fields(sample["map_lookup"]))
        if isinstance(sample.get("map_property"), Mapping):
            frame["descriptor_properties"].append(_map_fields(sample["map_property"]))
        for collision in frame["collision_events"]:
            if isinstance(collision.get("map_property"), Mapping):
                frame["descriptor_properties"].append(_map_fields(collision["map_property"]))
        branch_events = _array(sample.get("branch_events", []))
        for event in branch_events:
            if isinstance(event, Mapping) and isinstance(event.get("descriptor_lookup"), Mapping):
                frame["descriptor_properties"].append(_map_fields(event["descriptor_lookup"]))
        frame["map_cells"] = _unique_dicts(frame["map_cells"])
        frame["descriptor_properties"] = _unique_dicts(frame["descriptor_properties"])
        frames.append(frame)
    return frames


def compact_trace(raw_trace: Mapping[str, Any], *, experiment: Experiment | None = None) -> dict[str, Any]:
    """Normalize a raw trace and retain only the fields used by matrix work."""
    samples = normalize_frame_timeline(raw_trace)
    frames = extract_trace_fields(samples)
    all_cells = _unique_dicts([cell for frame in frames for cell in frame["map_cells"]])
    all_descriptors = _unique_dicts([
        descriptor for frame in frames for descriptor in frame["descriptor_properties"]
    ])
    result: dict[str, Any] = {
        "schema": NORMALIZED_SCHEMA,
        "matrix_schema_version": MATRIX_SCHEMA_VERSION,
        "trace_schema_version": raw_trace.get("trace_schema_version", 1),
        "frames": frames,
        "map_cells": all_cells,
        "descriptor_properties": all_descriptors,
        "final_globals": copy.deepcopy(raw_trace.get("final_globals", {})),
    }
    if experiment is not None:
        result["experiment"] = {"name": experiment.name, "category": experiment.category, "level": experiment.level}
    return result


_PATH_TOKEN = re.compile(r"(?:^|\.)([^.\[\]]+)|\[([^\]]+)\]")


def _path_tokens(path: str) -> list[str]:
    tokens: list[str] = []
    consumed = 0
    for match in _PATH_TOKEN.finditer(path):
        if match.start() != consumed and path[consumed:match.start()] not in (".", ""):
            raise MatrixError(f"invalid output path: {path}")
        tokens.append(match.group(1) or match.group(2))
        consumed = match.end()
    if consumed != len(path):
        raise MatrixError(f"invalid output path: {path}")
    return tokens


def _path_values(value: Any, tokens: Sequence[str]) -> list[Any]:
    if not tokens:
        return [value]
    token, rest = tokens[0], tokens[1:]
    if token == "*":
        children = value if isinstance(value, list) else list(value.values()) if isinstance(value, Mapping) else []
        return [item for child in children for item in _path_values(child, rest)]
    if isinstance(value, Mapping) and token in value:
        return _path_values(value[token], rest)
    if isinstance(value, list):
        try:
            index = int(token)
        except ValueError:
            return []
        if 0 <= index < len(value):
            return _path_values(value[index], rest)
    return []


def output_values(payload: Mapping[str, Any], path: str) -> list[Any]:
    """Resolve dotted, indexed, and wildcard paths used by expectations."""
    return _path_values(payload, _path_tokens(path))


def _frame_path_values(payload: Mapping[str, Any], path: str) -> list[Any]:
    if path.startswith("frames[*]."):
        return output_values(payload, path)
    return [
        values[0] if len(values) == 1 else values
        for frame in payload.get("frames", [])
        for values in [_path_values(frame, _path_tokens(path))]
        if values
    ]


def assert_expected_outputs(normalized: Mapping[str, Any], expected: Mapping[str, Any]) -> dict[str, Any]:
    """Assert exact values and adjacent frame transitions.

    Exact paths use normal JSON paths, for example
    ``frames[1].position.after.x``.  Transition paths are frame-relative by
    default (``position.after.x``), or can use ``frames[*].`` explicitly.
    """
    failures: list[dict[str, Any]] = []
    exact = expected.get("exact", {}) or {}
    if not isinstance(exact, Mapping):
        raise MatrixError("expected_outputs.exact must be an object")
    for path, wanted in exact.items():
        values = output_values(normalized, str(path))
        if not values:
            failures.append({"kind": "exact", "path": str(path), "expected": wanted, "actual": None, "message": f"missing output at {path}"})
        elif any(value != wanted for value in values):
            failures.append({"kind": "exact", "path": str(path), "expected": wanted, "actual": values, "message": f"exact output mismatch at {path}"})
    transitions = expected.get("transitions", []) or []
    if not isinstance(transitions, Sequence) or isinstance(transitions, str):
        raise MatrixError("expected_outputs.transitions must be an array")
    frames = list(normalized.get("frames", []))
    for transition in transitions:
        if not isinstance(transition, Mapping):
            raise MatrixError("transition expectation must be an object")
        path = str(transition.get("path", ""))
        if not path:
            raise MatrixError("transition path is required")
        from_value = transition.get("from")
        to_value = transition.get("to")
        matched = []
        if path.startswith("frames[*]."):
            values = output_values(normalized, path)
            # A wildcard path is still compared in frame order.  The expected
            # transition must be present between adjacent values.
            pairs = list(zip(values, values[1:]))
            matched = [index for index, (left, right) in enumerate(pairs) if left == from_value and right == to_value]
        else:
            tokens = _path_tokens(path)
            values = [_path_values(frame, tokens) for frame in frames]
            pairs = []
            for index in range(len(values) - 1):
                left = values[index][0] if len(values[index]) == 1 else values[index]
                right = values[index + 1][0] if len(values[index + 1]) == 1 else values[index + 1]
                pairs.append((left, right))
            matched = [index for index, (left, right) in enumerate(pairs) if left == from_value and right == to_value]
        if "at" in transition:
            at = _integer(transition["at"], "transition at")
            matched = [index for index in matched if index == at or index + 1 == at]
        if not matched:
            failures.append({
                "kind": "transition", "path": path, "from": from_value, "to": to_value,
                "at": transition.get("at"), "message": f"expected transition not observed at {path}",
            })
    result = {"passed": not failures, "failures": failures, "exact_count": len(exact), "transition_count": len(transitions)}
    if failures:
        raise AssertionFailure(failures)
    return result


def _diff_values(left: Any, right: Any, path: str, changes: list[dict[str, Any]]) -> None:
    if isinstance(left, Mapping) and isinstance(right, Mapping):
        for key in sorted(set(left) | set(right), key=str):
            child = f"{path}.{key}" if path else str(key)
            if key not in left:
                changes.append({"path": child, "kind": "added", "left": None, "right": copy.deepcopy(right[key])})
            elif key not in right:
                changes.append({"path": child, "kind": "removed", "left": copy.deepcopy(left[key]), "right": None})
            else:
                _diff_values(left[key], right[key], child, changes)
        return
    if isinstance(left, list) and isinstance(right, list):
        for index in range(max(len(left), len(right))):
            child = f"{path}[{index}]"
            if index >= len(left):
                changes.append({"path": child, "kind": "added", "left": None, "right": copy.deepcopy(right[index])})
            elif index >= len(right):
                changes.append({"path": child, "kind": "removed", "left": copy.deepcopy(left[index]), "right": None})
            else:
                _diff_values(left[index], right[index], child, changes)
        return
    if left != right:
        changes.append({"path": path, "kind": "changed", "left": copy.deepcopy(left), "right": copy.deepcopy(right)})


def compare_traces(left: Mapping[str, Any], right: Mapping[str, Any]) -> dict[str, Any]:
    """Produce a pairwise field-level differential for two compact traces."""
    left_compact = left if left.get("schema") == NORMALIZED_SCHEMA else compact_trace(left)
    right_compact = right if right.get("schema") == NORMALIZED_SCHEMA else compact_trace(right)
    changes: list[dict[str, Any]] = []
    # Experiment identity, schema versions, and artifact metadata are not
    # observations.  Excluding them lets two independently captured runs of
    # the same behavior compare equal even when their names differ.
    compare_keys = ("frames", "map_cells", "descriptor_properties", "final_globals")
    left_view = {key: left_compact.get(key) for key in compare_keys}
    right_view = {key: right_compact.get(key) for key in compare_keys}
    _diff_values(left_view, right_view, "", changes)
    return {
        "schema": COMPARISON_SCHEMA,
        "matrix_schema_version": MATRIX_SCHEMA_VERSION,
        "equal": not changes,
        "left_frames": len(left_compact.get("frames", [])),
        "right_frames": len(right_compact.get("frames", [])),
        "change_count": len(changes),
        "changes": changes,
    }


def _fmt(value: Any) -> str:
    if value is None:
        return "-"
    if isinstance(value, (dict, list)):
        return json.dumps(value, sort_keys=True, separators=(",", ":"))
    return str(value)


def render_markdown_summary(
    experiment: Experiment,
    normalized: Mapping[str, Any] | None,
    *,
    assertions: Mapping[str, Any] | None = None,
    status: str = "completed",
    error: str | None = None,
) -> str:
    """Render a concise human-readable summary for one artifact directory."""
    lines = [
        f"# {experiment.name}", "", f"- Category: `{experiment.category}`",
        f"- Level: `{experiment.level or '-'}`",
        f"- Status: `{status}`",
    ]
    if error:
        lines.append(f"- Error: `{error}`")
    if assertions is not None:
        lines.append(f"- Assertions: `{'passed' if assertions.get('passed') else 'failed'}`")
    lines.extend(["", "| Sequence | Frame | Position | Velocity | Action/state | Callback writes | Collisions | MAP cells |", "| ---: | ---: | --- | --- | --- | ---: | ---: | ---: |"])
    for frame in (normalized or {}).get("frames", []):
        position = frame.get("position", {}).get("after") or frame.get("position", {}).get("before", {})
        velocity = frame.get("velocity", {}).get("after") or frame.get("velocity", {}).get("before", {})
        action = frame.get("action_state", {}).get("after") or frame.get("action_state", {}).get("before", {})
        lines.append(
            f"| {frame.get('sequence', '-')} | {frame.get('frame_index', '-')} | "
            f"`{_fmt(position)}` | `{_fmt(velocity)}` | `{_fmt(action)}` | "
            f"{len(frame.get('callback_writes', {}).get('record', []))} | "
            f"{len(frame.get('collision_events', []))} | {len(frame.get('map_cells', []))} |"
        )
    lines.append("")
    return "\n".join(lines)


def _write_json(path: Path, payload: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def _safe_name(value: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9._-]+", "-", value.strip()).strip(".-")
    return cleaned or "experiment"


def _hash_or_none(path: Path) -> str | None:
    try:
        return sha256(path) if path.is_file() else None
    except OSError:
        return None


@dataclass(frozen=True)
class RunnerConfig:
    repo_root: Path
    output_root: Path
    runtime_dir: Path
    startup_recording: Path
    url: str = "http://127.0.0.1:8386"
    token_file: Path | None = None
    launch: bool = False
    headless: bool = False
    startup_timeout: float = 15.0
    timeout: float = 30.0
    poll_interval: float = 0.05


@dataclass(frozen=True)
class RunResult:
    experiment: Experiment
    status: str
    manifest_path: Path
    normalized_path: Path | None
    comparison_path: Path | None = None


class ExperimentRunner:
    """Run each experiment in its own artifact transaction."""

    def __init__(self, config: RunnerConfig, api: ApiClient | None = None):
        self.config = config
        self.api = api

    def _artifact_paths(self, experiment: Experiment) -> dict[str, Path]:
        directory = self.config.output_root / _safe_name(experiment.category) / _safe_name(experiment.name)
        return {
            "directory": directory,
            "manifest": directory / "manifest.json",
            "raw_trace": directory / "raw-trace.json",
            "normalized": directory / "normalized.json",
            "assertions": directory / "assertions.json",
            "summary": directory / "summary.md",
            "failure": directory / "failure.json",
            "log": directory / "dosbox.log",
        }

    def _manifest(self, experiment: Experiment, paths: Mapping[str, Path], status: str) -> dict[str, Any]:
        runtime = self.config.runtime_dir
        executable = runtime / "QUIKY.EXE"
        archive = runtime / "NESTLE.DAT"
        tracer_paths = {
            "matrix_runner": Path(__file__),
            "quikytrace": self.config.repo_root / "research/tools/quikytrace.py",
            "player_lua": self.config.repo_root / "research/automation/quiky_player_trace.lua",
            "trace_common_lua": self.config.repo_root / "research/automation/quiky_trace_common.lua",
            "patch_watch_lua": self.config.repo_root / "research/automation/quiky_patch_watch.lua",
        }
        return {
            "schema": MANIFEST_SCHEMA,
            "matrix_schema_version": MATRIX_SCHEMA_VERSION,
            "status": status,
            "created_utc": datetime.now(timezone.utc).isoformat(),
            "experiment": experiment.to_dict(),
            "configuration": {
                "repo_root": str(self.config.repo_root.resolve()),
                "runtime_dir": str(runtime.resolve()),
                "startup_recording": str(self.config.startup_recording.resolve()),
                "timeout": self.config.timeout,
                "poll_interval": self.config.poll_interval,
                "launch": self.config.launch,
            },
            "inputs": {
                "executable": str(executable.resolve()),
                "executable_sha256": _hash_or_none(executable),
                "archive": str(archive.resolve()),
                "archive_sha256": _hash_or_none(archive),
                "startup_recording_sha256": _hash_or_none(self.config.startup_recording),
            },
            "tracer_hashes": {
                name: {"path": str(path.resolve()), "sha256": _hash_or_none(path)}
                for name, path in tracer_paths.items()
            },
            "outputs": {
                name: str(path.resolve())
                for name, path in paths.items()
                if name != "directory"
            },
        }

    @contextmanager
    def _api_session(self, paths: Mapping[str, Path]) -> Iterator[tuple[ApiClient, Mapping[str, Any], subprocess.Popen[bytes] | None]]:
        if self.api is not None:
            api = self.api
            yield api, api.get("/api/v1/dosbox/info"), None
            return
        process: subprocess.Popen[bytes] | None = None
        log_stream = None
        api: ApiClient | None = None
        try:
            if self.config.launch:
                port = _reserve_port()
                token = secrets.token_hex(32)
                env = os.environ.copy()
                env["DOSBOX_API_TOKEN"] = token
                if self.config.headless:
                    env["SDL_VIDEODRIVER"] = "dummy"
                    env["SDL_AUDIODRIVER"] = "dummy"
                runtime = self.config.runtime_dir.resolve()
                executable = runtime / "QUIKY.EXE"
                archive = runtime / "NESTLE.DAT"
                if not executable.is_file() or not archive.is_file():
                    raise MatrixError("runtime_dir must contain QUIKY.EXE and NESTLE.DAT")
                env["QUIKY_AUTOMATION_TARGET"] = str(executable)
                if runtime.name == "game":
                    env["DOSBOX_AUTOMATION_DATA_HOME"] = str(runtime.parent)
                log_stream = paths["log"].open("wb")
                command = [str(self.config.repo_root / "scripts/run-dosbox-automation.sh"), "--set", f"webserver_port={port}", "--set", f"mount_allowed_bases={runtime}", "--set", f"mount_allowed_image_roots={runtime}"]
                process = subprocess.Popen(command, cwd=self.config.repo_root, env=env, stdout=log_stream, stderr=subprocess.STDOUT)
                api = ApiClient(f"http://127.0.0.1:{port}", token)
                info = _wait_for_api(api, process, self.config.startup_timeout)
            else:
                api = ApiClient(self.config.url, discover_token(self.config.token_file))
                info = api.get("/api/v1/dosbox/info")
            yield api, info, process
        finally:
            if process is not None and api is not None:
                try:
                    api.post("/api/v1/control/shutdown")
                    process.wait(timeout=5)
                except (TraceError, subprocess.TimeoutExpired):
                    process.terminate()
                    try:
                        process.wait(timeout=3)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()
            if log_stream is not None:
                log_stream.close()

    def run(self, experiment: Experiment) -> RunResult:
        paths = self._artifact_paths(experiment)
        paths["directory"].mkdir(parents=True, exist_ok=True)
        manifest = self._manifest(experiment, paths, "running")
        _write_json(paths["manifest"], manifest)
        normalized_path: Path | None = None
        status = "failed"
        normalized: dict[str, Any] | None = None
        assertion_result: dict[str, Any] | None = None
        error_text: str | None = None
        error_type: str | None = None
        api: ApiClient | None = None
        try:
            with self._api_session(paths) as (api, info, _process):
                if not info.get("features", {}).get("debugger"):
                    raise MatrixError("the running dosbox-automation build has no debugger API")
                script_path = self.config.repo_root / "research/automation/quiky_player_trace.lua"
                player_config = build_player_config(
                    experiment, self.config.startup_recording,
                    timeout=self.config.timeout, poll_interval=self.config.poll_interval,
                )
                raw_trace, screenshots = trace_player_lua(api, script_path, player_config)
                # Keep this file byte-for-byte representationally equivalent
                # to the tracer result.  Run metadata belongs in manifest.json.
                _write_json(paths["raw_trace"], raw_trace)
                normalized = compact_trace(raw_trace, experiment=experiment)
                _write_json(paths["normalized"], normalized)
                normalized_path = paths["normalized"]
                try:
                    assertion_result = assert_expected_outputs(normalized, experiment.expected_outputs)
                except AssertionFailure as exc:
                    assertion_result = {"passed": False, "failures": exc.failures}
                    raise
                _write_json(paths["assertions"], assertion_result)
                status = "passed"
        except Exception as exc:
            error_text = str(exc)
            error_type = type(exc).__name__
            if assertion_result is None:
                assertion_result = {"passed": False, "failures": [{"message": error_text}]}
            _write_json(paths["assertions"], assertion_result)
            _write_json(paths["failure"], {"error": error_text, "type": type(exc).__name__})
            if api is not None and not isinstance(exc, AssertionFailure):
                try:
                    capture_failure(api, paths["raw_trace"], exc)
                except Exception:
                    pass
        finally:
            paths["summary"].write_text(
                render_markdown_summary(experiment, normalized, assertions=assertion_result, status=status, error=error_text),
                encoding="utf-8",
            )
            manifest = self._manifest(experiment, paths, status)
            if error_text:
                manifest["error"] = {"type": error_type, "message": error_text}
            if assertion_result is not None:
                manifest["assertions"] = assertion_result
            _write_json(paths["manifest"], manifest)
        return RunResult(experiment, status, paths["manifest"], normalized_path)

    def run_suite(self, experiments: Sequence[Experiment]) -> list[RunResult]:
        """Run each member independently so one failure never hides the rest."""
        return [self.run(experiment) for experiment in experiments]


def _reserve_port() -> int:
    import socket
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        return listener.getsockname()[1]


def _wait_for_api(api: ApiClient, process: subprocess.Popen[bytes], timeout: float) -> Mapping[str, Any]:
    deadline = time.monotonic() + timeout
    last_error: Exception | None = None
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise MatrixError(f"dosbox-automation exited with status {process.returncode}")
        try:
            return api.get("/api/v1/dosbox/info")
        except TraceError as exc:
            last_error = exc
            time.sleep(0.05)
    raise MatrixError(str(last_error or "DOSBox API did not become ready"))


def _load_artifact_trace(path: Path) -> dict[str, Any]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise MatrixError(f"cannot read trace {path}: {exc}") from exc
    if payload.get("schema") == RAW_SCHEMA:
        return compact_trace(payload["trace"])
    if payload.get("schema") == NORMALIZED_SCHEMA:
        return payload
    if "samples" in payload:
        return compact_trace(payload)
    raise MatrixError(f"{path} is not a player trace or normalized matrix trace")


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    run = subparsers.add_parser("run", help="run one experiment or a named suite")
    run.add_argument("--experiments-file", "--file", type=Path, required=True)
    run.add_argument("--experiment", action="append", default=[])
    run.add_argument("--suite")
    run.add_argument("--output-root", type=Path, required=True)
    run.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[2])
    run.add_argument("--runtime-dir", type=Path)
    run.add_argument("--startup-recording", type=Path)
    run.add_argument("--url", default="http://127.0.0.1:8386")
    run.add_argument("--token-file", type=Path)
    run.add_argument("--launch", action="store_true")
    run.add_argument("--headless", action="store_true")
    run.add_argument("--startup-timeout", type=float, default=15.0)
    run.add_argument("--timeout", type=float, default=30.0)
    run.add_argument("--poll-interval", type=float, default=0.05)
    run.add_argument("--pairwise", action="store_true", help="write adjacent pairwise differentials after the suite")

    compare = subparsers.add_parser("compare", help="compare two raw or normalized traces")
    compare.add_argument("left", type=Path)
    compare.add_argument("right", type=Path)
    compare.add_argument("--output", type=Path, required=True)

    rerun = subparsers.add_parser("rerun", help="rerun the experiment recorded by a failed manifest")
    rerun.add_argument("manifest", type=Path)
    rerun.add_argument("--output-root", type=Path)
    rerun.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[2])
    rerun.add_argument("--runtime-dir", type=Path)
    rerun.add_argument("--startup-recording", type=Path)
    rerun.add_argument("--url", default="http://127.0.0.1:8386")
    rerun.add_argument("--token-file", type=Path)
    rerun.add_argument("--launch", action="store_true")
    rerun.add_argument("--headless", action="store_true")
    rerun.add_argument("--startup-timeout", type=float, default=15.0)
    rerun.add_argument("--timeout", type=float, default=30.0)
    rerun.add_argument("--poll-interval", type=float, default=0.05)
    return parser


def _runner_from_args(args: argparse.Namespace, *, output_root: Path, repo_root: Path, runtime_dir: Path | None, startup_recording: Path | None) -> ExperimentRunner:
    repo_root = repo_root.resolve()
    return ExperimentRunner(RunnerConfig(
        repo_root=repo_root,
        output_root=output_root.resolve(),
        runtime_dir=(runtime_dir or repo_root / "game").resolve(),
        startup_recording=(startup_recording or repo_root / "research/automation/startup-to-input.json").resolve(),
        url=args.url,
        token_file=args.token_file,
        launch=args.launch,
        headless=args.headless,
        startup_timeout=args.startup_timeout,
        timeout=args.timeout,
        poll_interval=args.poll_interval,
    ))


def main(argv: Sequence[str] | None = None) -> int:
    args = _build_parser().parse_args(argv)
    try:
        if args.command == "compare":
            result = compare_traces(_load_artifact_trace(args.left), _load_artifact_trace(args.right))
            _write_json(args.output, result)
            print(f"wrote {result['change_count']} changes to {args.output}")
            return 0 if result["equal"] else 1
        if args.command == "rerun":
            manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
            experiment = Experiment.from_dict(manifest["experiment"])
            output_root = args.output_root or args.manifest.parents[2]
            runner = _runner_from_args(args, output_root=output_root, repo_root=args.repo_root, runtime_dir=args.runtime_dir, startup_recording=args.startup_recording)
            result = runner.run(experiment)
            print(f"{result.status}: {experiment.name} ({result.manifest_path})")
            return 0 if result.status == "passed" else 1
        catalog = load_catalog(args.experiments_file)
        experiments = catalog.select(names=args.experiment, suite=args.suite)
        runner = _runner_from_args(args, output_root=args.output_root, repo_root=args.repo_root, runtime_dir=args.runtime_dir, startup_recording=args.startup_recording)
        results = runner.run_suite(experiments)
        if args.pairwise and len(results) > 1:
            pairwise: list[dict[str, Any]] = []
            for left_result, right_result in zip(results, results[1:]):
                if left_result.normalized_path is None or right_result.normalized_path is None:
                    continue
                comparison = compare_traces(_load_artifact_trace(left_result.normalized_path), _load_artifact_trace(right_result.normalized_path))
                path = args.output_root / "comparisons" / f"{_safe_name(left_result.experiment.name)}-vs-{_safe_name(right_result.experiment.name)}.json"
                _write_json(path, comparison)
                pairwise.append({"left": left_result.experiment.name, "right": right_result.experiment.name, "path": str(path.resolve()), "equal": comparison["equal"]})
            _write_json(args.output_root / "comparisons" / "index.json", {"comparisons": pairwise})
        for result in results:
            print(f"{result.status}: {result.experiment.name} ({result.manifest_path})")
        return 0 if all(result.status == "passed" for result in results) else 1
    except (MatrixError, TraceError, OSError, KeyError, json.JSONDecodeError) as exc:
        print(f"player_trace_matrix: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
