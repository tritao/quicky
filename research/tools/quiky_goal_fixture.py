#!/usr/bin/env python3
"""Relocate one collectible ARE record into the deterministic player anchor."""

from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path

from quikyctl import (
    QuikyError,
    _parse_are_data,
    _u16be,
    parse_archive,
    patch_are_entity_data,
    replace_archive_entry,
)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--level", default="W1L1.ARE")
    parser.add_argument("--record-offset", type=lambda value: int(value, 0),
                        default=0x1782)
    parser.add_argument("--target-x", type=int, default=128)
    parser.add_argument("--target-y", type=int, default=400)
    parser.add_argument("--stream-cell", metavar="X,Y", default="2,6")
    parser.add_argument("--overwrite", action="store_true")
    return parser


def run(args: argparse.Namespace) -> int:
    archive_info = parse_archive(args.archive)
    entry = next((item for item in archive_info.entries if item.name == args.level), None)
    if entry is None:
        raise QuikyError(f"ARE entry not found: {args.level}")
    source = args.archive.read_bytes()
    are_data = source[entry.offset:entry.offset + entry.size]
    are_info = _parse_are_data(are_data, args.level)
    match = None
    for ref_index, reference in enumerate(are_info.references):
        for entity_index, entity in enumerate(reference.entities):
            if entity.record_offset == args.record_offset:
                match = ref_index, reference, entity_index, entity
                break
        if match:
            break
    if match is None:
        raise QuikyError(f"ARE record not found: {args.record_offset:#x}")
    ref_index, reference, entity_index, entity = match
    try:
        cell_x, cell_y = (int(part, 0) for part in args.stream_cell.split(","))
    except ValueError as exc:
        raise QuikyError("--stream-cell must look like X,Y") from exc
    if not (0 <= cell_x < are_info.layout_width and 0 <= cell_y < are_info.layout_height):
        raise QuikyError("--stream-cell is outside the ARE layout")
    region_x, region_y = cell_x * 64, cell_y * 64
    local_x, local_y = args.target_x - region_x, args.target_y - region_y
    if not all(0 <= value <= 0xffff for value in (local_x, local_y)):
        raise QuikyError("target position cannot be represented in the streamed region")
    moved = bytearray(are_data)
    table_offset = 0x160 + (cell_y * are_info.layout_width + cell_x) * 2
    previous_reference = _u16be(are_data, table_offset)
    moved[table_offset:table_offset + 2] = bytes((reference.reference >> 8,
                                                   reference.reference & 0xff))
    relocated = patch_are_entity_data(
        bytes(moved), reference.reference, entity_index,
        delta_x=local_x - entity.x, delta_y=local_y - entity.y,
    )
    removed = patch_are_entity_data(
        relocated, reference.reference, entity_index, entity_type=0,
    )
    args.output_dir.mkdir(parents=True, exist_ok=True)
    variants = {"goal": relocated, "remove-goal": removed}
    manifest_variants = []
    for name, variant in variants.items():
        variant_root = args.output_dir / name
        game_dir = variant_root / "game"
        if variant_root.exists() and any(variant_root.iterdir()) and not args.overwrite:
            raise QuikyError(f"refusing to overwrite {variant_root}")
        game_dir.mkdir(parents=True, exist_ok=True)
        for runtime_file in args.archive.parent.iterdir():
            if runtime_file.is_file() and runtime_file.name != args.archive.name:
                shutil.copy2(runtime_file, game_dir / runtime_file.name)
        archive_path = replace_archive_entry(
            args.archive, game_dir / args.archive.name, args.level, variant,
            overwrite=args.overwrite,
        )
        manifest_variants.append({"name": name, "directory": str(game_dir),
                                 "archive": str(archive_path)})
    manifest = {
        "schema": "quiky-goal-fixture-v1",
        "source_archive": str(args.archive), "level": args.level,
        "record_offset": args.record_offset, "entity_type": entity.entity_type,
        "reference": reference.reference, "reference_index": ref_index,
        "original_local": [entity.x, entity.y],
        "stream_cell": [cell_x, cell_y],
        "stream_cell_previous": previous_reference,
        "target_world": [args.target_x, args.target_y],
        "target_local": [local_x, local_y],
        "variants": manifest_variants,
    }
    (args.output_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(manifest, indent=2))
    return 0


def main() -> int:
    try:
        return run(build_parser().parse_args())
    except (OSError, QuikyError) as exc:
        print(f"quiky_goal_fixture: {exc}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
