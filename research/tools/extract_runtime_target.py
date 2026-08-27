#!/usr/bin/env python3
"""Extract a captured loaded callback byte window from a player trace.

The resulting raw image is intentionally imported into Ghidra as a separate
protected-mode program.  It is not part of QUIKY.EXE and must retain the
runtime selector/offset recorded by the trace.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--target-segment", type=lambda value: int(value, 0))
    parser.add_argument("--target-offset", type=lambda value: int(value, 0))
    args = parser.parse_args()

    payload = json.loads(args.trace.read_text(encoding="utf-8"))
    events = payload.get("events", [])
    if not isinstance(events, list) or not events:
        raise SystemExit("trace has no events")

    matches = []
    for event in events:
        for sample in event.get("samples", []):
            for watch in sample.get("execute_watches", []):
                if not watch.get("indirect_target"):
                    continue
                segment = watch.get("segment")
                offset = watch.get("offset")
                if args.target_segment is not None and segment != args.target_segment:
                    continue
                if args.target_offset is not None and offset != args.target_offset:
                    continue
                code_hex = watch.get("target_code_hex")
                if not isinstance(code_hex, str):
                    raise SystemExit("indirect target has no captured code bytes")
                try:
                    code = bytes.fromhex(code_hex)
                except ValueError as exc:
                    raise SystemExit(f"invalid target code hex: {exc}") from exc
                matches.append((segment, offset, code, watch))

    if not matches:
        raise SystemExit("no matching indirect target capture")
    segment, offset, code, watch = matches[0]
    if any((other[0], other[1], other[2]) != (segment, offset, code)
           for other in matches[1:]):
        raise SystemExit("matching target captures disagree")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(code)
    metadata = args.output.with_suffix(args.output.suffix + ".json")
    metadata.write_text(json.dumps({
        "schema": "quiky.runtime-loaded-target-image.v1",
        "trace": str(args.trace),
        "runtime_selector": segment,
        "entry_offset": offset,
        "byte_count": len(code),
        "return_expected": watch.get("return_expected"),
        "entry_registers": watch.get("registers"),
        "simulation_differences": watch.get("differences"),
        "target_object_differences": watch.get("target_object_differences"),
    }, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {len(code)} bytes for {segment:04X}:{offset:04X} to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
