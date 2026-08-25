#!/usr/bin/env python3
"""Run the controlled late-scene callback probe in an isolated DOSBox."""

from __future__ import annotations

import argparse
import json
import os
import secrets
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path

from quikytrace import (
    ApiClient,
    TraceError,
    capture_failure,
    lua_literal,
    reserve_local_port,
    sha256,
    wait_for_api,
)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--runtime-dir", type=Path, default=Path("game"))
    parser.add_argument("--select-level", default="W5L4")
    parser.add_argument("--route", type=int, default=1)
    parser.add_argument("--seed-score", type=int, default=0)
    parser.add_argument("--timeout", type=float, default=120.0)
    parser.add_argument("--startup-timeout", type=float, default=20.0)
    return parser


def run(args: argparse.Namespace) -> int:
    repo_root = Path(__file__).resolve().parents[2]
    runtime_dir = args.runtime_dir.resolve()
    executable = runtime_dir / "QUIKY.EXE"
    archive = runtime_dir / "NESTLE.DAT"
    script_path = repo_root / "research/automation/quiky_ending_object_probe.lua"
    startup_recording = repo_root / "research/automation/startup-to-input.json"
    if not executable.is_file() or not archive.is_file():
        raise TraceError("--runtime-dir must contain QUIKY.EXE and NESTLE.DAT")

    port = reserve_local_port()
    token = secrets.token_hex(32)
    env = os.environ.copy()
    env["DOSBOX_API_TOKEN"] = token
    env["QUIKY_AUTOMATION_TARGET"] = str(executable)
    env["SDL_VIDEODRIVER"] = "dummy"
    env["SDL_AUDIODRIVER"] = "dummy"
    if runtime_dir.name == "game":
        env["DOSBOX_AUTOMATION_DATA_HOME"] = str(runtime_dir.parent)

    log_path = args.output.with_suffix(args.output.suffix + ".dosbox.log")
    log_path.parent.mkdir(parents=True, exist_ok=True)
    log_stream = log_path.open("wb")
    process: subprocess.Popen[bytes] | None = None
    api: ApiClient | None = None
    try:
        command = [
            str(repo_root / "scripts/run-dosbox-automation.sh"),
            "--set", f"webserver_port={port}",
            "--set", f"mount_allowed_bases={runtime_dir}",
            "--set", f"mount_allowed_image_roots={runtime_dir}",
        ]
        process = subprocess.Popen(
            command, cwd=repo_root, env=env,
            stdout=log_stream, stderr=subprocess.STDOUT,
        )
        api = ApiClient(f"http://127.0.0.1:{port}", token)
        info = wait_for_api(api, process, args.startup_timeout)
        if not info.get("features", {}).get("debugger"):
            raise TraceError("the running dosbox-automation build has no debugger API")

        source = script_path.read_text(encoding="utf-8")
        prefix = "TRACE_CONFIG = " + lua_literal({
            "timeout_ms": round(args.timeout * 1000),
            "select_level": args.select_level,
            "route": args.route,
            "seed_score": args.seed_score,
        }) + "\n"
        api.request(
            "POST", "/api/v1/script/load?name=ending-object-probe",
            text_body=prefix + source,
        )
        api.post("/api/v1/script/start")

        recording = json.loads(startup_recording.read_text(encoding="utf-8"))
        deadline = time.monotonic() + args.timeout + args.startup_timeout
        replayed = False
        trace: dict | None = None
        while time.monotonic() < deadline:
            status = api.get("/api/v1/script/status")
            if status.get("state") == "error":
                raise TraceError(f"Lua trace failed: {status.get('error', 'unknown error')}")
            output = status.get("output", {})
            if output.get("awaiting_startup_replay") and not replayed:
                api.post("/api/v1/input/sequence", {"events": recording["events"]})
                replayed = True
            if isinstance(output.get("ending_trace"), dict):
                trace = output["ending_trace"]
                break
            time.sleep(0.05)
        if trace is None:
            raise TraceError("timed out waiting for ending_trace")

        artifact = {
            "trace_schema_version": 1,
            "created_utc": datetime.now(timezone.utc).isoformat(),
            "executable": str(executable),
            "executable_sha256": sha256(executable),
            "archive": str(archive),
            "archive_sha256": sha256(archive),
            "script": str(script_path),
            "script_sha256": sha256(script_path),
            "trace": trace,
        }
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(artifact, indent=2) + "\n", encoding="utf-8")
        return 0
    except Exception as error:
        if api is not None:
            capture_failure(api, args.output, error)
        raise
    finally:
        if process is not None and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)
        log_stream.close()


def main() -> int:
    args = build_parser().parse_args()
    try:
        return run(args)
    except TraceError as error:
        print(f"ending_object_probe: {error}", flush=True)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
