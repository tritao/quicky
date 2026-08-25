#!/usr/bin/env python3
"""Capture repeated BOB/ICO renderer entries for draw-order evidence."""

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
    if isinstance(value, list):
        return value
    if isinstance(value, dict):
        try:
            return [value[key] for key in sorted(value, key=int)]
        except (TypeError, ValueError):
            return value
    return value


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--level", default="W4L1")
    parser.add_argument("--focus", choices=("bob", "ico"), default="bob")
    parser.add_argument("--runtime-dir", type=Path,
                        help="isolated game directory containing QUIKY.EXE/NESTLE.DAT")
    parser.add_argument("--events", type=int, default=48)
    parser.add_argument("--timeout", type=float, default=5.0)
    parser.add_argument("--walk-objects", action="store_true",
                        help="walk successive objects in one renderer list pass")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.events < 1:
        parser.error("--events must be positive")
    repo_root = Path(__file__).resolve().parents[2]
    script_path = repo_root / "research/automation/quiky_renderer_census.lua"
    port = reserve_local_port()
    token = secrets.token_hex(32)
    env = os.environ.copy()
    env["DOSBOX_API_TOKEN"] = token
    runtime_dir = args.runtime_dir.resolve() if args.runtime_dir else None
    if runtime_dir is not None:
        if not (runtime_dir / "QUIKY.EXE").is_file() or not (runtime_dir / "NESTLE.DAT").is_file():
            parser.error("--runtime-dir must contain QUIKY.EXE and NESTLE.DAT")
        env["QUIKY_AUTOMATION_TARGET"] = str(runtime_dir / "QUIKY.EXE")
        if runtime_dir.name == "game":
            env["DOSBOX_AUTOMATION_DATA_HOME"] = str(runtime_dir.parent)
    log_path = args.output.with_suffix(args.output.suffix + ".dosbox.log")
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("wb") as log_stream:
        process = subprocess.Popen(
            [str(repo_root / "scripts/run-dosbox-automation.sh"),
             "--set", f"webserver_port={port}"] +
            ([] if runtime_dir is None else
             ["--set", f"mount_allowed_bases={runtime_dir}",
              "--set", f"mount_allowed_image_roots={runtime_dir}"]),
            cwd=repo_root, env=env, stdout=log_stream, stderr=subprocess.STDOUT,
        )
    api = ApiClient(f"http://127.0.0.1:{port}", token)
    try:
        info = wait_for_api(api, process, 15.0)
        prefix = (
            f"TRACE_LEVEL={json.dumps(args.level)}\n"
            f"TRACE_FOCUS={json.dumps(args.focus)}\n"
            f"TRACE_EVENT_LIMIT={args.events}\n"
            f"TRACE_TIMEOUT_MS={round(args.timeout * 1000)}\n"
            f"TRACE_WALK_OBJECTS={'true' if args.walk_objects else 'false'}\n"
        )
        api.request("POST", "/api/v1/script/load?name=quiky-renderer-census",
                    text_body=prefix + script_path.read_text(encoding="utf-8"))
        api.post("/api/v1/script/start")
        recording = json.loads(
            (repo_root / "research/automation/startup-to-input.json").read_text(
                encoding="utf-8"))
        deadline = time.monotonic() + args.timeout * (args.events + 3) + 30
        replayed = False
        while time.monotonic() < deadline:
            status = api.get("/api/v1/script/status")
            if status.get("state") == "error":
                raise TraceError(status.get("error", "renderer census failed"))
            output = status.get("output", {})
            if not replayed and output.get("awaiting_startup_replay"):
                api.post("/api/v1/input/sequence", recording)
                replayed = True
            if status.get("state") == "completed":
                ledger = {
                    "schema": "quiky-renderer-census-v1",
                    "dosbox": info,
                    "inputs": {"level": args.level, "focus": args.focus,
                               "events": args.events,
                               "walk_objects": args.walk_objects,
                               "runtime_dir": str(runtime_dir) if runtime_dir else None},
                    "executable": str((runtime_dir or (repo_root / "game")) / "QUIKY.EXE"),
                    "executable_sha256": sha256((runtime_dir or (repo_root / "game")) / "QUIKY.EXE"),
                    "archive": str((runtime_dir or (repo_root / "game")) / "NESTLE.DAT"),
                    "archive_sha256": sha256((runtime_dir or (repo_root / "game")) / "NESTLE.DAT"),
                    "script": str(script_path),
                    "script_sha256": sha256(script_path),
                    "checkpoints": output.get("checkpoints", {}),
                    "renderer_events": ordered(output.get("renderer_census", [])),
                }
                args.output.parent.mkdir(parents=True, exist_ok=True)
                args.output.write_text(json.dumps(ledger, indent=2) + "\n",
                                       encoding="utf-8")
                print(f"wrote renderer census to {args.output}")
                return 0
            time.sleep(0.05)
        raise TraceError("timed out waiting for renderer census")
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


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except TraceError as exc:
        print(f"quiky_renderer_census: {exc}", file=sys.stderr)
        raise SystemExit(1)
