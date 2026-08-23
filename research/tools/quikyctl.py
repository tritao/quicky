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
from typing import Any


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


def parse_map(path: Path) -> MapInfo:
    data = path.read_bytes()
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
        str(path),
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

    map_info = subparsers.add_parser("map-info", help="inspect a TLE1 MAP")
    map_info.add_argument("path", type=Path)
    map_info.add_argument("--json", action="store_true")

    ne_info = subparsers.add_parser("ne-info", help="inspect an MZ/NE executable")
    ne_info.add_argument("path", type=Path)
    ne_info.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        if args.command == "archive-list":
            _print_archive(parse_archive(args.path), args.json)
        elif args.command == "map-info":
            _print_map(parse_map(args.path), args.json)
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
