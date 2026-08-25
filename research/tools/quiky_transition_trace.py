#!/usr/bin/env python3
"""Capture title/menu/launch transition checkpoints and stopped frames."""

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
    """Turn the debugger's numeric-key table into a stable JSON list."""
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
    parser.add_argument("--timeout", type=float, default=15.0)
    parser.add_argument("--menu-font-limit", type=int, default=32)
    parser.add_argument("--startup-status-font-limit", type=int, default=0,
                        help="capture 03F4 numeric/status calls during title/menu startup")
    parser.add_argument("--startup-palette-limit", type=int, default=0,
                        help="capture startup DAC/CRTC writes before the settled menu checkpoint")
    parser.add_argument("--pause-before-launch", action="store_true",
                        help="start DOSBox paused so deferred menu setup breakpoints arm before QUIKY.EXE")
    parser.add_argument("--capture-menu-setup", action="store_true",
                        help="stop at the optional one-shot 0470 menu-label setup helper")
    parser.add_argument("--menu-key", default="",
                        help="optional key to press at menu-ready while palette-owner breakpoints are armed")
    parser.add_argument("--menu-nav-key", default="",
                        help="optional arrow key to press at menu-ready while fixed-label callsites are armed")
    parser.add_argument("--menu-nav-mask", default="",
                        help="optional selector action mask (hex accepted) to inject after menu input refresh")
    parser.add_argument("--menu-nav-only", action="store_true",
                        help="stop the trace after the menu navigation experiment")
    parser.add_argument("--output", type=Path, required=True)
    return parser


def run(args: argparse.Namespace) -> int:
    repo_root = Path(__file__).resolve().parents[2]
    script_path = repo_root / "research/automation/quiky_transition_trace.lua"
    recording_path = repo_root / "research/automation/startup-to-input.json"
    port = reserve_local_port()
    token = secrets.token_hex(32)
    env = os.environ.copy()
    env["DOSBOX_API_TOKEN"] = token
    log_path = args.output.with_suffix(args.output.suffix + ".dosbox.log")
    frame_dir = args.output.parent / (args.output.stem + "-frames")
    frame_dir.mkdir(parents=True, exist_ok=True)
    with log_path.open("wb") as log_stream:
        command = [str(repo_root / "scripts/run-dosbox-automation.sh"),
                   "--set", f"webserver_port={port}"]
        if args.pause_before_launch:
            command += ["-c", "pause"]
        process = subprocess.Popen(
            command,
            cwd=repo_root,
            env=env,
            stdout=log_stream,
            stderr=subprocess.STDOUT,
        )
    api = ApiClient(f"http://127.0.0.1:{port}", token)
    captured: dict[str, str] = {}
    released: set[str] = set()
    try:
        info = wait_for_api(api, process, 15.0)
        source = script_path.read_text(encoding="utf-8")
        script_prefix = (f"TRACE_TIMEOUT_MS={round(args.timeout * 1000)}\n"
                         f"TRACE_MENU_FONT_LIMIT={args.menu_font_limit}\n"
                         f"TRACE_STARTUP_STATUS_FONT_LIMIT={args.startup_status_font_limit}\n"
                         f"TRACE_STARTUP_PALETTE_LIMIT={args.startup_palette_limit}\n")
        if args.pause_before_launch:
            script_prefix += "TRACE_PAUSE_BEFORE_LAUNCH=true\n"
        if args.capture_menu_setup:
            script_prefix += "TRACE_CAPTURE_MENU_SETUP=true\n"
        if args.menu_key:
            script_prefix += f"TRACE_MENU_KEY={json.dumps(args.menu_key)}\n"
        if args.menu_nav_key:
            script_prefix += f"TRACE_MENU_NAV_KEY={json.dumps(args.menu_nav_key)}\n"
        if args.menu_nav_mask:
            int(args.menu_nav_mask, 0)
            script_prefix += f"TRACE_MENU_NAV_MASK={json.dumps(args.menu_nav_mask)}\n"
        if args.menu_nav_only:
            script_prefix += "TRACE_MENU_NAV_ONLY=true\n"
        api.request(
            "POST", "/api/v1/script/load?name=quiky-transition-trace",
            text_body=script_prefix + source,
        )
        api.post("/api/v1/script/start")
        recording = json.loads(recording_path.read_text(encoding="utf-8"))
        deadline = time.monotonic() + args.timeout * 8 + 45
        replayed = False
        while time.monotonic() < deadline:
            status = api.get("/api/v1/script/status")
            if status.get("state") == "error":
                raise TraceError(status.get("error", "transition trace failed"))
            output = status.get("output", {})
            if not replayed and output.get("awaiting_startup_replay"):
                api.post("/api/v1/input/sequence", recording)
                replayed = True
            checkpoint = output.get("transition_checkpoint")
            if isinstance(checkpoint, dict):
                name = checkpoint.get("name", "checkpoint")
                if name not in released:
                    frame_path = frame_dir / f"{name}.png"
                    frame_path.write_bytes(api.get_binary(
                        "/api/v1/video/frame?format=png&mode=rendered"))
                    captured[name] = str(frame_path)
                    released.add(name)
                    api.post("/api/v1/debug/continue")
            if status.get("state") == "completed":
                ledger = {
                    "schema": "quiky-transition-trace-v1",
                    "dosbox": info,
                    "executable": str(repo_root / "game/QUIKY.EXE"),
                    "executable_sha256": sha256(repo_root / "game/QUIKY.EXE"),
                    "archive": str(repo_root / "game/NESTLE.DAT"),
                    "archive_sha256": sha256(repo_root / "game/NESTLE.DAT"),
                    "script": str(script_path),
                    "script_sha256": sha256(script_path),
                    "checkpoints": ordered(
                        output.get("transition_checkpoints", [])),
                    "hud_candidates": ordered(
                        output.get("hud_candidates", [])),
                    "menu_font_events": ordered(
                        output.get("menu_font_events", [])),
                    "status_font_events": ordered(
                        output.get("status_font_events", [])),
                    "menu_palette_events": ordered(
                        output.get("menu_palette_events", [])),
                    "startup_palette_events": ordered(
                        output.get("startup_palette_events", [])),
                    "state_path_events": ordered(
                        output.get("state_path_events", [])),
                    "frames": captured,
                    "menu_font_limit": args.menu_font_limit,
                    "startup_status_font_limit": args.startup_status_font_limit,
                    "startup_palette_limit": args.startup_palette_limit,
                    "pause_before_launch": args.pause_before_launch,
                    "capture_menu_setup": args.capture_menu_setup,
                    "menu_key": args.menu_key,
                    "menu_key_event": output.get("menu_key_event"),
                    "menu_nav_key": args.menu_nav_key,
                    "menu_nav_event": output.get("menu_nav_event"),
                    "menu_nav_mask": args.menu_nav_mask,
                    "menu_nav_mask_event": output.get("menu_nav_mask_event"),
                    "menu_nav_only": args.menu_nav_only,
                    "timeout": output.get("transition_timeout"),
                }
                args.output.parent.mkdir(parents=True, exist_ok=True)
                args.output.write_text(json.dumps(ledger, indent=2) + "\n",
                                       encoding="utf-8")
                print(f"wrote transition trace to {args.output}")
                return 0
            time.sleep(0.05)
        raise TraceError("timed out waiting for transition trace")
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
    try:
        return run(args)
    except TraceError as exc:
        print(f"quiky_transition_trace: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
