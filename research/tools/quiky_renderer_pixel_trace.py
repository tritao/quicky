#!/usr/bin/env python3
"""Capture rendered frames at completed live BOB/ICO draw boundaries."""

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
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def ordered(value):
    """Turn debugger tables with numeric keys into stable JSON lists."""
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
    parser.add_argument("--runtime-dir", type=Path)
    parser.add_argument("--events", type=int, default=12)
    parser.add_argument("--timeout", type=float, default=10.0)
    parser.add_argument("--focus", choices=("bob", "ico", "both"), default="both")
    parser.add_argument("--patch-map-run", action="store_true",
                        help="debugger-only: patch five MAP cells to the first effect run")
    parser.add_argument("--patch-map-x", type=int, default=784)
    parser.add_argument("--patch-map-y", type=int, default=192)
    parser.add_argument("--patch-camera-x", type=int)
    parser.add_argument("--patch-camera-y", type=int)
    parser.add_argument("--bob-draw-slot", type=int,
                        help="debugger-only: limit a BOB draw-coordinate override to this slot")
    parser.add_argument("--bob-draw-x", type=int)
    parser.add_argument("--bob-draw-y", type=int)
    parser.add_argument("--force-bob-mode", type=lambda value: int(value, 0),
                        help="debugger-only: replace the BOB stack mode word (for example 0x100)")
    parser.add_argument("--force-bob-mode-slot", type=int,
                        help="limit --force-bob-mode to one logical BOB slot")
    parser.add_argument("--ack-delay-frames", type=int, default=20,
                        help="guest frames to retain each checkpoint after host acknowledgement")
    parser.add_argument("--screenshot-mode", choices=("rendered", "raw"), default="rendered",
                        help="video surface to save at renderer checkpoints")
    parser.add_argument("--output", type=Path, required=True)
    return parser


def run(args: argparse.Namespace) -> int:
    if args.events < 1:
        raise TraceError("--events must be positive")
    if args.ack_delay_frames < 0:
        raise TraceError("--ack-delay-frames cannot be negative")
    if (args.bob_draw_x is None) != (args.bob_draw_y is None):
        raise TraceError("--bob-draw-x and --bob-draw-y must be used together")
    if args.bob_draw_x is not None and args.bob_draw_slot is None:
        raise TraceError("--bob-draw-slot is required with --bob-draw-x/y")
    repo_root = Path(__file__).resolve().parents[2]
    script_path = repo_root / "research/automation/quiky_renderer_pixel_trace.lua"
    recording_path = repo_root / "research/automation/startup-to-input.json"
    runtime_dir = args.runtime_dir.resolve() if args.runtime_dir else None
    executable = (runtime_dir or (repo_root / "game")) / "QUIKY.EXE"
    archive = (runtime_dir or (repo_root / "game")) / "NESTLE.DAT"
    if not executable.is_file() or not archive.is_file():
        raise TraceError("runtime directory must contain QUIKY.EXE and NESTLE.DAT")
    port = reserve_local_port()
    token = secrets.token_hex(32)
    env = os.environ.copy()
    env["DOSBOX_API_TOKEN"] = token
    if runtime_dir is not None:
        env["QUIKY_AUTOMATION_TARGET"] = str(executable)
        if runtime_dir.name == "game":
            env["DOSBOX_AUTOMATION_DATA_HOME"] = str(runtime_dir.parent)
    log_path = args.output.with_suffix(args.output.suffix + ".dosbox.log")
    frame_dir = args.output.parent / (args.output.stem + "-frames")
    frame_dir.mkdir(parents=True, exist_ok=True)
    with log_path.open("wb") as log_stream:
        process = subprocess.Popen(
            [str(repo_root / "scripts/run-dosbox-automation.sh"),
             "--set", f"webserver_port={port}"] +
            ([] if runtime_dir is None else
             ["--set", f"mount_allowed_bases={runtime_dir}",
              "--set", f"mount_allowed_image_roots={runtime_dir}" ]),
            cwd=repo_root, env=env, stdout=log_stream, stderr=subprocess.STDOUT,
        )
    api = ApiClient(f"http://127.0.0.1:{port}", token)
    captured: list[dict[str, object]] = []
    seen_sequence = 0
    try:
        info = wait_for_api(api, process, 15.0)
        prefix = "".join((
            f"TRACE_LEVEL={json.dumps(args.level)}\n",
            f"TRACE_FOCUS={json.dumps(args.focus)}\n",
            f"TRACE_EVENT_LIMIT={args.events}\n",
            f"TRACE_TIMEOUT_MS={round(args.timeout * 1000)}\n",
            f"TRACE_PATCH_MAP_RUN={'true' if args.patch_map_run else 'false'}\n",
            f"TRACE_PATCH_MAP_X={args.patch_map_x}\n",
            f"TRACE_PATCH_MAP_Y={args.patch_map_y}\n",
            (f"TRACE_PATCH_CAMERA_X={args.patch_camera_x}\n"
             if args.patch_camera_x is not None else ""),
            (f"TRACE_PATCH_CAMERA_Y={args.patch_camera_y}\n"
             if args.patch_camera_y is not None else ""),
            (f"TRACE_BOB_DRAW_SLOT={args.bob_draw_slot}\n"
             if args.bob_draw_slot is not None else ""),
            (f"TRACE_BOB_DRAW_X={args.bob_draw_x}\n"
             if args.bob_draw_x is not None else ""),
            (f"TRACE_BOB_DRAW_Y={args.bob_draw_y}\n"
             if args.bob_draw_y is not None else ""),
            (f"TRACE_FORCE_BOB_MODE={args.force_bob_mode}\n"
             if args.force_bob_mode is not None else ""),
            (f"TRACE_FORCE_BOB_MODE_SLOT={args.force_bob_mode_slot}\n"
             if args.force_bob_mode_slot is not None else ""),
            f"TRACE_ACK_DELAY_FRAMES={args.ack_delay_frames}\n",
        ))
        api.request("POST", "/api/v1/script/load?name=quiky-renderer-pixel-trace",
                    text_body=prefix + script_path.read_text(encoding="utf-8"))
        api.post("/api/v1/script/start")
        recording = json.loads(recording_path.read_text(encoding="utf-8"))
        deadline = time.monotonic() + args.timeout * (args.events + 4) + 45
        replayed = False
        while time.monotonic() < deadline:
            status = api.get("/api/v1/script/status")
            if status.get("state") == "error":
                raise TraceError(status.get("error", "pixel trace failed"))
            output = status.get("output", {})
            if not replayed and output.get("awaiting_startup_replay"):
                api.post("/api/v1/input/sequence", recording)
                replayed = True
            checkpoint = output.get("renderer_pixel_checkpoint")
            if isinstance(checkpoint, dict):
                sequence = int(checkpoint.get("sequence", 0))
                if sequence > seen_sequence:
                    stage = str(checkpoint.get("stage", "draw"))
                    frame_path = frame_dir / f"{sequence:03d}-{stage}.png"
                    frame_path.write_bytes(api.get_binary(
                        f"/api/v1/video/frame?format=png&mode={args.screenshot_mode}"))
                    captured.append({
                        "sequence": sequence,
                        "stage": stage,
                        "frame_path": str(frame_path),
                        "sha256": sha256(frame_path),
                        "guest": checkpoint,
                    })
                    seen_sequence = sequence
                    api.post("/api/v1/debug/continue")
            if status.get("state") == "completed":
                ledger = {
                    "schema": "quiky-renderer-pixel-trace-v1",
                    "dosbox": info,
                    "inputs": {"level": args.level, "events": args.events,
                               "patch_map_run": args.patch_map_run,
                               "patch_map_x": args.patch_map_x,
                               "patch_map_y": args.patch_map_y,
                               "patch_camera_x": args.patch_camera_x,
                               "patch_camera_y": args.patch_camera_y,
                               "bob_draw_slot": args.bob_draw_slot,
                               "bob_draw_x": args.bob_draw_x,
                               "bob_draw_y": args.bob_draw_y,
                               "force_bob_mode": args.force_bob_mode,
                               "force_bob_mode_slot": args.force_bob_mode_slot,
                               "ack_delay_frames": args.ack_delay_frames,
                               "screenshot_mode": args.screenshot_mode,
                               "runtime_dir": str(runtime_dir) if runtime_dir else None},
                    "executable": str(executable),
                    "executable_sha256": sha256(executable),
                    "archive": str(archive),
                    "archive_sha256": sha256(archive),
                    "script": str(script_path),
                    "script_sha256": sha256(script_path),
                    "checkpoints": output.get("checkpoints", {}),
                    "events": ordered(output.get("renderer_pixel_checkpoints", [])),
                    "frames": captured,
                }
                args.output.parent.mkdir(parents=True, exist_ok=True)
                args.output.write_text(json.dumps(ledger, indent=2) + "\n",
                                       encoding="utf-8")
                print(f"wrote renderer pixel trace to {args.output}")
                return 0
            time.sleep(0.05)
        raise TraceError("timed out waiting for renderer pixel trace")
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
        print(f"quiky_renderer_pixel_trace: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
