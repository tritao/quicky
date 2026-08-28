"""Interactive DOS capture sessions and deterministic run processing."""

from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

from .common import ToolError, file_fingerprint, read_json, tools_root, write_json
from .runs import stage_run_files
from .state import import_input, import_trace, save_state_jsonl

CAPTURE_SCHEMA = "quiky.capture-session-v1"


def _mark_incomplete(capture: Path, temporary: Path,
                     manifest_path: Path, manifest: dict) -> None:
    manifest["status"] = "incomplete"
    if temporary.is_file():
        partial = capture / "capture.partial.jsonl"
        temporary.replace(partial)
        fingerprint = file_fingerprint(partial)
        fingerprint["path"] = "capture.partial.jsonl"
        manifest["files"] = {"capture.partial.jsonl": fingerprint}
    write_json(manifest_path, manifest)


def process_capture(capture: Path, run: Path, *, name: str,
                    profile: str) -> None:
    manifest = read_json(capture / "manifest.json")
    if (not isinstance(manifest, dict) or manifest.get("schema") != CAPTURE_SCHEMA or
            manifest.get("status") != "complete"):
        raise ToolError(f"capture is not complete: {capture}")
    source = capture / "capture.json"
    actual = file_fingerprint(source)
    expected = manifest.get("files", {}).get("capture.json", {})
    if (actual["size"] != expected.get("size") or
            actual["sha256"] != expected.get("sha256")):
        raise ToolError(f"capture fingerprint mismatch: {source}")
    with tempfile.TemporaryDirectory(prefix="quiky-capture-process-") as temp:
        expected_state = Path(temp) / "expected-state.jsonl"
        save_state_jsonl(expected_state, import_trace(source, profile))
        provenance = {"dos_capture": file_fingerprint(source)}
        for input_name, fingerprint in manifest.get("inputs", {}).items():
            provenance[input_name] = fingerprint
        stage_run_files(
            run, name=name, profile=profile, input_rows=import_input(source),
            expected_state=expected_state,
            provenance=provenance)


def capture_session(*, name: str, level: str, runtime_dir: Path,
                    profile: str, capture_only: bool, captures_root: Path,
                    runs_root: Path) -> tuple[Path, Path | None]:
    if not name or Path(name).name != name:
        raise ToolError("capture name must be one path component")
    if (len(level) != 4 or level[0] != "W" or level[1] not in "12345" or
            level[2] != "L" or level[3] not in "1234"):
        raise ToolError("capture level must look like W1L1")
    capture, run = captures_root / name, runs_root / name
    if capture.exists() and any(capture.iterdir()):
        raise ToolError(f"capture directory is not empty: {capture}")
    inputs = {}
    for input_name, path in (
            ("executable", runtime_dir / "QUIKY.EXE"),
            ("archive", runtime_dir / "NESTLE.DAT"),
            ("trace_script", tools_root().parent / "automation/quiky_player_trace.lua")):
        if not path.is_file():
            raise ToolError(f"capture input does not exist: {path}")
        inputs[input_name] = file_fingerprint(path)
    capture.mkdir(parents=True, exist_ok=True)
    temporary = capture / ".capture.json.tmp"
    manifest_path = capture / "manifest.json"
    manifest = {"schema": CAPTURE_SCHEMA, "format_version": 1, "name": name,
                "status": "recording", "level": level, "files": {}}
    manifest["inputs"] = inputs
    write_json(manifest_path, manifest)
    command = [
        sys.executable, str(tools_root() / "quikytrace.py"),
        "--launch", "--runtime-dir", str(runtime_dir),
        "--output", str(temporary), "--select-level", level,
        "--player-trace", "--player-focus-callback", "--player-capture-record",
        "--player-parity-capture", "--interactive-capture",
        "--player-frames-between", "0", "--timeout", "86400",
    ]
    try:
        completed = subprocess.run(command, check=False)
    except KeyboardInterrupt as exc:
        _mark_incomplete(capture, temporary, manifest_path, manifest)
        raise ToolError("interactive capture interrupted") from exc
    if completed.returncode or not temporary.is_file():
        _mark_incomplete(capture, temporary, manifest_path, manifest)
        raise ToolError(f"interactive capture failed with status {completed.returncode}")
    source = capture / "capture.json"
    temporary.replace(source)
    fingerprint = file_fingerprint(source)
    fingerprint["path"] = "capture.json"
    manifest.update(status="complete", files={"capture.json": fingerprint})
    write_json(manifest_path, manifest)
    if capture_only:
        return capture, None
    process_capture(capture, run, name=name, profile=profile)
    return capture, run
