#!/usr/bin/env python3
"""Run a low-overhead B33B linked-object phase/lifecycle sample."""

from __future__ import annotations

import argparse
import json
import os
import secrets
import subprocess
import time
import urllib.parse
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from high_effect_trace import lua_literal, sha256
from quikytrace import ApiClient, TraceError, discover_token, reserve_local_port, wait_for_api


@dataclass(frozen=True)
class PhaseLifecycleConfig:
    startup_recording: Path
    sample_count: int = 120
    sample_interval: int = 1
    warmup_frames: int = 0
    timeout: float = 300.0
    select_level: str = "W1L3"
    selector_frames: int = 60
    scan_limit: int = 16
    force_phase: int | None = None
    force_transition: int | None = None
    force_x: int | None = None
    force_y: int | None = None


def lua_config(config: PhaseLifecycleConfig) -> dict[str, Any]:
    return {
        "schema_version": 1,
        "timeout_ms": round(config.timeout * 1000),
        "select_level": config.select_level,
        "selector_frames": config.selector_frames,
        "sample_count": config.sample_count,
        "sample_interval": config.sample_interval,
        "warmup_frames": config.warmup_frames,
        "scan_limit": config.scan_limit,
        "force_phase": config.force_phase,
        "force_transition": config.force_transition,
        "force_x": config.force_x,
        "force_y": config.force_y,
    }


def trace_lifecycle(api: ApiClient, script_path: Path,
                    config: PhaseLifecycleConfig) -> dict[str, Any]:
    source = script_path.read_text(encoding="utf-8")
    name = urllib.parse.quote("quiky-phase-lifecycle-trace")
    prefix = "TRACE_CONFIG = " + lua_literal(lua_config(config)) + "\n"
    api.request("POST", f"/api/v1/script/load?name={name}",
                text_body=prefix + source)
    api.post("/api/v1/script/start")
    recording = json.loads(config.startup_recording.read_text(encoding="utf-8"))
    deadline = time.monotonic() + config.timeout + 30
    replayed = False
    while time.monotonic() < deadline:
        status = api.get("/api/v1/script/status")
        if status.get("state") == "error":
            raise TraceError(
                f"Lua phase-lifecycle trace failed: {status.get('error', 'unknown error')}"
            )
        if not replayed and status.get("output", {}).get("awaiting_startup_replay"):
            api.post("/api/v1/input/sequence", {"events": recording["events"]})
            replayed = True
        result = status.get("output", {}).get("phase_lifecycle_trace")
        if isinstance(result, dict):
            return result
        if status.get("state") == "completed":
            raise TraceError("phase-lifecycle script completed without output")
        time.sleep(0.1)
    raise TraceError("timed out waiting for the Lua phase-lifecycle trace")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--runtime-dir", type=Path, default=Path("game"))
    parser.add_argument("--startup-recording", type=Path,
                        default=Path("research/automation/startup-to-input.json"))
    parser.add_argument("--select-level", default="W1L3")
    parser.add_argument("--samples", type=int, default=120)
    parser.add_argument("--sample-interval", type=int, default=1)
    parser.add_argument("--warmup-frames", type=int, default=0)
    parser.add_argument("--scan-limit", type=int, default=16)
    parser.add_argument("--force-phase", type=lambda value: int(value, 0))
    parser.add_argument("--force-transition", type=lambda value: int(value, 0))
    parser.add_argument("--force-x", type=int)
    parser.add_argument("--force-y", type=int)
    parser.add_argument("--timeout", type=float, default=300.0)
    parser.add_argument("--headless", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.samples < 1 or args.sample_interval < 1 or args.warmup_frames < 0:
        raise TraceError("sample counts and intervals must be positive; warmup cannot be negative")
    if not 1 <= args.scan_limit <= 64:
        raise TraceError("--scan-limit must be between 1 and 64")
    if args.force_phase is not None and not 0 <= args.force_phase <= 0xff:
        raise TraceError("--force-phase must be between 0 and 255")
    if args.force_transition is not None and not 0 <= args.force_transition <= 0xff:
        raise TraceError("--force-transition must be between 0 and 255")
    if (args.force_x is None) != (args.force_y is None):
        raise TraceError("--force-x and --force-y must be used together")
    for name in ("force_x", "force_y"):
        value = getattr(args, name)
        if value is not None and not -0x8000 <= value <= 0x7fff:
            raise TraceError(f"--{name.replace('_', '-')} must be signed 16-bit")
    repo_root = Path(__file__).resolve().parents[2]
    output = args.output if args.output.is_absolute() else repo_root / args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    startup = args.startup_recording
    if not startup.is_absolute(): startup = repo_root / startup
    runtime_dir = args.runtime_dir
    if not runtime_dir.is_absolute():
        runtime_dir = repo_root / runtime_dir
    runtime_dir = runtime_dir.resolve()
    if not (runtime_dir / "QUIKY.EXE").is_file() or not (runtime_dir / "NESTLE.DAT").is_file():
        raise TraceError("--runtime-dir must contain QUIKY.EXE and NESTLE.DAT")
    port = reserve_local_port()
    token = secrets.token_hex(32)
    env = os.environ.copy()
    env["DOSBOX_API_TOKEN"] = token
    env["QUIKY_AUTOMATION_TARGET"] = str(runtime_dir / "QUIKY.EXE")
    env["DOSBOX_AUTOMATION_DATA_HOME"] = str(runtime_dir.parent)
    if args.headless:
        env["SDL_VIDEODRIVER"] = "dummy"
        env["SDL_AUDIODRIVER"] = "dummy"
    log_path = output.with_suffix(output.suffix + ".dosbox.log")
    with log_path.open("wb") as log_stream:
        command = [str(repo_root / "scripts/run-dosbox-automation.sh"),
                   "--set", f"webserver_port={port}",
                   "--set", f"mount_allowed_bases={runtime_dir}",
                   "--set", f"mount_allowed_image_roots={runtime_dir}"]
        process = subprocess.Popen(command, cwd=repo_root, env=env,
                                   stdout=log_stream, stderr=subprocess.STDOUT)
        api = ApiClient(f"http://127.0.0.1:{port}", token)
        try:
            info = wait_for_api(api, process, 20.0)
            if not info.get("features", {}).get("debugger"):
                raise TraceError("running dosbox-automation build has no debugger API")
            config = PhaseLifecycleConfig(
                startup_recording=startup,
                sample_count=args.samples,
                sample_interval=args.sample_interval,
                warmup_frames=args.warmup_frames,
                timeout=args.timeout,
                select_level=args.select_level,
                scan_limit=args.scan_limit,
                force_phase=args.force_phase,
                force_transition=args.force_transition,
                force_x=args.force_x,
                force_y=args.force_y,
            )
            result = trace_lifecycle(
                api, repo_root / "research/automation/quiky_phase_lifecycle_trace.lua",
                config,
            )
            envelope = {
                "schema": "quiky-phase-lifecycle-trace-v1",
                "created_utc": datetime.now(timezone.utc).isoformat(),
                "dosbox": info,
                "script": str(repo_root / "research/automation/quiky_phase_lifecycle_trace.lua"),
                "script_sha256": sha256(repo_root / "research/automation/quiky_phase_lifecycle_trace.lua"),
                "startup_recording": str(startup),
                "startup_recording_sha256": sha256(startup),
                "config": lua_config(config),
                "trace": result,
            }
            output.write_text(json.dumps(envelope, indent=2) + "\n", encoding="utf-8")
            print(f"wrote phase-lifecycle trace to {output}")
            return 0
        finally:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)


if __name__ == "__main__":
    raise SystemExit(main())
