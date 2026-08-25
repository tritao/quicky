#!/usr/bin/env python3
"""Capture Quiky's runtime SFX table and gameplay SFX calls."""

from __future__ import annotations

import argparse
import json
import os
import secrets
import subprocess
import sys
import time
import urllib.parse
from datetime import datetime, timezone
from pathlib import Path

from quikytrace import (
    ApiClient,
    TraceError,
    capture_failure,
    discover_token,
    reserve_local_port,
    sha256,
    wait_for_api,
)


def lua_literal(value: object) -> str:
    if value is None:
        return "nil"
    if value is True:
        return "true"
    if value is False:
        return "false"
    if isinstance(value, (int, float)):
        return repr(value)
    if isinstance(value, str):
        return json.dumps(value)
    if isinstance(value, dict):
        return "{" + ",".join(
            f"[{json.dumps(str(key))}]={lua_literal(item)}"
            for key, item in value.items()
        ) + "}"
    if isinstance(value, (list, tuple)):
        return "{" + ",".join(lua_literal(item) for item in value) + "}"
    raise TypeError(type(value).__name__)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", default="http://127.0.0.1:8386")
    parser.add_argument("--token-file", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--timeout", type=float, default=120.0)
    parser.add_argument("--event-timeout-ms", type=int, default=1200)
    parser.add_argument("--max-events", type=int, default=32)
    parser.add_argument("--settle-frames", type=int, default=240)
    parser.add_argument("--action-attempts", type=int, default=5,
                        help="maximum SFX waits per held-key action")
    parser.add_argument("--action-settle-frames", type=int, default=30,
                        help="guest frames to let the level run after each action")
    parser.add_argument("--inter-event-settle-frames", type=int, default=30,
                        help="guest frames between forced effect requests")
    parser.add_argument("--action-profile", choices=("basic", "explore", "extended", "pickup", "traverse", "traverse-jump", "traverse-up", "traverse-alt", "alt-probe", "alt-replay"), default="basic",
                        help="input action sequence; pickup tests stationary contact after placement")
    parser.add_argument("--teleport-player", nargs=2, type=int, metavar=("X", "Y"),
                        help="research-only: place the live player record at world coordinates")
    parser.add_argument("--teleport-object", nargs=3, type=int, metavar=("INDEX", "X", "Y"),
                        help="research-only: place one live object-pool record at world coordinates")
    parser.add_argument("--camera", nargs=2, type=int, metavar=("X", "Y"),
                        help="research-only: override the live camera world coordinates")
    parser.add_argument("--warmup-key",
                        help="research-only: move before teleporting, to stream a target object")
    parser.add_argument("--warmup-frames", type=int, default=0,
                        help="guest frames for the optional pre-teleport movement")
    parser.add_argument("--force-all-ids", action="store_true",
                        help="research-only: drive IDs 0..13 through the live SFX boundary")
    parser.add_argument("--force-id", type=int, choices=range(14),
                        help="research-only: drive one selected ID through the live boundary")
    parser.add_argument("--collision-pair", nargs=2, type=int, metavar=("FIRST", "SECOND"),
                        help="research-only: activate FIRST, then trace SECOND against the active voice")
    parser.add_argument("--collision-high-bit", action="store_true",
                        help="research-only: set the second collision table priority high bit")
    parser.add_argument("--selection-voice", type=int, choices=range(4),
                        help="research-only: patch one forced effect's logical voice selector")
    parser.add_argument("--mute-music", action="store_true",
                        help="research-only: clear the music activity flag before capture")
    parser.add_argument("--select-level",
                        help="use the in-game selector to launch W1L1..W5L4")
    parser.add_argument("--selector-frames", type=int, default=60)
    parser.add_argument("--driver-probe-count", type=int, default=0,
                        help="capture entries to the runtime audio driver after the input sweep")
    parser.add_argument("--event-driver-probe-count", type=int, default=0,
                        help="research-only: capture consecutive driver updates inside each SFX event")
    parser.add_argument("--interpreter-probe-count", type=int, default=0,
                        help="capture packed effect-word interpreter entries after each SFX")
    parser.add_argument("--macro-probe-count", type=int, default=0,
                        help="capture decoded macro commands after each SFX")
    parser.add_argument("--mixer-probe-count", type=int, default=0,
                        help="capture selected-voice mixer commits after each SFX")
    parser.add_argument("--priority-probe-count", type=int, default=0,
                        help="capture effect voice priority/replacement decisions")
    parser.add_argument("--priority-only", action="store_true",
                        help="research-only: trace only the priority selection boundary")
    parser.add_argument("--priority-status-probe-count", type=int, default=0,
                        help="research-only: capture the selected voice across driver updates")
    parser.add_argument("--priority-irq-status-probe-count", type=int, default=0,
                        help="research-only: synchronize selected-voice samples to audio IRQs")
    parser.add_argument("--mix-probe-count", type=int, default=0,
                        help="capture raw-sample to output-buffer mixer entries")
    parser.add_argument("--output-probe-count", type=int, default=0,
                        help="capture mixed-word to DMA-byte conversion entries")
    parser.add_argument("--output-input-word", type=lambda value: int(value, 0),
                        help="research-only: replace the first mixed word before output conversion")
    parser.add_argument("--irq-probe", action="store_true",
                        help="capture one Sound Blaster IRQ after each traced SFX")
    parser.add_argument("--dma-probe", action="store_true",
                        help="capture one Sound Blaster DMA setup after each traced SFX")
    parser.add_argument("--pool-probe", action="store_true",
                        help="capture the live object pool at each traced SFX entry")
    parser.add_argument("--callsite-probe", action="store_true",
                        help="capture the ID 8/2 gameplay object callsites and pool transitions")
    parser.add_argument("--targeted-probe", choices=("id2", "id2-crab", "id11", "id11-natural", "contact-producer"),
                        help="capture the selected static guard and state transition")
    parser.add_argument("--audio-capture", type=Path)
    parser.add_argument("--audio-tail-frames", type=int, default=0,
                        help="guest frames to record after the traced event before stopping audio capture")
    parser.add_argument("--launch", action="store_true")
    parser.add_argument("--headless", action="store_true")
    parser.add_argument("--startup-timeout", type=float, default=15.0)
    args = parser.parse_args()
    if args.force_all_ids and args.force_id is not None:
        parser.error("--force-all-ids and --force-id are mutually exclusive")
    if args.collision_pair is not None:
        if any(effect_id < 0 or effect_id > 13 for effect_id in args.collision_pair):
            parser.error("--collision-pair IDs must be between 0 and 13")
        if args.force_all_ids or args.force_id is not None:
            parser.error("--collision-pair cannot be combined with --force-all-ids or --force-id")
    elif args.collision_high_bit:
        parser.error("--collision-high-bit requires --collision-pair")
    if args.selection_voice is not None and args.force_id is None:
        parser.error("--selection-voice requires --force-id")
    if args.audio_tail_frames < 0:
        parser.error("--audio-tail-frames cannot be negative")

    repo_root = Path(__file__).resolve().parents[2]
    process = None
    log_stream = None
    if args.launch:
        port = reserve_local_port()
        token = secrets.token_hex(32)
        env = os.environ.copy()
        env["DOSBOX_API_TOKEN"] = token
        if args.headless:
            env["SDL_VIDEODRIVER"] = "dummy"
            env["SDL_AUDIODRIVER"] = "dummy"
        args.output.parent.mkdir(parents=True, exist_ok=True)
        log_stream = args.output.with_suffix(args.output.suffix + ".dosbox.log").open("wb")
        process = subprocess.Popen(
            [str(repo_root / "scripts/run-dosbox-automation.sh"),
             "--set", f"webserver_port={port}"],
            cwd=repo_root, env=env, stdout=log_stream, stderr=subprocess.STDOUT,
        )
        api = ApiClient(f"http://127.0.0.1:{port}", token)
        info = wait_for_api(api, process, args.startup_timeout)
    else:
        api = ApiClient(args.url, discover_token(args.token_file))
        info = api.get("/api/v1/dosbox/info")

    script_path = repo_root / "research/automation/quiky_sfx_trace.lua"
    audio_path = args.audio_capture.resolve() if args.audio_capture else None
    config = {
        "timeout_ms": round(args.timeout * 1000),
        "event_timeout_ms": args.event_timeout_ms,
        "max_events": args.max_events,
        "settle_frames": args.settle_frames,
        "action_attempts": args.action_attempts,
        "action_settle_frames": args.action_settle_frames,
        "inter_event_settle_frames": args.inter_event_settle_frames,
        "action_profile": args.action_profile,
        "teleport_player": {
            "x": args.teleport_player[0], "y": args.teleport_player[1]
        } if args.teleport_player else None,
        "teleport_object": {
            "index": args.teleport_object[0],
            "x": args.teleport_object[1], "y": args.teleport_object[2],
        } if args.teleport_object else None,
        "camera_x": args.camera[0] if args.camera else None,
        "camera_y": args.camera[1] if args.camera else None,
        "warmup_key": args.warmup_key or "",
        "warmup_frames": args.warmup_frames,
        "force_all_ids": args.force_all_ids,
        "force_id": args.force_id,
        "collision_pair": args.collision_pair,
        "collision_high_bit": args.collision_high_bit,
        "selection_voice": args.selection_voice,
        "mute_music": args.mute_music,
        "select_level": args.select_level or "",
        "selector_frames": args.selector_frames,
        "driver_probe_count": args.driver_probe_count,
        "event_driver_probe_count": args.event_driver_probe_count,
        "interpreter_probe_count": args.interpreter_probe_count,
        "macro_probe_count": args.macro_probe_count,
        "mixer_probe_count": args.mixer_probe_count,
        "priority_probe_count": args.priority_probe_count,
        "priority_only": args.priority_only,
        "priority_status_probe_count": args.priority_status_probe_count,
        "priority_irq_status_probe_count": args.priority_irq_status_probe_count,
        "mix_probe_count": args.mix_probe_count,
        "output_probe_count": args.output_probe_count,
        "output_input_word": args.output_input_word,
        "irq_probe": args.irq_probe,
        "dma_probe": args.dma_probe,
        "pool_probe": args.pool_probe,
        "callsite_probe": args.callsite_probe,
        "targeted_probe": args.targeted_probe or "",
        "audio_capture_path": str(audio_path) if audio_path else "",
        "audio_tail_frames": args.audio_tail_frames,
    }
    source = "TRACE_CONFIG = " + lua_literal(config) + "\n" + script_path.read_text(
        encoding="utf-8"
    )

    try:
        api.request(
            "POST",
            "/api/v1/script/load?name=" + urllib.parse.quote("quiky-sfx-trace"),
            text_body=source,
        )
        api.post("/api/v1/script/start")
        recording = json.loads(
            (repo_root / "research/automation/startup-to-input.json").read_text()
        )
        deadline = time.monotonic() + args.timeout + 30
        replayed = False
        trace = None
        while time.monotonic() < deadline:
            status = api.get("/api/v1/script/status")
            if status.get("state") == "error":
                raise TraceError(status.get("error", "Lua trace failed"))
            if not replayed and status.get("output", {}).get("awaiting_startup_replay"):
                api.post("/api/v1/input/sequence", {"events": recording["events"]})
                replayed = True
            trace = status.get("output", {}).get("trace")
            if isinstance(trace, dict) and status.get("state") == "completed":
                break
            time.sleep(0.05)
        if not isinstance(trace, dict):
            raise TraceError("timed out waiting for SFX trace")

        ledger = {
            "schema": "quiky-sfx-trace-ledger-v1",
            "created_utc": datetime.now(timezone.utc).isoformat(),
            "dosbox": info,
            "inputs": {
                "executable": str(repo_root / "game/QUIKY.EXE"),
                "executable_sha256": sha256(repo_root / "game/QUIKY.EXE"),
                "archive": str(repo_root / "game/NESTLE.DAT"),
                "archive_sha256": sha256(repo_root / "game/NESTLE.DAT"),
                "fx_gate": "set DS:613F=1 after gameplay audio-load entry",
                "select_level": args.select_level,
            },
            "script": str(script_path),
            "script_sha256": sha256(script_path),
            "trace": trace,
        }
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(ledger, indent=2) + "\n")
        print(f"wrote SFX trace to {args.output}")
    except Exception as exc:
        capture_failure(api, args.output, exc)
        raise
    finally:
        if process is not None:
            try:
                api.post("/api/v1/control/shutdown")
                process.wait(timeout=5)
            except Exception:
                process.terminate()
                try:
                    process.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
            if log_stream is not None:
                log_stream.close()
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except TraceError as exc:
        print(f"quikysfxtrace: {exc}", file=sys.stderr)
        raise SystemExit(1)
