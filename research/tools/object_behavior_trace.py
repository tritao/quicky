#!/usr/bin/env python3
"""Trace one non-player ARE object's callback state transitions."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import secrets
import subprocess
import sys
import time
import urllib.parse
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from quikytrace import (
    ApiClient,
    TraceError,
    discover_token,
    reserve_local_port,
    wait_for_api,
)


@dataclass(frozen=True)
class ObjectBehaviorConfig:
    record_offset: int
    entity_type: int
    samples: int
    startup_recording: Path
    timeout: float = 60.0
    poll_interval: float = 0.05
    select_level: str | None = None
    selector_frames: int = 60
    camera_x: int | None = None
    camera_y: int | None = None
    sprite_init_offset: int = 0
    align_object_to_player: bool = False
    trace_overlap: bool = False
    trace_collision: bool = False
    trace_platform: bool = False
    trace_bump: bool = False
    force_active_player_bounds: bool = False
    align_y_offset: int = 0
    force_velocity_x: int | None = None
    force_velocity_y: int | None = None
    force_platform_ready: bool = False


def lua_config(config: ObjectBehaviorConfig) -> dict[str, Any]:
    return {
        "schema_version": 1,
        "timeout_ms": round(config.timeout * 1000),
        "record_offset": config.record_offset,
        "entity_type": config.entity_type,
        "samples": config.samples,
        "select_level": config.select_level or "",
        "selector_frames": config.selector_frames,
        "camera_x": config.camera_x,
        "camera_y": config.camera_y,
        "sprite_init_offset": config.sprite_init_offset,
        "align_object_to_player": config.align_object_to_player,
        "trace_overlap": config.trace_overlap,
        "trace_collision": config.trace_collision,
        "trace_platform": config.trace_platform,
        "trace_bump": config.trace_bump,
        "force_active_player_bounds": config.force_active_player_bounds,
        "align_y_offset": config.align_y_offset,
        "force_velocity_x": config.force_velocity_x,
        "force_velocity_y": config.force_velocity_y,
        "force_platform_ready": config.force_platform_ready,
    }


def lua_literal(value: Any) -> str:
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
        return "{" + ",".join(
            f"[{json.dumps(str(key))}]={lua_literal(item)}"
            for key, item in value.items()
        ) + "}"
    raise TypeError(f"cannot encode {type(value).__name__} as Lua")


def ordered_lua_array(value: Any) -> list[Any]:
    if isinstance(value, list):
        return value
    if isinstance(value, dict):
        pairs: list[tuple[int, Any]] = []
        for key, item in value.items():
            try:
                pairs.append((int(key), item))
            except (TypeError, ValueError):
                return []
        return [item for _, item in sorted(pairs)]
    return []


def normalize_behavior_trace(trace: dict[str, Any]) -> dict[str, Any]:
    samples = ordered_lua_array(trace.get("samples", []))
    for sample in samples:
        if isinstance(sample, dict):
            sample["changed_bytes"] = ordered_lua_array(
                sample.get("changed_bytes", [])
            )
    trace["samples"] = samples
    return trace


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def trace_object_behavior(
    api: ApiClient, script_path: Path, config: ObjectBehaviorConfig,
) -> dict[str, Any]:
    source = script_path.read_text(encoding="utf-8")
    name = urllib.parse.quote("quiky-object-behavior-trace")
    prefix = "TRACE_CONFIG = " + lua_literal(lua_config(config)) + "\n"
    api.request("POST", f"/api/v1/script/load?name={name}",
                text_body=prefix + source)
    api.post("/api/v1/script/start")
    recording = json.loads(config.startup_recording.read_text(encoding="utf-8"))
    deadline = time.monotonic() + config.timeout + 20
    replayed = False
    while time.monotonic() < deadline:
        status = api.get("/api/v1/script/status")
        if status.get("state") == "error":
            raise TraceError(
                f"Lua object behavior trace failed: {status.get('error', 'unknown error')}"
            )
        if not replayed and status.get("output", {}).get("awaiting_startup_replay"):
            api.post("/api/v1/input/sequence", {"events": recording["events"]})
            replayed = True
        result = status.get("output", {}).get("behavior_trace")
        if isinstance(result, dict):
            return normalize_behavior_trace(result)
        if status.get("state") == "completed":
            raise TraceError("Lua object behavior trace completed without output")
        time.sleep(config.poll_interval)
    raise TraceError("timed out waiting for the Lua object behavior trace")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--record-offset", type=lambda value: int(value, 0),
                        default=0x1792)
    parser.add_argument("--entity-type", type=lambda value: int(value, 0),
                        required=True)
    parser.add_argument("--samples", type=int, default=32)
    parser.add_argument("--timeout", type=float, default=60.0)
    parser.add_argument("--poll-interval", type=float, default=0.05)
    parser.add_argument("--select-level")
    parser.add_argument("--selector-frames", type=int, default=60)
    parser.add_argument("--camera-x", type=int)
    parser.add_argument("--camera-y", type=int)
    parser.add_argument(
        "--sprite-init-offset", type=lambda value: int(value, 0), default=0,
        help="post-initializer breakpoint used to obtain the live object callback",
    )
    parser.add_argument(
        "--align-object-to-player", action="store_true",
        help="write the live player's fixed-point position into the traced object before its first callback",
    )
    parser.add_argument(
        "--trace-overlap", action="store_true",
        help="capture the shared collectible overlap helper and clear branch",
    )
    parser.add_argument(
        "--trace-collision", action="store_true",
        help="capture normal-object visibility, bounds, MAP, and offscreen helpers",
    )
    parser.add_argument(
        "--trace-platform", action="store_true",
        help="capture moving-platform MAP, offscreen, and player-carry helpers",
    )
    parser.add_argument(
        "--trace-bump", action="store_true",
        help="capture the BUMP player-range and hazard-effect branch",
    )
    parser.add_argument(
        "--force-active-player-bounds", action="store_true",
        help="use the original active-player vertical bound (-40..0) for the controlled overlap probe",
    )
    parser.add_argument(
        "--align-y-offset", type=int, default=0,
        help="integer-pixel Y offset from the live player when aligning the traced object",
    )
    parser.add_argument("--force-velocity-x", type=lambda value: int(value, 0),
                        help="debugger-only fixed-point X velocity written before the first callback")
    parser.add_argument("--force-velocity-y", type=lambda value: int(value, 0),
                        help="debugger-only fixed-point Y velocity written before the first callback")
    parser.add_argument(
        "--force-platform-ready", action="store_true",
        help="debugger-only: clear the platform carry latch before its first callback",
    )
    parser.add_argument("--startup-recording", type=Path,
                        default=Path("research/automation/startup-to-input.json"))
    parser.add_argument("--url", default="http://127.0.0.1:8386")
    parser.add_argument("--token-file", type=Path)
    parser.add_argument("--launch", action="store_true")
    parser.add_argument("--headless", action="store_true")
    parser.add_argument("--startup-timeout", type=float, default=15.0)
    parser.add_argument("--runtime-dir", type=Path)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.samples < 1:
        raise TraceError("--samples must be positive")
    if not 0 <= args.entity_type <= 0xff:
        raise TraceError("--entity-type must be between 0 and 255")
    if args.select_level is not None and len(args.select_level) != 4:
        raise TraceError("--select-level must look like W4L1")
    if (args.camera_x is None) != (args.camera_y is None):
        raise TraceError("--camera-x and --camera-y must be used together")
    for name in ("camera_x", "camera_y"):
        value = getattr(args, name)
        if value is not None and not 0 <= value <= 0xffff:
            raise TraceError(f"--{name.replace('_', '-')} must be between 0 and 65535")

    repo_root = Path(__file__).resolve().parents[2]
    startup_recording = args.startup_recording
    if not startup_recording.is_absolute():
        startup_recording = repo_root / startup_recording
    script_path = repo_root / "research/automation/quiky_object_behavior_trace.lua"
    output = args.output if args.output.is_absolute() else repo_root / args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    process = None
    log_stream = None

    if args.launch:
        port = reserve_local_port()
        token = secrets.token_hex(32)
        env = os.environ.copy()
        env["DOSBOX_API_TOKEN"] = token
        if args.runtime_dir is not None:
            runtime_dir = args.runtime_dir.resolve()
            if not (runtime_dir / "QUIKY.EXE").is_file() or not (runtime_dir / "NESTLE.DAT").is_file():
                raise TraceError("--runtime-dir must contain QUIKY.EXE and NESTLE.DAT")
            env["QUIKY_AUTOMATION_TARGET"] = str(runtime_dir / "QUIKY.EXE")
            env["DOSBOX_AUTOMATION_DATA_HOME"] = str(runtime_dir.parent)
        if args.headless:
            env["SDL_VIDEODRIVER"] = "dummy"
            env["SDL_AUDIODRIVER"] = "dummy"
        log_path = output.with_suffix(output.suffix + ".dosbox.log")
        log_stream = log_path.open("wb")
        command = [str(repo_root / "scripts/run-dosbox-automation.sh"),
                   "--set", f"webserver_port={port}"]
        if args.runtime_dir is not None:
            runtime_dir = args.runtime_dir.resolve()
            command.extend(["--set", f"mount_allowed_bases={runtime_dir}",
                            "--set", f"mount_allowed_image_roots={runtime_dir}"])
        process = subprocess.Popen(command, cwd=repo_root, env=env,
                                   stdout=log_stream, stderr=subprocess.STDOUT)
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

    try:
        if not info.get("features", {}).get("debugger"):
            raise TraceError("the running dosbox-automation build has no debugger API")
        config = ObjectBehaviorConfig(
            record_offset=args.record_offset,
            entity_type=args.entity_type,
            samples=args.samples,
            startup_recording=startup_recording,
            timeout=args.timeout,
            poll_interval=args.poll_interval,
            select_level=args.select_level,
            selector_frames=args.selector_frames,
        camera_x=args.camera_x,
        camera_y=args.camera_y,
        sprite_init_offset=args.sprite_init_offset,
        align_object_to_player=args.align_object_to_player,
        trace_overlap=args.trace_overlap,
        trace_collision=args.trace_collision,
        trace_platform=args.trace_platform,
        trace_bump=args.trace_bump,
        force_active_player_bounds=args.force_active_player_bounds,
        align_y_offset=args.align_y_offset,
        force_velocity_x=args.force_velocity_x,
        force_velocity_y=args.force_velocity_y,
        force_platform_ready=args.force_platform_ready,
        )
        trace = trace_object_behavior(api, script_path, config)
        envelope = {
            "schema": "quiky-object-behavior-v1",
            "created_utc": datetime.now(timezone.utc).isoformat(),
            "dosbox": info,
            "trace_kind": "object-behavior",
            "script": str(script_path),
            "script_sha256": sha256(script_path),
            "startup_recording": str(startup_recording),
            "startup_recording_sha256": sha256(startup_recording),
            "config": lua_config(config),
            "events": [trace],
        }
        output.write_text(json.dumps(envelope, indent=2) + "\n", encoding="utf-8")
        print(f"wrote object behavior trace to {output}")
        return 0
    finally:
        if process is not None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)
        if log_stream is not None:
            log_stream.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except TraceError as exc:
        print(f"object-behavior-trace: {exc}", file=sys.stderr)
        raise SystemExit(1)
