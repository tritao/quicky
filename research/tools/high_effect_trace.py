#!/usr/bin/env python3
"""Trace transient high-address object callbacks and target handoffs."""

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

from quikytrace import ApiClient, TraceError, discover_token, reserve_local_port, wait_for_api


@dataclass(frozen=True)
class HighEffectConfig:
    frames: int
    startup_recording: Path
    frame_step: int = 30
    timeout: float = 60.0
    poll_interval: float = 0.05
    select_level: str | None = None
    selector_frames: int = 60
    input_key: str = ""
    input_frames: int = 0
    input_samples: int = 0
    target_x_delta: int = 0
    target_y_delta: int = -10
    target_cursor_offset: int = 0x2A
    screenshot: Path | None = None
    screenshot_mode: str = "rendered"


def lua_config(config: HighEffectConfig) -> dict[str, Any]:
    return {
        "schema_version": 1,
        "timeout_ms": round(config.timeout * 1000),
        "frames": config.frames,
        "frame_step": config.frame_step,
        "select_level": config.select_level or "",
        "selector_frames": config.selector_frames,
        "input_key": config.input_key,
        "input_frames": config.input_frames,
        "input_samples": config.input_samples,
        "target_x_delta": config.target_x_delta,
        "target_y_delta": config.target_y_delta,
        "target_cursor_offset": config.target_cursor_offset,
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


def normalize_high_effect_trace(trace: dict[str, Any]) -> dict[str, Any]:
    trace["frames"] = ordered_lua_array(trace.get("frames", []))
    trace["callback_events"] = ordered_lua_array(trace.get("callback_events", []))
    trace["spawned_effect_events"] = ordered_lua_array(
        trace.get("spawned_effect_events", [])
    )
    for event in trace["callback_events"]:
        if isinstance(event, dict):
            event["related_hits"] = ordered_lua_array(event.get("related_hits", []))
    for frame in trace["frames"]:
        if isinstance(frame, dict):
            pool = frame.get("pool")
            if isinstance(pool, dict):
                for key in ("objects", "high_objects"):
                    pool[key] = ordered_lua_array(pool.get(key, []))
    return trace


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def trace_high_effect(
    api: ApiClient, script_path: Path, config: HighEffectConfig,
) -> dict[str, Any]:
    source = script_path.read_text(encoding="utf-8")
    name = urllib.parse.quote("quiky-high-effect-trace")
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
                f"Lua high-effect trace failed: {status.get('error', 'unknown error')}"
            )
        if not replayed and status.get("output", {}).get("awaiting_startup_replay"):
            api.post("/api/v1/input/sequence", {"events": recording["events"]})
            replayed = True
        result = status.get("output", {}).get("high_effect_trace")
        if isinstance(result, dict):
            normalized = normalize_high_effect_trace(result)
            if config.screenshot is not None:
                api.post("/api/v1/debug/continue")
                time.sleep(0.05)
                config.screenshot.parent.mkdir(parents=True, exist_ok=True)
                config.screenshot.write_bytes(api.get_binary(
                    "/api/v1/video/frame?format=png&mode=" + config.screenshot_mode
                ))
            return normalized
        if status.get("state") == "completed":
            raise TraceError("Lua high-effect trace completed without output")
        time.sleep(config.poll_interval)
    raise TraceError("timed out waiting for the Lua high-effect trace")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--frames", type=int, default=240)
    parser.add_argument("--frame-step", type=int, default=30)
    parser.add_argument("--timeout", type=float, default=60.0)
    parser.add_argument("--poll-interval", type=float, default=0.05)
    parser.add_argument("--select-level")
    parser.add_argument("--selector-frames", type=int, default=60)
    parser.add_argument("--input-key", default="")
    parser.add_argument("--input-frames", type=int, default=0)
    parser.add_argument("--input-samples", type=int, default=0)
    parser.add_argument("--target-x-delta", type=int, default=0)
    parser.add_argument("--target-y-delta", type=int, default=-10)
    parser.add_argument("--target-cursor-offset", type=lambda value: int(value, 0),
                        default=0x2A)
    parser.add_argument("--screenshot", type=Path,
                        help="capture the final DOSBox frame after the trace")
    parser.add_argument("--screenshot-mode", choices=("rendered", "raw"),
                        default="rendered")
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
    if args.frames < 1:
        raise TraceError("--frames must be positive")
    if args.frame_step < 1:
        raise TraceError("--frame-step must be positive")
    if args.select_level is not None and len(args.select_level) != 4:
        raise TraceError("--select-level must look like W4L1")
    if args.input_frames < 0 or args.input_samples < 0:
        raise TraceError("input frame counts must not be negative")
    for name in ("target_x_delta", "target_y_delta"):
        value = getattr(args, name)
        if not -0x8000 <= value <= 0x7fff:
            raise TraceError(f"--{name.replace('_', '-')} must be signed 16-bit")
    if not 0 <= args.target_cursor_offset <= 0xffff:
        raise TraceError("--target-cursor-offset must be between 0 and 65535")

    repo_root = Path(__file__).resolve().parents[2]
    startup_recording = args.startup_recording
    if not startup_recording.is_absolute():
        startup_recording = repo_root / startup_recording
    script_path = repo_root / "research/automation/quiky_high_effect_trace.lua"
    output = args.output if args.output.is_absolute() else repo_root / args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    screenshot = args.screenshot
    if screenshot is not None and not screenshot.is_absolute():
        screenshot = repo_root / screenshot
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
        config = HighEffectConfig(
            frames=args.frames,
            startup_recording=startup_recording,
            frame_step=args.frame_step,
            timeout=args.timeout,
            poll_interval=args.poll_interval,
            select_level=args.select_level,
            selector_frames=args.selector_frames,
            input_key=args.input_key,
            input_frames=args.input_frames,
            input_samples=args.input_samples,
            target_x_delta=args.target_x_delta,
            target_y_delta=args.target_y_delta,
            target_cursor_offset=args.target_cursor_offset,
            screenshot=screenshot,
            screenshot_mode=args.screenshot_mode,
        )
        trace = trace_high_effect(api, script_path, config)
        envelope = {
            "schema": "quiky-high-effect-trace-v1",
            "created_utc": datetime.now(timezone.utc).isoformat(),
            "dosbox": info,
            "trace_kind": "high-effect",
            "script": str(script_path),
            "script_sha256": sha256(script_path),
            "startup_recording": str(startup_recording),
            "startup_recording_sha256": sha256(startup_recording),
            "config": lua_config(config),
            "events": [trace],
        }
        output.write_text(json.dumps(envelope, indent=2) + "\n", encoding="utf-8")
        print(f"wrote high-effect trace to {output}")
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
        print(f"high-effect-trace: {exc}", file=sys.stderr)
        raise SystemExit(1)
