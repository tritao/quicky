#!/usr/bin/env python3
"""Generate the PyGhidra audit manifest from the player closure ledger."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
CLOSURE = ROOT / "research/ghidra/player-callback-closure.json"

SELECTORS = {
    "01D7": 1,
    "01E7": 2,
    "01F7": 3,
    "0207": 4,
    "0227": 5,
    "0237": 6,
}


def parse_address(value: str) -> tuple[int, int]:
    selector, offset = value.split(":", 1)
    return SELECTORS[selector.upper()], int(offset, 16)


def symbol(segment: int, offset: int, kind: str, name: str,
           confidence: str, evidence: list[str], **extra: Any) -> dict[str, Any]:
    result: dict[str, Any] = {
        "segment": segment,
        "offset": f"{offset:04X}",
        "kind": kind,
        "name": name,
        "confidence": confidence,
        "closures": ["player-callback"],
        "evidence": evidence,
    }
    result.update(extra)
    return result


def generate(closure: dict[str, Any]) -> dict[str, Any]:
    if closure.get("schema") != "quiky.player-callback-closure.v2":
        raise ValueError("unsupported player closure schema")

    symbols: list[dict[str, Any]] = []
    ranges: dict[str, list[str]] = {}
    for item in closure["functions"]:
        segment, offset = parse_address(item["address"])
        evidence = list(item.get("evidence", []))
        if item.get("range") is None:
            symbols.append(symbol(
                segment, offset, "label", item["name"],
                item["confidence"], evidence,
            ))
            continue
        start, end = (value.upper() for value in item["range"])
        ranges[f"{segment}:{offset:04X}"] = [start, end]
        symbols.append(symbol(
            segment, offset, "function", item["name"],
            item["confidence"], evidence, range=[start, end],
        ))

    for item in closure.get("address_labels", []):
        segment, offset = parse_address(item["address"])
        symbols.append(symbol(
            segment, offset, "label", item["name"],
            item["confidence"], list(item.get("evidence", [])),
        ))

    for item in closure["typed_model"]["globals"]:
        address = item["address"]
        if not address.upper().startswith("DS:"):
            continue
        offset = int(address[3:], 16)
        evidence = [f"typed_model:{address}"]
        if item.get("role"):
            evidence.append(item["role"])
        symbols.append(symbol(
            6, offset, "label", item["name"], "mechanical", evidence,
        ))

    symbols.sort(key=lambda item: (item["segment"], int(item["offset"], 16)))
    return {
        "schema": "quiky-ghidra-analysis-v1",
        "source_ledger": "research/ghidra/player-callback-closure.json",
        "executable": {
            "path": closure["source"]["executable"],
            "sha256": closure["source"]["sha256"],
        },
        "ghidra": {
            "tested_version": "12.1.3",
            "language": "x86:LE:16:Protected Mode:default",
            "project_layout": "separate-raw-ne-segments",
        },
        "segments": {
            "2": {"program": "/QUIKY_SEG02.bin", "runtime_selector": "01E7",
                  "required_in_raw_project": True},
            "3": {"program": "/QUIKY_SEG03.bin", "runtime_selector": "01F7",
                  "required_in_raw_project": True},
            "6": {"program": "/QUIKY_SEG06.bin", "runtime_selector": "0237",
                  "required_in_raw_project": True},
        },
        "function_ranges": ranges,
        "symbols": symbols,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=CLOSURE)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    closure = json.loads(args.manifest.read_text(encoding="utf-8"))
    rendered = json.dumps(generate(closure), indent=2) + "\n"
    if args.check:
        if args.output.read_text(encoding="utf-8") != rendered:
            raise SystemExit(f"FAIL: generated audit manifest drift at {args.output}")
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
