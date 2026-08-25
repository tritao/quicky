#!/usr/bin/env python3
"""Capture protected-mode palette helpers before game launch."""

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
    parser.add_argument("--events", type=int, default=1,
                        help="palette helper calls to collect")
    parser.add_argument("--capture-page-start", action="store_true",
                        help="also capture native VGA CRTC page-start writes")
    parser.add_argument("--output", type=Path, required=True)
    return parser


def run(args: argparse.Namespace) -> int:
    repo_root = Path(__file__).resolve().parents[2]
    script_path = repo_root / "research/automation/quiky_early_palette_trace.lua"
    port = reserve_local_port()
    token = secrets.token_hex(32)
    env = os.environ.copy()
    env["DOSBOX_API_TOKEN"] = token
    log_path = args.output.with_suffix(args.output.suffix + ".dosbox.log")
    log_path.parent.mkdir(parents=True, exist_ok=True)
    command = [
        str(repo_root / "scripts/run-dosbox-automation.sh"),
        "--set", f"webserver_port={port}",
        "-c", "pause",
    ]
    with log_path.open("wb") as log_stream:
        process = subprocess.Popen(
            command,
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
        api.request(
            "POST",
            "/api/v1/script/load?name=quiky-early-palette-trace",
            text_body=(
                f"TRACE_TIMEOUT_MS={round(args.timeout * 1000)}\n"
                f"TRACE_EVENT_LIMIT={args.events}\n"
                f"TRACE_CAPTURE_PAGE_START={'true' if args.capture_page_start else 'false'}\n"
                + script_path.read_text(encoding="utf-8")
            ),
        )
        api.post("/api/v1/script/start")
        deadline = time.monotonic() + args.timeout * (args.events + 2) + 30
        while time.monotonic() < deadline:
            status = api.get("/api/v1/script/status")
            if status.get("state") == "error":
                raise TraceError(status.get("error", "early palette trace failed"))
            if status.get("state") == "completed":
                output = status.get("output", {})
                ledger = {
                    "schema": "quiky-early-palette-trace-v1",
                    "dosbox": info,
                    "inputs": {
                        "events": args.events,
                        "capture_page_start": args.capture_page_start,
                        "executable": str(repo_root / "game/QUIKY.EXE"),
                        "executable_sha256": sha256(repo_root / "game/QUIKY.EXE"),
                        "archive": str(repo_root / "game/NESTLE.DAT"),
                        "archive_sha256": sha256(repo_root / "game/NESTLE.DAT"),
                    },
                    "engine": "lua-debugger-api",
                    "script": str(script_path),
                    "script_sha256": sha256(script_path),
                    "first_palette_event": output.get("first_palette_event", {}),
                    "palette_events": output.get("palette_events", []),
                }
                args.output.parent.mkdir(parents=True, exist_ok=True)
                args.output.write_text(json.dumps(ledger, indent=2) + "\n",
                                       encoding="utf-8")
                print(f"wrote early palette trace to {args.output}")
                return 0
            time.sleep(0.05)
        raise TraceError("timed out waiting for the early palette trace")
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
    if args.events < 1:
        raise SystemExit("--events must be positive")
    try:
        return run(args)
    except TraceError as exc:
        print(f"quikyearlypalette: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
