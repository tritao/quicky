#!/usr/bin/env python3
"""Capture the native PCC palette staging/ramp/DAC setup chain."""

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


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--timeout", type=float, default=30.0)
    parser.add_argument("--events", type=int, default=3)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--skip-startup-replay", action="store_true",
                        help="leave the title/intro input untouched instead of replaying startup-to-input.json")
    return parser


def run(args: argparse.Namespace) -> int:
    if args.timeout <= 0 or args.events < 1:
        raise TraceError("timeout must be positive and events must be at least one")
    repo_root = Path(__file__).resolve().parents[2]
    script_path = repo_root / "research/automation/quiky_palette_owner_trace.lua"
    recording_path = repo_root / "research/automation/startup-to-input.json"
    port = reserve_local_port()
    token = secrets.token_hex(32)
    env = os.environ.copy()
    env["DOSBOX_API_TOKEN"] = token
    log_path = args.output.with_suffix(args.output.suffix + ".dosbox.log")
    log_path.parent.mkdir(parents=True, exist_ok=True)
    process = subprocess.Popen(
        [str(repo_root / "scripts/run-dosbox-automation.sh"),
         "--set", f"webserver_port={port}", "-c", "pause"],
        cwd=repo_root, env=env,
        stdout=log_path.open("wb"), stderr=subprocess.STDOUT,
    )
    api = ApiClient(f"http://127.0.0.1:{port}", token)
    try:
        info = wait_for_api(api, process, 15.0)
        if not info.get("features", {}).get("debugger"):
            raise TraceError("the running DOSBox build has no debugger API")
        api.request(
            "POST", "/api/v1/script/load?name=quiky-palette-owner-trace",
            text_body=(f"TRACE_TIMEOUT_MS={round(args.timeout * 1000)}\n"
                       f"TRACE_EVENT_LIMIT={args.events}\n"
                       + script_path.read_text(encoding="utf-8")),
        )
        api.post("/api/v1/script/start")
        deadline = time.monotonic() + args.timeout * (args.events + 2) + 30
        replayed = args.skip_startup_replay
        while time.monotonic() < deadline:
            status = api.get("/api/v1/script/status")
            if status.get("state") == "error":
                raise TraceError(status.get("error", "palette owner trace failed"))
            output = status.get("output", {})
            if not replayed and output.get("ready_for_release"):
                api.post("/api/v1/input/sequence",
                         json.loads(recording_path.read_text(encoding="utf-8")))
                replayed = True
            if status.get("state") == "completed":
                target = repo_root / "game/QUIKY.EXE"
                archive = repo_root / "game/NESTLE.DAT"
                ledger = {
                    "schema": "quiky-palette-owner-trace-v1",
                    "dosbox": info,
                    "executable": str(target),
                    "executable_sha256": sha256(target),
                    "archive": str(archive),
                    "archive_sha256": sha256(archive),
                    "script": str(script_path),
                    "script_sha256": sha256(script_path),
                    "events": output.get("palette_owner_events", []),
                    "events_requested": args.events,
                    "timeout": args.timeout,
                }
                args.output.write_text(json.dumps(ledger, indent=2) + "\n",
                                       encoding="utf-8")
                print(f"wrote palette owner trace to {args.output}")
                return 0
            time.sleep(0.05)
        raise TraceError("timed out waiting for palette owner trace")
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
    try:
        return run(build_parser().parse_args())
    except TraceError as exc:
        print(f"quiky_palette_owner_trace: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
