#!/usr/bin/env python3
"""Build the optional archive/runtime matrix for descriptor flag 0x10.

This report deliberately separates archive placement from the existing
controlled collision traces.  Placement alone cannot justify a gameplay name;
the executable-level branch behavior remains the acceptance criterion.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from collections import Counter
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))

from descriptor_static_report import build_report  # noqa: E402
from quikyctl import _parse_map_data_with_cells, parse_archive  # noqa: E402


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def build_matrix(executable: Path, archive: Path,
                 collision_evidence: Path, sample_limit: int = 12) -> dict:
    static = build_report(executable)
    archive_bytes = archive.read_bytes()
    archive_info = parse_archive(archive)
    collision = json.loads(collision_evidence.read_text(encoding="utf-8"))
    runtime_cases = []
    wanted_cases = {
        "current_trace_flags_0x10",
        "current_trace_flags_0x50",
        "current_trace_flags_0x00",
        "direct_descriptor_flags_0x20",
        "direct_descriptor_flags_0x40",
        "direct_descriptor_flags_0x60",
        "vertical_motion_patch_flags_0x10",
        "natural_right_tile_0x28",
    }
    for case in collision.get("runtime_cases", []):
        if case.get("case") in wanted_cases:
            runtime_cases.append(case)

    maps_by_world: dict[str, list[dict]] = {f"W{i}": [] for i in range(1, 6)}
    for entry in archive_info.entries:
        name = entry.name.upper()
        if not name.endswith(".MAP") or len(name) < 2:
            continue
        world = name[:2]
        if world not in maps_by_world:
            continue
        payload = archive_bytes[entry.offset:entry.offset + entry.size]
        info, cells = _parse_map_data_with_cells(payload, entry.name)
        records = static["initializers"][world]["records"]
        flag_ids = {record["tile_id"] for record in records
                    if record["flags"] & 0x10}
        occurrences = []
        property_values: Counter[int] = Counter()
        id_counts: Counter[int] = Counter()
        for index, raw in enumerate(cells):
            tile_id = raw & 0x1FF
            if tile_id not in flag_ids:
                continue
            x, y = index % info.width, index // info.width
            upper = raw >> 9
            property_values[upper] += 1
            id_counts[tile_id] += 1
            if len(occurrences) < sample_limit:
                neighbors = {}
                for label, nx, ny in (
                    ("left", x - 1, y), ("right", x + 1, y),
                    ("up", x, y - 1), ("down", x, y + 1),
                ):
                    if 0 <= nx < info.width and 0 <= ny < info.height:
                        neighbor = cells[ny * info.width + nx]
                        neighbors[label] = {
                            "tile_id": neighbor & 0x1FF,
                            "raw": f"0x{neighbor:04x}",
                        }
                occurrences.append({
                    "x": x,
                    "y": y,
                    "tile_id": tile_id,
                    "descriptor": f"0x{records[tile_id]['flags']:04x}",
                    "raw": f"0x{raw:04x}",
                    "upper_field": f"0x{upper:02x}",
                    "neighbors": neighbors,
                })
        maps_by_world[world].append({
            "map": entry.name,
            "width": info.width,
            "height": info.height,
            "flag10_occurrences": sum(id_counts.values()),
            "tile_id_counts": {f"0x{k:03x}": v for k, v in sorted(id_counts.items())},
            "upper_field_counts": {f"0x{k:02x}": v
                                    for k, v in sorted(property_values.items())},
            "sample_limit": sample_limit,
            "samples": occurrences,
        })

    return {
        "schema": "quiky-optional-flag10-matrix-v1",
        "executable": str(executable),
        "executable_sha256": sha256(executable),
        "archive": str(archive),
        "archive_sha256": sha256(archive),
        "flag": "0x0010",
        "static_status": "mechanical_x_retry_suppression",
        "historical_name_status": "unresolved",
        "recommendation": "implement neutral suppress_x_retry_bit_10; do not infer floor/ceiling/one-way",
        "worlds": maps_by_world,
        "controlled_runtime_cases": runtime_cases,
        "acceptance": {
            "required_for_name": "same directional/vertical invariant in two worlds and two trajectories",
            "current_result": "branch behavior is confirmed; archive placement does not supply a unique historical name",
            "stop_rule": "retain the neutral name unless a new paired trajectory meets the requirement",
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--executable", type=Path, default=Path("game/QUIKY.EXE"))
    parser.add_argument("--archive", type=Path, default=Path("game/NESTLE.DAT"))
    parser.add_argument("--collision-evidence", type=Path,
                        default=Path("research/notes/descriptor-collision-evidence.json"))
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--sample-limit", type=int, default=12)
    args = parser.parse_args()
    if args.sample_limit < 1:
        parser.error("--sample-limit must be positive")
    result = build_matrix(args.executable, args.archive,
                          args.collision_evidence, args.sample_limit)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(args.output),
                      "sha256": sha256(args.output),
                      "worlds": {k: len(v) for k, v in result["worlds"].items()}},
                     indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
