#!/usr/bin/env python3
"""Run the normal-lifecycle secondary MAP-loader probe."""

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

TOOLS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS_DIR))
from quikytrace import ApiClient, TraceError, reserve_local_port, wait_for_api  # noqa: E402


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--level", default="W1L3")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--runtime-dir", type=Path,
                        default=Path(__file__).resolve().parents[2] / "game")
    parser.add_argument("--timeout", type=float, default=90.0)
    parser.add_argument("--selector-frames", type=int, default=80)
    parser.add_argument("--diagnostic-steps", type=int, default=48)
    parser.add_argument("--drive-wait", action="store_true",
                        help="debugger-only: release DS:819E once at 48BB")
    parser.add_argument("--skip-wait", action="store_true",
                        help="debugger-only: jump over the 48BB wait loop")
    parser.add_argument("--no-wait-break", action="store_true",
                        help="diagnostic: let 48B5/48BB run while watching post-wait state")
    parser.add_argument("--diagnostic", action="store_true",
                        help="capture transition-gate writes without mutating guest state")
    parser.add_argument("--force-gate", action="store_true",
                        help="debugger-only: bypass the state-machine gate clear")
    parser.add_argument("--post-input-key", default="",
                        help="diagnostic: inject one real key after level launch, e.g. KBD_2")
    parser.add_argument("--event-before-secondary", action="store_true",
                        help="controlled: press --post-input-key at level dispatch and release at the first player callback while waiting uninstrumented for 3861")
    parser.add_argument("--timer-audit", action="store_true",
                        help="controlled: rearm the timer IRQ breakpoint while waiting for the secondary loader")
    parser.add_argument("--timer-state-trace", action="store_true",
                        help="diagnostic: trace DS:819E around the timer IRQ and timed-wait instructions")
    parser.add_argument("--timer-post-wait-audit", action="store_true",
                        help="diagnostic: after 48BB, wait only for unmodified timer IRQ delivery")
    parser.add_argument("--pending-loader-audit", action="store_true",
                        help="controlled diagnostic: bypass the observed pending timer wait and trace toward 3861")
    parser.add_argument("--post-input-hold-events", type=int, default=8,
                        help="diagnostic: keep the injected key held across this many barriers")
    parser.add_argument("--gameplay-key", default="",
                        help="diagnostic: start a real player key after KBD_2 event, e.g. KBD_right")
    parser.add_argument("--gameplay-hold-events", type=int, default=40,
                        help="diagnostic: keep the gameplay key held across this many barriers")
    parser.add_argument("--gameplay-at-launch", action="store_true",
                        help="diagnostic: press gameplay key before the post-launch event")
    parser.add_argument("--force-player-fall", action="store_true",
                        help="debugger-only: set the player Y below the 43D0 boundary")
    parser.add_argument("--force-event", action="store_true",
                        help="debugger-only: set DS:89E6 at the 48E6 consumer gate")
    parser.add_argument("--pending-focus", action="store_true",
                        help="diagnostic: repeatedly trace the 4C43..505D pending-state barrier")
    parser.add_argument("--writer-focus", action="store_true",
                        help="diagnostic: run a bounded gameplay window watching only MAP writers")
    args = parser.parse_args()
    if args.writer_focus:
        args.diagnostic = True
    if args.timer_state_trace:
        args.diagnostic = True
    if args.timer_post_wait_audit:
        args.diagnostic = True
    if args.pending_loader_audit:
        args.diagnostic = True

    repo_root = Path(__file__).resolve().parents[2]
    runtime_dir = args.runtime_dir.resolve()
    binary = os.environ.get(
        "DOSBOX_AUTOMATION_BIN",
        str(repo_root / "research/build/dosbox-automation-debug/dosbox_with_debugger"),
    )
    if not Path(binary).is_file():
        raise TraceError(f"DOSBOX_AUTOMATION_BIN is not a file: {binary}")
    port = reserve_local_port()
    token = secrets.token_hex(32)
    env = os.environ.copy()
    env.update({
        "DOSBOX_API_TOKEN": token,
        "QUIKY_AUTOMATION_TARGET": str(runtime_dir / "QUIKY.EXE"),
        "DOSBOX_AUTOMATION_BIN": binary,
        "SDL_VIDEODRIVER": "dummy",
        "SDL_AUDIODRIVER": "dummy",
        "DOSBOX_AUTOMATION_DATA_HOME": str(runtime_dir.parent),
    })
    args.output.parent.mkdir(parents=True, exist_ok=True)
    log_stream = args.output.with_suffix(args.output.suffix + ".dosbox.log").open("wb")
    process = subprocess.Popen(
        [str(repo_root / "scripts/run-dosbox-automation.sh"), "--set",
         f"webserver_port={port}", "--set", f"mount_allowed_bases={runtime_dir}",
         "--set", f"mount_allowed_image_roots={runtime_dir}"],
        cwd=repo_root, env=env, stdout=log_stream, stderr=subprocess.STDOUT,
    )
    api = ApiClient(f"http://127.0.0.1:{port}", token)
    source = repo_root / "research/automation/quiky_secondary_lifecycle_trace.lua"
    try:
        info = wait_for_api(api, process, 20.0)
        if not info.get("features", {}).get("debugger"):
            raise TraceError("DOSBox automation build has no debugger API")
        prefix = (f"TRACE_TIMEOUT_MS={round(args.timeout * 1000)}\n"
                  f"TRACE_SELECT_LEVEL={json.dumps(args.level)}\n"
                  f"TRACE_SELECTOR_FRAMES={args.selector_frames}\n"
                  f"TRACE_SECONDARY_DIAGNOSTIC_STEPS={args.diagnostic_steps}\n"
                  f"TRACE_SECONDARY_DRIVE_WAIT={'true' if args.drive_wait else 'false'}\n"
                  f"TRACE_SECONDARY_SKIP_WAIT={'true' if args.skip_wait else 'false'}\n"
                  f"TRACE_SECONDARY_NO_WAIT_BREAK={'true' if args.no_wait_break else 'false'}\n"
                  f"TRACE_SECONDARY_DIAGNOSTIC={'true' if args.diagnostic else 'false'}\n"
                  f"TRACE_SECONDARY_FORCE_GATE={'true' if args.force_gate else 'false'}\n"
                  f"TRACE_SECONDARY_POST_INPUT_KEY={json.dumps(args.post_input_key)}\n"
                  f"TRACE_SECONDARY_EVENT_BEFORE_SECONDARY={'true' if args.event_before_secondary else 'false'}\n"
                  f"TRACE_SECONDARY_TIMER_AUDIT={'true' if args.timer_audit else 'false'}\n"
                  f"TRACE_SECONDARY_TIMER_STATE_TRACE={'true' if args.timer_state_trace else 'false'}\n"
                  f"TRACE_SECONDARY_TIMER_POST_WAIT_AUDIT={'true' if args.timer_post_wait_audit else 'false'}\n"
                  f"TRACE_SECONDARY_PENDING_LOADER_AUDIT={'true' if args.pending_loader_audit else 'false'}\n"
                  f"TRACE_SECONDARY_POST_INPUT_HOLD_EVENTS={args.post_input_hold_events}\n"
                  f"TRACE_SECONDARY_GAMEPLAY_KEY={json.dumps(args.gameplay_key)}\n"
                  f"TRACE_SECONDARY_GAMEPLAY_HOLD_EVENTS={args.gameplay_hold_events}\n"
                  f"TRACE_SECONDARY_GAMEPLAY_AT_LAUNCH={'true' if args.gameplay_at_launch else 'false'}\n"
                  f"TRACE_SECONDARY_FORCE_PLAYER_FALL={'true' if args.force_player_fall else 'false'}\n"
                  f"TRACE_SECONDARY_FORCE_EVENT={'true' if args.force_event else 'false'}\n"
                  f"TRACE_SECONDARY_PENDING_FOCUS={'true' if args.pending_focus else 'false'}\n"
                  f"TRACE_SECONDARY_WRITER_FOCUS={'true' if args.writer_focus else 'false'}\n")
        api.request("POST", "/api/v1/script/load?name=quiky-secondary-lifecycle",
                    text_body=prefix + source.read_text(encoding="utf-8"))
        api.post("/api/v1/script/start")
        recording = json.loads((repo_root / "research/automation/startup-to-input.json")
                               .read_text(encoding="utf-8"))
        deadline = time.monotonic() + args.timeout + 30
        replayed = False
        while time.monotonic() < deadline:
            status = api.get("/api/v1/script/status")
            if not replayed and status.get("output", {}).get("awaiting_startup_replay"):
                api.post("/api/v1/input/sequence", {"events": recording["events"]})
                replayed = True
            result = status.get("output", {}).get("result")
            if isinstance(result, dict):
                payload = {
                    "schema": "quiky-secondary-map-lifecycle-trace-v1",
                    "level": args.level,
                    "executable": str(runtime_dir / "QUIKY.EXE"),
                    "executable_sha256": sha256(runtime_dir / "QUIKY.EXE"),
                    "archive": str(runtime_dir / "NESTLE.DAT"),
                    "archive_sha256": sha256(runtime_dir / "NESTLE.DAT"),
                    "script": str(source), "script_sha256": sha256(source),
                    "result": result,
                }
                args.output.write_text(json.dumps(payload, indent=2) + "\n",
                                       encoding="utf-8")
                print(json.dumps({"output": str(args.output),
                                  "delta": result.get("mutation", {}).get("delta"),
                                  "events": len(result.get("events", []))}, indent=2))
                return 0
            if status.get("state") == "error":
                try:
                    status["cpu"] = api.get("/api/v1/cpu/state")
                    status["debugger"] = api.get("/api/v1/debug/state")
                except Exception:
                    pass
                args.output.with_suffix(args.output.suffix + ".failure.json").write_text(
                    json.dumps(status, indent=2) + "\n", encoding="utf-8")
                raise TraceError(status.get("error", "Lua trace failed"))
            time.sleep(0.05)
        raise TraceError("timed out waiting for secondary lifecycle trace")
    finally:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)
        log_stream.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except TraceError as exc:
        print(str(exc), file=sys.stderr)
        raise SystemExit(2)
