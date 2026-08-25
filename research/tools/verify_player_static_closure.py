#!/usr/bin/env python3
"""Verify the pinned inputs and address inventory for the player closure."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "research/ghidra/player-static-closure.json"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_offset(value: str) -> int:
    return int(value, 0)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=MANIFEST)
    parser.add_argument(
        "--exe", type=Path, default=ROOT / "game/QUIKY.EXE",
    )
    parser.add_argument(
        "--segment", type=Path,
        default=ROOT / "research/build/ghidra-segments/QUIKY_SEG03.bin",
    )
    args = parser.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    expected_exe = manifest["executable"]
    actual_exe_hash = sha256(args.exe)
    actual_segment_hash = sha256(args.segment)

    failures: list[str] = []
    if actual_exe_hash != expected_exe["sha256"]:
        failures.append(
            f"executable SHA-256 {actual_exe_hash} != {expected_exe['sha256']}"
        )
    if args.segment.stat().st_size != expected_exe["image_size"]:
        failures.append(
            f"segment size {args.segment.stat().st_size} != {expected_exe['image_size']}"
        )
    if actual_segment_hash != expected_exe["image_sha256"]:
        failures.append(
            f"segment SHA-256 {actual_segment_hash} != {expected_exe['image_sha256']}"
        )

    image_size = args.segment.stat().st_size
    for function in manifest["functions"]:
        entry = parse_offset(function["entry"])
        end = parse_offset(function["end"])
        if not (0 <= entry < end <= image_size):
            failures.append(
                f"invalid range {function['name']}: {function['entry']}..{function['end']}"
            )

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}")
        return 1

    print(f"OK: {len(manifest['functions'])} functions")
    print(f"OK: QUIKY.EXE {actual_exe_hash}")
    print(f"OK: segment-3 {image_size} bytes, {actual_segment_hash}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

