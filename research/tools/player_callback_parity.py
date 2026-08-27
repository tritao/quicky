#!/usr/bin/env python3
"""Run one captured-DOS to C++ player callback parity comparison."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path

from player_parity_compare import compare
from player_replay_manifest import ReplayManifestError, build_manifest, write_tsv


def classify_mismatch(item: dict[str, object]) -> str:
    field = item.get("field")
    if field == "pre_record":
        return "trace_normalization"
    if field == "post_record":
        return "incorrect_cpp_arithmetic_or_ordering_or_unclosed_contact_boundary"
    if field == "probes":
        return "missing_descriptor_data_or_trace_normalization"
    if field == "global_writes":
        return "missing_callback_global_input_or_opaque_helper_side_effect"
    if field in ("effects", "factory_objects"):
        return "opaque_helper_or_effect_side_effect"
    return "trace_normalization"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path, required=True)
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--map", required=True, dest="map_resource")
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--candidate", type=Path)
    parser.add_argument("--keep-manifest", type=Path)
    parser.add_argument("--max-report", type=int, default=8)
    parser.add_argument(
        "--require-complete", action="store_true",
        help="fail when either trace omits a comparable callback field",
    )
    args = parser.parse_args(argv)

    try:
        manifest = build_manifest(args.original)
    except ReplayManifestError as exc:
        print(f"player-parity: {exc}", file=sys.stderr)
        return 2

    if not args.binary.is_file():
        print(f"player-parity: missing tracer binary: {args.binary}", file=sys.stderr)
        return 2

    with tempfile.TemporaryDirectory(prefix="quiky-player-parity-") as directory:
        directory_path = Path(directory)
        manifest_path = directory_path / "replay.tsv"
        candidate = args.candidate or directory_path / "candidate.json"
        write_tsv(manifest, manifest_path)
        if args.keep_manifest is not None:
            args.keep_manifest.parent.mkdir(parents=True, exist_ok=True)
            write_tsv(manifest, args.keep_manifest)

        command = [
            str(args.binary),
            str(args.archive),
            args.map_resource,
            "0",
            "0",
            "0",
            "0",
            str(candidate),
            "--replay-tsv",
            str(manifest_path),
        ]
        completed = subprocess.run(command, text=True, capture_output=True)
        if completed.returncode != 0:
            if completed.stdout:
                print(completed.stdout, end="")
            if completed.stderr:
                print(completed.stderr, end="", file=sys.stderr)
            return completed.returncode

        mismatches = compare(args.original, candidate,
                             require_complete=args.require_complete)
        if mismatches:
            print(f"MISMATCH callbacks={len(mismatches)}")
            for item in mismatches[:args.max_report]:
                report = dict(item)
                report["classification"] = classify_mismatch(item)
                print(json.dumps(report, sort_keys=True))
            if manifest["unmapped_globals"]:
                print("UNMAPPED_REPLAY_GLOBALS")
                print(json.dumps(manifest["unmapped_globals"], sort_keys=True))
            if manifest["unresolved_fields"]:
                print("UNRESOLVED_REPLAY_FIELDS")
                print(json.dumps(manifest["unresolved_fields"], sort_keys=True))
            return 1

    print("OK: player callback parity")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
