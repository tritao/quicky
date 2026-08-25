#!/usr/bin/env python3
"""Trace one non-player ARE object's callback state transitions."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import secrets
import subprocess
import sys
import time
import urllib.parse
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from quikytrace import (
    ApiClient,
    TraceError,
    discover_token,
    reserve_local_port,
    wait_for_api,
)


@dataclass(frozen=True)
class ObjectBehaviorConfig:
    record_offset: int
    entity_type: int
    samples: int
    startup_recording: Path
    timeout: float = 60.0
    poll_interval: float = 0.05
    select_level: str | None = None
    selector_frames: int = 60
    initial_camera_x: int | None = None
    initial_camera_y: int | None = None
    prestream_input_key: str | None = None
    prestream_input_frames: int = 0
    camera_x: int | None = None
    camera_y: int | None = None
    sprite_init_offset: int = 0
    align_object_to_player: bool = False
    trace_overlap: bool = False
    trace_collision: bool = False
    trace_platform: bool = False
    trace_bump: bool = False
    trace_contact: bool = False
    trace_effect_table: bool = False
    effect_table_attempts: int = 64
    trace_stream_lifecycle: bool = False
    lifecycle_return_camera_x: int = 700
    lifecycle_return_camera_y: int = 350
    force_active_player_bounds: bool = False
    force_bump_player_state: bool = False
    force_cloud_player_state: bool = False
    trace_cloud_consumers: bool = False
    cloud_consumer_offset: int = 0
    trace_cloud_outer_renderer: bool = False
    trace_cloud_hardware_renderer: bool = False
    cloud_hardware_frames: int = 8
    force_contact_gate: bool = False
    align_x_offset: int = 0
    align_y_offset: int = 0
    force_velocity_x: int | None = None
    force_velocity_y: int | None = None
    force_platform_ready: bool = False
    reload_after_collect: bool = False
    reload_level: str | None = None
    reload_wait_frames: int = 30
    force_tile_mask: int | None = None
    trace_puzzle_completion: bool = False
    force_completion_outer_state: bool = False
    force_completion_wait_release: bool = False
    puzzle_probe_frames: int = 120


def lua_config(config: ObjectBehaviorConfig) -> dict[str, Any]:
    return {
        "schema_version": 1,
        "timeout_ms": round(config.timeout * 1000),
        "record_offset": config.record_offset,
        "entity_type": config.entity_type,
        "samples": config.samples,
        "select_level": config.select_level or "",
        "selector_frames": config.selector_frames,
        "initial_camera_x": config.initial_camera_x,
        "initial_camera_y": config.initial_camera_y,
        "prestream_input_key": config.prestream_input_key or "",
        "prestream_input_frames": config.prestream_input_frames,
        "camera_x": config.camera_x,
        "camera_y": config.camera_y,
        "sprite_init_offset": config.sprite_init_offset,
        "align_object_to_player": config.align_object_to_player,
        "trace_overlap": config.trace_overlap,
        "trace_collision": config.trace_collision,
        "trace_platform": config.trace_platform,
        "trace_bump": config.trace_bump,
        "trace_contact": config.trace_contact,
        "trace_effect_table": config.trace_effect_table,
        "effect_table_attempts": config.effect_table_attempts,
        "trace_stream_lifecycle": config.trace_stream_lifecycle,
        "lifecycle_return_camera_x": config.lifecycle_return_camera_x,
        "lifecycle_return_camera_y": config.lifecycle_return_camera_y,
        "force_active_player_bounds": config.force_active_player_bounds,
        "force_bump_player_state": config.force_bump_player_state,
        "force_cloud_player_state": config.force_cloud_player_state,
        "trace_cloud_consumers": config.trace_cloud_consumers,
        "cloud_consumer_offset": config.cloud_consumer_offset,
        "trace_cloud_outer_renderer": config.trace_cloud_outer_renderer,
        "trace_cloud_hardware_renderer": config.trace_cloud_hardware_renderer,
        "cloud_hardware_frames": config.cloud_hardware_frames,
        "force_contact_gate": config.force_contact_gate,
        "align_x_offset": config.align_x_offset,
        "align_y_offset": config.align_y_offset,
        "force_velocity_x": config.force_velocity_x,
        "force_velocity_y": config.force_velocity_y,
        "force_platform_ready": config.force_platform_ready,
        "reload_after_collect": config.reload_after_collect,
        "reload_level": config.reload_level or "",
        "reload_wait_frames": config.reload_wait_frames,
        "force_tile_mask": config.force_tile_mask,
        "trace_puzzle_completion": config.trace_puzzle_completion,
        "force_completion_outer_state": config.force_completion_outer_state,
        "force_completion_wait_release": config.force_completion_wait_release,
        "puzzle_probe_frames": config.puzzle_probe_frames,
    }


def lua_literal(value: Any) -> str:
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
    raise TypeError(f"cannot encode {type(value).__name__} as Lua")


def ordered_lua_array(value: Any) -> list[Any]:
    if isinstance(value, list):
        return value
    if isinstance(value, dict):
        pairs: list[tuple[int, Any]] = []
        for key, item in value.items():
            try:
                pairs.append((int(key), item))
            except (TypeError, ValueError):
                return []
        return [item for _, item in sorted(pairs)]
    return []


def normalize_behavior_trace(trace: dict[str, Any]) -> dict[str, Any]:
    samples = ordered_lua_array(trace.get("samples", []))
    for sample in samples:
        if isinstance(sample, dict):
            sample["changed_bytes"] = ordered_lua_array(
                sample.get("changed_bytes", [])
            )
    trace["samples"] = samples
    effect_probe = trace.get("effect_table_probe")
    if isinstance(effect_probe, dict):
        effect_probe["events"] = ordered_lua_array(
            effect_probe.get("events", [])
        )
    cloud_probe = trace.get("cloud_consumer_probe")
    if isinstance(cloud_probe, dict):
        cloud_probe["reader_offsets"] = ordered_lua_array(
            cloud_probe.get("reader_offsets", [])
        )
        cloud_samples = ordered_lua_array(cloud_probe.get("samples", []))
        for sample in cloud_samples:
            if isinstance(sample, dict):
                sample["hits"] = ordered_lua_array(sample.get("hits", []))
        cloud_probe["samples"] = cloud_samples
    return trace


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def trace_object_behavior(
    api: ApiClient, script_path: Path, config: ObjectBehaviorConfig,
) -> dict[str, Any]:
    source = script_path.read_text(encoding="utf-8")
    name = urllib.parse.quote("quiky-object-behavior-trace")
    prefix = "TRACE_CONFIG = " + lua_literal(lua_config(config)) + "\n"
    api.request("POST", f"/api/v1/script/load?name={name}",
                text_body=prefix + source)
    api.post("/api/v1/script/start")
    recording = json.loads(config.startup_recording.read_text(encoding="utf-8"))
    deadline = time.monotonic() + config.timeout + 20
    replayed = False
    while time.monotonic() < deadline:
        status = api.get("/api/v1/script/status")
        if status.get("state") == "error":
            raise TraceError(
                f"Lua object behavior trace failed: {status.get('error', 'unknown error')}"
            )
        if not replayed and status.get("output", {}).get("awaiting_startup_replay"):
            api.post("/api/v1/input/sequence", {"events": recording["events"]})
            replayed = True
        result = status.get("output", {}).get("behavior_trace")
        if isinstance(result, dict):
            return normalize_behavior_trace(result)
        if status.get("state") == "completed":
            raise TraceError("Lua object behavior trace completed without output")
        time.sleep(config.poll_interval)
    raise TraceError("timed out waiting for the Lua object behavior trace")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--record-offset", type=lambda value: int(value, 0),
                        default=0x1792)
    parser.add_argument("--entity-type", type=lambda value: int(value, 0),
                        required=True)
    parser.add_argument("--samples", type=int, default=32)
    parser.add_argument("--timeout", type=float, default=60.0)
    parser.add_argument("--poll-interval", type=float, default=0.05)
    parser.add_argument("--select-level")
    parser.add_argument("--selector-frames", type=int, default=60)
    parser.add_argument("--initial-camera-x", type=int,
                        help="debugger-only: set DS:81C0 before authored ARE streaming")
    parser.add_argument("--initial-camera-y", type=int,
                        help="debugger-only: set DS:81C4 before authored ARE streaming")
    parser.add_argument("--prestream-input-key",
                        help="debugger-only: hold a key while the authored ARE stream advances")
    parser.add_argument("--prestream-input-frames", type=int, default=0,
                        help="frames to hold --prestream-input-key before target declaration search")
    parser.add_argument("--camera-x", type=int)
    parser.add_argument("--camera-y", type=int)
    parser.add_argument(
        "--sprite-init-offset", type=lambda value: int(value, 0), default=0,
        help="post-initializer breakpoint used to obtain the live object callback",
    )
    parser.add_argument(
        "--align-object-to-player", action="store_true",
        help="write the live player's fixed-point position into the traced object before its first callback",
    )
    parser.add_argument(
        "--trace-overlap", action="store_true",
        help="capture the shared collectible overlap helper and clear branch",
    )
    parser.add_argument(
        "--trace-collision", action="store_true",
        help="capture normal-object visibility, bounds, MAP, and offscreen helpers",
    )
    parser.add_argument(
        "--trace-platform", action="store_true",
        help="capture moving-platform MAP, offscreen, and player-carry helpers",
    )
    parser.add_argument(
        "--trace-bump", action="store_true",
        help="capture the BUMP player-range and sound-trigger branch",
    )
    parser.add_argument(
        "--trace-contact", action="store_true",
        help="capture normal-enemy player-contact response branches",
    )
    parser.add_argument(
        "--trace-effect-table", action="store_true",
        help="after the traced callback, watch pending-effect, player-producer, and 4519/45AB table events",
    )
    parser.add_argument(
        "--effect-table-attempts", type=int, default=64,
        help="maximum breakpoint events for --trace-effect-table (default 64)",
    )
    parser.add_argument(
        "--trace-stream-lifecycle", action="store_true",
        help="capture off-camera removal followed by re-stream of the same ARE record",
    )
    parser.add_argument(
        "--lifecycle-return-camera-x", type=int, default=700,
        help="camera X used for the lifecycle re-stream phase",
    )
    parser.add_argument(
        "--lifecycle-return-camera-y", type=int, default=350,
        help="camera Y used for the lifecycle re-stream phase",
    )
    parser.add_argument(
        "--force-active-player-bounds", action="store_true",
        help="use the original active-player vertical bound (-40..0) for the controlled overlap probe",
    )
    parser.add_argument(
        "--force-bump-player-state", action="store_true",
        help="debugger-only: enable the BUMP helper's player-state gate (DS:89EA=0, player+0x37 nonzero)",
    )
    parser.add_argument(
        "--force-cloud-player-state", action="store_true",
        help="debugger-only: point the cloud bounds gate at its object and enable the DS:89E6 write",
    )
    parser.add_argument(
        "--trace-cloud-consumers", action="store_true",
        help="trace player-side readers of the cloud DS:89E6 state",
    )
    parser.add_argument(
        "--cloud-consumer-offset", type=lambda value: int(value, 0), default=0,
        help="one player-side DS:89E6 reader to capture (0x4087 or 0x4406)",
    )
    parser.add_argument(
        "--trace-cloud-outer-renderer", action="store_true",
        help="capture the main-loop cloud state and render-queue consumers",
    )
    parser.add_argument(
        "--trace-cloud-hardware-renderer", action="store_true",
        help="capture the special cloud VGA/BOB blitter entry and its descriptor",
    )
    parser.add_argument(
        "--cloud-hardware-frames", type=int, default=8,
        help="number of special cloud blitter entries to capture",
    )
    parser.add_argument(
        "--force-contact-gate", action="store_true",
        help="debugger-only: enable the normal-enemy player-range gate and align its shared integer player coordinates",
    )
    parser.add_argument(
        "--align-x-offset", type=int, default=0,
        help="integer-pixel X offset from the live player when aligning the traced object",
    )
    parser.add_argument(
        "--align-y-offset", type=int, default=0,
        help="integer-pixel Y offset from the live player when aligning the traced object",
    )
    parser.add_argument("--force-velocity-x", type=lambda value: int(value, 0),
                        help="debugger-only fixed-point X velocity written before the first callback")
    parser.add_argument("--force-velocity-y", type=lambda value: int(value, 0),
                        help="debugger-only fixed-point Y velocity written before the first callback")
    parser.add_argument(
        "--force-platform-ready", action="store_true",
        help="debugger-only: clear the platform carry latch before its first callback",
    )
    parser.add_argument(
        "--reload-after-collect", action="store_true",
        help="after a pickup clears its callback, re-enter the native level selector and probe reconstruction",
    )
    parser.add_argument(
        "--reload-level",
        help="level selector to use for --reload-after-collect (defaults to --select-level)",
    )
    parser.add_argument(
        "--reload-wait-frames", type=int, default=30,
        help="frames to let gameplay run before the reload selector probe",
    )
    parser.add_argument(
        "--force-tile-mask", type=lambda value: int(value, 0),
        help="debugger-only: write DS:60D8 before each traced callback (for final-letter completion probes)",
    )
    parser.add_argument(
        "--trace-puzzle-completion", action="store_true",
        help="capture the DS:60D8 display consumer and post-collection gameplay state",
    )
    parser.add_argument(
        "--force-completion-outer-state", action="store_true",
        help="debugger-only: seed DS:89E6 so the outer cloud-state consumer invokes the completion/effect consumer",
    )
    parser.add_argument(
        "--force-completion-wait-release", action="store_true",
        help="debugger-only: release the PIT presentation waits so the DS:85DB transition handoff can be traced",
    )
    parser.add_argument(
        "--puzzle-probe-frames", type=int, default=120,
        help="frames to run after the final-letter callback before sampling transition state",
    )
    parser.add_argument("--startup-recording", type=Path,
                        default=Path("research/automation/startup-to-input.json"))
    parser.add_argument("--url", default="http://127.0.0.1:8386")
    parser.add_argument("--token-file", type=Path)
    parser.add_argument("--launch", action="store_true")
    parser.add_argument("--headless", action="store_true")
    parser.add_argument("--startup-timeout", type=float, default=15.0)
    parser.add_argument("--runtime-dir", type=Path)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.samples < 1:
        raise TraceError("--samples must be positive")
    if not 0 <= args.entity_type <= 0xff:
        raise TraceError("--entity-type must be between 0 and 255")
    if args.select_level is not None and len(args.select_level) != 4:
        raise TraceError("--select-level must look like W4L1")
    if args.reload_level is not None and len(args.reload_level) != 4:
        raise TraceError("--reload-level must look like W4L1")
    if args.reload_after_collect and args.select_level is None and args.reload_level is None:
        raise TraceError("--reload-after-collect requires --select-level or --reload-level")
    if args.reload_wait_frames < 0:
        raise TraceError("--reload-wait-frames must be non-negative")
    if args.effect_table_attempts < 1:
        raise TraceError("--effect-table-attempts must be positive")
    if args.force_tile_mask is not None and not 0 <= args.force_tile_mask <= 0xffff:
        raise TraceError("--force-tile-mask must be between 0 and 65535")
    if args.puzzle_probe_frames < 0:
        raise TraceError("--puzzle-probe-frames must be non-negative")
    if args.prestream_input_frames < 0:
        raise TraceError("--prestream-input-frames must be non-negative")
    if args.prestream_input_frames and not args.prestream_input_key:
        raise TraceError("--prestream-input-frames requires --prestream-input-key")
    if args.trace_cloud_consumers and args.cloud_consumer_offset not in (0x4087, 0x4406):
        raise TraceError("--cloud-consumer-offset must be 0x4087 or 0x4406")
    if args.trace_cloud_outer_renderer and args.entity_type != 0x28:
        raise TraceError("--trace-cloud-outer-renderer requires --entity-type 0x28")
    if args.trace_cloud_hardware_renderer and args.entity_type != 0x28:
        raise TraceError("--trace-cloud-hardware-renderer requires --entity-type 0x28")
    if not 0 <= args.lifecycle_return_camera_x <= 0xffff:
        raise TraceError("--lifecycle-return-camera-x must be between 0 and 65535")
    if not 0 <= args.lifecycle_return_camera_y <= 0xffff:
        raise TraceError("--lifecycle-return-camera-y must be between 0 and 65535")
    if args.cloud_hardware_frames < 1:
        raise TraceError("--cloud-hardware-frames must be positive")
    if (args.camera_x is None) != (args.camera_y is None):
        raise TraceError("--camera-x and --camera-y must be used together")
    if (args.initial_camera_x is None) != (args.initial_camera_y is None):
        raise TraceError("--initial-camera-x and --initial-camera-y must be used together")
    for name in ("camera_x", "camera_y"):
        value = getattr(args, name)
        if value is not None and not 0 <= value <= 0xffff:
            raise TraceError(f"--{name.replace('_', '-')} must be between 0 and 65535")
    for name in ("initial_camera_x", "initial_camera_y"):
        value = getattr(args, name)
        if value is not None and not 0 <= value <= 0xffff:
            raise TraceError(f"--{name.replace('_', '-')} must be between 0 and 65535")

    repo_root = Path(__file__).resolve().parents[2]
    startup_recording = args.startup_recording
    if not startup_recording.is_absolute():
        startup_recording = repo_root / startup_recording
    script_path = repo_root / "research/automation/quiky_object_behavior_trace.lua"
    output = args.output if args.output.is_absolute() else repo_root / args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    process = None
    log_stream = None

    if args.launch:
        port = reserve_local_port()
        token = secrets.token_hex(32)
        env = os.environ.copy()
        env["DOSBOX_API_TOKEN"] = token
        if args.runtime_dir is not None:
            runtime_dir = args.runtime_dir.resolve()
            if not (runtime_dir / "QUIKY.EXE").is_file() or not (runtime_dir / "NESTLE.DAT").is_file():
                raise TraceError("--runtime-dir must contain QUIKY.EXE and NESTLE.DAT")
            env["QUIKY_AUTOMATION_TARGET"] = str(runtime_dir / "QUIKY.EXE")
            env["DOSBOX_AUTOMATION_DATA_HOME"] = str(runtime_dir.parent)
        if args.headless:
            env["SDL_VIDEODRIVER"] = "dummy"
            env["SDL_AUDIODRIVER"] = "dummy"
        log_path = output.with_suffix(output.suffix + ".dosbox.log")
        log_stream = log_path.open("wb")
        command = [str(repo_root / "scripts/run-dosbox-automation.sh"),
                   "--set", f"webserver_port={port}"]
        if args.runtime_dir is not None:
            runtime_dir = args.runtime_dir.resolve()
            command.extend(["--set", f"mount_allowed_bases={runtime_dir}",
                            "--set", f"mount_allowed_image_roots={runtime_dir}"])
        process = subprocess.Popen(command, cwd=repo_root, env=env,
                                   stdout=log_stream, stderr=subprocess.STDOUT)
        api = ApiClient(f"http://127.0.0.1:{port}", token)
        try:
            info = wait_for_api(api, process, args.startup_timeout)
        except Exception:
            process.terminate()
            process.wait(timeout=5)
            log_stream.close()
            raise
    else:
        api = ApiClient(args.url, discover_token(args.token_file))
        info = api.get("/api/v1/dosbox/info")

    try:
        if not info.get("features", {}).get("debugger"):
            raise TraceError("the running dosbox-automation build has no debugger API")
        config = ObjectBehaviorConfig(
            record_offset=args.record_offset,
            entity_type=args.entity_type,
            samples=args.samples,
            startup_recording=startup_recording,
            timeout=args.timeout,
            poll_interval=args.poll_interval,
            select_level=args.select_level,
            selector_frames=args.selector_frames,
            initial_camera_x=args.initial_camera_x,
            initial_camera_y=args.initial_camera_y,
            prestream_input_key=args.prestream_input_key,
            prestream_input_frames=args.prestream_input_frames,
        camera_x=args.camera_x,
        camera_y=args.camera_y,
        sprite_init_offset=args.sprite_init_offset,
        align_object_to_player=args.align_object_to_player,
        trace_overlap=args.trace_overlap,
        trace_collision=args.trace_collision,
        trace_platform=args.trace_platform,
        trace_bump=args.trace_bump,
        trace_contact=args.trace_contact,
        trace_effect_table=args.trace_effect_table,
        effect_table_attempts=args.effect_table_attempts,
        trace_stream_lifecycle=args.trace_stream_lifecycle,
        lifecycle_return_camera_x=args.lifecycle_return_camera_x,
        lifecycle_return_camera_y=args.lifecycle_return_camera_y,
        force_active_player_bounds=args.force_active_player_bounds,
        force_bump_player_state=args.force_bump_player_state,
        force_cloud_player_state=args.force_cloud_player_state,
        trace_cloud_consumers=args.trace_cloud_consumers,
        cloud_consumer_offset=args.cloud_consumer_offset,
        trace_cloud_outer_renderer=args.trace_cloud_outer_renderer,
        trace_cloud_hardware_renderer=args.trace_cloud_hardware_renderer,
        cloud_hardware_frames=args.cloud_hardware_frames,
        force_contact_gate=args.force_contact_gate,
        align_x_offset=args.align_x_offset,
        align_y_offset=args.align_y_offset,
        force_velocity_x=args.force_velocity_x,
        force_velocity_y=args.force_velocity_y,
        force_platform_ready=args.force_platform_ready,
        reload_after_collect=args.reload_after_collect,
        reload_level=args.reload_level,
        reload_wait_frames=args.reload_wait_frames,
        force_tile_mask=args.force_tile_mask,
        trace_puzzle_completion=args.trace_puzzle_completion,
        force_completion_outer_state=args.force_completion_outer_state,
        force_completion_wait_release=args.force_completion_wait_release,
        puzzle_probe_frames=args.puzzle_probe_frames,
        )
        trace = trace_object_behavior(api, script_path, config)
        envelope = {
            "schema": "quiky-object-behavior-v1",
            "created_utc": datetime.now(timezone.utc).isoformat(),
            "dosbox": info,
            "trace_kind": "object-behavior",
            "script": str(script_path),
            "script_sha256": sha256(script_path),
            "startup_recording": str(startup_recording),
            "startup_recording_sha256": sha256(startup_recording),
            "config": lua_config(config),
            "events": [trace],
        }
        output.write_text(json.dumps(envelope, indent=2) + "\n", encoding="utf-8")
        print(f"wrote object behavior trace to {output}")
        return 0
    finally:
        if process is not None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)
        if log_stream is not None:
            log_stream.close()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except TraceError as exc:
        print(f"object-behavior-trace: {exc}", file=sys.stderr)
        raise SystemExit(1)
