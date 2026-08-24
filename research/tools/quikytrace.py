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
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from quikyctl import build_are_type_catalog

LOOKUP = (0x0207, 0x18C7)
RESOURCE_STATE_OFFSET = 0x97E4
RESOURCE_STATE_SIZE = 12


class TraceError(Exception):
    """Raised when a trace cannot be completed safely."""


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
    api: ApiClient, script_path: Path, record_offset: int, entity_type: int,
    timeout: float, poll_interval: float, startup_recording: Path,
    capture_delay_frames: int = 0, lifetime_samples: int = 0,
    state_machine_samples: int = 0,
    state_machine_camera_x: int | None = None,
    state_machine_keep_camera: bool = False,
    state_machine_position_x: int | None = None,
    state_machine_position_y: int | None = None,
    state_machine_force_emission: bool = False,
    sprite_init_offset: int = 0, capture_frames: int = 1,
    frame_step: int = 30, screenshot: Path | None = None,
    screenshot_mode: str = "rendered", select_level: str | None = None,
    selector_frames: int = 60,
) -> tuple[dict[str, Any], list[Path]]:
    prefix = (
        f"TRACE_TIMEOUT_MS={round(timeout * 1000)}\n"
        f"TRACE_RECORD_OFFSET={record_offset}\n"
        f"TRACE_ENTITY_TYPE={entity_type}\n"
        f"TRACE_CAPTURE_DELAY_FRAMES={capture_delay_frames}\n"
        f"TRACE_LIFETIME_SAMPLES={lifetime_samples}\n"
        f"TRACE_STATE_MACHINE_SAMPLES={state_machine_samples}\n"
        f"TRACE_STATE_MACHINE_CAMERA_X={state_machine_camera_x if state_machine_camera_x is not None else -1}\n"
        f"TRACE_STATE_MACHINE_KEEP_CAMERA={'true' if state_machine_keep_camera else 'false'}\n"
        f"TRACE_STATE_MACHINE_POSITION_X={state_machine_position_x if state_machine_position_x is not None else -1}\n"
        f"TRACE_STATE_MACHINE_POSITION_Y={state_machine_position_y if state_machine_position_y is not None else -1}\n"
        f"TRACE_STATE_MACHINE_FORCE_EMISSION={'true' if state_machine_force_emission else 'false'}\n"
        f"TRACE_SPRITE_INIT_OFFSET={sprite_init_offset}\n"
        f"TRACE_CAPTURE_FRAMES={capture_frames}\n"
        f"TRACE_FRAME_STEP={frame_step}\n"
        f"TRACE_SELECT_LEVEL={json.dumps(select_level or '')}\n"
        f"TRACE_SELECTOR_FRAMES={selector_frames}\n"
    )
    source = script_path.read_text(encoding="utf-8")
    name = urllib.parse.quote("quiky-entity-trace")
    api.request("POST", f"/api/v1/script/load?name={name}",
                text_body=prefix + source)
    api.post("/api/v1/script/start")
    deadline = time.monotonic() + timeout + 20
    recording = json.loads(startup_recording.read_text(encoding="utf-8"))
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
            if capture_frames <= 1:
                if screenshot is not None and not captured:
                    # The inert branch can leave the presentation surface
                    # blank while stopped; advance once after the script has
                    # published its final state before taking a one-frame
                    # compatibility screenshot.
                    api.post("/api/v1/debug/continue")
                    time.sleep(0.05)
                    screenshot.parent.mkdir(parents=True, exist_ok=True)
                    screenshot.write_bytes(api.get_binary(
                        f"/api/v1/video/frame?format=png&mode={screenshot_mode}"
                    ))
                    captured.append(screenshot)
                return entity, captured
            capture_index = entity.get("capture_index")
            if isinstance(capture_index, int) and capture_index not in acknowledged:
                if screenshot is not None:
                    frame_path = screenshot if capture_frames == 1 else screenshot.with_name(
                        f"{screenshot.stem}-frame-{capture_index:03d}{screenshot.suffix}"
                    )
                    frame_path.parent.mkdir(parents=True, exist_ok=True)
                    frame_path.write_bytes(api.get_binary(
                        f"/api/v1/video/frame?format=png&mode={screenshot_mode}"
                    ))
                    captured.append(frame_path)
                acknowledged.add(capture_index)
                api.post("/api/v1/debug/continue")
            if status.get("state") == "completed":
                return entity, captured
        if status.get("state") == "completed":
            raise TraceError("Lua entity trace completed without entity output")
        time.sleep(poll_interval)
    raise TraceError("timed out waiting for the Lua entity trace")


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
    parser.add_argument("--state-machine-keep-camera", action="store_true",
                        help="keep the overridden camera X through the final capture")
    parser.add_argument("--state-machine-position-x", type=int,
                        help="temporarily override the traced object's integer X position")
    parser.add_argument("--state-machine-position-y", type=int,
                        help="temporarily override the traced object's integer Y position")
    parser.add_argument("--state-machine-force-emission", action="store_true",
                        help="temporarily widen the bounds helper for a controlled 0x1f-0x21 emission probe")
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
    if args.lifetime_samples < 0:
        raise TraceError("--lifetime-samples cannot be negative")
    if args.state_machine_samples < 0:
        raise TraceError("--state-machine-samples cannot be negative")
    if args.state_machine_camera_x is not None and not 0 <= args.state_machine_camera_x <= 0xffff:
        raise TraceError("--state-machine-camera-x must be between 0 and 65535")
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
    dispatch_script_path = repo_root / "research/automation/quiky_dispatch_trace.lua"
    startup_recording = repo_root / "research/automation/startup-to-input.json"
    screenshot_bytes = None
    entity_screenshots: list[Path] = []
    try:
        if not info.get("features", {}).get("debugger"):
            raise TraceError("the running dosbox-automation build has no debugger API")
        if args.dispatch_table:
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
            entity, entity_screenshots = trace_entity_lua(
                api, entity_script_path, args.entity_record_offset,
                args.entity_type, args.timeout, args.poll_interval,
                startup_recording, args.screenshot_delay_frames,
                args.lifetime_samples, args.state_machine_samples,
                args.state_machine_camera_x,
                args.state_machine_keep_camera,
                args.state_machine_position_x, args.state_machine_position_y,
                args.state_machine_force_emission,
                args.sprite_init_offset,
                args.capture_frames, args.frame_step, args.screenshot,
                args.screenshot_mode,
                args.select_level, args.selector_frames,
            )
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
        if args.screenshot is not None and not entity_screenshots:
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
        "schema": "quiky-resource-trace-v1", "created_utc": datetime.now(timezone.utc).isoformat(),
        "dosbox": info,
        "inputs": {"executable": str(executable), "executable_sha256": sha256(executable),
                   "archive": str(archive), "archive_sha256": sha256(archive),
                   "prepare_w1l3": args.prepare_w1l3,
                   "navigate_w1l3": args.navigate_w1l3,
                   "navigate_level": args.navigate_level or ("W1L3" if args.navigate_w1l3 else None),
                   "select_level": args.select_level,
                   "tail_count": args.tail_count},
        "engine": "lua-debugger-api",
        "trace_kind": ("dispatch" if args.dispatch_table else
                       "entity" if args.entity_record_offset is not None else "resource"),
        "script": str(script_path),
        "script_sha256": sha256(script_path),
        "startup_recording": str(startup_recording)
        if args.navigate_w1l3 or args.navigate_level or args.select_level else None,
        "startup_recording_sha256": sha256(startup_recording)
        if args.navigate_w1l3 or args.navigate_level or args.select_level else None,
        "breakpoint": {"segment": LOOKUP[0], "offset": LOOKUP[1]}, "events": events,
    }
    if entity_screenshots:
        ledger["screenshots"] = [str(path) for path in entity_screenshots]
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
