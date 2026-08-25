#!/usr/bin/env python3
"""Run the debugger-only puzzle completion/level-transition trace."""

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
    parser.add_argument("--goal-mask", type=lambda value: int(value, 0),
                        default=0x7f,
                        help="debugger-only DS:60D8 seed (default 0x7f)")
    parser.add_argument("--runtime-dir", type=Path,
                        help="directory containing QUIKY.EXE/NESTLE.DAT for a fixture run")
    parser.add_argument("--native-goal", action="store_true",
                        help="do not seed flags; use a fixture's native letter callbacks")
    parser.add_argument("--native-cloud-focus", action="store_true",
                        help="watch the native cloud callback's DS:89E6 writer")
    parser.add_argument("--force-player-x", type=int)
    parser.add_argument("--force-player-y", type=int)
    parser.add_argument("--deep", action="store_true",
                        help="arm only the deep exit gate with a long timeout")
    parser.add_argument("--timeout", type=float, default=20.0)
    parser.add_argument("--output", type=Path, required=True)
    return parser


def run(args: argparse.Namespace) -> int:
    if not 0 <= args.goal_mask <= 0xffff:
        raise TraceError("--goal-mask must be between 0 and 0xffff")
    repo_root = Path(__file__).resolve().parents[2]
    script_path = repo_root / "research/automation/quiky_goal_transition_trace.lua"
    recording_path = repo_root / "research/automation/startup-to-input.json"
    runtime_dir = (args.runtime_dir.resolve() if args.runtime_dir
                   else (repo_root / "game").resolve())
    executable = runtime_dir / "QUIKY.EXE"
    archive = runtime_dir / "NESTLE.DAT"
    if not executable.is_file() or not archive.is_file():
        raise TraceError("--runtime-dir must contain QUIKY.EXE and NESTLE.DAT")
    port = reserve_local_port()
    token = secrets.token_hex(32)
    env = os.environ.copy()
    env["DOSBOX_API_TOKEN"] = token
    env["QUIKY_AUTOMATION_TARGET"] = str(executable)
    if runtime_dir.name == "game":
        env["DOSBOX_AUTOMATION_DATA_HOME"] = str(runtime_dir.parent)
    log_path = args.output.with_suffix(args.output.suffix + ".dosbox.log")
    frame_dir = args.output.parent / (args.output.stem + "-frames")
    frame_dir.mkdir(parents=True, exist_ok=True)
    with log_path.open("wb") as log_stream:
        process = subprocess.Popen(
            [str(repo_root / "scripts/run-dosbox-automation.sh"),
             "--set", f"webserver_port={port}",
             "--set", f"mount_allowed_bases={runtime_dir}",
             "--set", f"mount_allowed_image_roots={runtime_dir}"],
            cwd=repo_root, env=env, stdout=log_stream,
            stderr=subprocess.STDOUT,
        )
    api = ApiClient(f"http://127.0.0.1:{port}", token)
    captured: dict[str, str] = {}
    released: set[str] = set()
    try:
        info = wait_for_api(api, process, 15.0)
        source = script_path.read_text(encoding="utf-8")
        prefix = (f"TRACE_TIMEOUT_MS={round(args.timeout * 1000)}\n"
                  f"TRACE_OPTIONAL_TIMEOUT_MS={30000 if args.deep else 1000}\n"
                  f"TRACE_DEEP_ONLY={'true' if args.deep else 'false'}\n"
                  f"TRACE_GOAL_MASK={args.goal_mask}\n"
                  f"TRACE_NATIVE_GOAL={'true' if args.native_goal else 'false'}\n"
                  f"TRACE_NATIVE_CLOUD_FOCUS={'true' if args.native_cloud_focus else 'false'}\n"
                  + (f"TRACE_FORCE_PLAYER_X={args.force_player_x}\n"
                     if args.force_player_x is not None else "")
                  + (f"TRACE_FORCE_PLAYER_Y={args.force_player_y}\n"
                     if args.force_player_y is not None else ""))
        api.request("POST", "/api/v1/script/load?name=quiky-goal-transition-trace",
                    text_body=prefix + source)
        api.post("/api/v1/script/start")
        recording = json.loads(recording_path.read_text(encoding="utf-8"))
        deadline = time.monotonic() + args.timeout * 8 + 45
        replayed = False
        while time.monotonic() < deadline:
            status = api.get("/api/v1/script/status")
            if status.get("state") == "error":
                raise TraceError(status.get("error", "goal transition trace failed"))
            output = status.get("output", {})
            if not replayed and output.get("awaiting_startup_replay"):
                api.post("/api/v1/input/sequence", recording)
                replayed = True
            checkpoint = output.get("goal_transition_checkpoint")
            if isinstance(checkpoint, dict):
                name = checkpoint.get("name", "checkpoint")
                if name not in released:
                    frame_path = frame_dir / f"{name}.png"
                    # A headless run can reach the debugger checkpoint while
                    # DOSBox has no presented frame.  The frame is optional
                    # evidence; the guest-state checkpoint is not.  Preserve
                    # the trace instead of turning a 503 from the video API
                    # into a false negative for the completion path.
                    try:
                        frame_path.write_bytes(api.get_binary(
                            "/api/v1/video/frame?format=png&mode=rendered"))
                    except TraceError as exc:
                        captured[name] = {"unavailable": str(exc)}
                    else:
                        captured[name] = str(frame_path)
                    released.add(name)
                    api.post("/api/v1/debug/continue")
            if status.get("state") == "completed":
                ledger = {
                    "schema": "quiky-goal-transition-trace-v1",
                    "goal_mask_seed": args.goal_mask,
                    "native_goal": args.native_goal,
                    "runtime_dir": str(runtime_dir),
                    "dosbox": info,
                    "executable": str(executable),
                    "executable_sha256": sha256(executable),
                    "archive": str(archive),
                    "archive_sha256": sha256(archive),
                    "script": str(script_path),
                    "script_sha256": sha256(script_path),
                    "checkpoints": ordered(output.get(
                        "goal_transition_checkpoints", [])),
                    "frames": captured,
                    "timeout": output.get("goal_transition_timeout"),
                }
                args.output.parent.mkdir(parents=True, exist_ok=True)
                args.output.write_text(json.dumps(ledger, indent=2) + "\n",
                                       encoding="utf-8")
                print(f"wrote goal transition trace to {args.output}")
                return 0
            time.sleep(0.05)
        raise TraceError("timed out waiting for goal transition trace")
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
        print(f"quiky_goal_transition_trace: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
