#!/usr/bin/env python3
"""Trace Quiky resource lookups through dosbox-automation's local API."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import secrets
import socket
import struct
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from quikyctl import build_are_type_catalog

LOOKUP = (0x0207, 0x18C7)
RESOURCE_STATE_OFFSET = 0x97E4
RESOURCE_STATE_SIZE = 12
TRACE_SCHEMA_VERSION = 1
LEDGER_SCHEMA = "quiky-resource-trace-v1"


class TraceError(Exception):
    """Raised when a trace cannot be completed safely."""


@dataclass(frozen=True)
class StateMachineTraceConfig:
    """Guest-side controls for the 0x1f-0x21 effect-state experiment."""

    samples: int = 0
    camera_x: int | None = None
    camera_y: int | None = None
    keep_camera: bool = False
    position_x: int | None = None
    position_y: int | None = None
    force_emission: bool = False
    patch_map_run: bool = False
    force_state: int | None = None
    warmup_frames: int = 0
    map_patch_y_offset: int = 0


@dataclass(frozen=True)
class EntityTraceConfig:
    """Host and guest settings for one deterministic entity trace.

    The nested state-machine object is intentional: it keeps experiment-only
    controls out of the generic entity probe configuration while still
    allowing Python to serialize one complete request to the guest Lua
    script.
    """

    record_offset: int
    entity_type: int
    startup_recording: Path
    timeout: float = 30.0
    poll_interval: float = 0.05
    capture_delay_frames: int = 0
    lifetime_samples: int = 0
    state_machine: StateMachineTraceConfig = field(default_factory=StateMachineTraceConfig)
    sprite_init_offset: int = 0
    capture_frames: int = 1
    frame_step: int = 30
    screenshot: Path | None = None
    screenshot_mode: str = "rendered"
    select_level: str | None = None
    selector_frames: int = 60
    source_scan: bool = False
    movement_key: str = ""
    movement_frames: int = 0
    return_key: str = ""
    return_frames: int = 0
    movement_camera_x: int | None = None
    movement_camera_y: int | None = None


@dataclass(frozen=True)
class MemoryPatch:
    """One callback-scoped, reversible guest-memory mutation."""

    space: str
    offset: int
    width: int
    value: int
    selector: int | None = None


def parse_memory_patch(value: str) -> MemoryPatch:
    """Parse SPACE:OFFSET:WIDTH=VALUE (or selector:SEL:OFFSET:WIDTH=VALUE)."""
    try:
        target, raw_value = value.split("=", 1)
        parts = target.split(":")
        if parts[0] == "selector" and len(parts) == 4:
            space, raw_selector, raw_offset, raw_width = parts
            selector = int(raw_selector, 0)
        elif parts[0] in ("ds", "player") and len(parts) == 3:
            space, raw_offset, raw_width = parts
            selector = None
        else:
            raise ValueError
        widths = {"u8": 1, "u16": 2, "u32": 4}
        width = widths[raw_width.lower()]
        offset = int(raw_offset, 0)
        patch_value = int(raw_value, 0)
    except (KeyError, ValueError) as exc:
        raise argparse.ArgumentTypeError(
            "patch must be ds:OFFSET:u8|u16|u32=VALUE, "
            "player:OFFSET:u8|u16|u32=VALUE, or "
            "selector:SELECTOR:OFFSET:u8|u16|u32=VALUE"
        ) from exc
    if not 0 <= offset <= 0xFFFF:
        raise argparse.ArgumentTypeError("patch offset must be between 0 and 0xffff")
    if selector is not None and not 0 <= selector <= 0xFFFF:
        raise argparse.ArgumentTypeError("patch selector must be between 0 and 0xffff")
    if not 0 <= patch_value < 1 << (width * 8):
        raise argparse.ArgumentTypeError(f"patch value does not fit {raw_width.lower()}")
    return MemoryPatch(space, offset, width, patch_value, selector)


@dataclass(frozen=True)
class PlayerTraceConfig:
    """Settings for the player/object-pool and MAP-consumer probe."""

    startup_recording: Path
    timeout: float = 30.0
    poll_interval: float = 0.05
    samples: int = 8
    frames_between: int = 30
    focus_callback: bool = False
    focus_callback_offset: int = 0x3FF8
    map_focus: bool = False
    collision_focus: bool = False
    property_focus: bool = False
    property_helper_offset: int | None = None
    branch_focus: bool = False
    branch_patch_tile: int | None = None
    collision_patch_tile: int | None = None
    collision_patch_side: str = "left"
    branch_patch_flags: int | None = None
    descriptor_census: bool = False
    descriptor_count: int = 512
    map_width: int = 270
    map_height: int = 30
    probe_spawn_emitter: bool = False
    probe_release_emitter: bool = False
    input_key: str | None = None
    input_key_switch: str | None = None
    input_switch_sample: int = 0
    input_key_secondary: str | None = None
    secondary_pulse_frames: int = 0
    secondary_start_sample: int = 1
    secondary_end_sample: int = 0
    input_frames: int = 0
    input_samples: int = 0
    capture_player_record: bool = False
    collision_event_limit: int = 96
    collision_repeat_limit: int = 3
    transition_focus: bool = False
    transition_steps: int = 48
    transition_hold_events: int = 48
    transition_force_player_fall: bool = False
    transition_probe_frames: int = 0
    transition_probe_tail_frames: int = 0
    transition_probe_tail_camera_x: int = 0
    transition_warmup_frames: int = 0
    select_level: str | None = None
    selector_frames: int = 60
    screenshot: Path | None = None
    screenshot_mode: str = "rendered"
    patches: tuple[MemoryPatch, ...] = ()


def lua_literal(value: Any) -> str:
    """Encode JSON-like values as a safe Lua table literal.

    JSON strings and booleans are valid Lua syntax.  Tables use bracketed
    string keys so this also remains correct if a future config key is not a
    Lua identifier.
    """
    if value is None:
        return "nil"
    if value is True:
        return "true"
    if value is False:
        return "false"
    if isinstance(value, (int, float)):
        return repr(value)
    if isinstance(value, str):
        return json.dumps(value)
    if isinstance(value, dict):
        entries = [
            f"[{json.dumps(str(key))}]={lua_literal(item)}"
            for key, item in value.items()
        ]
        return "{" + ",".join(entries) + "}"
    if isinstance(value, (list, tuple)):
        return "{" + ",".join(lua_literal(item) for item in value) + "}"
    raise TypeError(f"cannot encode {type(value).__name__} as Lua")


def entity_trace_lua_config(config: EntityTraceConfig) -> dict[str, Any]:
    """Return the guest-visible portion of an entity trace configuration."""
    state_machine = config.state_machine
    return {
        "schema_version": TRACE_SCHEMA_VERSION,
        "timeout_ms": round(config.timeout * 1000),
        "record_offset": config.record_offset,
        "entity_type": config.entity_type,
        "capture_delay_frames": config.capture_delay_frames,
        "lifetime_samples": config.lifetime_samples,
        "state_machine": {
            "samples": state_machine.samples,
            "camera_x": state_machine.camera_x,
            "camera_y": state_machine.camera_y,
            "keep_camera": state_machine.keep_camera,
            "position_x": state_machine.position_x,
            "position_y": state_machine.position_y,
            "force_emission": state_machine.force_emission,
            "patch_map_run": state_machine.patch_map_run,
            "force_state": state_machine.force_state,
            "warmup_frames": state_machine.warmup_frames,
            "map_patch_y_offset": state_machine.map_patch_y_offset,
        },
        "sprite_init_offset": config.sprite_init_offset,
        "capture_frames": config.capture_frames,
        "frame_step": config.frame_step,
        "select_level": config.select_level or "",
        "selector_frames": config.selector_frames,
        "source_scan": config.source_scan,
        "movement_key": config.movement_key,
        "movement_frames": config.movement_frames,
        "return_key": config.return_key,
        "return_frames": config.return_frames,
        "movement_camera_x": config.movement_camera_x,
        "movement_camera_y": config.movement_camera_y,
    }


def player_trace_lua_config(config: PlayerTraceConfig) -> dict[str, Any]:
    """Return the guest-visible portion of a player trace configuration."""
    return {
        "schema_version": TRACE_SCHEMA_VERSION,
        "timeout_ms": round(config.timeout * 1000),
        "samples": config.samples,
        "frames_between": config.frames_between,
        "focus_callback": config.focus_callback,
        "focus_callback_offset": config.focus_callback_offset,
        "map_focus": config.map_focus,
        "collision_focus": config.collision_focus,
        "property_focus": config.property_focus,
        "property_helper_offset": config.property_helper_offset,
        "branch_focus": config.branch_focus,
        "branch_patch_tile": config.branch_patch_tile,
        "collision_patch_tile": config.collision_patch_tile,
        "collision_patch_side": config.collision_patch_side,
        "branch_patch_flags": config.branch_patch_flags,
        "descriptor_census": config.descriptor_census,
        "descriptor_count": config.descriptor_count,
        "map_width": config.map_width,
        "map_height": config.map_height,
        "probe_spawn_emitter": config.probe_spawn_emitter,
        "probe_release_emitter": config.probe_release_emitter,
        "input_key": config.input_key or "",
        "input_key_switch": config.input_key_switch or "",
        "input_switch_sample": config.input_switch_sample,
        "input_key_secondary": config.input_key_secondary or "",
        "secondary_pulse_frames": config.secondary_pulse_frames,
        "secondary_start_sample": config.secondary_start_sample,
        "secondary_end_sample": config.secondary_end_sample,
        "input_frames": config.input_frames,
        "input_samples": config.input_samples,
        "capture_player_record": config.capture_player_record,
        "collision_event_limit": config.collision_event_limit,
        "collision_repeat_limit": config.collision_repeat_limit,
        "transition_focus": config.transition_focus,
        "transition_steps": config.transition_steps,
        "transition_hold_events": config.transition_hold_events,
        "transition_force_player_fall": config.transition_force_player_fall,
        "transition_probe_frames": config.transition_probe_frames,
        "transition_probe_tail_frames": config.transition_probe_tail_frames,
        "transition_probe_tail_camera_x": config.transition_probe_tail_camera_x,
        "transition_warmup_frames": config.transition_warmup_frames,
        "select_level": config.select_level or "",
        "selector_frames": config.selector_frames,
        "patches": [
            {
                "space": patch.space,
                "selector": patch.selector,
                "offset": patch.offset,
                "width": patch.width,
                "value": patch.value,
            }
            for patch in config.patches
        ],
    }


def compose_player_trace_source(script_path: Path, config: PlayerTraceConfig) -> str:
    """Compose shared mechanics and the focused player probe into one chunk."""
    common_path = script_path.with_name("quiky_trace_common.lua")
    patch_watch_path = script_path.with_name("quiky_patch_watch.lua")
    try:
        common_source = common_path.read_text(encoding="utf-8")
        patch_watch_source = patch_watch_path.read_text(encoding="utf-8")
        player_source = script_path.read_text(encoding="utf-8")
    except OSError as exc:
        raise TraceError(f"cannot compose player trace: {exc}") from exc
    prefix = "TRACE_CONFIG = " + lua_literal(player_trace_lua_config(config)) + "\n"
    return prefix + common_source + "\n" + patch_watch_source + "\n" + player_source


class ApiClient:
    def __init__(self, base_url: str, token: str, timeout: float = 5.0):
        self.base_url = base_url.rstrip("/")
        self.token = token
        self.timeout = timeout

    def request(self, method: str, path: str, body: Any = None, *, binary: bool = False,
                text_body: str | None = None) -> Any:
        headers = {"Authorization": f"Bearer {self.token}"}
        data = None
        if text_body is not None:
            data = text_body.encode("utf-8")
            headers["Content-Type"] = "text/plain"
        elif body is not None:
            data = json.dumps(body).encode()
            headers["Content-Type"] = "application/json"
        if binary:
            headers["Accept"] = "application/octet-stream"
        request = urllib.request.Request(self.base_url + path, data=data, headers=headers, method=method)
        try:
            with urllib.request.urlopen(request, timeout=self.timeout) as response:
                payload = response.read()
                return payload if binary else (json.loads(payload) if payload else {})
        except urllib.error.HTTPError as exc:
            detail = exc.read().decode("utf-8", "replace")
            raise TraceError(f"API {method} {path} failed ({exc.code}): {detail}") from exc
        except urllib.error.URLError as exc:
            raise TraceError(f"cannot reach {self.base_url}: {exc.reason}") from exc

    def get(self, path: str) -> Any:
        return self.request("GET", path)

    def get_binary(self, path: str) -> bytes:
        return self.request("GET", path, binary=True)

    def post(self, path: str, body: Any = None) -> Any:
        return self.request("POST", path, body)

def _u16(data: bytes, offset: int) -> int:
    if offset + 2 > len(data):
        raise TraceError("truncated stack data")
    return struct.unpack_from("<H", data, offset)[0]


def decode_lookup_call(stack: bytes) -> dict[str, int]:
    """Decode the far return address and far Pascal-string pointer."""
    if len(stack) < 8:
        raise TraceError("resource lookup stack needs at least 8 bytes")
    return {
        "return_offset": _u16(stack, 0), "return_segment": _u16(stack, 2),
        "path_offset": _u16(stack, 4), "path_segment": _u16(stack, 6),
    }


def decode_resource_state(data: bytes) -> dict[str, int]:
    if len(data) != RESOURCE_STATE_SIZE:
        raise TraceError("resource state must contain exactly 12 bytes")
    end, start, size = struct.unpack("<III", data)
    return {"start": start, "end": end, "size": size}


def trace_resources_lua(
    api: ApiClient, script_path: Path, count: int, timeout: float,
    poll_interval: float, prepare_w1l3: bool, navigate_w1l3: bool,
    selector_frames: int, startup_recording: Path | None = None,
    navigate_level: str | None = None, select_level: str | None = None,
    tail_count: int = 0,
) -> list[dict[str, Any]]:
    source = script_path.read_text(encoding="utf-8")
    requested_level = navigate_level or ("W1L3" if navigate_w1l3 else "")
    prefix = (
        f"TRACE_COUNT={count}\n"
        f"TRACE_TIMEOUT_MS={round(timeout * 1000)}\n"
        f"TRACE_PREPARE_W1L3={'true' if prepare_w1l3 else 'false'}\n"
        f"TRACE_NAVIGATE_W1L3={'true' if navigate_w1l3 else 'false'}\n"
        f"TRACE_NAVIGATE_LEVEL={json.dumps(requested_level)}\n"
        f"TRACE_SELECT_LEVEL={json.dumps(select_level or '')}\n"
        f"TRACE_SELECTOR_FRAMES={selector_frames}\n"
        f"TRACE_TAIL_COUNT={tail_count}\n"
    )
    name = urllib.parse.quote("quiky-resource-trace")
    api.request("POST", f"/api/v1/script/load?name={name}", text_body=prefix + source)
    api.post("/api/v1/script/start")
    deadline = time.monotonic() + timeout * ((count + tail_count) * 2 + 1) + 15
    if navigate_w1l3 or navigate_level or select_level:
        if startup_recording is None:
            raise TraceError("navigation requires a startup input recording")
        recording = json.loads(startup_recording.read_text(encoding="utf-8"))
        while time.monotonic() < deadline:
            status = api.get("/api/v1/script/status")
            if status.get("state") == "error":
                raise TraceError(f"Lua trace failed: {status.get('error', 'unknown error')}")
            if status.get("output", {}).get("awaiting_startup_replay"):
                api.post("/api/v1/input/sequence", {"events": recording["events"]})
                break
            time.sleep(poll_interval)
        else:
            raise TraceError("Lua trace did not request the startup replay")
    while time.monotonic() < deadline:
        status = api.get("/api/v1/script/status")
        if status.get("state") == "completed":
            raw_events = status.get("output", {}).get("events", {})
            if isinstance(raw_events, list):
                return raw_events
            return [raw_events[key] for key in sorted(raw_events, key=int)]
        if status.get("state") == "error":
            raise TraceError(f"Lua trace failed: {status.get('error', 'unknown error')}")
        time.sleep(poll_interval)
    raise TraceError("timed out waiting for the Lua trace script")


def trace_entity_lua(
    api: ApiClient, script_path: Path, config: EntityTraceConfig,
) -> tuple[dict[str, Any], list[Path]]:
    """Run one entity probe with one structured guest configuration."""
    prefix = "TRACE_CONFIG = " + lua_literal(entity_trace_lua_config(config)) + "\n"
    source = script_path.read_text(encoding="utf-8")
    name = urllib.parse.quote("quiky-entity-trace")
    api.request("POST", f"/api/v1/script/load?name={name}",
                text_body=prefix + source)
    api.post("/api/v1/script/start")
    deadline = time.monotonic() + config.timeout + 20
    recording = json.loads(config.startup_recording.read_text(encoding="utf-8"))
    replayed = False
    captured: list[Path] = []
    acknowledged: set[int] = set()
    while time.monotonic() < deadline:
        status = api.get("/api/v1/script/status")
        if status.get("state") == "error":
            raise TraceError(
                f"Lua entity trace failed: {status.get('error', 'unknown error')}"
            )
        if not replayed and status.get("output", {}).get("awaiting_startup_replay"):
            api.post("/api/v1/input/sequence", {"events": recording["events"]})
            replayed = True
        entity = status.get("output", {}).get("entity")
        if isinstance(entity, dict):
            if config.capture_frames <= 1:
                if config.screenshot is not None and not captured:
                    # The inert branch can leave the presentation surface
                    # blank while stopped; advance once after the script has
                    # published its final state before taking a one-frame
                    # compatibility screenshot.
                    api.post("/api/v1/debug/continue")
                    time.sleep(0.05)
                    config.screenshot.parent.mkdir(parents=True, exist_ok=True)
                    config.screenshot.write_bytes(api.get_binary(
                        f"/api/v1/video/frame?format=png&mode={config.screenshot_mode}"
                    ))
                    captured.append(config.screenshot)
                return entity, captured
            capture_index = entity.get("capture_index")
            if isinstance(capture_index, int) and capture_index not in acknowledged:
                if config.screenshot is not None:
                    if config.capture_frames == 1:
                        frame_path = config.screenshot
                    else:
                        frame_path = config.screenshot.with_name(
                            f"{config.screenshot.stem}-frame-"
                            f"{capture_index:03d}{config.screenshot.suffix}"
                        )
                    frame_path.parent.mkdir(parents=True, exist_ok=True)
                    frame_path.write_bytes(api.get_binary(
                        f"/api/v1/video/frame?format=png&mode={config.screenshot_mode}"
                    ))
                    captured.append(frame_path)
                acknowledged.add(capture_index)
                api.post("/api/v1/debug/continue")
            if status.get("state") == "completed":
                return entity, captured
        if status.get("state") == "completed":
            raise TraceError("Lua entity trace completed without entity output")
        time.sleep(config.poll_interval)
    raise TraceError("timed out waiting for the Lua entity trace")


def trace_player_lua(
    api: ApiClient, script_path: Path, config: PlayerTraceConfig,
) -> tuple[dict[str, Any], list[Path]]:
    """Run the player/object-pool probe with one structured configuration."""
    source = compose_player_trace_source(script_path, config)
    name = urllib.parse.quote("quiky-player-trace")
    api.request("POST", f"/api/v1/script/load?name={name}",
                text_body=source)
    api.post("/api/v1/script/start")
    deadline = time.monotonic() + config.timeout + 20
    recording = json.loads(config.startup_recording.read_text(encoding="utf-8"))
    replayed = False
    captured: list[Path] = []
    while time.monotonic() < deadline:
        status = api.get("/api/v1/script/status")
        if status.get("state") == "error":
            raise TraceError(
                f"Lua player trace failed: {status.get('error', 'unknown error')}"
            )
        if not replayed and status.get("output", {}).get("awaiting_startup_replay"):
            api.post("/api/v1/input/sequence", {"events": recording["events"]})
            replayed = True
        result = status.get("output", {}).get("player_trace")
        if isinstance(result, dict):
            if config.screenshot is not None and not captured:
                api.post("/api/v1/debug/continue")
                time.sleep(0.05)
                config.screenshot.parent.mkdir(parents=True, exist_ok=True)
                config.screenshot.write_bytes(api.get_binary(
                    f"/api/v1/video/frame?format=png&mode={config.screenshot_mode}"
                ))
                captured.append(config.screenshot)
            return result, captured
        if status.get("state") == "completed":
            raise TraceError("Lua player trace completed without output")
        time.sleep(config.poll_interval)
    raise TraceError("timed out waiting for the Lua player trace")


def trace_dispatch_lua(
    api: ApiClient, script_path: Path, entity_types: list[int], timeout: float,
    poll_interval: float, startup_recording: Path,
) -> dict[str, Any]:
    type_table = ", ".join(str(value) for value in entity_types)
    source = script_path.read_text(encoding="utf-8")
    prefix = (
        f"TRACE_TIMEOUT_MS={round(timeout * 1000)}\n"
        f"TRACE_ENTITY_TYPES={{{type_table}}}\n"
    )
    name = urllib.parse.quote("quiky-dispatch-trace")
    api.request("POST", f"/api/v1/script/load?name={name}",
                text_body=prefix + source)
    api.post("/api/v1/script/start")
    deadline = time.monotonic() + timeout + 15
    recording = json.loads(startup_recording.read_text(encoding="utf-8"))
    replayed = False
    while time.monotonic() < deadline:
        status = api.get("/api/v1/script/status")
        if status.get("state") == "error":
            raise TraceError(
                f"Lua dispatch trace failed: {status.get('error', 'unknown error')}"
            )
        if not replayed and status.get("output", {}).get("awaiting_startup_replay"):
            api.post("/api/v1/input/sequence", {"events": recording["events"]})
            replayed = True
        dispatch = status.get("output", {}).get("dispatch")
        if isinstance(dispatch, dict):
            return dispatch
        if status.get("state") == "completed":
            raise TraceError("Lua dispatch trace completed without output")
        time.sleep(poll_interval)
    raise TraceError("timed out waiting for the Lua dispatch trace")


def ordered_lua_array(value: Any) -> list[Any]:
    """Normalize arrays emitted as either JSON arrays or numeric-key objects."""
    if value is None:
        return []
    if isinstance(value, list):
        return value
    if isinstance(value, dict):
        return [value[key] for key in sorted(value, key=int)]
    raise TraceError("Lua output is not an array")


def normalize_entity_trace(entity: dict[str, Any]) -> dict[str, Any]:
    """Normalize Lua table arrays while retaining the raw trace fields."""
    entity.setdefault("trace_schema_version", TRACE_SCHEMA_VERSION)
    entity["lifetime_samples"] = ordered_lua_array(
        entity.get("lifetime_samples", [])
    )
    state_machine_samples = ordered_lua_array(
        entity.get("state_machine_samples", [])
    )
    for sample in state_machine_samples:
        sample["nested_calls"] = ordered_lua_array(
            sample.get("nested_calls", [])
        )
    entity["state_machine_samples"] = state_machine_samples
    entity["state_machine_object_updates"] = ordered_lua_array(
        entity.get("state_machine_object_updates", [])
    )
    entity["state_machine_callback_candidates"] = ordered_lua_array(
        entity.get("state_machine_callback_candidates", [])
    )
    for update in entity["state_machine_object_updates"]:
        lookup = update.get("animation_lookup", {})
        lookup["raw_prefix"] = ordered_lua_array(
            lookup.get("raw_prefix", [])
        )
        lookup["raw_bytes"] = ordered_lua_array(
            lookup.get("raw_bytes", [])
        )
    dedicated_lookup = entity.get("animation_lookup")
    if dedicated_lookup is not None:
        dedicated_lookup["raw_prefix"] = ordered_lua_array(
            dedicated_lookup.get("raw_prefix", [])
        )
        dedicated_lookup["raw_bytes"] = ordered_lua_array(
            dedicated_lookup.get("raw_bytes", [])
        )
    entity["animation_candidates"] = ordered_lua_array(
        entity.get("animation_candidates", [])
    )
    entity["update_candidates"] = ordered_lua_array(
        entity.get("update_candidates", [])
    )
    entity["frames"] = ordered_lua_array(entity.get("frames", []))
    for frame in entity["frames"]:
        lifecycle = frame.get("lifecycle")
        if not isinstance(lifecycle, dict):
            continue
        pool = lifecycle.get("pool")
        if isinstance(pool, dict):
            pool["records"] = ordered_lua_array(pool.get("records", []))
            pool["source_matches"] = ordered_lua_array(
                pool.get("source_matches", [])
            )
        scheduler = lifecycle.get("scheduler")
        if isinstance(scheduler, dict):
            scheduler["banks"] = ordered_lua_array(scheduler.get("banks", []))
            for bank in scheduler["banks"]:
                if isinstance(bank, dict):
                    bank["entries"] = ordered_lua_array(
                        bank.get("entries", [])
                    )
    return entity


def normalize_player_trace(trace: dict[str, Any]) -> dict[str, Any]:
    """Normalize pool/sample arrays emitted by the player Lua probe."""
    trace.setdefault("trace_schema_version", TRACE_SCHEMA_VERSION)
    samples = ordered_lua_array(trace.get("samples", []))
    for sample in samples:
        pool = sample.get("pool")
        if isinstance(pool, dict):
            pool["objects"] = ordered_lua_array(pool.get("objects", []))
            pool["kind_0x64"] = ordered_lua_array(pool.get("kind_0x64", []))
        scheduler = sample.get("scheduler")
        if isinstance(scheduler, dict):
            scheduler["entries"] = ordered_lua_array(
                scheduler.get("entries", [])
            )
        if "related_breakpoints" in sample:
            sample["related_breakpoints"] = ordered_lua_array(
                sample.get("related_breakpoints", [])
            )
        if "collisions" in sample:
            sample["collisions"] = ordered_lua_array(sample.get("collisions", []))
        if "map_lookups" in sample:
            sample["map_lookups"] = ordered_lua_array(sample.get("map_lookups", []))
        if "map_properties" in sample:
            sample["map_properties"] = ordered_lua_array(
                sample.get("map_properties", [])
            )
        if "branch_events" in sample:
            sample["branch_events"] = ordered_lua_array(
                sample.get("branch_events", [])
            )
        if "mutation_ledger" in sample:
            sample["mutation_ledger"] = ordered_lua_array(
                sample.get("mutation_ledger", [])
            )
            for mutation in sample["mutation_ledger"]:
                mutation["original_bytes"] = ordered_lua_array(
                    mutation.get("original_bytes", [])
                )
                mutation["replacement_bytes"] = ordered_lua_array(
                    mutation.get("replacement_bytes", [])
                )
        callback = sample.get("player_callback")
        if isinstance(callback, dict):
            if "writes" in callback:
                callback["writes"] = ordered_lua_array(callback.get("writes", []))
            if "global_writes" in callback:
                callback["global_writes"] = ordered_lua_array(
                    callback.get("global_writes", [])
                )
    census = trace.get("descriptor_census")
    if isinstance(census, dict):
        table = census.get("descriptor_table")
        if isinstance(table, dict):
            table["entries"] = ordered_lua_array(table.get("entries", []))
        loaded_map = census.get("map")
        if isinstance(loaded_map, dict):
            loaded_map["cells"] = ordered_lua_array(
                loaded_map.get("cells", [])
            )
            loaded_map["flag_candidates"] = ordered_lua_array(
                loaded_map.get("flag_candidates", [])
            )
    trace["samples"] = samples
    for key in ("final_pool",):
        pool = trace.get(key)
        if isinstance(pool, dict):
            pool["objects"] = ordered_lua_array(pool.get("objects", []))
            pool["kind_0x64"] = ordered_lua_array(pool.get("kind_0x64", []))
    return trace


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def discover_token(explicit: Path | None) -> str:
    if value := os.environ.get("DOSBOX_API_TOKEN"):
        return value.strip()
    repo_root = Path(__file__).resolve().parents[2]
    candidates = ([explicit] if explicit else []) + [
        repo_root / "research/build/dosbox-automation-debug/webserver/api_token",
        Path.home() / ".config/dosbox-automation/webserver/api_token",
    ]
    for candidate in candidates:
        if candidate and candidate.is_file():
            return candidate.read_text(encoding="ascii").strip()
    raise TraceError("API token not found; pass --token-file or set DOSBOX_API_TOKEN")


def reserve_local_port() -> int:
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        return listener.getsockname()[1]


def wait_for_api(api: ApiClient, process: subprocess.Popen[bytes], timeout: float) -> dict[str, Any]:
    deadline = time.monotonic() + timeout
    last_error = "API did not become ready"
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise TraceError(f"dosbox-automation exited with status {process.returncode}")
        try:
            return api.get("/api/v1/dosbox/info")
        except TraceError as exc:
            last_error = str(exc)
            time.sleep(0.05)
    raise TraceError(last_error)


def capture_failure(api: ApiClient, output: Path, error: Exception) -> None:
    """Preserve the guest frame and observable API state before shutdown."""
    output.parent.mkdir(parents=True, exist_ok=True)
    diagnostic: dict[str, Any] = {"error": str(error)}
    for name, path in (
        ("script", "/api/v1/script/status"),
        ("debugger", "/api/v1/debug/state"),
        ("cpu", "/api/v1/cpu/state"),
    ):
        try:
            diagnostic[name] = api.get(path)
        except TraceError as exc:
            diagnostic[name] = {"capture_error": str(exc)}
    try:
        frame = api.get_binary("/api/v1/video/frame?format=png&mode=rendered")
        output.with_suffix(output.suffix + ".failure.png").write_bytes(frame)
    except TraceError as exc:
        diagnostic["frame"] = {"capture_error": str(exc)}
    output.with_suffix(output.suffix + ".failure.json").write_text(
        json.dumps(diagnostic, indent=2) + "\n", encoding="utf-8"
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", default="http://127.0.0.1:8386")
    parser.add_argument("--token-file", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--count", type=int, default=2)
    parser.add_argument("--timeout", type=float, default=30.0)
    parser.add_argument("--poll-interval", type=float, default=0.05)
    parser.add_argument("--prepare-w1l3", action="store_true", help="continue from 01D7:491D, select W1L3, and inject Space")
    parser.add_argument("--navigate-w1l3", action="store_true",
                        help="launch from the menu and redirect W1L1 resource paths to W1L3 at lookup time")
    parser.add_argument("--navigate-level",
                        help="launch from the menu and redirect W1L1 resource paths to a four-character level such as W4L1")
    parser.add_argument("--select-level",
                        help="launch the cheat level selector and choose a four-character level such as W4L1")
    parser.add_argument("--selector-frames", type=int, default=60)
    parser.add_argument("--tail-count", type=int, default=0,
                        help="continue after the initial resource batch and collect optional lazy lookups")
    parser.add_argument("--launch", action="store_true", help="launch and own an isolated dosbox-automation process")
    parser.add_argument("--headless", action="store_true", help="use dummy SDL video/audio drivers with --launch")
    parser.add_argument("--startup-timeout", type=float, default=15.0)
    parser.add_argument("--runtime-dir", type=Path,
                        help="isolated runtime directory containing QUIKY.EXE and NESTLE.DAT")
    parser.add_argument("--entity-record-offset", type=lambda value: int(value, 0))
    parser.add_argument("--entity-type", type=lambda value: int(value, 0), default=0x2B)
    parser.add_argument("--player-trace", action="store_true",
                        help="trace the live object pool and 16-pixel MAP lookups")
    parser.add_argument("--player-samples", type=int, default=8,
                        help="number of player/object-pool samples")
    parser.add_argument("--player-frames-between", type=int, default=30,
                        help="guest frames between player/object-pool samples")
    parser.add_argument("--player-focus-callback", action="store_true",
                        help="break directly on the selected player callback (default 01F7:3FF8)")
    parser.add_argument("--player-callback-offset", type=lambda value: int(value, 0),
                        default=0x3FF8,
                        help="player callback offset for --player-focus-callback (default 0x3ff8)")
    parser.add_argument("--player-map-focus", action="store_true",
                        help="break on the 01F7:3376 MAP helper used by player collision probes")
    parser.add_argument("--player-collision-focus", action="store_true",
                        help="break on the candidate player collision helpers 6484/648e/3a8a")
    parser.add_argument("--player-property-focus", action="store_true",
                        help="break on raw MAP/tile-property helpers 5c27/5cc3")
    parser.add_argument("--player-property-helper", type=lambda value: int(value, 0),
                        choices=(0x5C27, 0x5CC3),
                        help="limit --player-property-focus to helper 0x5c27 or 0x5cc3")
    parser.add_argument("--player-branch-focus", action="store_true",
                        help="trace the 01F7:3D02 descriptor branch masks and return path")
    parser.add_argument("--player-branch-patch-tile", type=lambda value: int(value, 0),
                        help="debugger-only: substitute this low-9-bit tile at the 3D02 probe")
    parser.add_argument("--player-collision-patch-tile", type=lambda value: int(value, 0),
                        help="debugger-only: substitute this low-9-bit tile at the 3DF2 MAP probe")
    parser.add_argument("--player-collision-patch-side", choices=("left", "right", "both"),
                        default="left",
                        help="3DF2 patch probe: x-5, x+5, or both (default left)")
    parser.add_argument("--player-branch-patch-flags", type=lambda value: int(value, 0),
                        help="debugger-only: substitute descriptor +2 flags at the 3D02 probe")
    parser.add_argument("--player-descriptor-census", action="store_true",
                        help="dump the loaded descriptor table and MAP cells")
    parser.add_argument("--player-descriptor-count", type=int, default=512,
                        help="number of descriptor entries to read (default 512)")
    parser.add_argument("--player-map-width", type=int, default=270,
                        help="loaded MAP width for descriptor census (default 270)")
    parser.add_argument("--player-map-height", type=int, default=30,
                        help="loaded MAP height for descriptor census (default 30)")
    parser.add_argument("--player-probe-spawn-emitter", action="store_true",
                        help="debugger-only: force the player post-update target-emitter spawn path")
    parser.add_argument("--player-probe-release-emitter", action="store_true",
                        help="debugger-only: clear the first emitter target at 45AB and trace 470C teardown")
    parser.add_argument("--player-input-key",
                        help="hold a DOSBox keyboard key between player samples, e.g. KBD_right")
    parser.add_argument("--player-input-key-switch",
                        help="replace --player-input-key at the selected sample, e.g. KBD_left")
    parser.add_argument("--player-input-switch-sample", type=int, default=0,
                        help="first post-baseline sample using --player-input-key-switch")
    parser.add_argument("--player-input-key-2",
                        help="hold a second key alongside --player-input-key, e.g. KBD_up")
    parser.add_argument("--player-secondary-pulse-frames", type=int, default=0,
                        help="toggle the secondary key every N frames during each input hold")
    parser.add_argument("--player-secondary-start-sample", type=int, default=1,
                        help="post-baseline sample at which the secondary key starts (default 1)")
    parser.add_argument("--player-secondary-end-sample", type=int, default=0,
                        help="last post-baseline sample receiving the secondary key (0 means no end)")
    parser.add_argument("--player-input-frames", type=int, default=0,
                        help="guest frames to hold --player-input-key before each post-baseline sample")
    parser.add_argument("--player-input-samples", type=int, default=0,
                        help="number of post-baseline samples that receive the input hold (0 means all)")
    parser.add_argument("--player-capture-record", action="store_true",
                        help="capture the complete 0x78-byte player record before/after each callback")
    parser.add_argument("--player-patch", action="append", type=parse_memory_patch,
                        default=[], metavar="TARGET:WIDTH=VALUE",
                        help="reversible callback patch, e.g. player:0x3e:u16=0")
    parser.add_argument("--player-collision-event-limit", type=int, default=96,
                        help="maximum nested helper breakpoints per callback")
    parser.add_argument("--player-collision-repeat-limit", type=int, default=3,
                        help="same-breakpoint repeats tolerated before helper tracing is abandoned")
    parser.add_argument("--player-transition-focus", action="store_true",
                        help="trace DS:89EA transition callers 19E6/199D/43D0/1BC4/3AB3")
    parser.add_argument("--player-transition-steps", type=int, default=48,
                        help="number of transition breakpoints to observe")
    parser.add_argument("--player-transition-hold-events", type=int, default=48,
                        help="keep --player-input-key held across this many transition events")
    parser.add_argument("--player-transition-force-fall", action="store_true",
                        help="debugger-only: set player Y below the 43D0 boundary")
    parser.add_argument("--player-transition-probe-frames", type=int, default=0,
                        help="guest frames to advance between transition breakpoint passes")
    parser.add_argument("--player-transition-probe-tail-frames", type=int, default=0,
                        help="when the tail camera threshold is reached, use this probe interval")
    parser.add_argument("--player-transition-probe-tail-camera-x", type=int, default=0,
                        help="camera X threshold that switches to --player-transition-probe-tail-frames")
    parser.add_argument("--player-transition-warmup-frames", type=int, default=0,
                        help="guest frames to run with transition input held before arming probes")
    parser.add_argument("--dispatch-table", action="store_true",
                        help="capture dispatch entries for every normal ARE type")
    parser.add_argument("--screenshot", type=Path,
                        help="save the rendered frame when the trace completes")
    parser.add_argument("--screenshot-delay-frames", type=int, default=0,
                        help="wait this many guest frames after the entity match")
    parser.add_argument("--lifetime-samples", type=int, default=0,
                        help="record this many matching leaf update calls")
    parser.add_argument("--state-machine-samples", type=int, default=0,
                        help="record this many update-entry samples for types 0x1f-0x21")
    parser.add_argument("--state-machine-camera-x", type=int,
                        help="temporarily override DS:81c0 while sampling 0x1f-0x21")
    parser.add_argument("--state-machine-camera-y", type=int,
                        help="temporarily override DS:81c4 while sampling 0x1f-0x21")
    parser.add_argument("--state-machine-keep-camera", action="store_true",
                        help="keep the overridden camera X/Y through the final capture")
    parser.add_argument("--state-machine-position-x", type=int,
                        help="temporarily override the traced object's integer X position")
    parser.add_argument("--state-machine-position-y", type=int,
                        help="temporarily override the traced object's integer Y position")
    parser.add_argument("--state-machine-force-emission", action="store_true",
                        help="temporarily widen the bounds helper for a controlled 0x1f-0x21 emission probe")
    parser.add_argument("--state-machine-patch-map-run", action="store_true",
                        help="debugger-only: patch five live MAP cells to the first effect run")
    parser.add_argument("--state-machine-force-state", type=lambda value: int(value, 0),
                        help="debugger-only: seed object +0x32 update-state once before dispatch")
    parser.add_argument("--state-machine-warmup-frames", type=int, default=0,
                        help="guest frames to run after object creation before sampling 0x8e4b")
    parser.add_argument("--state-machine-map-patch-y-offset", type=int, default=0,
                        help="debugger-only: add this pixel offset to the five-cell MAP patch row")
    parser.add_argument("--sprite-init-offset", type=lambda value: int(value, 0),
                        default=0, help="break at a type-specific sprite initializer")
    parser.add_argument("--capture-frames", type=int, default=1,
                        help="capture this many synchronized entity frames")
    parser.add_argument("--frame-step", type=int, default=30,
                        help="guest frames between synchronized entity frames")
    parser.add_argument("--screenshot-mode", choices=("rendered", "raw"),
                        default="rendered")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.count < 1:
        raise TraceError("--count must be positive")
    if args.player_samples < 1:
        raise TraceError("--player-samples must be positive")
    if args.player_frames_between < 0:
        raise TraceError("--player-frames-between cannot be negative")
    if args.player_input_frames < 0:
        raise TraceError("--player-input-frames cannot be negative")
    if args.player_input_samples < 0:
        raise TraceError("--player-input-samples cannot be negative")
    if args.player_input_key_2 and not args.player_input_key:
        raise TraceError("--player-input-key-2 requires --player-input-key")
    if args.player_input_key_switch and not args.player_input_key:
        raise TraceError("--player-input-key-switch requires --player-input-key")
    if args.player_input_key_switch and args.player_input_switch_sample < 2:
        raise TraceError("--player-input-switch-sample must be at least 2")
    if args.player_input_switch_sample and not args.player_input_key_switch:
        raise TraceError("--player-input-switch-sample requires --player-input-key-switch")
    if args.player_secondary_pulse_frames < 0:
        raise TraceError("--player-secondary-pulse-frames cannot be negative")
    if args.player_secondary_start_sample < 1:
        raise TraceError("--player-secondary-start-sample must be positive")
    if args.player_secondary_end_sample < 0:
        raise TraceError("--player-secondary-end-sample cannot be negative")
    if (args.player_secondary_end_sample and
            args.player_secondary_end_sample < args.player_secondary_start_sample):
        raise TraceError("--player-secondary-end-sample must not precede the start sample")
    if args.player_transition_steps < 1:
        raise TraceError("--player-transition-steps must be positive")
    if args.player_transition_hold_events < 0:
        raise TraceError("--player-transition-hold-events cannot be negative")
    if args.player_transition_probe_frames < 0:
        raise TraceError("--player-transition-probe-frames cannot be negative")
    if args.player_transition_probe_tail_frames < 0:
        raise TraceError("--player-transition-probe-tail-frames cannot be negative")
    if args.player_transition_probe_tail_camera_x < 0:
        raise TraceError("--player-transition-probe-tail-camera-x cannot be negative")
    if args.player_transition_probe_tail_frames and not args.player_transition_probe_tail_camera_x:
        raise TraceError("--player-transition-probe-tail-frames requires --player-transition-probe-tail-camera-x")
    if args.player_transition_warmup_frames < 0:
        raise TraceError("--player-transition-warmup-frames cannot be negative")
    if args.player_input_frames and not args.player_input_key:
        raise TraceError("--player-input-frames requires --player-input-key")
    if args.player_capture_record and not args.player_focus_callback:
        raise TraceError("--player-capture-record requires --player-focus-callback")
    if args.player_collision_event_limit < 1:
        raise TraceError("--player-collision-event-limit must be positive")
    if args.player_collision_repeat_limit < 1:
        raise TraceError("--player-collision-repeat-limit must be positive")
    if args.player_property_focus and args.player_map_focus:
        raise TraceError("--player-property-focus cannot be combined with --player-map-focus")
    if args.player_branch_focus and (args.player_map_focus or args.player_collision_focus or
                                    args.player_property_focus):
        raise TraceError("--player-branch-focus cannot be combined with another player focus mode")
    if args.player_branch_patch_tile is not None and not args.player_branch_focus:
        raise TraceError("--player-branch-patch-tile requires --player-branch-focus")
    if args.player_branch_patch_flags is not None and not args.player_branch_focus:
        raise TraceError("--player-branch-patch-flags requires --player-branch-focus")
    if args.player_branch_patch_tile is not None and not 0 <= args.player_branch_patch_tile <= 0x1ff:
        raise TraceError("--player-branch-patch-tile must be between 0 and 511")
    if args.player_collision_patch_tile is not None and not (
            args.player_collision_focus or args.player_property_focus):
        raise TraceError("--player-collision-patch-tile requires collision or property focus")
    if args.player_collision_patch_tile is not None and not args.player_focus_callback:
        raise TraceError("--player-collision-patch-tile requires --player-focus-callback")
    if args.player_collision_patch_tile is not None and not 0 <= args.player_collision_patch_tile <= 0x1ff:
        raise TraceError("--player-collision-patch-tile must be between 0 and 511")
    if args.player_branch_patch_flags is not None and not 0 <= args.player_branch_patch_flags <= 0xffff:
        raise TraceError("--player-branch-patch-flags must be between 0 and 0xffff")
    if args.player_descriptor_count < 1 or args.player_descriptor_count > 512:
        raise TraceError("--player-descriptor-count must be between 1 and 512")
    if args.player_map_width < 1 or args.player_map_height < 1:
        raise TraceError("--player-map-width/height must be positive")
    if args.player_property_helper is not None and not args.player_property_focus:
        raise TraceError("--player-property-helper requires --player-property-focus")
    if not 0 <= args.player_callback_offset <= 0xffff:
        raise TraceError("--player-callback-offset must be between 0 and 65535")
    if args.player_probe_release_emitter and not args.player_focus_callback:
        raise TraceError("--player-probe-release-emitter requires --player-focus-callback")
    if (args.player_focus_callback and args.player_callback_offset == 0x3f27
            and args.player_samples != 1):
        raise TraceError("--player-focus-callback 0x3f27 requires --player-samples 1; use 0x3ff8 for repeated updates")
    if args.lifetime_samples < 0:
        raise TraceError("--lifetime-samples cannot be negative")
    if args.state_machine_samples < 0:
        raise TraceError("--state-machine-samples cannot be negative")
    if args.state_machine_warmup_frames < 0:
        raise TraceError("--state-machine-warmup-frames cannot be negative")
    if args.state_machine_map_patch_y_offset % 16:
        raise TraceError("--state-machine-map-patch-y-offset must be a multiple of 16")
    if args.state_machine_force_state is not None and not 0 <= args.state_machine_force_state <= 0xffff:
        raise TraceError("--state-machine-force-state must be between 0 and 65535")
    if args.state_machine_camera_x is not None and not 0 <= args.state_machine_camera_x <= 0xffff:
        raise TraceError("--state-machine-camera-x must be between 0 and 65535")
    if args.state_machine_camera_y is not None and not 0 <= args.state_machine_camera_y <= 0xffff:
        raise TraceError("--state-machine-camera-y must be between 0 and 65535")
    if (args.state_machine_position_x is None) != (args.state_machine_position_y is None):
        raise TraceError("--state-machine-position-x and --state-machine-position-y must be used together")
    for name in ("state_machine_position_x", "state_machine_position_y"):
        value = getattr(args, name)
        if value is not None and not 0 <= value <= 0xffff:
            raise TraceError(f"--{name.replace('_', '-')} must be between 0 and 65535")
    if args.capture_frames < 1:
        raise TraceError("--capture-frames must be positive")
    if args.frame_step < 0:
        raise TraceError("--frame-step cannot be negative")
    if args.tail_count < 0:
        raise TraceError("--tail-count cannot be negative")
    if args.prepare_w1l3 and args.navigate_w1l3:
        raise TraceError("--prepare-w1l3 and --navigate-w1l3 are mutually exclusive")
    if args.navigate_w1l3 and args.navigate_level:
        raise TraceError("--navigate-w1l3 and --navigate-level are mutually exclusive")
    if args.prepare_w1l3 and args.navigate_level:
        raise TraceError("--prepare-w1l3 and --navigate-level are mutually exclusive")
    if args.select_level and (args.prepare_w1l3 or args.navigate_w1l3 or args.navigate_level):
        raise TraceError("--select-level cannot be combined with another level navigation mode")
    if args.player_trace and (args.dispatch_table or args.entity_record_offset is not None):
        raise TraceError("--player-trace cannot be combined with --dispatch-table or --entity-record-offset")
    for option_name, option_value in (("navigate-level", args.navigate_level),
                                      ("select-level", args.select_level)):
        if option_value is not None and (
                len(option_value) != 4 or option_value[0] != "W" or
                option_value[1] not in "12345" or option_value[2] != "L" or
                option_value[3] not in "1234"):
            raise TraceError(f"--{option_name} must look like W4L1")
    if args.entity_record_offset is not None and (args.prepare_w1l3 or args.navigate_w1l3):
        raise TraceError("entity tracing cannot be combined with level navigation modes")
    if args.entity_record_offset is not None and args.navigate_level:
        raise TraceError("entity tracing cannot be combined with level navigation modes")
    if args.dispatch_table and (args.entity_record_offset is not None or
                                args.prepare_w1l3 or args.navigate_w1l3 or
                                args.navigate_level or args.select_level):
        raise TraceError("--dispatch-table cannot be combined with another trace mode")
    repo_root = Path(__file__).resolve().parents[2]
    process = None
    log_stream = None
    if args.launch:
        port = reserve_local_port()
        token = secrets.token_hex(32)
        env = os.environ.copy()
        env["DOSBOX_API_TOKEN"] = token
        if args.runtime_dir is not None:
            runtime_dir = args.runtime_dir.resolve()
            executable = runtime_dir / "QUIKY.EXE"
            if not executable.is_file() or not (runtime_dir / "NESTLE.DAT").is_file():
                raise TraceError("--runtime-dir must contain QUIKY.EXE and NESTLE.DAT")
            env["QUIKY_AUTOMATION_TARGET"] = str(executable)
            if runtime_dir.name == "game":
                env["DOSBOX_AUTOMATION_DATA_HOME"] = str(runtime_dir.parent)
        if args.headless:
            env["SDL_VIDEODRIVER"] = "dummy"
            env["SDL_AUDIODRIVER"] = "dummy"
        log_path = args.output.with_suffix(args.output.suffix + ".dosbox.log")
        log_path.parent.mkdir(parents=True, exist_ok=True)
        log_stream = log_path.open("wb")
        launch_command = [str(repo_root / "scripts/run-dosbox-automation.sh"),
                          "--set", f"webserver_port={port}"]
        if args.runtime_dir is not None:
            launch_command.extend(
                ["--set", f"mount_allowed_bases={runtime_dir}",
                 "--set", f"mount_allowed_image_roots={runtime_dir}"]
            )
        process = subprocess.Popen(
            launch_command,
            cwd=repo_root, env=env, stdout=log_stream, stderr=subprocess.STDOUT,
        )
        api = ApiClient(f"http://127.0.0.1:{port}", token)
        try:
            info = wait_for_api(api, process, args.startup_timeout)
        except Exception:
            process.terminate()
            process.wait(timeout=5)
            log_stream.close()
            raise
    else:
        api = ApiClient(args.url, discover_token(args.token_file))
        info = api.get("/api/v1/dosbox/info")
    runtime_root = args.runtime_dir.resolve() if args.runtime_dir else repo_root / "game"
    executable, archive = runtime_root / "QUIKY.EXE", runtime_root / "NESTLE.DAT"
    script_path = repo_root / "research/automation/quiky_resource_trace.lua"
    entity_script_path = repo_root / "research/automation/quiky_entity_trace.lua"
    player_script_path = repo_root / "research/automation/quiky_player_trace.lua"
    dispatch_script_path = repo_root / "research/automation/quiky_dispatch_trace.lua"
    startup_recording = repo_root / "research/automation/startup-to-input.json"
    screenshot_bytes = None
    entity_screenshots: list[Path] = []
    player_screenshots: list[Path] = []
    try:
        if not info.get("features", {}).get("debugger"):
            raise TraceError("the running dosbox-automation build has no debugger API")
        if args.player_trace:
            player_config = PlayerTraceConfig(
                startup_recording=startup_recording,
                timeout=args.timeout,
                poll_interval=args.poll_interval,
                samples=args.player_samples,
                frames_between=args.player_frames_between,
                focus_callback=args.player_focus_callback,
                focus_callback_offset=args.player_callback_offset,
                map_focus=args.player_map_focus,
                collision_focus=args.player_collision_focus,
                property_focus=args.player_property_focus,
                property_helper_offset=args.player_property_helper,
                branch_focus=args.player_branch_focus,
                branch_patch_tile=args.player_branch_patch_tile,
                collision_patch_tile=args.player_collision_patch_tile,
                collision_patch_side=args.player_collision_patch_side,
                branch_patch_flags=args.player_branch_patch_flags,
                descriptor_census=args.player_descriptor_census,
                descriptor_count=args.player_descriptor_count,
                map_width=args.player_map_width,
                map_height=args.player_map_height,
                probe_spawn_emitter=args.player_probe_spawn_emitter,
                probe_release_emitter=args.player_probe_release_emitter,
                input_key=args.player_input_key,
                input_key_switch=args.player_input_key_switch,
                input_switch_sample=args.player_input_switch_sample,
                input_key_secondary=args.player_input_key_2,
                secondary_pulse_frames=args.player_secondary_pulse_frames,
                secondary_start_sample=args.player_secondary_start_sample,
                secondary_end_sample=args.player_secondary_end_sample,
                input_frames=args.player_input_frames,
                input_samples=args.player_input_samples,
                capture_player_record=args.player_capture_record,
                patches=tuple(args.player_patch),
                collision_event_limit=args.player_collision_event_limit,
                collision_repeat_limit=args.player_collision_repeat_limit,
                transition_focus=args.player_transition_focus,
                transition_steps=args.player_transition_steps,
                transition_hold_events=args.player_transition_hold_events,
                transition_force_player_fall=args.player_transition_force_fall,
                transition_probe_frames=args.player_transition_probe_frames,
                transition_probe_tail_frames=args.player_transition_probe_tail_frames,
                transition_probe_tail_camera_x=args.player_transition_probe_tail_camera_x,
                transition_warmup_frames=args.player_transition_warmup_frames,
                select_level=args.select_level,
                selector_frames=args.selector_frames,
                screenshot=args.screenshot,
                screenshot_mode=args.screenshot_mode,
            )
            player_trace, player_screenshots = trace_player_lua(
                api, player_script_path, player_config,
            )
            player_trace = normalize_player_trace(player_trace)
            events = [player_trace]
            script_path = player_script_path
        elif args.dispatch_table:
            normal_types = [
                item.entity_type for item in build_are_type_catalog(archive)
                if item.entity_type not in (0x65, 0x66, 0x67)
            ]
            dispatch = trace_dispatch_lua(
                api, dispatch_script_path, normal_types, args.timeout,
                args.poll_interval, startup_recording,
            )
            events = ordered_lua_array(dispatch.get("entries", []))
            for event in events:
                event["raw_bytes"] = ordered_lua_array(event.get("raw_bytes", []))
            script_path = dispatch_script_path
        elif args.entity_record_offset is not None:
            entity_config = EntityTraceConfig(
                record_offset=args.entity_record_offset,
                entity_type=args.entity_type,
                startup_recording=startup_recording,
                timeout=args.timeout,
                poll_interval=args.poll_interval,
                capture_delay_frames=args.screenshot_delay_frames,
                lifetime_samples=args.lifetime_samples,
                state_machine=StateMachineTraceConfig(
                    samples=args.state_machine_samples,
                    camera_x=args.state_machine_camera_x,
                    camera_y=args.state_machine_camera_y,
                    keep_camera=args.state_machine_keep_camera,
                    position_x=args.state_machine_position_x,
                    position_y=args.state_machine_position_y,
                    force_emission=args.state_machine_force_emission,
                    patch_map_run=args.state_machine_patch_map_run,
                    force_state=args.state_machine_force_state,
                    warmup_frames=args.state_machine_warmup_frames,
                    map_patch_y_offset=args.state_machine_map_patch_y_offset,
                ),
                sprite_init_offset=args.sprite_init_offset,
                capture_frames=args.capture_frames,
                frame_step=args.frame_step,
                screenshot=args.screenshot,
                screenshot_mode=args.screenshot_mode,
                select_level=args.select_level,
                selector_frames=args.selector_frames,
            )
            entity, entity_screenshots = trace_entity_lua(
                api, entity_script_path, entity_config,
            )
            entity = normalize_entity_trace(entity)
            events = [entity]
            script_path = entity_script_path
        else:
            events = trace_resources_lua(
                api, script_path, args.count, args.timeout, args.poll_interval,
                args.prepare_w1l3, args.navigate_w1l3, args.selector_frames,
                startup_recording,
                args.navigate_level or ("W1L3" if args.navigate_w1l3 else None),
                args.select_level,
                args.tail_count,
            )
        if args.screenshot is not None and not entity_screenshots and not player_screenshots:
            screenshot_bytes = api.get_binary(
                "/api/v1/video/frame?format=png&mode=rendered"
            )
    except Exception as exc:
        capture_failure(api, args.output, exc)
        raise
    finally:
        if process is not None:
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
    if args.screenshot is not None and screenshot_bytes is not None:
        args.screenshot.parent.mkdir(parents=True, exist_ok=True)
        args.screenshot.write_bytes(screenshot_bytes)
    ledger = {
        "schema": LEDGER_SCHEMA,
        "trace_schema_version": TRACE_SCHEMA_VERSION,
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "dosbox": info,
        "inputs": {"executable": str(executable), "executable_sha256": sha256(executable),
                   "archive": str(archive), "archive_sha256": sha256(archive),
                   "prepare_w1l3": args.prepare_w1l3,
                   "navigate_w1l3": args.navigate_w1l3,
                   "navigate_level": args.navigate_level or ("W1L3" if args.navigate_w1l3 else None),
                   "select_level": args.select_level,
                   "tail_count": args.tail_count},
        "engine": "lua-debugger-api",
        "trace_kind": ("player" if args.player_trace else
                       "dispatch" if args.dispatch_table else
                       "entity" if args.entity_record_offset is not None else "resource"),
        "script": str(script_path),
        "script_sha256": sha256(script_path),
        "startup_recording": str(startup_recording)
        if args.navigate_w1l3 or args.navigate_level or args.select_level or args.player_trace else None,
        "startup_recording_sha256": sha256(startup_recording)
        if args.navigate_w1l3 or args.navigate_level or args.select_level or args.player_trace else None,
        "breakpoint": {"segment": LOOKUP[0], "offset": LOOKUP[1]}, "events": events,
    }
    if entity_screenshots:
        ledger["screenshots"] = [str(path) for path in entity_screenshots]
    if player_screenshots:
        ledger["screenshots"] = [str(path) for path in player_screenshots]
    if args.dispatch_table:
        ledger["data_selector"] = dispatch.get("data_selector")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(ledger, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {len(events)} trace events to {args.output}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except TraceError as exc:
        print(f"quikytrace: {exc}", file=sys.stderr)
        raise SystemExit(1)
