#!/usr/bin/env python3
"""Print NE relocation records with segment-relative call targets.

The executable stores unresolved relocation slots as zeros.  This tool reads
the NE segment relocation tables without modifying the executable and reports
the target segment/offset and the runtime selector mapping already confirmed
for QUIKY.EXE.
"""

from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path


SELECTORS = {
    1: 0x01D7,
    2: 0x01E7,
    3: 0x01F7,
    4: 0x0207,
    5: 0x0227,
    6: 0x0237,
}


def read_relocations(path: Path) -> list[dict[str, int | None]]:
    data = path.read_bytes()
    if data[:2] != b"MZ":
        raise ValueError("not an MZ executable")
    ne = struct.unpack_from("<I", data, 0x3C)[0]
    if data[ne : ne + 2] != b"NE":
        raise ValueError("MZ executable does not contain an NE header")
    shift = struct.unpack_from("<H", data, ne + 0x32)[0]
    count = struct.unpack_from("<H", data, ne + 0x1C)[0]
    table = ne + struct.unpack_from("<H", data, ne + 0x22)[0]
    records: list[dict[str, int | None]] = []

    for segment in range(1, count + 1):
        sector_offset, length, _flags, _min_alloc = struct.unpack_from(
            "<HHHH", data, table + (segment - 1) * 8
        )
        if length == 0:
            continue
        raw = sector_offset << shift
        relocation_table = raw + length
        relocation_count = struct.unpack_from("<H", data, relocation_table)[0]
        for index in range(relocation_count):
            offset = relocation_table + 2 + index * 8
            source_type, flags, source, target_segment, target_offset = struct.unpack_from(
                "<BBHHH", data, offset
            )
            instruction = source - 1 if source and data[raw + source - 1] == 0x9A else None
            records.append(
                {
                    "segment": segment,
                    "source": source,
                    "instruction": instruction,
                    "source_type": source_type,
                    "flags": flags,
                    "target_segment": target_segment,
                    "target_offset": target_offset,
                    "target_selector": SELECTORS.get(target_segment),
                }
            )
    return records


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--segment", type=int)
    parser.add_argument("--start", type=lambda value: int(value, 0))
    parser.add_argument("--end", type=lambda value: int(value, 0))
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    records = read_relocations(args.executable)
    if args.segment is not None:
        records = [item for item in records if item["segment"] == args.segment]
    if args.start is not None:
        records = [
            item for item in records
            if item["instruction"] is not None and item["instruction"] >= args.start
        ]
    if args.end is not None:
        records = [
            item for item in records
            if item["instruction"] is not None and item["instruction"] < args.end
        ]

    if args.json:
        print(json.dumps(records, indent=2))
    else:
        print("segment instruction source target selector flags type")
        for item in records:
            instruction = "-" if item["instruction"] is None else f"0x{item['instruction']:04x}"
            selector = "-" if item["target_selector"] is None else f"0x{item['target_selector']:04x}"
            print(
                f"{item['segment']:>7} {instruction:>11} 0x{item['source']:04x} "
                f"{item['target_segment']}:{item['target_offset']:04x} {selector:>8} "
                f"0x{item['flags']:02x} 0x{item['source_type']:02x}"
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
