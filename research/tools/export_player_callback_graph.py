#!/usr/bin/env python3
"""Export the audited player-closure call graph from the rename ledger.

The raw BinaryLoader project cannot resolve NE far-call relocations and is
intentionally imported with analysis disabled.  This export therefore uses
the ledger's address/call-site records as the authoritative graph, preserving
the exact source and target addresses that were recovered from raw bytes and
relocation tables.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from quiky_ne import parse_address as parse_quiky_address


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "research/ghidra/player-callback-closure.json"


def address(value: str) -> tuple[int, int]:
    parsed = parse_quiky_address(value)
    return parsed.segment, parsed.offset


def export(manifest: dict) -> dict:
    functions = {address(item["address"]): item for item in manifest["functions"]}
    edges: list[dict] = []
    for source in manifest["functions"]:
        source_segment, source_offset = address(source["address"])
        for callee in source["callees"]:
            # Opaque runtime-selected targets are retained in the closure
            # ledger as address-named metadata, but have no static call-site
            # edge to export.
            if isinstance(callee, str):
                continue
            target_segment, target_offset = address(callee["address"])
            for raw_site in callee["site"].split(","):
                call_site = int(raw_site, 16)
                target = functions.get((target_segment, target_offset))
                edges.append({
                    "source": f"{source_segment}:{source_offset:04X}",
                    "source_name": source["name"],
                    "call_site": f"{source_segment}:{call_site:04X}",
                    "target": f"{target_segment}:{target_offset:04X}",
                    "target_name": callee["name"],
                    "classification": callee["classification"],
                    "flags": callee["flags"],
                    "target_ledger_name": target["name"] if target else None,
                })
    edges.sort(key=lambda edge: (
        address(edge["source"]), address(edge["call_site"]), address(edge["target"])
    ))
    return {
        "schema": "quiky.player-callback-callgraph.v1",
        "source": manifest["source"],
        "root": manifest["root"],
        "graph_kind": "ledger-callsites",
        "edges": edges,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=MANIFEST)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    graph = export(manifest)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(graph, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {args.output} ({len(graph['edges'])} edges)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
