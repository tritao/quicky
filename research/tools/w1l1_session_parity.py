#!/usr/bin/env python3
"""Run and compare the seeded native W1L1 session replay.

The archived DOS pool trace does not publish ``DS:646C``.  This runner derives
the observed leaf PRNG ring from that trace, injects it into the native
emitter, and then invokes the normalized session comparator.  Missing DOS
capture fields remain explicit coverage gaps; comparable mismatches always
fail the command.
"""

from __future__ import annotations

import argparse
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

from derive_w1l1_leaf_ring import LeafRingError, derive_ring
from w1l1_session_compare import (
    SessionTraceError,
    compare,
    load_payload,
)


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_BINARY = ROOT / "build/engine/quiky-w1l1-trace"
DEFAULT_ARCHIVE = ROOT / "game/NESTLE.DAT"
DEFAULT_MAP = "W1L1.MAP"
DEFAULT_LEADING_HEX = "000000000000"


def build_command(
    binary: Path,
    archive: Path,
    map_resource: str,
    output: Path,
    *,
    frames: int,
    action_flags: str,
    input_tsv: Path | None,
    leaf_prng_index: int,
    leaf_prng_ring_hex: str,
    input_jsonl: Path | None = None,
) -> list[str]:
    """Build the exact native emitter command used by the parity gate."""

    command = [
        str(binary),
        str(archive),
        map_resource,
        str(output),
    ]
    if input_tsv is not None and input_jsonl is not None:
        raise SessionTraceError("input TSV and input JSONL cannot be combined")
    if input_jsonl is not None:
        command.extend(("--input-jsonl", str(input_jsonl)))
    elif input_tsv is not None:
        command.extend(("--input-tsv", str(input_tsv)))
    else:
        command.extend(("--frames", str(frames), "--action-flags", action_flags))
    command.extend((
        "--leaf-prng-index", str(leaf_prng_index),
        "--leaf-prng-ring-hex", leaf_prng_ring_hex,
    ))
    return command


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--original", type=Path, required=True,
        help="captured DOS session trace used as the expected side",
    )
    parser.add_argument("--binary", type=Path, default=DEFAULT_BINARY)
    parser.add_argument("--archive", type=Path, default=DEFAULT_ARCHIVE)
    parser.add_argument("--map", dest="map_resource", default=DEFAULT_MAP)
    parser.add_argument("--frames", type=int, default=4)
    parser.add_argument("--action-flags", default="0")
    parser.add_argument("--input-tsv", type=Path)
    parser.add_argument("--input-jsonl", type=Path)
    parser.add_argument("--leaf-prng-index", type=int, default=0)
    parser.add_argument("--leading-hex", default=DEFAULT_LEADING_HEX)
    parser.add_argument("--max-report", type=int, default=8)
    parser.add_argument(
        "--require-complete", action="store_true",
        help="also fail when the DOS capture omits a comparable field",
    )
    args = parser.parse_args(argv)

    if args.frames < 0:
        parser.error("--frames must be non-negative")
    if args.leaf_prng_index < 0 or args.leaf_prng_index > 0xff:
        parser.error("--leaf-prng-index must be in the uint8 range")
    try:
        leading = bytes.fromhex(args.leading_hex)
    except ValueError as exc:
        parser.error(f"--leading-hex is not hexadecimal: {exc}")
    if len(leading) > 0x100:
        parser.error("--leading-hex is larger than the 256-byte ring")

    for path, label in ((args.original, "original trace"),
                        (args.binary, "native trace binary"),
                        (args.archive, "archive")):
        if not path.exists():
            print(f"w1l1-session: {label} does not exist: {path}", file=sys.stderr)
            return 2
    if args.input_tsv is not None and not args.input_tsv.exists():
        print(f"w1l1-session: input TSV does not exist: {args.input_tsv}",
              file=sys.stderr)
        return 2
    if args.input_jsonl is not None and not args.input_jsonl.exists():
        print(f"w1l1-session: input JSONL does not exist: {args.input_jsonl}",
              file=sys.stderr)
        return 2
    if args.input_tsv is not None and args.input_jsonl is not None:
        parser.error("--input-tsv and --input-jsonl cannot be combined")

    try:
        ring, rows = derive_ring(load_payload(args.original), leading)
    except (LeafRingError, SessionTraceError) as exc:
        print(f"w1l1-session: cannot derive leaf PRNG ring: {exc}", file=sys.stderr)
        return 2

    with tempfile.TemporaryDirectory(prefix="quiky-w1l1-parity-") as directory:
        candidate = Path(directory) / "candidate.json"
        command = build_command(
            args.binary,
            args.archive,
            args.map_resource,
            candidate,
            frames=args.frames,
            action_flags=args.action_flags,
            input_tsv=args.input_tsv,
            input_jsonl=args.input_jsonl,
            leaf_prng_index=args.leaf_prng_index,
            leaf_prng_ring_hex=ring.hex(),
        )
        completed = subprocess.run(command, capture_output=True, text=True)
        if completed.returncode != 0:
            print(
                f"w1l1-session: native emitter failed ({completed.returncode})",
                file=sys.stderr,
            )
            if completed.stdout:
                print(completed.stdout, file=sys.stderr, end="")
            if completed.stderr:
                print(completed.stderr, file=sys.stderr, end="")
            return 2

        try:
            mismatches, coverage = compare(args.original, candidate)
        except SessionTraceError as exc:
            print(f"w1l1-session: {exc}", file=sys.stderr)
            return 2

    print(
        f"replayed W1L1 frames={args.frames if args.input_tsv is None else 'input-tsv'} "
        f"leaf_objects={len(rows)} ring_index={args.leaf_prng_index} "
        f"command={shlex.join(command)}"
    )
    if mismatches:
        print(f"MISMATCH fields={len(mismatches)} coverage_gaps={len(coverage)}")
        for item in mismatches[:args.max_report]:
            print(item)
        if coverage:
            print("COVERAGE GAPS")
            for item in coverage[:args.max_report]:
                print(item)
        return 1
    if coverage and args.require_complete:
        print(f"INCOMPLETE fields=0 coverage_gaps={len(coverage)}")
        for item in coverage[:args.max_report]:
            print(item)
        return 2
    print(f"OK: W1L1 session parity fields; coverage_gaps={len(coverage)}")
    for item in coverage[:args.max_report]:
        print(item)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
