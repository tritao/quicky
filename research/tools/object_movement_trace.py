#!/usr/bin/env python3
"""Drive a live ARE object with real keyboard input and capture its pool state."""

from __future__ import annotations

import argparse
import json
import os
import secrets
import subprocess
import sys
import time
import urllib.parse
from datetime import datetime, timezone
from pathlib import Path

from quikytrace import (
    ApiClient,
    EntityTraceConfig,
    TraceError,
    discover_token,
    entity_trace_lua_config,
    lua_literal,
    normalize_entity_trace,
    reserve_local_port,
    sha256,
    wait_for_api,
)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--record-offset", type=lambda value: int(value, 0),
                        default=0x1792)
    parser.add_argument("--entity-type", type=lambda value: int(value, 0),
                        default=0x2B)
    parser.add_argument("--capture-frames", type=int, default=32)
    parser.add_argument("--frame-step", type=int, default=5)
    parser.add_argument("--movement-key", default="KBD_right")
    parser.add_argument("--movement-frames", type=int, default=240)
    parser.add_argument("--timeout", type=float, default=90.0)
    parser.add_argument("--poll-interval", type=float, default=0.05)
    parser.add_argument("--select-level")
    parser.add_argument("--selector-frames", type=int, default=60)
    parser.add_argument("--startup-recording", type=Path,
                        default=Path("research/automation/startup-to-input.json"))
    parser.add_argument("--url", default="http://127.0.0.1:8386")
    parser.add_argument("--token-file", type=Path)
    parser.add_argument("--launch", action="store_true")
    parser.add_argument("--headless", action="store_true")
    parser.add_argument("--startup-timeout", type=float, default=15.0)
    parser.add_argument("--runtime-dir", type=Path)
    return parser


def run_trace(api: ApiClient, script_path: Path, config: EntityTraceConfig,
              movement_key: str, movement_frames: int) -> dict:
    source = script_path.read_text(encoding="utf-8")
    name = urllib.parse.quote("quiky-object-movement-trace")
    prefix = "TRACE_CONFIG = " + lua_literal(entity_trace_lua_config(config)) + "\n"
    api.request("POST", f"/api/v1/script/load?name={name}",
                text_body=prefix + source)
    api.post("/api/v1/script/start")
    recording = json.loads(config.startup_recording.read_text(encoding="utf-8"))
    deadline = time.monotonic() + config.timeout + 30
    replayed = False
    acknowledged: set[int] = set()
    movement_sent = False
    final_entity: dict | None = None

    while time.monotonic() < deadline:
        status = api.get("/api/v1/script/status")
        if status.get("state") == "error":
            raise TraceError(
                f"Lua object movement trace failed: {status.get('error', 'unknown error')}"
            )
        if not replayed and status.get("output", {}).get("awaiting_startup_replay"):
            api.post("/api/v1/input/sequence", {"events": recording["events"]})
            replayed = True

        entity = status.get("output", {}).get("entity")
        if isinstance(entity, dict):
            final_entity = entity
            capture_index = entity.get("capture_index")
            if isinstance(capture_index, int) and capture_index not in acknowledged:
                if capture_index == 0 and not movement_sent:
                    api.post("/api/v1/input/sequence", {"events": [
                        {"frame": 0, "type": "key", "key": movement_key,
                         "pressed": True},
                        {"frame": movement_frames, "type": "key",
                         "key": movement_key, "pressed": False},
                    ]})
                    movement_sent = True
                acknowledged.add(capture_index)
                api.post("/api/v1/debug/continue")
            if capture_index == config.capture_frames - 1:
                return normalize_entity_trace(final_entity)
        if status.get("state") == "completed" and final_entity is not None:
            return normalize_entity_trace(final_entity)
        time.sleep(config.poll_interval)
    raise TraceError("timed out waiting for the object movement trace")


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.capture_frames < 1:
        raise TraceError("--capture-frames must be positive")
    if args.frame_step < 0 or args.movement_frames < 1:
        raise TraceError("--frame-step must be nonnegative and --movement-frames positive")
    if args.select_level is not None and len(args.select_level) != 4:
        raise TraceError("--select-level must look like W4L1")

    repo_root = Path(__file__).resolve().parents[2]
    startup_recording = args.startup_recording
    if not startup_recording.is_absolute():
        startup_recording = repo_root / startup_recording
    output = args.output if args.output.is_absolute() else repo_root / args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    script_path = repo_root / "research/automation/quiky_entity_trace.lua"
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
        log_stream = output.with_suffix(output.suffix + ".dosbox.log").open("wb")
        command = [str(repo_root / "scripts/run-dosbox-automation.sh"),
                   "--set", f"webserver_port={port}"]
        if args.runtime_dir is not None:
            runtime_dir = args.runtime_dir.resolve()
            command.extend(["--set", f"mount_allowed_bases={runtime_dir}",
                            "--set", f"mount_allowed_image_roots={runtime_dir}"])
        process = subprocess.Popen(command, cwd=repo_root, env=env,
                                   stdout=log_stream, stderr=subprocess.STDOUT)
        api = ApiClient(f"http://127.0.0.1:{port}", token)
        info = wait_for_api(api, process, args.startup_timeout)
    else:
        api = ApiClient(args.url, discover_token(args.token_file))
        info = api.get("/api/v1/dosbox/info")

    try:
        if not info.get("features", {}).get("debugger"):
            raise TraceError("the running dosbox-automation build has no debugger API")
        config = EntityTraceConfig(
            record_offset=args.record_offset,
            entity_type=args.entity_type,
            startup_recording=startup_recording,
            timeout=args.timeout,
            poll_interval=args.poll_interval,
            capture_frames=args.capture_frames,
            frame_step=args.frame_step,
            select_level=args.select_level,
            selector_frames=args.selector_frames,
        )
        entity = run_trace(api, script_path, config,
                           args.movement_key, args.movement_frames)
        envelope = {
            "schema": "quiky-object-movement-v1",
            "created_utc": datetime.now(timezone.utc).isoformat(),
            "dosbox": info,
            "trace_kind": "object-movement",
            "script": str(script_path),
            "script_sha256": sha256(script_path),
            "startup_recording": str(startup_recording),
            "startup_recording_sha256": sha256(startup_recording),
            "config": {
                "record_offset": config.record_offset,
                "entity_type": config.entity_type,
                "capture_frames": config.capture_frames,
                "frame_step": config.frame_step,
                "movement_key": args.movement_key,
                "movement_frames": args.movement_frames,
                "select_level": config.select_level or "",
            },
            "events": [entity],
        }
        output.write_text(json.dumps(envelope, indent=2) + "\n", encoding="utf-8")
        print(f"wrote object movement trace to {output}")
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
        print(f"object-movement-trace: {exc}", file=sys.stderr)
        raise SystemExit(1)
