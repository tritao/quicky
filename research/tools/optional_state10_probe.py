#!/usr/bin/env python3
"""Package the bounded state-10 live probe, including tooling failures."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--failure", type=Path, required=True)
    parser.add_argument("--static-notes", type=Path,
                        default=Path("research/notes/ghidra-analysis.md"))
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    failure = json.loads(args.failure.read_text(encoding="utf-8"))
    result = {
        "schema": "quiky-optional-state10-probe-v1",
        "status": "tooling_limited",
        "attempt": {
            "runtime_variant": "research/build/state-machine-target-variant/baseline/game",
            "level": "W1L1",
            "entity_type": "0x20",
            "entity_record_offset": "0x1930",
            "forced_state": 9,
            "state10_y_offset": 48,
            "position": {"x": 768, "y": 272},
            "camera": {"x": 768, "y": 272},
            "samples": 1,
            "warmup_frames": 30,
            "timeout_seconds": 75,
            "force_emission": True,
            "patch_map_run": True,
        },
        "failure_source": str(args.failure),
        "failure_sha256": sha256(args.failure),
        "failure": failure,
        "static_expectation": {
            "source": str(args.static_notes),
            "source_sha256": sha256(args.static_notes),
            "callback": "01F7:8E4B",
            "lookup": "01F7:3376",
            "writer": "01F7:16CE",
            "state10_effect_cells": 5,
            "state10_y_offset": "0x30",
            "state10_x_offsets": ["0x10", "0x20", "0x30", "0x40", "0x50"],
            "termination": "clear object+0x18",
            "published_coordinates": ["DS:8828", "DS:882A"],
        },
        "acceptance": {
            "required": "one live trace showing five state-10 writes followed by object termination",
            "current_result": "instruction-limit failure before a state-machine sample; state 10 remains static-only",
            "stop_rule": "do not reinterpret the instruction-limit failure as evidence against the static state-10 path",
        },
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(args.output), "sha256": sha256(args.output),
                      "status": result["status"]}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
