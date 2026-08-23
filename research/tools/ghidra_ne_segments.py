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
import struct
from pathlib import Path


def read_segments(executable: Path) -> list[dict[str, int | str]]:
    data = executable.read_bytes()
    if data[:2] != b"MZ":
        raise ValueError(f"not an MZ executable: {executable}")
    ne_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if data[ne_offset : ne_offset + 2] != b"NE":
        raise ValueError(f"not an NE executable: {executable}")

    shift = struct.unpack_from("<H", data, ne_offset + 0x32)[0]
    count = struct.unpack_from("<H", data, ne_offset + 0x1C)[0]
    table = ne_offset + 0x40
    segments: list[dict[str, int | str]] = []

    for number in range(1, count + 1):
        offset, length, flags, min_alloc = struct.unpack_from(
            "<HHHH", data, table + (number - 1) * 8
        )
        memory_length = max(length, min_alloc) or 0x10000
        file_offset = offset << shift
        file_length = 0 if length == 0 else max(0, min(length, len(data) - file_offset))
        runtime_selectors = {
            1: 0x01D7,
            2: 0x01E7,
            3: 0x01F7,
            4: 0x0207,
            5: 0x0227,
            6: 0x0237,
        }
        segments.append(
            {
                "number": number,
                "file_offset": file_offset,
                "file_length": file_length,
                "memory_length": memory_length,
                "flags": flags,
                "min_alloc": min_alloc,
                "runtime_selector": (
                    f"{runtime_selectors[number]:04X}"
                    if number in runtime_selectors
                    else None
                ),
                "filename": f"QUIKY_SEG{number:02d}.bin",
            }
        )
    return segments


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    data = args.executable.read_bytes()
    segments = read_segments(args.executable)
    args.output.mkdir(parents=True, exist_ok=True)
    manifest = {
        "executable": str(args.executable.resolve()),
        "ne_segment_shift": struct.unpack_from(
            "<H", data, struct.unpack_from("<I", data, 0x3C)[0] + 0x32
        )[0],
        "language": "x86:LE:16:Protected Mode:default",
        "segments": [],
    }

    for segment in segments:
        start = int(segment["file_offset"])
        end = start + int(segment["file_length"])
        payload = data[start:end]
        path = args.output / str(segment["filename"])
        if payload:
            path.write_bytes(payload)
            path_value: str | None = str(path.resolve())
        else:
            path_value = None
        manifest["segments"].append({**segment, "path": path_value})

    (args.output / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    print(json.dumps(manifest, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
