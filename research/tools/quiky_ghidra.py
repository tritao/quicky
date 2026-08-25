#!/usr/bin/env python3
"""Validate Quiky's Ghidra manifest and audit an annotated project with PyGhidra.

Manifest validation has no Ghidra dependency. ``audit-project`` starts the
bundled PyGhidra JVM, opens the existing raw-segment project, verifies the
canonical player-closure symbols, and decompiles its function entries. It is a
read-only proof of concept; the Java annotation scripts remain authoritative
until manifest application reaches output parity.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any


class AnalysisError(Exception):
    """Raised when the manifest or Ghidra project disagrees with evidence."""


def load_manifest(path: Path, repository: Path) -> dict[str, Any]:
    try:
        manifest = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise AnalysisError(f"{path}: cannot read manifest: {exc}") from exc
    if manifest.get("schema") != "quiky-ghidra-analysis-v1":
        raise AnalysisError("unsupported Ghidra manifest schema")
    executable = manifest.get("executable")
    if not isinstance(executable, dict):
        raise AnalysisError("manifest has no executable object")
    executable_path = repository / str(executable.get("path", ""))
    try:
        digest = hashlib.sha256(executable_path.read_bytes()).hexdigest()
    except OSError as exc:
        raise AnalysisError(f"cannot read {executable_path}: {exc}") from exc
    if digest != executable.get("sha256"):
        raise AnalysisError(f"{executable_path}: executable SHA-256 mismatch")
    segments = manifest.get("segments")
    symbols = manifest.get("symbols")
    if not isinstance(segments, dict) or not isinstance(symbols, list):
        raise AnalysisError("manifest segments/symbols have invalid types")
    ranges = manifest.get("function_ranges", {})
    if not isinstance(ranges, dict):
        raise AnalysisError("manifest function_ranges has invalid type")
    for identity, value in ranges.items():
        if not isinstance(identity, str) or ":" not in identity:
            raise AnalysisError(f"invalid function range key {identity}")
        if not isinstance(value, list) or len(value) != 2:
            raise AnalysisError(f"invalid function range for {identity}")
        try:
            start, end = (int(str(item), 16) for item in value)
        except ValueError as exc:
            raise AnalysisError(f"invalid function range for {identity}") from exc
        if start >= end:
            raise AnalysisError(f"empty function range for {identity}")
    identities: set[tuple[int, int]] = set()
    names: set[str] = set()
    for index, symbol in enumerate(symbols):
        if not isinstance(symbol, dict):
            raise AnalysisError(f"symbol {index} is not an object")
        segment = symbol.get("segment")
        if str(segment) not in segments:
            raise AnalysisError(f"symbol {index} uses unknown segment {segment}")
        try:
            offset = int(str(symbol.get("offset")), 16)
        except ValueError as exc:
            raise AnalysisError(f"symbol {index} has an invalid offset") from exc
        identity = (segment, offset)
        if identity in identities:
            raise AnalysisError(f"duplicate symbol at segment {segment}:{offset:04X}")
        identities.add(identity)
        name = symbol.get("name")
        if not isinstance(name, str) or not name:
            raise AnalysisError(f"symbol {index} has no name")
        if name in names:
            raise AnalysisError(f"duplicate symbol name {name}")
        names.add(name)
        if symbol.get("confidence") not in {"confirmed", "mechanical", "provisional"}:
            raise AnalysisError(f"symbol {name} has invalid confidence")
        if not symbol.get("evidence"):
            raise AnalysisError(f"symbol {name} has no evidence")
    return manifest


def audit_project(manifest: dict[str, Any], project_dir: Path,
                  project_name: str, install_dir: Path | None) -> None:
    try:
        import pyghidra
    except ImportError as exc:
        raise AnalysisError(
            "PyGhidra is not installed; use Ghidra's support/pyghidraRun or "
            "install the bundled wheel in a virtual environment"
        ) from exc
    pyghidra.start(install_dir=install_dir)
    from ghidra.app.decompiler import DecompInterface

    segments = manifest["segments"]
    ranges = manifest.get("function_ranges", {})
    with pyghidra.open_project(project_dir, project_name) as project:
        for segment_number, segment in segments.items():
            selected = [
                symbol for symbol in manifest["symbols"]
                if symbol["segment"] == int(segment_number)
            ]
            if not selected:
                continue
            try:
                context = pyghidra.program_context(project, segment["program"])
                program = context.__enter__()
            except FileNotFoundError:
                if segment.get("required_in_raw_project", True):
                    raise AnalysisError(
                        f"required program {segment['program']} is absent"
                    )
                print(f"skip {segment_number}: optional raw data program absent")
                continue
            try:
                decompiler = DecompInterface()
                decompiler.openProgram(program)
                try:
                    for symbol in selected:
                        address = program.getAddressFactory().getDefaultAddressSpace().getAddress(
                            int(symbol["offset"], 16)
                        )
                        if symbol["kind"] == "label":
                            found = program.getSymbolTable().getPrimarySymbol(address)
                            actual = found.getName() if found is not None else None
                        else:
                            function = program.getFunctionManager().getFunctionAt(address)
                            if function is None:
                                raise AnalysisError(
                                    f"{segment_number}:{symbol['offset']} has no function"
                                )
                            actual = function.getName()
                            range_spec = symbol.get("range") or ranges.get(
                                f"{segment_number}:{int(symbol['offset'], 16):04X}"
                            )
                            if range_spec is not None:
                                range_start, range_end = (
                                    int(value, 16) for value in range_spec
                                )
                                body = function.getBody()
                                body_min = body.getMinAddress().getOffset()
                                body_max = body.getMaxAddress().getOffset()
                                if (body_min != range_start or
                                    body_max != range_end - 1 or
                                    body.getNumAddresses() != range_end - range_start):
                                    raise AnalysisError(
                                        f"{symbol['name']} has body "
                                        f"{body_min:04X}..{body_max:04X}, expected "
                                        f"{range_start:04X}..{range_end - 1:04X}"
                                    )
                            if not function.getComment():
                                raise AnalysisError(
                                    f"{symbol['name']} has no semantic comment"
                                )
                            result = decompiler.decompileFunction(
                                function, 120, pyghidra.task_monitor(120)
                            )
                            if not result.decompileCompleted():
                                raise AnalysisError(
                                    f"cannot decompile {symbol['name']}: {result.getErrorMessage()}"
                                )
                        if actual != symbol["name"]:
                            raise AnalysisError(
                                f"{segment_number}:{symbol['offset']} expected "
                                f"{symbol['name']}, found {actual}"
                            )
                        print(f"ok {segment_number}:{symbol['offset']} {actual}")
                finally:
                    decompiler.dispose()
            finally:
                context.__exit__(None, None, None)


def main() -> int:
    repository = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path,
                        default=repository / "research/ghidra/quiky-analysis.json")
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("validate")
    audit = subparsers.add_parser("audit-project")
    audit.add_argument("--project-dir", type=Path, required=True)
    audit.add_argument("--project-name", required=True)
    audit.add_argument("--install-dir", type=Path)
    args = parser.parse_args()
    try:
        manifest = load_manifest(args.manifest, repository)
        if args.command == "audit-project":
            audit_project(manifest, args.project_dir, args.project_name,
                          args.install_dir)
        else:
            print(f"validated {len(manifest['symbols'])} symbols")
    except AnalysisError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
