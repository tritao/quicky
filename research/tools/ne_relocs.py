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
from pathlib import Path

from quiky_ne import read_ne


def read_relocations(path: Path) -> list[dict[str, int | None]]:
    return read_ne(path).relocation_dicts()


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
