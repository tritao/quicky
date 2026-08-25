#!/usr/bin/env python3
"""Run the controlled original SCORE.DAT writer entry in DOSBox."""

from __future__ import annotations

import argparse
import json
import os
import secrets
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path

from quikytrace import (ApiClient, TraceError, capture_failure, lua_literal,
                        reserve_local_port, sha256, wait_for_api)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--runtime-dir", type=Path, default=Path("game"))
    parser.add_argument("--timeout", type=float, default=30.0)
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[2]
    runtime = args.runtime_dir.resolve()
    exe, archive = runtime / "QUIKY.EXE", runtime / "NESTLE.DAT"
    if not exe.is_file() or not archive.is_file():
        raise TraceError("--runtime-dir must contain QUIKY.EXE and NESTLE.DAT")
    port = reserve_local_port()
    token = secrets.token_hex(32)
    env = os.environ.copy()
    env.update({
        "DOSBOX_API_TOKEN": token,
        "QUIKY_AUTOMATION_TARGET": str(exe),
        "SDL_VIDEODRIVER": "dummy",
        "SDL_AUDIODRIVER": "dummy",
    })
    env["DOSBOX_AUTOMATION_DATA_HOME"] = str(runtime.parent)
    output_log = args.output.with_suffix(args.output.suffix + ".dosbox.log")
    output_log.parent.mkdir(parents=True, exist_ok=True)
    stream = output_log.open("wb")
    process = None
    api = None
    try:
        process = subprocess.Popen([
            str(root / "scripts/run-dosbox-automation.sh"),
            "--set", f"webserver_port={port}",
            "--set", f"mount_allowed_bases={runtime}",
            "--set", f"mount_allowed_image_roots={runtime}",
        ], cwd=root, env=env, stdout=stream, stderr=subprocess.STDOUT)
        api = ApiClient(f"http://127.0.0.1:{port}", token)
        info = wait_for_api(api, process, 20.0)
        if not info.get("features", {}).get("debugger"):
            raise TraceError("debugger API unavailable")
        source = (root / "research/automation/quiky_score_writer_probe.lua").read_text()
        api.request("POST", "/api/v1/script/load?name=score-writer-probe",
                    text_body="TRACE_CONFIG = " + lua_literal({
                        "timeout_ms": round(args.timeout * 1000),
                    }) + "\n" + source)
        api.post("/api/v1/script/start")
        recording = json.loads((root / "research/automation/startup-to-input.json").read_text())
        deadline = time.monotonic() + args.timeout + 20
        replayed = False
        trace = None
        while time.monotonic() < deadline:
            status = api.get("/api/v1/script/status")
            if status.get("state") == "error":
                raise TraceError(status.get("error", "Lua trace failed"))
            output = status.get("output", {})
            if output.get("awaiting_startup_replay") and not replayed:
                api.post("/api/v1/input/sequence", {"events": recording["events"]})
                replayed = True
            if isinstance(output.get("score_writer_trace"), dict):
                trace = output["score_writer_trace"]
                break
            time.sleep(0.05)
        if trace is None:
            raise TraceError("timed out waiting for score_writer_trace")
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps({
            "trace_schema_version": 1,
            "created_utc": datetime.now(timezone.utc).isoformat(),
            "executable": str(exe), "executable_sha256": sha256(exe),
            "archive": str(archive), "archive_sha256": sha256(archive),
            "trace": trace,
        }, indent=2) + "\n")
        return 0
    except Exception as error:
        if api is not None:
            capture_failure(api, args.output, error)
        print(f"score_writer_probe: {error}", flush=True)
        return 2
    finally:
        if process is not None and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)
        stream.close()


if __name__ == "__main__":
    raise SystemExit(main())
