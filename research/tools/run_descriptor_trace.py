#!/usr/bin/env python3
"""Run the focused descriptor/MAP construction Lua probe.

The DOSBox binary is selected with ``DOSBOX_AUTOMATION_BIN`` so a worktree can
reuse a debugger build produced in another checkout without copying generated
artifacts into source control.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import secrets
import subprocess
import time
from pathlib import Path
import sys

TOOLS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS_DIR))

from quikytrace import ApiClient, TraceError, reserve_local_port, wait_for_api  # noqa: E402


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def run_trace(level: str, output: Path, runtime_dir: Path, timeout: float,
              selector_frames: int, secondary: bool, map_writer: bool) -> dict:
    repo_root = Path(__file__).resolve().parents[2]
    binary = os.environ.get(
        "DOSBOX_AUTOMATION_BIN",
        str(repo_root / "research/build/dosbox-automation-debug/dosbox_with_debugger"),
    )
    if not Path(binary).is_file():
        raise TraceError(f"DOSBOX_AUTOMATION_BIN is not a file: {binary}")
    if not (runtime_dir / "QUIKY.EXE").is_file() or not (runtime_dir / "NESTLE.DAT").is_file():
        raise TraceError(f"runtime directory lacks QUIKY.EXE/NESTLE.DAT: {runtime_dir}")

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
    output.parent.mkdir(parents=True, exist_ok=True)
    log_stream = output.with_suffix(output.suffix + ".dosbox.log").open("wb")
    command = [
        str(repo_root / "scripts/run-dosbox-automation.sh"),
        "--set", f"webserver_port={port}",
        "--set", f"mount_allowed_bases={runtime_dir}",
        "--set", f"mount_allowed_image_roots={runtime_dir}",
    ]
    process = subprocess.Popen(command, cwd=repo_root, env=env,
                               stdout=log_stream, stderr=subprocess.STDOUT)
    api = ApiClient(f"http://127.0.0.1:{port}", token)
    try:
        info = wait_for_api(api, process, 20.0)
        if not info.get("features", {}).get("debugger"):
            raise TraceError("DOSBox automation build has no debugger API")
        source_path = repo_root / "research/automation/quiky_descriptor_construction_trace.lua"
        prefix = (
            f"TRACE_TIMEOUT_MS={round(timeout * 1000)}\n"
            f"TRACE_SELECT_LEVEL={json.dumps(level)}\n"
            f"TRACE_SELECTOR_FRAMES={selector_frames}\n"
            f"TRACE_SECONDARY={'true' if secondary else 'false'}\n"
            f"TRACE_MAP_WRITER={'true' if map_writer else 'false'}\n"
        )
        api.request("POST", "/api/v1/script/load?name=quiky-descriptor-construction",
                    text_body=prefix + source_path.read_text(encoding="utf-8"))
        api.post("/api/v1/script/start")
        recording = json.loads(
            (repo_root / "research/automation/startup-to-input.json").read_text(
                encoding="utf-8"
            )
        )
        deadline = time.monotonic() + timeout + 30
        replayed = False
        while time.monotonic() < deadline:
            status = api.get("/api/v1/script/status")
            if not replayed and status.get("output", {}).get("awaiting_startup_replay"):
                api.post("/api/v1/input/sequence", {"events": recording["events"]})
                replayed = True
            result = status.get("output", {}).get("result")
            if isinstance(result, dict):
                payload = {
                    "schema": "quiky-runtime-descriptor-construction-trace-v1",
                    "level": level,
                    "executable": str(runtime_dir / "QUIKY.EXE"),
                    "executable_sha256": sha256(runtime_dir / "QUIKY.EXE"),
                    "archive": str(runtime_dir / "NESTLE.DAT"),
                    "archive_sha256": sha256(runtime_dir / "NESTLE.DAT"),
                    "script": str(source_path),
                    "script_sha256": sha256(source_path),
                    "result": result,
                }
                output.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
                return payload
            if status.get("state") == "error":
                detail = status.get("error", "Lua trace failed")
                try:
                    status["cpu"] = api.get("/api/v1/cpu/state")
                    status["debugger"] = api.get("/api/v1/debug/state")
                except Exception:
                    pass
                output.with_suffix(output.suffix + ".failure.json").write_text(
                    json.dumps(status, indent=2) + "\n", encoding="utf-8"
                )
                raise TraceError(detail)
            time.sleep(0.05)
        raise TraceError("timed out waiting for descriptor construction trace")
    finally:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)
        log_stream.close()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--level", default="W1L1")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--runtime-dir", type=Path,
                        default=Path(__file__).resolve().parents[2] / "game")
    parser.add_argument("--timeout", type=float, default=90.0)
    parser.add_argument("--selector-frames", type=int, default=80)
    parser.add_argument("--secondary", action="store_true",
                        help="continue into the secondary MAP loader path")
    parser.add_argument("--map-writer", action="store_true",
                        help="run the controlled 01F7:5C9D MAP-writer probe")
    args = parser.parse_args()
    try:
        payload = run_trace(args.level, args.output, args.runtime_dir.resolve(),
                            args.timeout, args.selector_frames, args.secondary,
                            args.map_writer)
    except TraceError as exc:
        parser.error(str(exc))
    print(json.dumps({
        "output": str(args.output),
        "level": payload["level"],
        "descriptor_count": payload["result"]["descriptor_after"]["count"],
        "map_mutation_delta": payload["result"]["mutation"]["delta"],
        "secondary_map_mutation_delta": (
            payload["result"]["secondary"]["mutation"]["delta"]
            if payload["result"].get("secondary") else None
        ),
        "map_writer_after_word": (
            payload["result"]["map_writer"]["after_word"]
            if payload["result"].get("map_writer") else None
        ),
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
