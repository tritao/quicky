#!/usr/bin/env python3
"""Build paired same-region ARE variants for a renderer overlap experiment."""

from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path

from quikyctl import (
    QuikyError,
    _parse_are_data,
    _u16be,
    iter_are_entity_placements,
    parse_archive,
    patch_are_entity_data,
    replace_archive_entry,
)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--level", default="W1L1.ARE")
    parser.add_argument("--first-offset", type=lambda value: int(value, 0),
                        default=0x1624)
    parser.add_argument("--second-offset", type=lambda value: int(value, 0),
                        default=0x162A)
    parser.add_argument("--target-x", type=int)
    parser.add_argument("--target-y", type=int)
    parser.add_argument("--first-target-x", type=int,
                        help="independent world X for the first record")
    parser.add_argument("--first-target-y", type=int,
                        help="independent world Y for the first record")
    parser.add_argument("--second-target-x", type=int,
                        help="independent world X for the second record")
    parser.add_argument("--second-target-y", type=int,
                        help="independent world Y for the second record")
    parser.add_argument("--stream-cell", metavar="X,Y",
                        help="relocate the shared reference into this streamed cell")
    parser.add_argument("--second-type", type=lambda value: int(value, 0),
                        help="override the second record type for a normal-BOB overlap fixture")
    parser.add_argument("--overwrite", action="store_true")
    return parser


def run(args: argparse.Namespace) -> int:
    archive_info = parse_archive(args.archive)
    entry = next((item for item in archive_info.entries
                  if item.name == args.level), None)
    if entry is None:
        raise QuikyError(f"archive entry not found: {args.level}")
    source = args.archive.read_bytes()
    are_data = source[entry.offset:entry.offset + entry.size]
    are_info = _parse_are_data(are_data, args.level)
    by_offset = {
        entity.record_offset: (reference, index, entity)
        for reference in are_info.references
        for index, entity in enumerate(reference.entities)
    }
    try:
        first_ref, first_index, first = by_offset[args.first_offset]
        second_ref, second_index, second = by_offset[args.second_offset]
    except KeyError as exc:
        raise QuikyError(f"ARE record not found: {exc.args[0]:#x}") from exc
    if first_ref.reference != second_ref.reference:
        raise QuikyError("overlap records must share one streamed ARE reference")

    reference_index = next(
        index for index, value in enumerate(
            range(are_info.layout_word_count)
        ) if _u16be(are_data, 0x160 + index * 2) == first_ref.reference
    )
    original_region_x = (reference_index % are_info.layout_width) * 64
    original_region_y = (reference_index // are_info.layout_width) * 64
    region_x, region_y = original_region_x, original_region_y
    stream_cell = None
    if args.stream_cell:
        try:
            stream_cell = tuple(int(part, 0) for part in args.stream_cell.split(","))
        except ValueError as exc:
            raise QuikyError("--stream-cell must look like X,Y") from exc
        if len(stream_cell) != 2:
            raise QuikyError("--stream-cell must look like X,Y")
        cell_x, cell_y = stream_cell
        if not (0 <= cell_x < are_info.layout_width and
                0 <= cell_y < are_info.layout_height):
            raise QuikyError("--stream-cell is outside the ARE layout")
        target_index = cell_y * are_info.layout_width + cell_x
        previous = _u16be(are_data, 0x160 + target_index * 2)
        moved = bytearray(are_data)
        moved[0x160 + target_index * 2:0x162 + target_index * 2] = bytes(
            (first_ref.reference >> 8, first_ref.reference & 0xff)
        )
        are_data = bytes(moved)
        region_x, region_y = cell_x * 64, cell_y * 64
    target_x = region_x + first.x if args.target_x is None else args.target_x
    target_y = region_y + first.y if args.target_y is None else args.target_y
    first_world = (
        target_x if args.first_target_x is None else args.first_target_x,
        target_y if args.first_target_y is None else args.first_target_y,
    )
    second_world = (
        target_x if args.second_target_x is None else args.second_target_x,
        target_y if args.second_target_y is None else args.second_target_y,
    )
    first_local = (first_world[0] - region_x, first_world[1] - region_y)
    second_local = (second_world[0] - region_x, second_world[1] - region_y)
    for label, local in (("first", first_local), ("second", second_local)):
        if not all(0 <= value <= 0xffff for value in local):
            raise QuikyError(f"{label} target position cannot be represented in the shared region")

    overlapped = patch_are_entity_data(
        are_data, first_ref.reference, first_index,
        delta_x=first_local[0] - first.x, delta_y=first_local[1] - first.y,
    )
    # The second patch uses the original entity index; both edits are in the
    # same declaration, so applying it to the already-patched buffer is safe.
    overlapped = patch_are_entity_data(
        overlapped, second_ref.reference, second_index,
        delta_x=second_local[0] - second.x, delta_y=second_local[1] - second.y,
        entity_type=args.second_type,
    )

    variants = {
        "overlap": overlapped,
        "remove-first": patch_are_entity_data(
            overlapped, first_ref.reference, first_index, entity_type=0,
        ),
        "remove-second": patch_are_entity_data(
            overlapped, second_ref.reference, second_index, entity_type=0,
        ),
    }
    args.output_dir.mkdir(parents=True, exist_ok=True)
    manifest_variants = []
    runtime_root = args.archive.parent
    for name, variant_are in variants.items():
        variant_root = args.output_dir / name
        game_dir = variant_root / "game"
        if variant_root.exists() and any(variant_root.iterdir()) and not args.overwrite:
            raise QuikyError(f"refusing to overwrite {variant_root}")
        game_dir.mkdir(parents=True, exist_ok=True)
        for runtime_file in runtime_root.iterdir():
            if runtime_file.is_file() and runtime_file.name != args.archive.name:
                shutil.copy2(runtime_file, game_dir / runtime_file.name)
        variant_archive = replace_archive_entry(
            args.archive, game_dir / args.archive.name, args.level, variant_are,
            overwrite=args.overwrite,
        )
        manifest_variants.append({
            "name": name,
            "directory": str(game_dir),
            "archive": str(variant_archive),
            "removed": ("first" if name == "remove-first" else
                        "second" if name == "remove-second" else None),
        })
    manifest = {
        "schema": "quiky-overlap-v1",
        "source_archive": str(args.archive),
        "level": args.level,
        "reference": first_ref.reference,
        "region_origin": [region_x, region_y],
        "original_region_origin": [original_region_x, original_region_y],
        "stream_cell": list(stream_cell) if stream_cell else None,
        "stream_cell_previous": previous if stream_cell else None,
        "records": {
            "first": {"offset": args.first_offset, "type": first.entity_type,
                       "original": [region_x + first.x, region_y + first.y]},
        "second": {"offset": args.second_offset,
                    "type": (second.entity_type if args.second_type is None
                              else args.second_type),
                        "original": [region_x + second.x, region_y + second.y]},
        },
        "target_world": [target_x, target_y],
        "target_local": [target_x - region_x, target_y - region_y],
        "record_targets": {"first": list(first_world), "second": list(second_world)},
        "variants": manifest_variants,
    }
    (args.output_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    print(json.dumps(manifest, indent=2))
    return 0


def main() -> int:
    args = build_parser().parse_args()
    try:
        return run(args)
    except (OSError, QuikyError) as exc:
        print(f"quiky_overlap: {exc}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
