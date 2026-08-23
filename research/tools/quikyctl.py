#!/usr/bin/env python3
"""Small, dependency-free inspection tools for Tricky Quiky Games II."""

from __future__ import annotations

import argparse
import json
import struct
import sys
from collections import Counter
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any, Optional


ARE_LAYOUT_OFFSET = 0x160
ARE_LAYOUT_SIZE = 0x1380
ARE_DECLARATION_OFFSET = 0x14E0
ARE_FIRST_RECORD_OFFSET = 0x14E8


class QuikyError(Exception):
    """Raised when a file is not a valid supported Quiky structure."""


@dataclass(frozen=True)
class ArchiveEntry:
    name: str
    offset: int
    size: int


@dataclass(frozen=True)
class ArchiveInfo:
    path: str
    file_size: int
    directory_offset: int
    stored_count_minus_one: int
    entries: tuple[ArchiveEntry, ...]


@dataclass(frozen=True)
class ArchiveTypeSummary:
    extension: str
    count: int
    bytes: int


@dataclass(frozen=True)
class ArchiveAsset:
    name: str
    extension: str
    offset: int
    size: int
    map_width: Optional[int] = None
    map_height: Optional[int] = None
    map_max_tile: Optional[int] = None
    are_unique_references: Optional[int] = None
    are_entity_count: Optional[int] = None
    are_type_count: Optional[int] = None


@dataclass(frozen=True)
class ArchiveIndex:
    path: str
    file_size: int
    directory_offset: int
    entry_count: int
    type_counts: tuple[ArchiveTypeSummary, ...]
    assets: tuple[ArchiveAsset, ...]


@dataclass(frozen=True)
class MapInfo:
    path: str
    magic: str
    width: int
    height: int
    unknown: int
    expected_size: int
    actual_size: int
    cell_count: int
    max_tile: int
    property_values: tuple[tuple[int, int], ...]


@dataclass(frozen=True)
class AREEntity:
    record_offset: int
    entity_type: int
    x: int
    y: int


@dataclass(frozen=True)
class AREReference:
    reference: int
    target_offset: int
    layout_occurrences: int
    entities: tuple[AREEntity, ...]


@dataclass(frozen=True)
class AREInfo:
    path: str
    file_size: int
    layout_offset: int
    layout_size: int
    layout_word_count: int
    zero_word_count: int
    blank_word_count: int
    reference_count: int
    unique_reference_count: int
    declaration_offset: int
    first_record_offset: int
    declaration_size: int
    entity_count: int
    entity_types: tuple[tuple[int, int], ...]
    references: tuple[AREReference, ...]


@dataclass(frozen=True)
class NESegment:
    number: int
    table_offset: int
    file_offset: int
    raw_length: int
    length: int
    flags: int
    minimum_allocation: int


@dataclass(frozen=True)
class NEInfo:
    path: str
    file_size: int
    mz_header_size: int
    ne_offset: int
    linker_version: str
    entry_table_offset: int
    entry_table_length: int
    flags: int
    automatic_data_segment: int
    initial_cs: int
    initial_ip: int
    initial_ss: int
    initial_sp: int
    segment_count: int
    module_reference_count: int
    sector_shift: int
    target_os: int
    segments: tuple[NESegment, ...]


def _u16le(data: bytes, offset: int) -> int:
    try:
        return struct.unpack_from("<H", data, offset)[0]
    except struct.error as exc:
        raise QuikyError(f"truncated file at offset 0x{offset:x}") from exc


def _u32le(data: bytes, offset: int) -> int:
    try:
        return struct.unpack_from("<I", data, offset)[0]
    except struct.error as exc:
        raise QuikyError(f"truncated file at offset 0x{offset:x}") from exc


def _u16be(data: bytes, offset: int) -> int:
    try:
        return struct.unpack_from(">H", data, offset)[0]
    except struct.error as exc:
        raise QuikyError(f"truncated file at offset 0x{offset:x}") from exc


def parse_archive(path: Path) -> ArchiveInfo:
    data = path.read_bytes()
    if len(data) < 8:
        raise QuikyError("NESTLE.DAT is shorter than its trailer")

    directory_offset = _u32le(data, len(data) - 8)
    stored_count_minus_one = _u32le(data, len(data) - 4)
    directory_end = len(data) - 8
    if directory_offset > directory_end:
        raise QuikyError("directory offset lies outside the archive")

    entries: list[ArchiveEntry] = []
    cursor = directory_offset
    previous_offset = -1
    while cursor < directory_end:
        if cursor + 2 > directory_end:
            raise QuikyError("truncated directory filename length")
        name_length = _u16le(data, cursor)
        cursor += 2
        if name_length == 0:
            raise QuikyError(f"empty filename at directory offset 0x{cursor - 2:x}")
        if cursor + name_length + 4 > directory_end:
            raise QuikyError("truncated directory entry")
        name_bytes = data[cursor : cursor + name_length]
        cursor += name_length
        try:
            name = name_bytes.decode("ascii")
        except UnicodeDecodeError as exc:
            raise QuikyError("directory filename is not ASCII") from exc
        payload_offset = _u32le(data, cursor)
        cursor += 4
        if payload_offset < previous_offset or payload_offset > directory_offset:
            raise QuikyError(f"invalid payload offset for {name}")
        entries.append(ArchiveEntry(name, payload_offset, 0))
        previous_offset = payload_offset

    if cursor != directory_end:
        raise QuikyError("directory does not end immediately before trailer")
    if not entries:
        raise QuikyError("archive has no directory entries")
    if stored_count_minus_one != len(entries) - 1:
        raise QuikyError(
            "trailer count mismatch: "
            f"stored {stored_count_minus_one}, parsed {len(entries) - 1}"
        )

    sized_entries = tuple(
        ArchiveEntry(
            entry.name,
            entry.offset,
            (entries[index + 1].offset if index + 1 < len(entries) else directory_offset)
            - entry.offset,
        )
        for index, entry in enumerate(entries)
    )
    return ArchiveInfo(
        str(path),
        len(data),
        directory_offset,
        stored_count_minus_one,
        sized_entries,
    )


def _archive_extension(name: str) -> str:
    suffix = Path(name).suffix
    return suffix[1:].upper() if suffix else "<none>"


def extract_archive(
    path: Path, output_dir: Path, overwrite: bool = False
) -> tuple[Path, ...]:
    data = path.read_bytes()
    info = parse_archive(path)
    output_dir.mkdir(parents=True, exist_ok=True)
    root = output_dir.resolve()

    planned: list[tuple[ArchiveEntry, Path]] = []
    names: set[str] = set()
    for entry in info.entries:
        name = entry.name
        if (
            not name
            or name in (".", "..")
            or "/" in name
            or "\\" in name
            or ":" in name
        ):
            raise QuikyError(f"unsafe archive filename: {name!r}")
        if name in names:
            raise QuikyError(f"duplicate archive filename: {name}")
        names.add(name)
        target = (root / name).resolve()
        if target.parent != root:
            raise QuikyError(f"archive filename escapes output directory: {name}")
        if target.exists() or target.is_symlink():
            if not overwrite:
                raise QuikyError(
                    f"refusing to overwrite {target}; use --overwrite to allow it"
                )
        planned.append((entry, target))

    for entry, target in planned:
        target.write_bytes(data[entry.offset : entry.offset + entry.size])
    return tuple(target for _, target in planned)


def index_archive(path: Path) -> ArchiveIndex:
    data = path.read_bytes()
    info = parse_archive(path)
    type_counts: Counter[str] = Counter()
    type_bytes: Counter[str] = Counter()
    assets: list[ArchiveAsset] = []

    for entry in info.entries:
        extension = _archive_extension(entry.name)
        type_counts[extension] += 1
        type_bytes[extension] += entry.size
        payload = data[entry.offset : entry.offset + entry.size]
        asset = ArchiveAsset(entry.name, extension, entry.offset, entry.size)
        if extension == "MAP":
            map_info = _parse_map_data(payload, entry.name)
            asset = ArchiveAsset(
                entry.name,
                extension,
                entry.offset,
                entry.size,
                map_info.width,
                map_info.height,
                map_info.max_tile,
            )
        elif extension == "ARE":
            are_info = _parse_are_data(payload, entry.name)
            asset = ArchiveAsset(
                entry.name,
                extension,
                entry.offset,
                entry.size,
                are_unique_references=are_info.unique_reference_count,
                are_entity_count=are_info.entity_count,
                are_type_count=len(are_info.entity_types),
            )
        assets.append(asset)

    summaries = tuple(
        ArchiveTypeSummary(extension, type_counts[extension], type_bytes[extension])
        for extension in sorted(type_counts)
    )
    return ArchiveIndex(
        str(path),
        info.file_size,
        info.directory_offset,
        len(info.entries),
        summaries,
        tuple(assets),
    )


def _parse_map_data(data: bytes, path: str) -> MapInfo:
    if len(data) < 10:
        raise QuikyError("MAP is shorter than its 10-byte header")
    magic_bytes = data[:4]
    if magic_bytes != b"TLE1":
        raise QuikyError(f"unsupported MAP signature {magic_bytes!r}")
    width = _u16be(data, 4)
    height = _u16be(data, 6)
    unknown = _u16be(data, 8)
    expected_size = 10 + (2 * width * height)
    if len(data) < expected_size:
        raise QuikyError(
            f"MAP is truncated: expected {expected_size} bytes, got {len(data)}"
        )
    cells = struct.unpack_from(f">{width * height}H", data, 10)
    properties = Counter(cell >> 9 for cell in cells)
    max_tile = max((cell & 0x1FF for cell in cells), default=0)
    return MapInfo(
        path,
        magic_bytes.decode("ascii"),
        width,
        height,
        unknown,
        expected_size,
        len(data),
        len(cells),
        max_tile,
        tuple(sorted(properties.items())),
    )


def parse_map(path: Path) -> MapInfo:
    return _parse_map_data(path.read_bytes(), str(path))


def _parse_are_data(data: bytes, path: str) -> AREInfo:
    if len(data) < ARE_DECLARATION_OFFSET:
        raise QuikyError(
            f"ARE is shorter than its fixed layout region: {len(data)} bytes"
        )

    layout_end = ARE_LAYOUT_OFFSET + ARE_LAYOUT_SIZE
    if layout_end != ARE_DECLARATION_OFFSET:
        raise QuikyError("internal ARE layout constants are inconsistent")
    layout_words = struct.unpack_from(
        f">{ARE_LAYOUT_SIZE // 2}H", data, ARE_LAYOUT_OFFSET
    )
    reference_counts = Counter(
        value for value in layout_words if value not in (0, 0xFFFF)
    )

    references: list[AREReference] = []
    entity_types: Counter[int] = Counter()
    entity_count = 0
    for reference in sorted(reference_counts):
        target_offset = ARE_LAYOUT_OFFSET + reference
        if target_offset < ARE_FIRST_RECORD_OFFSET or target_offset + 2 > len(data):
            raise QuikyError(
                f"ARE reference 0x{reference:04x} points outside declarations"
            )

        cursor = target_offset
        entities: list[AREEntity] = []
        while True:
            if cursor + 2 > len(data):
                raise QuikyError(
                    f"ARE declaration at 0x{target_offset:x} is unterminated"
                )
            entity_type = _u16be(data, cursor)
            cursor += 2
            if entity_type == 0xFFFF:
                break
            if cursor + 4 > len(data):
                raise QuikyError(
                    f"ARE entity at 0x{cursor - 2:x} is truncated"
                )
            x = _u16be(data, cursor)
            y = _u16be(data, cursor + 2)
            cursor += 4
            entities.append(AREEntity(target_offset, entity_type, x, y))
            entity_types[entity_type] += 1
            entity_count += 1

        references.append(
            AREReference(
                reference,
                target_offset,
                reference_counts[reference],
                tuple(entities),
            )
        )

    return AREInfo(
        path,
        len(data),
        ARE_LAYOUT_OFFSET,
        ARE_LAYOUT_SIZE,
        len(layout_words),
        layout_words.count(0),
        layout_words.count(0xFFFF),
        sum(reference_counts.values()),
        len(reference_counts),
        ARE_DECLARATION_OFFSET,
        ARE_FIRST_RECORD_OFFSET,
        len(data) - ARE_DECLARATION_OFFSET,
        entity_count,
        tuple(sorted(entity_types.items())),
        tuple(references),
    )


def parse_are(path: Path) -> AREInfo:
    return _parse_are_data(path.read_bytes(), str(path))


def parse_ne(path: Path) -> NEInfo:
    data = path.read_bytes()
    if len(data) < 0x40 or data[:2] != b"MZ":
        raise QuikyError("file is not an MZ executable")
    ne_offset = _u32le(data, 0x3C)
    if ne_offset + 0x40 > len(data) or data[ne_offset : ne_offset + 2] != b"NE":
        raise QuikyError("MZ executable does not contain an NE header")

    linker_version = f"{data[ne_offset + 2]}.{data[ne_offset + 3]}"
    segment_count = _u16le(data, ne_offset + 0x1C)
    sector_shift = _u16le(data, ne_offset + 0x32)
    segment_table_relative = _u16le(data, ne_offset + 0x22)
    segment_table = ne_offset + segment_table_relative
    segment_table_end = segment_table + (segment_count * 8)
    if segment_table_end > len(data):
        raise QuikyError("NE segment table lies outside the file")

    segments: list[NESegment] = []
    sector_size = 1 << sector_shift
    for index in range(segment_count):
        table_offset = segment_table + (index * 8)
        sector_offset = _u16le(data, table_offset)
        raw_length = _u16le(data, table_offset + 2)
        length = raw_length if raw_length else 0x10000
        segments.append(
            NESegment(
                index + 1,
                table_offset,
                sector_offset * sector_size,
                raw_length,
                length,
                _u16le(data, table_offset + 4),
                _u16le(data, table_offset + 6),
            )
        )

    return NEInfo(
        str(path),
        len(data),
        _u16le(data, 8) * 16,
        ne_offset,
        linker_version,
        _u16le(data, ne_offset + 4),
        _u16le(data, ne_offset + 6),
        _u16le(data, ne_offset + 0x0C),
        _u16le(data, ne_offset + 0x0E),
        _u16le(data, ne_offset + 0x16),
        _u16le(data, ne_offset + 0x14),
        _u16le(data, ne_offset + 0x1A),
        _u16le(data, ne_offset + 0x18),
        segment_count,
        _u16le(data, ne_offset + 0x1E),
        sector_shift,
        data[ne_offset + 0x36],
        tuple(segments),
    )


def _as_json(value: Any) -> Any:
    if hasattr(value, "__dataclass_fields__"):
        return {key: _as_json(item) for key, item in asdict(value).items()}
    if isinstance(value, tuple):
        return [_as_json(item) for item in value]
    if isinstance(value, list):
        return [_as_json(item) for item in value]
    if isinstance(value, dict):
        return {key: _as_json(item) for key, item in value.items()}
    return value


def _print_archive(info: ArchiveInfo, as_json: bool) -> None:
    if as_json:
        print(json.dumps(_as_json(info), indent=2))
        return
    print(f"archive: {info.path}")
    print(f"size: {info.file_size} bytes")
    print(f"directory: 0x{info.directory_offset:x}")
    print(f"entries: {len(info.entries)}")
    print("offset\tsize\tname")
    for entry in info.entries:
        print(f"0x{entry.offset:08x}\t{entry.size}\t{entry.name}")


def _print_archive_index(index: ArchiveIndex, as_json: bool) -> None:
    if as_json:
        print(json.dumps(_as_json(index), indent=2))
        return
    print(f"archive: {index.path}")
    print(f"size: {index.file_size} bytes")
    print(f"directory: 0x{index.directory_offset:x}")
    print(f"entries: {index.entry_count}")
    print("type\tcount\tbytes")
    for summary in index.type_counts:
        print(f"{summary.extension}\t{summary.count}\t{summary.bytes}")
    print("offset\tsize\ttype\tname\tdetails")
    for asset in index.assets:
        details = ""
        if asset.extension == "MAP":
            details = (
                f"{asset.map_width}x{asset.map_height}, "
                f"max tile {asset.map_max_tile}"
            )
        elif asset.extension == "ARE":
            details = (
                f"{asset.are_unique_references} refs, "
                f"{asset.are_entity_count} entities, "
                f"{asset.are_type_count} types"
            )
        print(
            f"0x{asset.offset:08x}\t{asset.size}\t{asset.extension}\t"
            f"{asset.name}\t{details}"
        )


def _print_are(info: AREInfo, as_json: bool, show_entities: bool) -> None:
    if as_json:
        print(json.dumps(_as_json(info), indent=2))
        return
    print(f"ARE: {info.path}")
    print(f"size: {info.file_size} bytes")
    print(
        f"layout: 0x{info.layout_offset:x}-0x{info.layout_offset + info.layout_size:x} "
        f"({info.layout_word_count} big-endian words)"
    )
    print(f"layout words equal to 0: {info.zero_word_count}")
    print(f"layout words equal to 0xffff: {info.blank_word_count}")
    print(f"reference occurrences: {info.reference_count}")
    print(f"unique references: {info.unique_reference_count}")
    print(f"declarations: 0x{info.declaration_offset:x}, {info.declaration_size} bytes")
    print(f"decoded entities: {info.entity_count}")
    print("entity types:")
    for entity_type, count in info.entity_types:
        print(f"  0x{entity_type:04x}: {count}")
    print("references:")
    for reference in info.references:
        print(
            f"  0x{reference.reference:04x} -> 0x{reference.target_offset:04x}; "
            f"occurrences {reference.layout_occurrences}; "
            f"entities {len(reference.entities)}"
        )
        if show_entities:
            for entity in reference.entities:
                print(
                    f"    type 0x{entity.entity_type:04x} "
                    f"at ({entity.x}, {entity.y})"
                )


def _print_map(info: MapInfo, as_json: bool) -> None:
    if as_json:
        print(json.dumps(_as_json(info), indent=2))
        return
    print(f"map: {info.path}")
    print(f"signature: {info.magic}")
    print(f"dimensions: {info.width}x{info.height}")
    print(f"unknown: 0x{info.unknown:04x}")
    print(f"size: {info.actual_size} bytes (expected {info.expected_size})")
    print(f"cells: {info.cell_count}")
    print(f"maximum tile: {info.max_tile}")
    print("property values:")
    for value, count in info.property_values:
        print(f"  0x{value:02x}: {count}")


def _print_ne(info: NEInfo, as_json: bool) -> None:
    if as_json:
        print(json.dumps(_as_json(info), indent=2))
        return
    print(f"executable: {info.path}")
    print(f"format: MZ/NE, {info.file_size} bytes")
    print(f"NE header: 0x{info.ne_offset:x}")
    print(f"linker: {info.linker_version}")
    print(f"entry point: segment {info.initial_cs}, offset 0x{info.initial_ip:x}")
    print(f"stack: segment {info.initial_ss}, offset 0x{info.initial_sp:x}")
    print(f"segments: {info.segment_count}")
    print(f"sector size: {1 << info.sector_shift} bytes")
    print(f"target OS: {info.target_os}")
    print("number\tfile offset\traw length\tlength\tflags\tmin alloc")
    for segment in info.segments:
        print(
            f"{segment.number}\t0x{segment.file_offset:08x}\t"
            f"0x{segment.raw_length:04x}\t{segment.length}\t"
            f"0x{segment.flags:04x}\t0x{segment.minimum_allocation:04x}"
        )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="quikyctl")
    subparsers = parser.add_subparsers(dest="command", required=True)

    archive = subparsers.add_parser("archive-list", help="inspect NESTLE.DAT")
    archive.add_argument("path", type=Path)
    archive.add_argument("--json", action="store_true")

    archive_index = subparsers.add_parser(
        "archive-index", help="validate and summarize archive assets"
    )
    archive_index.add_argument("path", type=Path)
    archive_index.add_argument("--json", action="store_true")

    archive_extract = subparsers.add_parser(
        "archive-extract", help="safely extract all archive assets"
    )
    archive_extract.add_argument("path", type=Path)
    archive_extract.add_argument("output_dir", type=Path)
    archive_extract.add_argument("--overwrite", action="store_true")

    map_info = subparsers.add_parser("map-info", help="inspect a TLE1 MAP")
    map_info.add_argument("path", type=Path)
    map_info.add_argument("--json", action="store_true")

    are_info = subparsers.add_parser("are-info", help="inspect an ARE object file")
    are_info.add_argument("path", type=Path)
    are_info.add_argument("--json", action="store_true")
    are_info.add_argument(
        "--entities", action="store_true", help="show decoded entity coordinates"
    )

    ne_info = subparsers.add_parser("ne-info", help="inspect an MZ/NE executable")
    ne_info.add_argument("path", type=Path)
    ne_info.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        if args.command == "archive-list":
            _print_archive(parse_archive(args.path), args.json)
        elif args.command == "archive-index":
            _print_archive_index(index_archive(args.path), args.json)
        elif args.command == "archive-extract":
            extracted = extract_archive(args.path, args.output_dir, args.overwrite)
            print(f"extracted {len(extracted)} files to {args.output_dir}")
        elif args.command == "map-info":
            _print_map(parse_map(args.path), args.json)
        elif args.command == "are-info":
            _print_are(parse_are(args.path), args.json, args.entities)
        elif args.command == "ne-info":
            _print_ne(parse_ne(args.path), args.json)
        else:
            raise QuikyError(f"unknown command: {args.command}")
    except (OSError, QuikyError) as exc:
        print(f"quikyctl: error: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
