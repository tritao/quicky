#!/usr/bin/env python3
"""Run the debugger-side palette upload trace for a selected level launch."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import secrets
import subprocess
import sys
import time
from pathlib import Path

from quikytrace import ApiClient, TraceError, reserve_local_port, wait_for_api


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def normalize_json_arrays(value):
    """Turn Lua's numeric-key tables into ordered JSON arrays."""
    if isinstance(value, dict):
        normalized = {key: normalize_json_arrays(item)
                      for key, item in value.items()}
        numeric = []
        for key in normalized:
            try:
                number = int(key)
            except (TypeError, ValueError):
                break
            if str(number) != str(key) or number < 1:
                break
            numeric.append((number, normalized[key]))
        else:
            if sorted(number for number, _ in numeric) == list(
                    range(1, len(numeric) + 1)):
                return [item for _, item in sorted(numeric)]
        return normalized
    if isinstance(value, list):
        return [normalize_json_arrays(item) for item in value]
    return value


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--level", default="W4L1", help="level selector target, e.g. W4L1")
    parser.add_argument("--samples", type=int, default=8)
    parser.add_argument(
        "--startup-samples", type=int, default=0,
        help="optional palette events to capture before the selector replay",
    )
    parser.add_argument(
        "--callsite", action="store_true",
        help="trace the fade helper's 0207:0924 call site instead of DAC I/O",
    )
    parser.add_argument("--post-input-key",
                        help="hold a keyboard key after launch while tracing palette writes")
    parser.add_argument("--post-input-frames", type=int, default=0,
                        help="guest frames to hold --post-input-key (0 disables phase)")
    parser.add_argument("--post-samples", type=int, default=8,
                        help="palette events to collect during post-input phase")
    parser.add_argument("--timeout", type=float, default=30.0)
    parser.add_argument("--output", type=Path, required=True)
    return parser


def run(args: argparse.Namespace) -> int:
    repo_root = Path(__file__).resolve().parents[2]
    script_path = repo_root / "research/automation/quiky_palette_trace.lua"
    recording_path = repo_root / "research/automation/startup-to-input.json"
    port = reserve_local_port()
    token = secrets.token_hex(32)
    env = os.environ.copy()
    env["DOSBOX_API_TOKEN"] = token
    log_path = args.output.with_suffix(args.output.suffix + ".dosbox.log")
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("wb") as log_stream:
        process = subprocess.Popen(
            [str(repo_root / "scripts/run-dosbox-automation.sh"),
             "--set", f"webserver_port={port}"],
            cwd=repo_root,
            env=env,
            stdout=log_stream,
            stderr=subprocess.STDOUT,
        )
    api = ApiClient(f"http://127.0.0.1:{port}", token)
    try:
        info = wait_for_api(api, process, 15.0)
        if not info.get("features", {}).get("debugger"):
            raise TraceError("the running dosbox-automation build has no debugger API")
        source = script_path.read_text(encoding="utf-8")
        prefix = (
            f"TRACE_LEVEL={json.dumps(args.level)}\n"
            f"TRACE_SAMPLE_COUNT={args.samples}\n"
            f"TRACE_STARTUP_SAMPLES={args.startup_samples}\n"
            f"TRACE_PALETTE_CALLSITE={'true' if args.callsite else 'false'}\n"
            f"TRACE_POST_INPUT_KEY={json.dumps(args.post_input_key or '')}\n"
            f"TRACE_POST_INPUT_FRAMES={args.post_input_frames}\n"
            f"TRACE_POST_SAMPLES={args.post_samples}\n"
            f"TRACE_TIMEOUT_MS={round(args.timeout * 1000)}\n"
        )
        api.request(
            "POST",
            "/api/v1/script/load?name=quiky-palette-trace",
            text_body=prefix + source,
        )
        api.post("/api/v1/script/start")
        deadline = time.monotonic() + args.timeout * (args.samples + 3) + 30
        replayed = False
        while time.monotonic() < deadline:
            status = api.get("/api/v1/script/status")
            if status.get("state") == "error":
                raise TraceError(status.get("error", "palette trace failed"))
            if (not replayed and
                    status.get("output", {}).get("awaiting_startup_replay")):
                recording = json.loads(recording_path.read_text(encoding="utf-8"))
                api.post("/api/v1/input/sequence", recording)
                replayed = True
            if status.get("state") == "completed":
                output = status.get("output", {})
                ledger = {
                    "schema": "quiky-palette-trace-v1",
                    "dosbox": info,
                    "inputs": {
                        "level": args.level,
                        "samples": args.samples,
                        "executable": str(repo_root / "game/QUIKY.EXE"),
                        "executable_sha256": sha256(repo_root / "game/QUIKY.EXE"),
                        "archive": str(repo_root / "game/NESTLE.DAT"),
                        "archive_sha256": sha256(repo_root / "game/NESTLE.DAT"),
                    },
                    "engine": "lua-debugger-api",
                    "script": str(script_path),
                    "script_sha256": sha256(script_path),
                    "startup_recording": str(recording_path),
                    "startup_recording_sha256": sha256(recording_path),
                    "checkpoints": output.get("checkpoints", {}),
                    "startup_palette_events": normalize_json_arrays(
                        output.get("startup_palette_events", [])),
                    "palette_events": normalize_json_arrays(
                        output.get("palette_events", [])),
                    "post_palette_events": normalize_json_arrays(
                        output.get("post_palette_events", [])),
                }
                args.output.parent.mkdir(parents=True, exist_ok=True)
                args.output.write_text(json.dumps(ledger, indent=2) + "\n",
                                       encoding="utf-8")
                print(f"wrote palette trace to {args.output}")
                return 0
            time.sleep(0.05)
        raise TraceError("timed out waiting for the palette trace")
    finally:
        try:
            api.post("/api/v1/control/shutdown")
        except Exception:
            process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.terminate()
            process.wait(timeout=3)


def main() -> int:
    args = build_parser().parse_args()
    if len(args.level) != 4 or args.level[0] != "W" or args.level[2] != "L":
        raise SystemExit("--level must look like W4L1")
    if args.samples < 1:
        raise SystemExit("--samples must be positive")
    if args.startup_samples < 0:
        raise SystemExit("--startup-samples cannot be negative")
    if args.post_input_frames < 0:
        raise SystemExit("--post-input-frames cannot be negative")
    if args.post_samples < 1:
        raise SystemExit("--post-samples must be positive")
    try:
        return run(args)
    except TraceError as exc:
        print(f"quikypalette: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
