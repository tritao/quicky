#!/usr/bin/env python3
"""Extract the file-backed segments from a DOS NE executable for Ghidra.

Ghidra's NE loader currently rejects QUIKY.EXE, so this produces independent
raw 16-bit segment images.  Each image starts at offset zero, preserving the
original segment-relative addresses used by the debugger and the NE relocation
table.  A JSON manifest records the original file offsets, lengths, flags, and
the runtime selectors observed for this executable.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from quiky_ne import read_ne


def read_segments(executable: Path) -> list[dict[str, int | str | None]]:
    return read_ne(executable).segment_dicts()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument(
        "--pad-to-memory",
        action="store_true",
        help="append zero-filled NE allocation space for raw Ghidra images",
    )
    args = parser.parse_args()

    image = read_ne(args.executable)
    segments = image.segment_dicts()
    args.output.mkdir(parents=True, exist_ok=True)
    manifest = {
        "executable": str(args.executable.resolve()),
        "ne_segment_shift": image.segment_shift,
        "language": "x86:LE:16:Protected Mode:default",
        "segments": [],
    }

    for segment in segments:
        number = int(segment["number"])
        payload = image.memory_bytes(number) if args.pad_to_memory else image.raw_bytes(number)
        # Keep zero-length NE segments as metadata only.  Padding is useful for
        # file-backed BSS tails (notably SEG06), but a pure allocation segment
        # has no raw image to import.
        if int(segment["file_length"]) == 0:
            payload = b""
        path = args.output / str(segment["filename"])
        if payload:
            path.write_bytes(payload)
            path_value: str | None = str(path.resolve())
        else:
            path_value = None
        manifest["segments"].append({
            **segment,
            "image_length": len(payload),
            "zero_filled": len(payload) > int(segment["file_length"]),
            "path": path_value,
        })

    (args.output / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    print(json.dumps(manifest, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
