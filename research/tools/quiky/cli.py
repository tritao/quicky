"""Unified frontend for Quiky research and canonical parity runs."""

from __future__ import annotations

import argparse
import sys
import tempfile
from pathlib import Path

from .capture import capture_session, process_capture
from .common import ToolError, file_fingerprint, run_compat_script, tools_root
from .runs import (replay_run, stage_run_files, validate_run_directory,
                   verify_run_directory)
from .state import PROFILES, import_input, import_trace, save_state_jsonl


ALIASES = {
    "frame": "scene_frame_compare.py",
    "trace": "quikytrace.py",
    "ghidra-ne-segments": "ghidra_ne_segments.py",
}


def _top_parser() -> argparse.ArgumentParser:
    return argparse.ArgumentParser(
        prog="quiky",
        description=(
            "Quiky research tooling and strict recorded-run parity."
        ),
        epilog=(
            "Commands: capture, run import|replay|validate|verify, frame, trace, verify SCRIPT, "
            "ghidra SCRIPT.\n"
            "Examples: quiky run import RUN --name NAME --profile exact "
            "--expected-trace DOS.json; quiky run verify RUN"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )


def _run_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="quiky run")
    operations = parser.add_subparsers(dest="operation", required=True)
    importer = operations.add_parser("import")
    importer.add_argument("directory", type=Path)
    importer.add_argument("--name", required=True)
    importer.add_argument("--profile", choices=PROFILES, required=True)
    importer.add_argument("--expected-trace", type=Path, required=True)
    replay = operations.add_parser("replay")
    replay.add_argument("directory", type=Path)
    replay.add_argument("--binary", type=Path,
                        default=Path("build/engine/quiky-parity-replay"))
    replay.add_argument("--archive", type=Path)
    replay.add_argument("--map", dest="map_resource")
    replay.add_argument("--player-bob")
    replay.add_argument("--leaf-prng-index", type=int)
    replay.add_argument("--leaf-prng-ring-hex")
    for name in ("validate", "verify"):
        operation = operations.add_parser(name)
        operation.add_argument("directory", type=Path)
    return parser


def _capture_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="quiky capture")
    parser.add_argument("name")
    parser.add_argument("--level", default="W1L1")
    parser.add_argument("--runtime-dir", type=Path, default=Path("game"))
    parser.add_argument("--profile", choices=PROFILES, default="exact")
    parser.add_argument("--capture-only", action="store_true")
    parser.add_argument(
        "--diagnostic", action="store_true",
        help="retain full callback and MAP-property diagnostics for collision analysis",
    )
    parser.add_argument("--captures-root", type=Path, default=Path("research/captures"))
    parser.add_argument("--runs-root", type=Path, default=Path("research/runs"))
    return parser


def _capture_process_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="quiky capture process")
    parser.add_argument("capture", type=Path)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--name")
    parser.add_argument("--profile", choices=PROFILES, default="exact")
    parser.add_argument("--recover-incomplete", action="store_true")
    return parser


def _capture(args: list[str]) -> int:
    if args and args[0] == "process":
        parsed = _capture_process_parser().parse_args(args[1:])
        process_capture(parsed.capture, parsed.run,
                        name=parsed.name or parsed.run.name,
                        profile=parsed.profile,
                        recover_incomplete=parsed.recover_incomplete)
        print(f"OK: recorded run created at {parsed.run}")
        return 0
    parsed = _capture_parser().parse_args(args)
    print(f"Launching capture {parsed.name!r}; close the game window to finish.")
    capture, run = capture_session(
        name=parsed.name, level=parsed.level, runtime_dir=parsed.runtime_dir,
        profile=parsed.profile, capture_only=parsed.capture_only,
        captures_root=parsed.captures_root, runs_root=parsed.runs_root,
        diagnostic=parsed.diagnostic)
    print(f"OK: capture finalized at {capture}")
    if run is not None:
        print(f"OK: recorded run created at {run}")
    return 0


def _dispatch_named(group: str, args: list[str]) -> int:
    if not args or args[0] in ("-h", "--help"):
        if group == "verify":
            print("usage: quiky verify SCRIPT [SCRIPT_ARGS ...]")
            print("       SCRIPT is a legacy research/tools/*.py verifier")
        else:
            print("usage: quiky ghidra SCRIPT [SCRIPT_ARGS ...]")
            print("       SCRIPT is a legacy research/tools/*.py Ghidra helper")
        return 0 if args else 2
    name = args[0]
    if name in ALIASES:
        name = ALIASES[name]
    if group == "verify" and not name.startswith("verify_") and "/" not in name:
        name = f"verify_{name}"
    if not name.endswith(".py") and "/" not in name:
        name += ".py"
    path = Path(name)
    if not path.is_absolute():
        path = tools_root() / path
    return run_compat_script(path, args[1:])


def main(argv: list[str] | None = None) -> int:
    args = list(sys.argv[1:] if argv is None else argv)
    if not args or args[0] in ("-h", "--help"):
        _top_parser().print_help()
        return 0
    command, tail = args[0], args[1:]
    try:
        if command == "capture":
            return _capture(tail)
        if command == "run":
            parsed = _run_parser().parse_args(tail)
            if parsed.operation == "import":
                with tempfile.TemporaryDirectory(prefix="quiky-run-import-") as temp:
                    root = Path(temp)
                    expected = root / "expected-state.jsonl"
                    save_state_jsonl(expected, import_trace(
                        parsed.expected_trace, parsed.profile))
                    stage_run_files(
                        parsed.directory, name=parsed.name,
                        profile=parsed.profile,
                        input_rows=import_input(parsed.expected_trace),
                        expected_state=expected,
                        provenance={
                            "expected_trace": file_fingerprint(parsed.expected_trace),
                        })
                print(f"OK: recorded run imported at {parsed.directory}")
                return 0
            if parsed.operation == "replay":
                replay_run(
                    parsed.directory, binary=parsed.binary,
                    archive=parsed.archive, map_resource=parsed.map_resource,
                    player_bob=parsed.player_bob,
                    leaf_prng_index=parsed.leaf_prng_index,
                    leaf_prng_ring_hex=parsed.leaf_prng_ring_hex)
                print(f"OK: replayed canonical state for {parsed.directory}")
                return 0
            if parsed.operation == "validate":
                validate_run_directory(parsed.directory)
                print(f"OK: recorded run {parsed.directory}")
                return 0
            mismatches, coverage = verify_run_directory(parsed.directory)
            if mismatches:
                print(f"MISMATCH fields={len(mismatches)} coverage_gaps={len(coverage)}")
                return 1
            print(f"OK: recorded run parity; coverage_gaps={len(coverage)}")
            return 0
        if command == "frame":
            return run_compat_script(ALIASES["frame"], tail)
        if command == "trace":
            return run_compat_script(ALIASES["trace"], tail)
        if command == "verify":
            return _dispatch_named("verify", tail)
        if command == "ghidra":
            return _dispatch_named("ghidra", tail)
    except SystemExit as exc:
        return int(exc.code or 0)
    except ToolError as exc:
        print(f"quiky: {exc}", file=sys.stderr)
        return 2
    print(f"quiky: unknown command {command!r}; use --help", file=sys.stderr)
    return 2
