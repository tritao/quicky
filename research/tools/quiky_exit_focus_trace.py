#!/usr/bin/env python3
"""Trace live writers and readers of the level-exit state flags."""

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
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--runtime-dir", type=Path,
                        help="directory containing game/QUIKY.EXE and NESTLE.DAT")
    parser.add_argument("--timeout", type=float, default=20.0)
    parser.add_argument("--events", type=int, default=32)
    parser.add_argument("--hold-frames", type=int, default=12000)
    parser.add_argument("--settle-frames", type=int, default=240,
                        help="frames to wait after selector dispatch before arming exit targets")
    parser.add_argument("--force-player-x", type=int,
                        help="optional world x to write at the first renderer boundary")
    parser.add_argument("--force-player-y", type=int,
                        help="optional world y to write at the first renderer boundary")
    parser.add_argument("--pretrigger-frames", type=int, default=0,
                        help="frames to wait after the initial player force before moving to the trigger")
    parser.add_argument("--post-force-player-x", type=int,
                        help="optional world x to write after --pretrigger-frames")
    parser.add_argument("--post-force-player-y", type=int,
                        help="optional world y to write after --pretrigger-frames")
    parser.add_argument("--capture-frames", action="store_true",
                        help="save rendered PNGs at selected transition checkpoints")
    parser.add_argument("--capture-names", default="completion-hud-call,completion-check,completion-branch,transition-setup",
                        help="comma-separated checkpoint names to capture")
    parser.add_argument("--capture-ack-delay-frames", type=int, default=4,
                        help="frames to leave the acknowledged capture barrier visible")
    parser.add_argument("--capture-mode", choices=("rendered", "raw"), default="rendered",
                        help="video surface to save for transition checkpoints")
    parser.add_argument("--post-transition-key", default="",
                        help="optional key to send after 1709 (for example KBD_space)")
    parser.add_argument("--post-transition-key-frames", type=int, default=8,
                        help="frames to hold --post-transition-key")
    parser.add_argument("--skip-palette-loop", action="store_true",
                        help="do not re-arm the hot full-palette loop breakpoint")
    parser.add_argument("--score-only", action="store_true",
                        help="arm only completion checkpoints and stop at 16C6")
    return parser


def run(args: argparse.Namespace) -> int:
    if (args.events < 1 or args.hold_frames < 1 or args.settle_frames < 0 or
            args.pretrigger_frames < 0 or args.capture_ack_delay_frames < 0 or
            args.post_transition_key_frames < 1):
        raise TraceError("--events and --hold-frames must be positive; frame delays cannot be negative")
    repo_root = Path(__file__).resolve().parents[2]
    script_path = repo_root / "research/automation/quiky_exit_focus_trace.lua"
    recording_path = repo_root / "research/automation/startup-to-input.json"
    # Match quikytrace's runtime-dir contract: the argument names the
    # directory that directly contains QUIKY.EXE and NESTLE.DAT.  This keeps
    # fixture runs isolated while allowing the DOSBox mount policy to admit
    # the alternate image root.
    target_root = (args.runtime_dir.resolve() if args.runtime_dir
                   else (repo_root / "game").resolve())
    target = target_root / "QUIKY.EXE"
    archive = target_root / "NESTLE.DAT"
    if not target.is_file() or not archive.is_file():
        raise TraceError(f"runtime directory lacks game/QUIKY.EXE and NESTLE.DAT: {target_root}")
    port = reserve_local_port()
    token = secrets.token_hex(32)
    env = os.environ.copy()
    env["DOSBOX_API_TOKEN"] = token
    env["QUIKY_AUTOMATION_TARGET"] = str(target)
    if target_root.name == "game":
        env["DOSBOX_AUTOMATION_DATA_HOME"] = str(target_root.parent)
    log_path = args.output.with_suffix(args.output.suffix + ".dosbox.log")
    with log_path.open("wb") as log_stream:
        process = subprocess.Popen(
            [str(repo_root / "scripts/run-dosbox-automation.sh"),
             "--set", f"webserver_port={port}",
             "--set", f"mount_allowed_bases={target_root}",
             "--set", f"mount_allowed_image_roots={target_root}"],
            cwd=repo_root, env=env, stdout=log_stream,
            stderr=subprocess.STDOUT,
        )
    api = ApiClient(f"http://127.0.0.1:{port}", token)
    frame_dir = args.output.parent / (args.output.stem + "-frames")
    captured_frames: list[dict[str, object]] = []
    seen_checkpoint = 0
    try:
        info = wait_for_api(api, process, 15.0)
        source = script_path.read_text(encoding="utf-8")
        prefix = (f"TRACE_TIMEOUT_MS={round(args.timeout * 1000)}\n"
                  f"TRACE_EVENT_LIMIT={args.events}\n"
                  f"TRACE_HOLD_FRAMES={args.hold_frames}\n"
                  f"TRACE_SETTLE_FRAMES={args.settle_frames}\n")
        if args.force_player_x is not None:
            prefix += f"TRACE_FORCE_PLAYER_X={args.force_player_x}\n"
        if args.force_player_y is not None:
            prefix += f"TRACE_FORCE_PLAYER_Y={args.force_player_y}\n"
        if args.pretrigger_frames:
            prefix += f"TRACE_PRETRIGGER_FRAMES={args.pretrigger_frames}\n"
        if args.post_force_player_x is not None:
            prefix += f"TRACE_POST_FORCE_PLAYER_X={args.post_force_player_x}\n"
        if args.post_force_player_y is not None:
            prefix += f"TRACE_POST_FORCE_PLAYER_Y={args.post_force_player_y}\n"
        if args.capture_frames:
            prefix += "TRACE_CAPTURE_FRAMES=true\n"
            prefix += f"TRACE_CAPTURE_NAMES={json.dumps(args.capture_names)}\n"
            prefix += f"TRACE_CAPTURE_ACK_DELAY_FRAMES={args.capture_ack_delay_frames}\n"
        if args.post_transition_key:
            prefix += f"TRACE_POST_TRANSITION_KEY={json.dumps(args.post_transition_key)}\n"
            prefix += f"TRACE_POST_TRANSITION_KEY_FRAMES={args.post_transition_key_frames}\n"
        if args.skip_palette_loop:
            prefix += "TRACE_PALETTE_LOOP=false\n"
        if args.score_only:
            prefix += "TRACE_SCORE_ONLY=true\n"
        api.request("POST", "/api/v1/script/load?name=quiky-exit-focus-trace",
                    text_body=prefix + source)
        api.post("/api/v1/script/start")
        recording = json.loads(recording_path.read_text(encoding="utf-8"))
        deadline = time.monotonic() + args.timeout * (args.events + 2) + 45
        replayed = False
        while time.monotonic() < deadline:
            status = api.get("/api/v1/script/status")
            if status.get("state") == "error":
                raise TraceError(status.get("error", "exit focus trace failed"))
            output = status.get("output", {})
            if not replayed and output.get("awaiting_startup_replay"):
                api.post("/api/v1/input/sequence", recording)
                replayed = True
            checkpoint = output.get("exit_focus_checkpoint")
            if args.capture_frames and isinstance(checkpoint, dict):
                sequence = int(checkpoint.get("sequence", 0))
                if sequence > seen_checkpoint:
                    name = str(checkpoint.get("name", "checkpoint"))
                    frame_path = frame_dir / f"{sequence:03d}-{name}.png"
                    frame_dir.mkdir(parents=True, exist_ok=True)
                    frame_path.write_bytes(api.get_binary(
                        f"/api/v1/video/frame?format=png&mode={args.capture_mode}"))
                    captured_frames.append({
                        "sequence": sequence,
                        "name": name,
                        "frame_path": str(frame_path),
                        "sha256": sha256(frame_path),
                        "guest": checkpoint,
                    })
                    seen_checkpoint = sequence
                    api.post("/api/v1/debug/continue")
            if status.get("state") == "completed":
                ledger = {
                    "schema": "quiky-exit-focus-trace-v1",
                    "runtime_dir": str(target_root),
                    "dosbox": info,
                    "executable": str(target),
                    "executable_sha256": sha256(target),
                    "archive": str(archive),
                    "archive_sha256": sha256(archive),
                    "script": str(script_path),
                    "script_sha256": sha256(script_path),
                    "inputs": {
                        "timeout": args.timeout,
                        "events": args.events,
                        "hold_frames": args.hold_frames,
                        "settle_frames": args.settle_frames,
                        "force_player_x": args.force_player_x,
                        "force_player_y": args.force_player_y,
                        "pretrigger_frames": args.pretrigger_frames,
                        "post_force_player_x": args.post_force_player_x,
                        "post_force_player_y": args.post_force_player_y,
                        "capture_frames": args.capture_frames,
                        "capture_names": args.capture_names,
                        "capture_ack_delay_frames": args.capture_ack_delay_frames,
                        "capture_mode": args.capture_mode,
                        "post_transition_key": args.post_transition_key,
                        "post_transition_key_frames": args.post_transition_key_frames,
                        "skip_palette_loop": args.skip_palette_loop,
                        "score_only": args.score_only,
                    },
                    "player_forced": output.get("exit_focus_player_forced"),
                    "pretrigger": output.get("exit_focus_pretrigger"),
                    "armed_targets": output.get("exit_focus_armed_targets") or {},
                    "events": output.get("exit_focus_events") or [],
                    "frames": captured_frames,
                    "timeout": output.get("exit_focus_timeout"),
                }
                args.output.parent.mkdir(parents=True, exist_ok=True)
                args.output.write_text(json.dumps(ledger, indent=2) + "\n",
                                       encoding="utf-8")
                print(f"wrote exit focus trace to {args.output}")
                return 0
            time.sleep(0.05)
        raise TraceError("timed out waiting for exit focus trace")
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
        print(f"quiky_exit_focus_trace: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
