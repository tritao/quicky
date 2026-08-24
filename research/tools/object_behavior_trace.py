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
    followup_passes: int = 0
    capture_pool: bool = True
    reactivate_camera_x: int | None = None
    reactivate_camera_y: int | None = None
    movement_key: str = ""


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
        "followup_passes": config.followup_passes,
        "capture_pool": config.capture_pool,
        "reactivate_camera_x": config.reactivate_camera_x,
        "reactivate_camera_y": config.reactivate_camera_y,
        "movement_key": config.movement_key,
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


def normalize_pool_tables(pool: Any) -> Any:
    if not isinstance(pool, dict):
        return pool
    banks = ordered_lua_array(pool.get("banks", []))
    for bank in banks:
        if isinstance(bank, dict):
            bank["entries"] = ordered_lua_array(bank.get("entries", []))
    pool["banks"] = banks
    return pool


def normalize_behavior_trace(trace: dict[str, Any]) -> dict[str, Any]:
    samples = ordered_lua_array(trace.get("samples", []))
    for sample in samples:
        if isinstance(sample, dict):
            sample["changed_bytes"] = ordered_lua_array(
                sample.get("changed_bytes", [])
            )
            for pool_name in ("pool_before", "pool_after"):
                normalize_pool_tables(sample.get(pool_name))
    trace["samples"] = samples

    followup_passes = ordered_lua_array(trace.get("followup_passes", []))
    for followup in followup_passes:
        if not isinstance(followup, dict):
            continue
        followup["entries"] = ordered_lua_array(followup.get("entries", []))
        for pool_name in ("pool", "end_pool"):
            normalize_pool_tables(followup.get(pool_name))
    trace["followup_passes"] = followup_passes

    reactivation = trace.get("reactivation")
    if isinstance(reactivation, dict):
        reactivation["stream_entries"] = ordered_lua_array(
            reactivation.get("stream_entries", [])
        )
        reactivation["declaration_call_sites"] = ordered_lua_array(
            reactivation.get("declaration_call_sites", [])
        )
        for snapshot_name in ("before", "after"):
            snapshot = reactivation.get(snapshot_name)
            if isinstance(snapshot, dict):
                normalize_pool_tables(snapshot.get("pool"))
        normalize_pool_tables(reactivation.get("initialized_pool"))
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
    parser.add_argument("--followup-passes", type=int, default=0,
                        help="capture scheduler entries through this many later passes")
    parser.add_argument("--no-pool-snapshots", dest="capture_pool",
                        action="store_false",
                        help="omit the expensive 64-entry pool snapshots")
    parser.add_argument("--reactivate-camera-x", type=int,
                        help="write this camera X after a rejected object and trace its ARE reactivation")
    parser.add_argument("--reactivate-camera-y", type=int,
                        help="write this camera Y after a rejected object and trace its ARE reactivation")
    parser.add_argument("--movement-key",
                        help="hold a guest key while callback samples run")
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
    if args.followup_passes < 0:
        raise TraceError("--followup-passes must not be negative")
    if not 0 <= args.entity_type <= 0xff:
        raise TraceError("--entity-type must be between 0 and 255")
    if args.select_level is not None and len(args.select_level) != 4:
        raise TraceError("--select-level must look like W4L1")
    if (args.camera_x is None) != (args.camera_y is None):
        raise TraceError("--camera-x and --camera-y must be used together")
    if (args.reactivate_camera_x is None) != (args.reactivate_camera_y is None):
        raise TraceError("--reactivate-camera-x and --reactivate-camera-y must be used together")
    for name in ("camera_x", "camera_y"):
        value = getattr(args, name)
        if value is not None and not 0 <= value <= 0xffff:
            raise TraceError(f"--{name.replace('_', '-')} must be between 0 and 65535")
    for name in ("reactivate_camera_x", "reactivate_camera_y"):
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
            followup_passes=args.followup_passes,
            capture_pool=args.capture_pool,
            reactivate_camera_x=args.reactivate_camera_x,
            reactivate_camera_y=args.reactivate_camera_y,
            movement_key=args.movement_key or "",
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
