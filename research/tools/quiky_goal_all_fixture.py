#!/usr/bin/env python3
"""Build a compact W1L1 fixture with all seven puzzle letters in one cell."""

from __future__ import annotations

import argparse
import json
import shutil
import struct
from pathlib import Path

from quikyctl import QuikyError, _parse_are_data, parse_archive, replace_archive_entry


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--level", default="W1L1.ARE")
    parser.add_argument("--target-x", type=int, default=768)
    parser.add_argument("--target-y", type=int, default=400)
    parser.add_argument("--stream-cell", metavar="X,Y", default="12,3")
    parser.add_argument(
        "--include-cloud", action="store_true",
        help="move the authored W1L1 cloud into an adjacent streamed cell",
    )
    parser.add_argument(
        "--cloud-stream-cell", metavar="X,Y",
        help="ARE cell for --include-cloud (defaults to one cell left)",
    )
    parser.add_argument("--overwrite", action="store_true")
    return parser


def _u16be(data: bytes, offset: int) -> int:
    return struct.unpack_from(">H", data, offset)[0]


def _put_u16be(data: bytearray, offset: int, value: int) -> None:
    struct.pack_into(">H", data, offset, value)


def run(args: argparse.Namespace) -> int:
    archive = parse_archive(args.archive)
    entry = next((item for item in archive.entries if item.name == args.level), None)
    if entry is None:
        raise QuikyError(f"ARE entry not found: {args.level}")
    source = args.archive.read_bytes()
    are_data = source[entry.offset:entry.offset + entry.size]
    info = _parse_are_data(are_data, args.level)
    letters = []
    clouds = []
    for reference in info.references:
        for index, entity in enumerate(reference.entities):
            if 0x79 <= entity.entity_type <= 0x7f:
                letters.append((reference.reference, index, entity))
            elif entity.entity_type == 0x28:
                clouds.append((reference.reference, index, entity))
    if len(letters) != 7:
        raise QuikyError(f"expected seven puzzle letters, found {len(letters)}")
    if args.include_cloud and len(clouds) != 1:
        raise QuikyError(f"expected one W1L1 cloud, found {len(clouds)}")
    try:
        cell_x, cell_y = (int(part, 0) for part in args.stream_cell.split(","))
    except ValueError as exc:
        raise QuikyError("--stream-cell must look like X,Y") from exc
    if not (0 <= cell_x < info.layout_width and 0 <= cell_y < info.layout_height):
        raise QuikyError("--stream-cell lies outside the ARE layout")
    region_x, region_y = cell_x * 64, cell_y * 64
    local_x, local_y = args.target_x - region_x, args.target_y - region_y
    if not (0 <= local_x <= 0xffff and 0 <= local_y <= 0xffff):
        raise QuikyError("target position cannot be represented in the streamed region")
    cloud_cell_x, cloud_cell_y = cell_x - 1, cell_y
    if args.cloud_stream_cell:
        try:
            cloud_cell_x, cloud_cell_y = (
                int(part, 0) for part in args.cloud_stream_cell.split(",")
            )
        except ValueError as exc:
            raise QuikyError("--cloud-stream-cell must look like X,Y") from exc
    if args.include_cloud and not (
        0 <= cloud_cell_x < info.layout_width and
        0 <= cloud_cell_y < info.layout_height
    ):
        raise QuikyError("cloud stream cell lies outside the ARE layout")
    cloud_local_x = args.target_x - cloud_cell_x * 64
    cloud_local_y = args.target_y - cloud_cell_y * 64
    if args.include_cloud and not (0 <= cloud_local_x <= 0xffff and
                                    0 <= cloud_local_y <= 0xffff):
        raise QuikyError("cloud position cannot be represented in its streamed region")

    references = sorted(info.references, key=lambda item: item.target_offset)
    # Declaration records are packed back-to-back.  Select the first start
    # whose *later preserved* declaration leaves enough bytes, allowing the
    # intervening references to be intentionally blanked in this fixture.
    compact_entities = letters
    declaration_bytes = len(compact_entities) * 6 + 2
    target_ref = next((item for item in references
                       if any(r.target_offset - item.target_offset >= declaration_bytes
                              for r in references
                              if r.target_offset > item.target_offset)), None)
    if target_ref is None:
        raise QuikyError("no declaration gap can hold the seven-letter fixture")
    target_offset = target_ref.target_offset
    next_offset = next((r.target_offset for r in references
                        if r.target_offset - target_offset >= declaration_bytes),
                       len(are_data))
    if next_offset - target_offset < declaration_bytes:
        raise QuikyError("selected declaration gap is too small")

    # The compact declaration replaces the old records in this gap.  Remove
    # layout pointers into every overwritten declaration except the new target.
    overwritten_refs = [r.reference for r in references
                        if target_offset <= r.target_offset <
                        target_offset + declaration_bytes]
    moved = bytearray(are_data)
    for index in range(info.layout_word_count):
        value = _u16be(are_data, info.layout_offset + index * 2)
        if value in overwritten_refs and value != target_ref.reference:
            _put_u16be(moved, info.layout_offset + index * 2, 0xffff)
        if args.include_cloud and value == clouds[0][0]:
            _put_u16be(moved, info.layout_offset + index * 2, 0xffff)
    cell_offset = info.layout_offset + (cell_y * info.layout_width + cell_x) * 2
    _put_u16be(moved, cell_offset, target_ref.reference)
    if args.include_cloud:
        cloud_cell_offset = info.layout_offset + (
            cloud_cell_y * info.layout_width + cloud_cell_x
        ) * 2
        _put_u16be(moved, cloud_cell_offset, clouds[0][0])
        _put_u16be(moved, clouds[0][2].record_offset + 2, cloud_local_x)
        _put_u16be(moved, clouds[0][2].record_offset + 4, cloud_local_y)
    cursor = target_offset
    for _reference, _index, entity in compact_entities:
        _put_u16be(moved, cursor, entity.entity_type)
        _put_u16be(moved, cursor + 2, local_x)
        _put_u16be(moved, cursor + 4, local_y)
        cursor += 6
    _put_u16be(moved, cursor, 0xffff)
    # Inert the seven original declarations so the fixture has exactly one
    # copy of each letter and collection order is deterministic.
    for reference, index, _entity in compact_entities:
        source_entity = next(r.entities[index] for r in info.references
                             if r.reference == reference)
        if source_entity.record_offset < target_offset or \
           source_entity.record_offset >= target_offset + declaration_bytes:
            _put_u16be(moved, source_entity.record_offset, 0)
    removed = bytearray(moved)
    for letter_index in range(7):
        _put_u16be(removed, target_offset + letter_index * 6, 0)

    args.output_dir.mkdir(parents=True, exist_ok=True)
    variants = []
    for name, variant in (("goal", bytes(moved)), ("remove-goal", bytes(removed))):
        root = args.output_dir / name
        game_dir = root / "game"
        if root.exists() and any(root.iterdir()) and not args.overwrite:
            raise QuikyError(f"refusing to overwrite {root}")
        game_dir.mkdir(parents=True, exist_ok=True)
        for runtime_file in args.archive.parent.iterdir():
            if runtime_file.is_file() and runtime_file.name != args.archive.name:
                shutil.copy2(runtime_file, game_dir / runtime_file.name)
        archive_path = replace_archive_entry(
            args.archive, game_dir / args.archive.name, args.level, variant,
            overwrite=args.overwrite,
        )
        variants.append({"name": name, "directory": str(game_dir),
                         "archive": str(archive_path)})
    manifest = {
        "schema": "quiky-goal-all-fixture-v1", "source_archive": str(args.archive),
        "level": args.level, "target_world": [args.target_x, args.target_y],
        "target_local": [local_x, local_y], "stream_cell": [cell_x, cell_y],
        "include_cloud": args.include_cloud,
        "cloud_stream_cell": ([cloud_cell_x, cloud_cell_y]
                               if args.include_cloud else None),
        "cloud_local": ([cloud_local_x, cloud_local_y]
                         if args.include_cloud else None),
        "target_reference": target_ref.reference,
        "target_declaration_offset": target_offset,
        "target_declaration_size": declaration_bytes,
        "overwritten_references": overwritten_refs,
        "letters": [{"type": entity.entity_type, "source_record_offset": entity.record_offset}
                    for _, _, entity in letters],
        "cloud": ([{"type": entity.entity_type,
                    "source_record_offset": entity.record_offset}
                   for _, _, entity in clouds]
                  if args.include_cloud else []),
        "variants": variants,
    }
    (args.output_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(manifest, indent=2))
    return 0


def main() -> int:
    try:
        return run(build_parser().parse_args())
    except (OSError, QuikyError) as exc:
        print(f"quiky_goal_all_fixture: {exc}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
