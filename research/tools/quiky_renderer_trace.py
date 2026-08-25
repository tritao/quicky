#!/usr/bin/env python3
"""Run the renderer-stage ordering trace for a selected level launch."""

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


def ordered(value):
    if isinstance(value, list):
        return value
    if isinstance(value, dict):
        try:
            return [value[key] for key in sorted(value, key=int)]
        except (TypeError, ValueError):
            return value
    return value


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--level", default="W4L1")
    parser.add_argument("--events", type=int, default=24)
    parser.add_argument("--timeout", type=float, default=10.0)
    parser.add_argument("--patch-map-flags", action="store_true",
                        help="debugger-only: set upper MAP cell flags on the first strip")
    parser.add_argument("--map-flag-mask", type=lambda value: int(value, 0),
                        default=0xfe00,
                        help="upper MAP bits to OR when --patch-map-flags is used")
    parser.add_argument("--rearm-special-bob", action="store_true",
                        help="rearm the special-object BOB list-builder breakpoint")
    parser.add_argument("--output", type=Path, required=True)
    return parser


def run(args: argparse.Namespace) -> int:
    repo_root = Path(__file__).resolve().parents[2]
    script_path = repo_root / "research/automation/quiky_renderer_order_trace.lua"
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
        source = script_path.read_text(encoding="utf-8")
        prefix = (
            f"TRACE_LEVEL={json.dumps(args.level)}\n"
            f"TRACE_EVENT_LIMIT={args.events}\n"
            f"TRACE_TIMEOUT_MS={round(args.timeout * 1000)}\n"
            f"TRACE_PATCH_MAP_FLAGS={'true' if args.patch_map_flags else 'false'}\n"
            f"TRACE_MAP_FLAG_MASK={args.map_flag_mask}\n"
            f"TRACE_REARM_SPECIAL_BOB={'true' if args.rearm_special_bob else 'false'}\n"
        )
        api.request("POST", "/api/v1/script/load?name=quiky-renderer-trace",
                    text_body=prefix + source)
        api.post("/api/v1/script/start")
        deadline = time.monotonic() + args.timeout * (args.events + 3) + 30
        replayed = False
        while time.monotonic() < deadline:
            status = api.get("/api/v1/script/status")
            if status.get("state") == "error":
                raise TraceError(status.get("error", "renderer trace failed"))
            if (not replayed and
                    status.get("output", {}).get("awaiting_startup_replay")):
                recording = json.loads(recording_path.read_text(encoding="utf-8"))
                api.post("/api/v1/input/sequence", recording)
                replayed = True
            if status.get("state") == "completed":
                output = status.get("output", {})
                ledger = {
                    "schema": "quiky-renderer-order-trace-v1",
                    "dosbox": info,
                    "inputs": {"level": args.level, "events": args.events,
                               "patch_map_flags": args.patch_map_flags,
                               "map_flag_mask": args.map_flag_mask,
                               "rearm_special_bob": args.rearm_special_bob},
                    "executable": str(repo_root / "game/QUIKY.EXE"),
                    "executable_sha256": sha256(repo_root / "game/QUIKY.EXE"),
                    "archive": str(repo_root / "game/NESTLE.DAT"),
                    "archive_sha256": sha256(repo_root / "game/NESTLE.DAT"),
                    "script": str(script_path),
                    "script_sha256": sha256(script_path),
                    "checkpoints": output.get("checkpoints", {}),
                    "renderer_events": ordered(output.get("renderer_events", [])),
                }
                args.output.parent.mkdir(parents=True, exist_ok=True)
                args.output.write_text(json.dumps(ledger, indent=2) + "\n",
                                       encoding="utf-8")
                print(f"wrote renderer trace to {args.output}")
                return 0
            time.sleep(0.05)
        raise TraceError("timed out waiting for renderer trace")
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
    if args.events < 1:
        raise SystemExit("--events must be positive")
    try:
        return run(args)
    except TraceError as exc:
        print(f"quiky_renderer_trace: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
