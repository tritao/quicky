#!/usr/bin/env python3
"""Capture the retail BIENE startup table by entering 01F7:0A43."""

from __future__ import annotations

import argparse
import json
import os
import secrets
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path

from quikytrace import ApiClient, TraceError, lua_literal, reserve_local_port, sha256, wait_for_api


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--runtime-dir", type=Path, default=Path("game"))
    parser.add_argument("--timeout", type=float, default=120.0)
    parser.add_argument("--startup-timeout", type=float, default=20.0)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[2]
    runtime_dir = args.runtime_dir.resolve()
    executable = runtime_dir / "QUIKY.EXE"
    archive = runtime_dir / "NESTLE.DAT"
    script_path = repo_root / "research/automation/quiky_biene_forced_table_capture.lua"
    startup_path = repo_root / "research/automation/startup-to-input.json"
    if not executable.is_file() or not archive.is_file():
        raise TraceError("--runtime-dir must contain QUIKY.EXE and NESTLE.DAT")

    port = reserve_local_port()
    token = secrets.token_hex(32)
    env = os.environ.copy()
    env.update({
        "DOSBOX_API_TOKEN": token,
        "QUIKY_AUTOMATION_TARGET": str(executable),
        "SDL_VIDEODRIVER": "dummy",
        "SDL_AUDIODRIVER": "dummy",
    })
    if runtime_dir.name == "game":
        env["DOSBOX_AUTOMATION_DATA_HOME"] = str(runtime_dir.parent)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    log_path = args.output.with_suffix(args.output.suffix + ".dosbox.log")
    with log_path.open("wb") as log_stream:
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
            info = wait_for_api(api, process, args.startup_timeout)
            if not info.get("features", {}).get("debugger"):
                raise TraceError("the running dosbox-automation build has no debugger API")
            prefix = "TRACE_CONFIG = " + lua_literal({
                "timeout_ms": round(args.timeout * 1000),
            }) + "\n"
            api.request("POST", "/api/v1/script/load?name=biene-forced-table-capture",
                        text_body=prefix + script_path.read_text(encoding="utf-8"))
            api.post("/api/v1/script/start")
            recording = json.loads(startup_path.read_text(encoding="utf-8"))
            deadline = time.monotonic() + args.timeout + args.startup_timeout
            replayed = False
            trace = None
            while time.monotonic() < deadline:
                status = api.get("/api/v1/script/status")
                if status.get("state") == "error":
                    raise TraceError(f"Lua trace failed: {status.get('error', 'unknown error')}")
                output = status.get("output", {})
                if output.get("awaiting_startup_replay") and not replayed:
                    api.post("/api/v1/input/sequence", {"events": recording["events"]})
                    replayed = True
                if isinstance(output.get("biene_startup_table"), dict):
                    trace = output["biene_startup_table"]
                    break
                time.sleep(0.05)
            if trace is None:
                raise TraceError("timed out waiting for biene_startup_table")
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
            args.output.write_text(json.dumps(artifact, indent=2) + "\n",
                                   encoding="utf-8")
            return 0
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


if __name__ == "__main__":
    raise SystemExit(main())
