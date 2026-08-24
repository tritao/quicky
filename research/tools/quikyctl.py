#!/usr/bin/env python3
"""Small, dependency-free inspection tools for Tricky Quiky Games II."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import struct
import sys
import zlib
from collections import Counter
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any, Optional


ARE_LAYOUT_OFFSET = 0x160
ARE_WIDTH_OFFSET = 0x0E
ARE_HEIGHT_OFFSET = 0x10
ARE_DECLARATION_OFFSET = 0x14E0
ARE_FIRST_RECORD_OFFSET = 0x14E8
DEFAULT_ENTITY_CATALOG = Path(__file__).resolve().parents[1] / "entity-types.json"
DEDICATED_ENTITY_HANDLERS = {
    0x65: "01F7:178D",
    0x66: "01F7:1798",
    0x67: "01F7:17A3",
}


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
    bob_record_count: Optional[int] = None
    bob_slot_min: Optional[int] = None
    bob_slot_max: Optional[int] = None


@dataclass(frozen=True)
class ArchiveIndex:
    path: str
    file_size: int
    directory_offset: int
    entry_count: int
    type_counts: tuple[ArchiveTypeSummary, ...]
    assets: tuple[ArchiveAsset, ...]


@dataclass(frozen=True)
class LevelRenderSummary:
    map_path: str
    output_path: str
    world: str
    map_width: int
    map_height: int
    pixel_width: int
    pixel_height: int
    tile_count: int
    invalid_tile_cells: int
    are_path: Optional[str]
    entity_count: int
    overlay_mapping: str


@dataclass(frozen=True)
class BOBRecord:
    record_offset: int
    slot: int
    origin_x: int
    origin_y: int
    width: int
    height: int
    code_offsets: tuple[int, ...]
    blitter_code: bytes


@dataclass(frozen=True)
class BOBInfo:
    path: str
    file_size: int
    records: tuple[BOBRecord, ...]


@dataclass(frozen=True)
class BOBRenderSummary:
    bob_path: str
    palette_path: str
    output_path: str
    record_index: int
    slot: int
    width: int
    height: int
    origin_x: int
    origin_y: int
    opaque_pixels: int


@dataclass(frozen=True)
class BOBSheetSummary:
    bob_path: str
    palette_path: str
    output_path: str
    columns: int
    rows: int
    cell_width: int
    cell_height: int
    slot_rows: tuple[tuple[int, ...], ...]


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
    layout_cells: tuple[tuple[int, int], ...]


@dataclass(frozen=True)
class AREEntityPlacement:
    level: str
    record_offset: int
    entity_type: int
    reference: int
    region_x: int
    region_y: int
    local_x: int
    local_y: int
    world_x: int
    world_y: int


@dataclass(frozen=True)
class ARETypeCandidate:
    entity_type: int
    levels: tuple[str, ...]
    occurrences: int
    sample: AREEntityPlacement
    dispatch_slot: Optional[str]
    dispatch_entry: Optional[str]


@dataclass(frozen=True)
class EntityTypeName:
    entity_type: int
    name: str
    confidence: str
    evidence: dict[str, Any]


@dataclass(frozen=True)
class DispatchEntry:
    entity_type: int
    slot: int
    offset: int
    object_class: int
    reserved: int
    raw_bytes: tuple[int, int, int, int]

    @property
    def group_key(self) -> tuple[int, int, int]:
        return (self.offset, self.object_class, self.reserved)


@dataclass(frozen=True)
class AREInfo:
    path: str
    file_size: int
    layout_offset: int
    layout_size: int
    layout_width: int
    layout_height: int
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
        elif extension == "BOB":
            bob_info = _parse_bob_data(payload, entry.name)
            slots = [record.slot for record in bob_info.records]
            asset = ArchiveAsset(
                entry.name,
                extension,
                entry.offset,
                entry.size,
                bob_record_count=len(bob_info.records),
                bob_slot_min=min(slots),
                bob_slot_max=max(slots),
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


def _parse_map_data_with_cells(
    data: bytes, path: str
) -> tuple[MapInfo, tuple[int, ...]]:
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
    info = MapInfo(
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
    return info, cells


def _parse_map_data(data: bytes, path: str) -> MapInfo:
    return _parse_map_data_with_cells(data, path)[0]


def parse_map(path: Path) -> MapInfo:
    return _parse_map_data(path.read_bytes(), str(path))


def _parse_are_data(data: bytes, path: str) -> AREInfo:
    if len(data) < ARE_DECLARATION_OFFSET:
        raise QuikyError(
            f"ARE is shorter than its fixed layout region: {len(data)} bytes"
        )

    layout_width = _u16be(data, ARE_WIDTH_OFFSET)
    layout_height = _u16be(data, ARE_HEIGHT_OFFSET)
    if layout_width == 0 or layout_height == 0:
        raise QuikyError("ARE has zero layout width or height")
    layout_size = layout_width * layout_height * 2
    layout_end = ARE_LAYOUT_OFFSET + layout_size
    if layout_end > ARE_DECLARATION_OFFSET:
        raise QuikyError("ARE layout overlaps its declaration region")
    layout_words = struct.unpack_from(
        f">{layout_width * layout_height}H", data, ARE_LAYOUT_OFFSET
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
            record_offset = cursor
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
            entities.append(AREEntity(record_offset, entity_type, x, y))
            entity_types[entity_type] += 1
            entity_count += 1

        references.append(
            AREReference(
                reference,
                target_offset,
                reference_counts[reference],
                tuple(entities),
                tuple(
                    (index % layout_width, index // layout_width)
                    for index, value in enumerate(layout_words)
                    if value == reference
                ),
            )
        )

    return AREInfo(
        path,
        len(data),
        ARE_LAYOUT_OFFSET,
        layout_size,
        layout_width,
        layout_height,
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


def iter_are_entity_placements(
    info: AREInfo, level: Optional[str] = None
) -> tuple[AREEntityPlacement, ...]:
    level_name = level or Path(info.path).name
    placements: list[AREEntityPlacement] = []
    for reference in info.references:
        for cell_x, cell_y in reference.layout_cells:
            region_x = cell_x * 64
            region_y = cell_y * 64
            for entity in reference.entities:
                placements.append(
                    AREEntityPlacement(
                        level_name,
                        entity.record_offset,
                        entity.entity_type,
                        reference.reference,
                        region_x,
                        region_y,
                        entity.x,
                        entity.y,
                        region_x + entity.x,
                        region_y + entity.y,
                    )
                )
    return tuple(placements)


def build_are_type_catalog(archive_path: Path) -> tuple[ARETypeCandidate, ...]:
    source = archive_path.read_bytes()
    archive = parse_archive(archive_path)
    by_type: dict[int, list[AREEntityPlacement]] = {}
    for entry in archive.entries:
        if _archive_extension(entry.name) != "ARE":
            continue
        payload = source[entry.offset : entry.offset + entry.size]
        info = _parse_are_data(payload, entry.name)
        for placement in iter_are_entity_placements(info, entry.name):
            by_type.setdefault(placement.entity_type, []).append(placement)

    candidates = []
    for entity_type, placements in sorted(by_type.items()):
        dedicated = entity_type in DEDICATED_ENTITY_HANDLERS
        candidates.append(
            ARETypeCandidate(
                entity_type,
                tuple(sorted({placement.level for placement in placements})),
                len(placements),
                placements[0],
                None if dedicated else f"DS:81D2+0x{entity_type * 4:03X}",
                DEDICATED_ENTITY_HANDLERS.get(entity_type),
            )
        )
    return tuple(candidates)


def select_entity_representative(
    archive_path: Path, entity_type: int, level: str = "W1L1.ARE",
    anchor_x: int = 768, anchor_y: int = 224,
) -> AREEntityPlacement:
    """Choose a deterministic record nearest a known streamed world anchor."""
    source = archive_path.read_bytes()
    archive = parse_archive(archive_path)
    normalized_level = level.upper()
    entry = next(
        (item for item in archive.entries if item.name.upper() == normalized_level),
        None,
    )
    if entry is None or _archive_extension(entry.name) != "ARE":
        raise QuikyError(f"ARE level not found in archive: {level}")
    payload = source[entry.offset : entry.offset + entry.size]
    placements = [
        item for item in iter_are_entity_placements(
            _parse_are_data(payload, entry.name), entry.name
        )
        if item.entity_type == entity_type
    ]
    if not placements:
        raise QuikyError(
            f"type 0x{entity_type:02x} does not occur in {entry.name}"
        )
    return min(
        placements,
        key=lambda item: (
            abs(item.world_x - anchor_x) + abs(item.world_y - anchor_y),
            item.record_offset, item.reference, item.world_y, item.world_x,
        ),
    )


def load_entity_type_names(
    path: Path = DEFAULT_ENTITY_CATALOG,
) -> dict[int, EntityTypeName]:
    raw = json.loads(path.read_text(encoding="utf-8"))
    if raw.get("schema") != "quiky-entity-types-v1" or not isinstance(
        raw.get("entries"), list
    ):
        raise QuikyError(f"unsupported entity catalog schema in {path}")
    result = {}
    for entry in raw["entries"]:
        entity_type = entry.get("type")
        name = entry.get("name")
        confidence = entry.get("confidence")
        if not isinstance(entity_type, int) or not isinstance(name, str):
            raise QuikyError(f"invalid entity catalog entry in {path}")
        if confidence not in ("confirmed", "probable", "unknown"):
            raise QuikyError(f"invalid confidence for entity type {entity_type}")
        if entity_type in result:
            raise QuikyError(f"duplicate entity type {entity_type} in {path}")
        result[entity_type] = EntityTypeName(
            entity_type, name, confidence, entry.get("evidence", {})
        )
    return result


def _find_are_reference(info: AREInfo, reference: Optional[int]) -> AREReference:
    if reference is None:
        if not info.references:
            raise QuikyError("ARE contains no declaration references")
        return info.references[0]
    for candidate in info.references:
        if candidate.reference == reference:
            return candidate
    raise QuikyError(f"ARE reference 0x{reference:04x} was not found")


def patch_are_entity_data(
    data: bytes,
    reference: Optional[int] = None,
    entity_index: int = 0,
    delta_x: int = 0,
    delta_y: int = 0,
    entity_type: Optional[int] = None,
) -> bytes:
    info = _parse_are_data(data, "<memory>")
    selected = _find_are_reference(info, reference)
    if entity_index < 0 or entity_index >= len(selected.entities):
        raise QuikyError(
            f"entity index {entity_index} is outside reference "
            f"0x{selected.reference:04x}"
        )
    entity = selected.entities[entity_index]
    record_offset = entity.record_offset
    new_x = entity.x + delta_x
    new_y = entity.y + delta_y
    new_type = entity.entity_type if entity_type is None else entity_type
    for label, value in (("x", new_x), ("y", new_y), ("type", new_type)):
        if value < 0 or value > 0xFFFF:
            raise QuikyError(f"patched entity {label} is outside u16 range")
    patched = bytearray(data)
    struct.pack_into(">H", patched, record_offset, new_type)
    struct.pack_into(">H", patched, record_offset + 2, new_x)
    struct.pack_into(">H", patched, record_offset + 4, new_y)
    return bytes(patched)


def move_are_reference_data(
    data: bytes,
    reference: Optional[int] = None,
    delta_x: int = 1,
    delta_y: int = 0,
    empty_value: int = 0xFFFF,
) -> bytes:
    info = _parse_are_data(data, "<memory>")
    selected = _find_are_reference(info, reference)
    layout_words = list(
        struct.unpack_from(f">{info.layout_word_count}H", data, ARE_LAYOUT_OFFSET)
    )
    source_index = layout_words.index(selected.reference)
    source_x = source_index % info.layout_width
    source_y = source_index // info.layout_width
    target_x = source_x + delta_x
    target_y = source_y + delta_y
    if not (0 <= target_x < info.layout_width and 0 <= target_y < info.layout_height):
        raise QuikyError(
            f"ARE layout move would leave the {info.layout_width}x{info.layout_height} layout"
        )
    target_index = target_y * info.layout_width + target_x
    if layout_words[target_index] not in (0, 0xFFFF):
        raise QuikyError(
            f"ARE layout target ({target_x}, {target_y}) is not empty"
        )
    if empty_value not in (0, 0xFFFF):
        raise QuikyError("ARE empty value must be 0 or 0xffff")
    layout_words[source_index] = empty_value
    layout_words[target_index] = selected.reference
    patched = bytearray(data)
    struct.pack_into(
        f">{info.layout_word_count}H", patched, ARE_LAYOUT_OFFSET, *layout_words
    )
    return bytes(patched)


def replace_archive_entry(
    path: Path,
    output_path: Path,
    name: str,
    replacement: bytes,
    overwrite: bool = False,
) -> Path:
    if path.resolve() == output_path.resolve():
        raise QuikyError("replacement archive must be a different path")
    info = parse_archive(path)
    if not any(entry.name == name for entry in info.entries):
        raise QuikyError(f"archive entry not found: {name}")
    if output_path.exists() and not overwrite:
        raise QuikyError(
            f"refusing to overwrite {output_path}; use --overwrite to allow it"
        )

    source = path.read_bytes()
    output = bytearray()
    offsets: list[int] = []
    for entry in info.entries:
        offsets.append(len(output))
        if entry.name == name:
            output.extend(replacement)
        else:
            output.extend(source[entry.offset : entry.offset + entry.size])
    directory_offset = len(output)
    for entry, offset in zip(info.entries, offsets):
        encoded_name = entry.name.encode("ascii")
        output.extend(struct.pack("<H", len(encoded_name)))
        output.extend(encoded_name)
        output.extend(struct.pack("<I", offset))
    output.extend(struct.pack("<II", directory_offset, len(info.entries) - 1))
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(output)
    return output_path


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def create_entity_variant(
    archive_path: Path,
    output_dir: Path,
    level_name: str,
    record_offset: int,
    inert_type: int = 0,
    overwrite: bool = False,
    stream_cell: Optional[tuple[int, int]] = None,
    target_type: Optional[int] = None,
    runtime_position: Optional[tuple[int, int]] = None,
) -> dict[str, Any]:
    if archive_path.resolve() == (output_dir / "NESTLE.DAT").resolve():
        raise QuikyError("variant output must not replace the source archive")
    archive = parse_archive(archive_path)
    entry = next((item for item in archive.entries if item.name == level_name), None)
    if entry is None:
        raise QuikyError(f"archive entry not found: {level_name}")
    source = archive_path.read_bytes()
    are_data = source[entry.offset : entry.offset + entry.size]
    are_info = _parse_are_data(are_data, level_name)
    matches = [
        (reference, index, entity)
        for reference in are_info.references
        for index, entity in enumerate(reference.entities)
        if entity.record_offset == record_offset
    ]
    if len(matches) != 1:
        raise QuikyError(
            f"record offset 0x{record_offset:x} matched {len(matches)} entities"
        )
    reference, entity_index, entity = matches[0]
    if runtime_position is not None and stream_cell is None:
        raise QuikyError("--runtime-position requires --stream-cell")
    baseline_are = are_data
    position_delta = None
    if runtime_position is not None:
        origin_x, origin_y = stream_cell[0] * 64, stream_cell[1] * 64
        local_x = runtime_position[0] - origin_x
        local_y = runtime_position[1] - origin_y
        if not (0 <= local_x <= 0xFFFF and 0 <= local_y <= 0xFFFF):
            raise QuikyError(
                "runtime position cannot be represented by the selected "
                "stream cell's u16 local coordinates"
            )
        position_delta = (local_x - entity.x, local_y - entity.y)
    if target_type is not None or position_delta is not None:
        baseline_are = patch_are_entity_data(
            baseline_are,
            reference.reference,
            entity_index,
            delta_x=position_delta[0] if position_delta else 0,
            delta_y=position_delta[1] if position_delta else 0,
            entity_type=target_type,
        )
    stream_redirect = None
    if stream_cell is not None:
        cell_x, cell_y = stream_cell
        if not (0 <= cell_x < are_info.layout_width and
                0 <= cell_y < are_info.layout_height):
            raise QuikyError(f"stream cell {stream_cell} lies outside ARE layout")
        layout_offset = ARE_LAYOUT_OFFSET + (
            cell_y * are_info.layout_width + cell_x
        ) * 2
        previous_reference = _u16be(are_data, layout_offset)
        # Preserve any target-type mutation already applied to the selected
        # declaration before relocating its reference into the streamed cell.
        relocated = bytearray(baseline_are)
        struct.pack_into(">H", relocated, layout_offset, reference.reference)
        baseline_are = bytes(relocated)
        stream_redirect = {
            "cell": [cell_x, cell_y],
            "layout_offset": layout_offset,
            "previous_reference": previous_reference,
            "selected_reference": reference.reference,
            "runtime_region_origin": [cell_x * 64, cell_y * 64],
        }
    removed_are = patch_are_entity_data(
        baseline_are,
        reference.reference,
        entity_index,
        entity_type=inert_type,
    )

    baseline_mutations = []
    if target_type is not None:
        baseline_mutations.append(
            f"type 0x{entity.entity_type:04x} -> target 0x{target_type:04x}"
        )
    if runtime_position is not None:
        baseline_mutations.append(
            f"runtime position -> ({runtime_position[0]},{runtime_position[1]})"
        )
    if stream_redirect:
        baseline_mutations.append("stream redirect")
    baseline_mutation = "; ".join(baseline_mutations) or "no mutation"

    runtime_dir = archive_path.parent
    variants = []
    for name, replacement, mutation in (
        (
            "baseline", baseline_are,
            f"record 0x{record_offset:x}: {baseline_mutation}",
        ),
        (
            "removed",
            removed_are,
            f"record 0x{record_offset:x}: type 0x{(target_type if target_type is not None else entity.entity_type):04x} "
            f"-> inert 0x{inert_type:04x}",
        ),
    ):
        variant_root = output_dir / name
        target = variant_root / "game"
        if variant_root.exists() and any(variant_root.iterdir()) and not overwrite:
            raise QuikyError(
                f"refusing to overwrite non-empty variant directory {variant_root}"
            )
        target.mkdir(parents=True, exist_ok=True)
        for runtime_file in runtime_dir.iterdir():
            if runtime_file.is_file() and runtime_file.name != archive_path.name:
                shutil.copy2(runtime_file, target / runtime_file.name)
        variant_archive = replace_archive_entry(
            archive_path,
            target / archive_path.name,
            level_name,
            replacement,
            overwrite=overwrite,
        )
        variants.append(
            {
                "name": name,
                "root": str(variant_root),
                "directory": str(target),
                "archive": str(variant_archive),
                "archive_sha256": _sha256(variant_archive),
                "mutation": mutation,
            }
        )

    placements = [
        placement
        for placement in iter_are_entity_placements(are_info, level_name)
        if placement.record_offset == record_offset
    ]
    manifest = {
        "source_archive": str(archive_path),
        "source_archive_sha256": _sha256(archive_path),
        "level": level_name,
        "record_offset": record_offset,
        "entity_type": target_type if target_type is not None else entity.entity_type,
        "original_entity_type": entity.entity_type,
        "target_type": target_type,
        "inert_type": inert_type,
        "runtime_position": list(runtime_position) if runtime_position else None,
        "placements": [_as_json(placement) for placement in placements],
        "stream_redirect": stream_redirect,
        "variants": variants,
    }
    output_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = output_dir / "manifest.json"
    if manifest_path.exists() and not overwrite:
        raise QuikyError(
            f"refusing to overwrite {manifest_path}; use --overwrite to allow it"
        )
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return manifest


def create_are_experiments(
    archive_path: Path,
    output_dir: Path,
    level_name: str = "W1L3.ARE",
    reference: Optional[int] = None,
    entity_index: int = 0,
    type_variant: int = 0x71,
    overwrite: bool = False,
) -> dict[str, Any]:
    info = parse_archive(archive_path)
    entry = next((candidate for candidate in info.entries if candidate.name == level_name), None)
    if entry is None:
        raise QuikyError(f"archive entry not found: {level_name}")
    source = archive_path.read_bytes()
    are_data = source[entry.offset : entry.offset + entry.size]
    are_info = _parse_are_data(are_data, level_name)
    selected = _find_are_reference(are_info, reference)
    if entity_index < 0 or entity_index >= len(selected.entities):
        raise QuikyError(
            f"entity index {entity_index} is outside reference "
            f"0x{selected.reference:04x}"
        )
    selected_entity = selected.entities[entity_index]
    x_delta = 0x10 if selected_entity.x <= 0x20 else -0x10
    y_delta = 0x10 if selected_entity.y <= 0x20 else -0x10
    selected_type = type_variant if type_variant != selected_entity.entity_type else 0x6F

    variants: list[tuple[str, bytes, str]] = [
        ("baseline", are_data, "no mutation"),
        (
            "entity-x-shift",
            patch_are_entity_data(
                are_data,
                selected.reference,
                entity_index,
                delta_x=x_delta,
            ),
            f"reference 0x{selected.reference:04x} entity {entity_index}: dx {x_delta:+#x}",
        ),
        (
            "entity-y-shift",
            patch_are_entity_data(
                are_data,
                selected.reference,
                entity_index,
                delta_y=y_delta,
            ),
            f"reference 0x{selected.reference:04x} entity {entity_index}: dy {y_delta:+#x}",
        ),
        (
            "entity-type-change",
            patch_are_entity_data(
                are_data,
                selected.reference,
                entity_index,
                entity_type=selected_type,
            ),
            f"reference 0x{selected.reference:04x} entity {entity_index}: "
            f"type 0x{selected_entity.entity_type:04x} -> 0x{selected_type:04x}",
        ),
    ]
    moved_name = "layout-shift"
    moved_data = None
    for move_x, move_y, suffix in ((1, 0, "right"), (-1, 0, "left"), (0, 1, "down"), (0, -1, "up")):
        try:
            moved_data = move_are_reference_data(
                are_data, selected.reference, move_x, move_y
            )
            moved_name = f"layout-shift-{suffix}"
            break
        except QuikyError:
            continue
    if moved_data is not None:
        variants.append(
            (
                moved_name,
                moved_data,
                f"reference 0x{selected.reference:04x}: layout cell move",
            )
        )

    output_dir.mkdir(parents=True, exist_ok=True)
    manifest_variants: list[dict[str, Any]] = []
    for variant_name, variant_are, mutation in variants:
        variant_dir = output_dir / variant_name
        variant_dir.mkdir(parents=True, exist_ok=True)
        variant_are_path = variant_dir / level_name
        if variant_are_path.exists() and not overwrite:
            raise QuikyError(
                f"refusing to overwrite {variant_are_path}; use --overwrite to allow it"
            )
        variant_are_path.write_bytes(variant_are)
        variant_archive = replace_archive_entry(
            archive_path,
            variant_dir / "NESTLE.DAT",
            level_name,
            variant_are,
            overwrite=overwrite,
        )
        manifest_variants.append(
            {
                "name": variant_name,
                "mutation": mutation,
                "are": str(variant_are_path),
                "archive": str(variant_archive),
                "observation": None,
            }
        )

    manifest = {
        "source_archive": str(archive_path),
        "level": level_name,
        "reference": selected.reference,
        "entity_index": entity_index,
        "original_entity": {
            "type": selected_entity.entity_type,
            "x": selected_entity.x,
            "y": selected_entity.y,
        },
        "variants": manifest_variants,
        "observation_note": "Run each archive in DOSBox and record the observed object position/type.",
    }
    manifest_path = output_dir / "manifest.json"
    if manifest_path.exists() and not overwrite:
        raise QuikyError(
            f"refusing to overwrite {manifest_path}; use --overwrite to allow it"
        )
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return manifest


def _parse_bob_data(data: bytes, path: str) -> BOBInfo:
    records: list[BOBRecord] = []
    cursor = 0
    while cursor < len(data):
        record_offset = cursor
        if cursor + 12 > len(data):
            raise QuikyError(f"{path}: truncated BOB record header at 0x{cursor:x}")
        slot, origin_x, origin_y, width, height, table_size = struct.unpack_from(
            "<6H", data, cursor
        )
        cursor += 12
        if width == 0 or height == 0:
            raise QuikyError(f"{path}: BOB slot {slot} has zero dimensions")
        if table_size == 0 or table_size % 2:
            raise QuikyError(f"{path}: BOB slot {slot} has an invalid offset table")
        if cursor + table_size + 2 > len(data):
            raise QuikyError(f"{path}: truncated BOB offset table for slot {slot}")
        code_offsets = struct.unpack_from(f"<{table_size // 2}H", data, cursor)
        cursor += table_size
        code_size = _u16le(data, cursor)
        cursor += 2
        if code_size == 0 or cursor + code_size > len(data):
            raise QuikyError(f"{path}: truncated BOB blitter for slot {slot}")
        blitter_code = data[cursor : cursor + code_size]
        cursor += code_size
        if any(left > right for left, right in zip(code_offsets, code_offsets[1:])):
            raise QuikyError(f"{path}: non-monotonic BOB offsets for slot {slot}")
        if code_offsets[-1] >= code_size:
            raise QuikyError(f"{path}: BOB offset outside blitter for slot {slot}")
        records.append(
            BOBRecord(
                record_offset, slot, origin_x, origin_y, width, height,
                tuple(code_offsets), blitter_code,
            )
        )
    if not records:
        raise QuikyError(f"{path}: empty BOB file")
    return BOBInfo(path, len(data), tuple(records))


def parse_bob(path: Path) -> BOBInfo:
    return _parse_bob_data(path.read_bytes(), str(path))


def find_archive_bob_slots(
    archive_path: Path, slots: set[int],
) -> tuple[dict[str, int], ...]:
    """Resolve logical sprite slots against every BOB stored in an archive."""
    source = archive_path.read_bytes()
    matches: list[dict[str, int]] = []
    for entry in parse_archive(archive_path).entries:
        if _archive_extension(entry.name) != "BOB":
            continue
        payload = source[entry.offset:entry.offset + entry.size]
        for index, record in enumerate(_parse_bob_data(payload, entry.name).records):
            if record.slot in slots:
                matches.append({
                    "slot": record.slot, "asset": entry.name,
                    "record_index": index, "record_offset": record.record_offset,
                    "origin_x": record.origin_x, "origin_y": record.origin_y,
                    "width": record.width, "height": record.height,
                })
    return tuple(sorted(matches, key=lambda item: (item["slot"], item["asset"])))


def decode_bob_record(record: BOBRecord) -> tuple[Optional[int], ...]:
    """Decode immediate VGA writes without executing the embedded x86 code."""
    pixels: list[Optional[int]] = [None] * (record.width * record.height)
    code = record.blitter_code
    phase = -1
    cursor = 0

    def write(displacement: int, value: int) -> None:
        x = (displacement % 88) * 4 + phase
        y = displacement // 88
        if x >= record.width or y >= record.height:
            raise QuikyError(
                f"BOB slot {record.slot}: blitter write ({x}, {y}) exceeds "
                f"{record.width}x{record.height} canvas"
            )
        pixels[y * record.width + x] = value

    while cursor < len(code):
        if code[cursor : cursor + 3] == b"\xee\xd0\xc0":
            phase = (phase + 1) % 4
            cursor += 3
        elif phase >= 0 and code[cursor : cursor + 2] == b"\xc6\x84" and cursor + 5 <= len(code):
            displacement = struct.unpack_from("<H", code, cursor + 2)[0]
            write(displacement, code[cursor + 4])
            cursor += 5
        elif phase >= 0 and code[cursor : cursor + 2] == b"\xc7\x84" and cursor + 6 <= len(code):
            displacement, value = struct.unpack_from("<HH", code, cursor + 2)
            write(displacement, value & 0xFF)
            write(displacement + 1, value >> 8)
            cursor += 6
        else:
            cursor += 1
    return tuple(pixels)


def _parse_pcx_palette(data: bytes, path: str) -> tuple[tuple[int, int, int], ...]:
    if len(data) < 769 or data[0] != 0x0A or data[3] != 0x08:
        raise QuikyError(f"{path} is not an 8-bit PCX palette")
    marker_offset = len(data) - 769
    if data[marker_offset] != 0x0C:
        raise QuikyError(f"{path} does not end with a PCX palette")
    palette = data[marker_offset + 1 : marker_offset + 769]
    return tuple(
        (palette[index], palette[index + 1], palette[index + 2])
        for index in range(0, len(palette), 3)
    )


def _decode_ico_tiles(data: bytes, path: str) -> tuple[tuple[int, ...], ...]:
    if not data or len(data) % 256:
        raise QuikyError(f"{path} is not a whole-tile ICO file")
    kellmap = data[0] >= 0x80
    tiles: list[tuple[int, ...]] = []
    for tile_offset in range(0, len(data), 256):
        source = data[tile_offset : tile_offset + 256]
        pixels: list[int] = []
        for y in range(16):
            for display_x in range(16):
                raw_x = ((display_x * 4) & 0x0F) + (display_x >> 2)
                color = source[y * 16 + raw_x]
                if kellmap:
                    if color >= 0xA0:
                        color = (color - 0xA0) + 32
                    elif color >= 0x90:
                        color = (color - 0x90) + 16
                pixels.append(color)
        tiles.append(tuple(pixels))
    return tuple(tiles)


def _png_chunk(kind: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + kind
        + payload
        + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)
    )


def _write_rgb_png(path: Path, width: int, height: int, pixels: bytes) -> None:
    if len(pixels) != width * height * 3:
        raise QuikyError("internal RGB image size mismatch")
    scanlines = bytearray()
    stride = width * 3
    for row in range(height):
        scanlines.append(0)
        start = row * stride
        scanlines.extend(pixels[start : start + stride])
    png = bytearray(b"\x89PNG\r\n\x1a\n")
    png.extend(
        _png_chunk(
            b"IHDR",
            struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0),
        )
    )
    png.extend(_png_chunk(b"IDAT", zlib.compress(bytes(scanlines), level=6)))
    png.extend(_png_chunk(b"IEND", b""))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(png)


def render_bob(
    bob_path: Path,
    palette_path: Path,
    output_path: Path,
    record_index: int = 0,
    slot: Optional[int] = None,
) -> BOBRenderSummary:
    info = parse_bob(bob_path)
    if slot is not None:
        matches = [
            (index, record)
            for index, record in enumerate(info.records)
            if record.slot == slot
        ]
        if not matches:
            raise QuikyError(f"{bob_path}: sprite slot {slot} is not present")
        if len(matches) != 1:
            raise QuikyError(f"{bob_path}: sprite slot {slot} is ambiguous")
        record_index, record = matches[0]
    else:
        if record_index < 0 or record_index >= len(info.records):
            raise QuikyError(f"{bob_path}: record index {record_index} is out of range")
        record = info.records[record_index]

    palette = _parse_pcx_palette(palette_path.read_bytes(), str(palette_path))
    decoded = decode_bob_record(record)
    rgb = bytearray()
    opaque_pixels = 0
    for index, color_index in enumerate(decoded):
        if color_index is None:
            x = index % record.width
            y = index // record.width
            color = (48, 48, 48) if ((x // 4) + (y // 4)) % 2 else (80, 80, 80)
        else:
            color = palette[color_index]
            opaque_pixels += 1
        rgb.extend(color)
    _write_rgb_png(output_path, record.width, record.height, bytes(rgb))
    return BOBRenderSummary(
        str(bob_path), str(palette_path), str(output_path), record_index,
        record.slot, record.width, record.height, record.origin_x,
        record.origin_y, opaque_pixels,
    )


def render_bob_sheet(
    bob_path: Path, palette_path: Path, output_path: Path, columns: int = 8,
) -> BOBSheetSummary:
    """Render every BOB record, sorted by slot, into a compact contact sheet."""
    if columns < 1:
        raise QuikyError("BOB sheet columns must be positive")
    info = parse_bob(bob_path)
    palette = _parse_pcx_palette(palette_path.read_bytes(), str(palette_path))
    records = sorted(info.records, key=lambda record: record.slot)
    cell_width = max(record.width for record in records) + 4
    cell_height = max(record.height for record in records) + 4
    rows = (len(records) + columns - 1) // columns
    width, height = columns * cell_width, rows * cell_height
    rgb = bytearray(width * height * 3)
    for y in range(height):
        for x in range(width):
            color = (42, 42, 42) if ((x // 4) + (y // 4)) % 2 else (68, 68, 68)
            offset = (y * width + x) * 3
            rgb[offset:offset + 3] = bytes(color)
    for index, record in enumerate(records):
        cell_x = (index % columns) * cell_width
        cell_y = (index // columns) * cell_height
        left = cell_x + (cell_width - record.width) // 2
        top = cell_y + (cell_height - record.height) // 2
        for pixel_index, color_index in enumerate(decode_bob_record(record)):
            if color_index is None:
                continue
            x = left + pixel_index % record.width
            y = top + pixel_index // record.width
            offset = (y * width + x) * 3
            rgb[offset:offset + 3] = bytes(palette[color_index])
    _write_rgb_png(output_path, width, height, bytes(rgb))
    slot_rows = tuple(
        tuple(record.slot for record in records[start:start + columns])
        for start in range(0, len(records), columns)
    )
    return BOBSheetSummary(
        str(bob_path), str(palette_path), str(output_path), columns, rows,
        cell_width, cell_height, slot_rows,
    )


def _entity_marker_color(entity_type: int) -> tuple[int, int, int]:
    return (
        96 + ((entity_type * 73) % 160),
        96 + ((entity_type * 131) % 160),
        96 + ((entity_type * 197) % 160),
    )


def _draw_entity_marker(
    pixels: bytearray,
    width: int,
    height: int,
    center_x: int,
    center_y: int,
    color: tuple[int, int, int],
) -> None:
    for dy in range(-4, 5):
        for dx in range(-4, 5):
            x = center_x + dx
            y = center_y + dy
            if x < 0 or y < 0 or x >= width or y >= height:
                continue
            if max(abs(dx), abs(dy)) == 4:
                draw_color = (0, 0, 0)
            else:
                draw_color = color
            pixel = (y * width + x) * 3
            pixels[pixel : pixel + 3] = bytes(draw_color)


def _overlay_are_entities(
    pixels: bytearray,
    pixel_width: int,
    pixel_height: int,
    are_data: bytes,
    are_info: AREInfo,
) -> None:
    layout_words = struct.unpack_from(
        f">{are_info.layout_word_count}H", are_data, ARE_LAYOUT_OFFSET
    )
    references = {reference.reference: reference for reference in are_info.references}
    for layout_index, reference_value in enumerate(layout_words):
        if reference_value in (0, 0xFFFF):
            continue
        reference = references[reference_value]
        layout_x = layout_index % are_info.layout_width
        layout_y = layout_index // are_info.layout_width
        for entity in reference.entities:
            center_x = layout_x * 64 + entity.x
            center_y = layout_y * 64 + entity.y
            _draw_entity_marker(
                pixels,
                pixel_width,
                pixel_height,
                center_x,
                center_y,
                _entity_marker_color(entity.entity_type),
            )


def render_level(
    map_path: Path,
    output_path: Path,
    assets_dir: Optional[Path] = None,
    are_path: Optional[Path] = None,
    overlay_entities: bool = True,
) -> LevelRenderSummary:
    assets_root = assets_dir or map_path.parent
    map_info, cells = _parse_map_data_with_cells(
        map_path.read_bytes(), str(map_path)
    )
    world = map_path.stem[:2].upper()
    if len(world) != 2 or not world.startswith("W"):
        raise QuikyError(f"cannot derive world prefix from MAP name {map_path.name}")
    palette_path = assets_root / f"{world}.PCC"
    tiles_path = assets_root / f"{world}.ICO"
    palette = _parse_pcx_palette(palette_path.read_bytes(), str(palette_path))
    tiles = _decode_ico_tiles(tiles_path.read_bytes(), str(tiles_path))

    pixel_width = map_info.width * 16
    pixel_height = map_info.height * 16
    pixels = bytearray(pixel_width * pixel_height * 3)
    invalid_tile_cells = 0
    for map_y in range(map_info.height):
        for map_x in range(map_info.width):
            tile_index = cells[map_y * map_info.width + map_x] & 0x1FF
            if tile_index >= len(tiles):
                invalid_tile_cells += 1
                tile_pixels = None
            else:
                tile_pixels = tiles[tile_index]
            for tile_y in range(16):
                output_y = map_y * 16 + tile_y
                for tile_x in range(16):
                    output_x = map_x * 16 + tile_x
                    pixel = (output_y * pixel_width + output_x) * 3
                    if tile_pixels is None:
                        pixels[pixel : pixel + 3] = b"\xff\x00\xff"
                    else:
                        pixels[pixel : pixel + 3] = bytes(
                            palette[tile_pixels[tile_y * 16 + tile_x]]
                        )

    selected_are = are_path
    if selected_are is None:
        candidate = assets_root / f"{map_path.stem}.ARE"
        if candidate.exists():
            selected_are = candidate
    are_info: Optional[AREInfo] = None
    if selected_are is not None:
        are_data = selected_are.read_bytes()
        are_info = _parse_are_data(are_data, str(selected_are))
        if overlay_entities:
            _overlay_are_entities(
                pixels,
                pixel_width,
                pixel_height,
                are_data,
                are_info,
            )

    _write_rgb_png(output_path, pixel_width, pixel_height, pixels)
    return LevelRenderSummary(
        str(map_path),
        str(output_path),
        world,
        map_info.width,
        map_info.height,
        pixel_width,
        pixel_height,
        len(tiles),
        invalid_tile_cells,
        str(selected_are) if selected_are is not None else None,
        are_info.entity_count if are_info is not None else 0,
        "engine-confirmed 64px ARE regions" if overlay_entities else "disabled",
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


def _print_are(
    info: AREInfo, as_json: bool, show_entities: bool,
    names: Optional[dict[int, EntityTypeName]] = None,
) -> None:
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
        suffix = ""
        if names and entity_type in names:
            named = names[entity_type]
            suffix = f" {named.name} [{named.confidence}]"
        print(f"  0x{entity_type:04x}: {count}{suffix}")
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


def _print_entity_catalog(
    catalog: tuple[ARETypeCandidate, ...], as_json: bool,
    names: Optional[dict[int, EntityTypeName]] = None,
    dispatch: Optional[dict[int, DispatchEntry]] = None,
    groups: bool = False,
) -> None:
    if groups:
        grouped: dict[str, list[int]] = {}
        for candidate in catalog:
            if candidate.entity_type in DEDICATED_ENTITY_HANDLERS:
                key = f"dedicated {DEDICATED_ENTITY_HANDLERS[candidate.entity_type]}"
            elif dispatch and candidate.entity_type in dispatch:
                key = format_dispatch_entry(dispatch[candidate.entity_type])
            else:
                key = "uncaptured"
            grouped.setdefault(key, []).append(candidate.entity_type)
        rows = [
            {"initializer": key, "types": values, "type_count": len(values)}
            for key, values in grouped.items()
        ]
        if as_json:
            print(json.dumps(rows, indent=2))
        else:
            print("initializer\ttypes")
            for row in rows:
                labels = ",".join(f"0x{value:02X}" for value in row["types"])
                print(f"{row['initializer']}\t{labels}")
        return
    if as_json:
        rows = []
        for candidate in catalog:
            row = _as_json(candidate)
            if names and candidate.entity_type in names:
                named = names[candidate.entity_type]
                row["name"] = named.name
                row["confidence"] = named.confidence
                row["evidence"] = named.evidence
                if initializer := named.evidence.get("initializer"):
                    row["dispatch_entry"] = initializer
            if dispatch and candidate.entity_type in dispatch:
                entry = dispatch[candidate.entity_type]
                row["dispatch_entry"] = format_dispatch_entry(entry)
                row["dispatch"] = _as_json(entry)
                row["dispatch_group"] = (
                    f"01F7:{entry.offset:04X}:class-{entry.object_class}:"
                    f"reserved-0x{entry.reserved:02X}"
                )
            rows.append(row)
        print(json.dumps(rows, indent=2))
        return
    print("type\tlevels\toccurrences\tsample world\tdispatch entry")
    for candidate in catalog:
        named = names.get(candidate.entity_type) if names else None
        captured = dispatch.get(candidate.entity_type) if dispatch else None
        dispatch_label = (
            format_dispatch_entry(captured) if captured else None
        ) or (
            named.evidence.get("initializer") if named else None
        ) or candidate.dispatch_entry or candidate.dispatch_slot or "unknown"
        type_label = f"0x{candidate.entity_type:04x}"
        if named:
            type_label += f" {named.name} [{named.confidence}]"
        print(
            f"{type_label}\t{','.join(candidate.levels)}\t"
            f"{candidate.occurrences}\t"
            f"({candidate.sample.world_x},{candidate.sample.world_y})\t{dispatch_label}"
        )


def format_dispatch_entry(entry: DispatchEntry) -> str:
    return f"01F7:{entry.offset:04X} class {entry.object_class}"


def load_dispatch_ledger(path: Path) -> dict[int, DispatchEntry]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    if payload.get("trace_kind") != "dispatch":
        raise QuikyError("dispatch ledger has the wrong trace_kind")
    result: dict[int, DispatchEntry] = {}
    raw_events = payload.get("events", [])
    if isinstance(raw_events, dict):
        raw_events = [raw_events[key] for key in sorted(raw_events, key=int)]
    if not isinstance(raw_events, list):
        raise QuikyError("dispatch ledger events must be an array")
    for raw in raw_events:
        if not isinstance(raw, dict):
            raise QuikyError("dispatch ledger event must be an object")
        raw_values = raw.get("raw_bytes", ())
        if isinstance(raw_values, dict):
            raw_values = [raw_values[key] for key in sorted(raw_values, key=int)]
        values = tuple(raw_values)
        if len(values) != 4 or any(not isinstance(value, int) or value < 0 or value > 255
                                   for value in values):
            raise QuikyError("dispatch ledger contains invalid raw bytes")
        entry = DispatchEntry(
            entity_type=int(raw["type"]), slot=int(raw["slot"]),
            offset=int(raw["offset"]),
            object_class=int(raw.get("object_class", raw.get("segment_ordinal", -1))),
            reserved=int(raw.get("reserved", raw.get("flags", -1))),
            raw_bytes=values,
        )
        expected_slot = 0x81D2 + entry.entity_type * 4
        if entry.slot != expected_slot:
            raise QuikyError(
                f"dispatch slot mismatch for type 0x{entry.entity_type:02x}"
            )
        if entry.raw_bytes != (
            entry.offset & 0xff, entry.offset >> 8,
            entry.object_class, entry.reserved,
        ):
            raise QuikyError(
                f"dispatch decode mismatch for type 0x{entry.entity_type:02x}"
            )
        if entry.entity_type in result:
            raise QuikyError(f"duplicate dispatch type 0x{entry.entity_type:02x}")
        result[entry.entity_type] = entry
    return result


def build_entity_experiment_plan(
    archive_path: Path, level: str,
    dispatch: Optional[dict[int, DispatchEntry]] = None,
) -> list[dict[str, Any]]:
    rows = []
    for candidate in build_are_type_catalog(archive_path):
        if level not in candidate.levels:
            continue
        selected = select_entity_representative(
            archive_path, candidate.entity_type, level,
        )
        captured = dispatch.get(candidate.entity_type) if dispatch else None
        callback = (
            format_dispatch_entry(captured) if captured else
            DEDICATED_ENTITY_HANDLERS.get(candidate.entity_type, "uncaptured")
        )
        rows.append({
            "type": candidate.entity_type,
            "level": level,
            "record_offset": selected.record_offset,
            "original_world": [selected.world_x, selected.world_y],
            "reference": selected.reference,
            "callback_group": callback,
            "command": (
                "python3 research/tools/quikyentity.py game/NESTLE.DAT "
                f"research/build/entity-{candidate.entity_type:02x}-auto "
                f"--type 0x{candidate.entity_type:02x} --overwrite"
            ),
        })
    return rows


def _print_entity_experiment_plan(rows: list[dict[str, Any]], as_json: bool) -> None:
    if as_json:
        print(json.dumps(rows, indent=2))
        return
    print("type\trecord\toriginal world\tcallback group")
    for row in rows:
        print(
            f"0x{row['type']:02X}\t0x{row['record_offset']:04X}\t"
            f"({row['original_world'][0]},{row['original_world'][1]})\t"
            f"{row['callback_group']}"
        )


def _print_level_summary(summary: LevelRenderSummary, as_json: bool) -> None:
    if as_json:
        print(json.dumps(_as_json(summary), indent=2))
        return
    print(f"rendered: {summary.output_path}")
    print(
        f"MAP: {summary.map_path} ({summary.map_width}x{summary.map_height} tiles, "
        f"{summary.pixel_width}x{summary.pixel_height} pixels)"
    )
    print(f"world: {summary.world}")
    print(f"tiles: {summary.tile_count}")
    print(f"invalid tile cells: {summary.invalid_tile_cells}")
    print(f"ARE: {summary.are_path or 'not found'}")
    print(f"entity records: {summary.entity_count}")
    print(f"overlay mapping: {summary.overlay_mapping}")


def _parse_int(value: str) -> int:
    try:
        return int(value, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"invalid integer: {value}") from exc


def _print_bob(info: BOBInfo, as_json: bool) -> None:
    records = [
        {
            "index": index,
            "record_offset": record.record_offset,
            "slot": record.slot,
            "origin_x": record.origin_x,
            "origin_y": record.origin_y,
            "width": record.width,
            "height": record.height,
            "offset_count": len(record.code_offsets),
            "blitter_size": len(record.blitter_code),
            "opaque_pixels": sum(
                value is not None for value in decode_bob_record(record)
            ),
        }
        for index, record in enumerate(info.records)
    ]
    if as_json:
        print(json.dumps({
            "path": info.path, "file_size": info.file_size,
            "record_count": len(info.records), "records": records,
        }, indent=2))
        return
    print(f"BOB: {info.path}")
    print(f"size: {info.file_size} bytes")
    print(f"records: {len(info.records)}")
    print("index\tslot\torigin\tdimensions\toffsets\tblitter\tpixels")
    for record in records:
        print(
            f"{record['index']}\t{record['slot']}\t"
            f"{record['origin_x']},{record['origin_y']}\t"
            f"{record['width']}x{record['height']}\t"
            f"{record['offset_count']}\t{record['blitter_size']}\t"
            f"{record['opaque_pixels']}"
        )


def _print_bob_render(summary: BOBRenderSummary, as_json: bool) -> None:
    if as_json:
        print(json.dumps(_as_json(summary), indent=2))
        return
    print(f"rendered BOB slot {summary.slot} to {summary.output_path}")
    print(
        f"record {summary.record_index}; {summary.width}x{summary.height}; "
        f"origin {summary.origin_x},{summary.origin_y}; "
        f"{summary.opaque_pixels} opaque pixels"
    )


def _print_bob_sheet(summary: BOBSheetSummary, as_json: bool) -> None:
    if as_json:
        print(json.dumps(_as_json(summary), indent=2))
        return
    print(f"rendered {sum(map(len, summary.slot_rows))} BOB sprites to "
          f"{summary.output_path}")
    for row, slots in enumerate(summary.slot_rows):
        print(f"row {row}: " + ", ".join(str(slot) for slot in slots))


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

    bob_info = subparsers.add_parser("bob-info", help="inspect compiled BOB sprites")
    bob_info.add_argument("path", type=Path)
    bob_info.add_argument("--json", action="store_true")

    bob_find = subparsers.add_parser(
        "bob-find", help="resolve logical sprite slots across archive BOBs"
    )
    bob_find.add_argument("path", type=Path)
    bob_find.add_argument("--slot", action="append", required=True,
                          type=_parse_int)
    bob_find.add_argument("--json", action="store_true")

    bob_render = subparsers.add_parser(
        "bob-render", help="safely decode one BOB sprite to PNG"
    )
    bob_render.add_argument("path", type=Path)
    bob_render.add_argument("--palette", required=True, type=Path)
    bob_render.add_argument("--output", required=True, type=Path)
    selection = bob_render.add_mutually_exclusive_group()
    selection.add_argument("--record-index", type=int, default=0)
    selection.add_argument("--slot", type=_parse_int)
    bob_render.add_argument("--json", action="store_true")

    bob_sheet = subparsers.add_parser(
        "bob-sheet", help="render every BOB sprite as a slot-ordered sheet"
    )
    bob_sheet.add_argument("path", type=Path)
    bob_sheet.add_argument("--palette", required=True, type=Path)
    bob_sheet.add_argument("--output", required=True, type=Path)
    bob_sheet.add_argument("--columns", type=int, default=8)
    bob_sheet.add_argument("--json", action="store_true")

    are_info = subparsers.add_parser("are-info", help="inspect an ARE object file")
    are_info.add_argument("path", type=Path)
    are_info.add_argument("--json", action="store_true")
    are_info.add_argument(
        "--entities", action="store_true", help="show decoded entity coordinates"
    )
    are_info.add_argument("--catalog", type=Path, default=DEFAULT_ENTITY_CATALOG)

    entity_catalog = subparsers.add_parser(
        "entity-catalog", help="catalog unique ARE entity types across an archive"
    )
    entity_catalog.add_argument("path", type=Path)
    entity_catalog.add_argument("--json", action="store_true")
    entity_catalog.add_argument("--catalog", type=Path, default=DEFAULT_ENTITY_CATALOG)
    entity_catalog.add_argument("--dispatch-ledger", type=Path)
    entity_catalog.add_argument(
        "--groups", action="store_true",
        help="group types by captured initializer (dedicated paths separately)",
    )

    entity_plan = subparsers.add_parser(
        "entity-experiment-plan",
        help="plan deterministic controlled experiments for one level",
    )
    entity_plan.add_argument("path", type=Path)
    entity_plan.add_argument("--level", default="W1L1.ARE")
    entity_plan.add_argument("--dispatch-ledger", type=Path)
    entity_plan.add_argument("--json", action="store_true")

    level_render = subparsers.add_parser(
        "level-render", help="render a MAP with its world tiles and ARE overlay"
    )
    level_render.add_argument("path", type=Path)
    level_render.add_argument("--output", required=True, type=Path)
    level_render.add_argument("--assets-dir", type=Path)
    level_render.add_argument("--are", type=Path)
    level_render.add_argument("--no-entities", action="store_true")
    level_render.add_argument("--json", action="store_true")

    are_patch = subparsers.add_parser(
        "are-patch", help="patch one raw ARE entity for an experiment"
    )
    are_patch.add_argument("path", type=Path)
    are_patch.add_argument("output", type=Path)
    are_patch.add_argument("--reference", type=_parse_int)
    are_patch.add_argument("--entity-index", type=int, default=0)
    are_patch.add_argument("--dx", type=_parse_int, default=0)
    are_patch.add_argument("--dy", type=_parse_int, default=0)
    are_patch.add_argument("--type", dest="entity_type", type=_parse_int)

    are_experiment = subparsers.add_parser(
        "are-experiment", help="generate isolated ARE mutation archives"
    )
    are_experiment.add_argument("path", type=Path)
    are_experiment.add_argument("output_dir", type=Path)
    are_experiment.add_argument("--level", default="W1L3.ARE")
    are_experiment.add_argument("--reference", type=_parse_int)
    are_experiment.add_argument("--entity-index", type=int, default=0)
    are_experiment.add_argument("--type-variant", type=_parse_int, default=0x71)
    are_experiment.add_argument("--overwrite", action="store_true")
    are_experiment.add_argument("--json", action="store_true")

    entity_variant = subparsers.add_parser(
        "entity-variant", help="create isolated baseline and inert-record runtimes"
    )
    entity_variant.add_argument("path", type=Path)
    entity_variant.add_argument("output_dir", type=Path)
    entity_variant.add_argument("--level", required=True)
    entity_variant.add_argument("--record-offset", required=True, type=_parse_int)
    entity_variant.add_argument("--inert-type", type=_parse_int, default=0)
    entity_variant.add_argument(
        "--target-type", type=_parse_int,
        help="replace the selected record in the baseline before creating the inert variant",
    )
    entity_variant.add_argument(
        "--stream-cell", nargs=2, type=_parse_int, metavar=("X", "Y"),
        help="also place the selected reference in an ARE layout cell for startup streaming",
    )
    entity_variant.add_argument(
        "--runtime-position", nargs=2, type=_parse_int, metavar=("X", "Y"),
        help="patch the selected entity's runtime world position after stream relocation",
    )
    entity_variant.add_argument("--overwrite", action="store_true")
    entity_variant.add_argument("--json", action="store_true")

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
        elif args.command == "bob-info":
            _print_bob(parse_bob(args.path), args.json)
        elif args.command == "bob-find":
            matches = find_archive_bob_slots(args.path, set(args.slot))
            if args.json:
                print(json.dumps(matches, indent=2))
            else:
                print("slot\tasset\trecord\torigin\tdimensions")
                for match in matches:
                    print(
                        f"{match['slot']}\t{match['asset']}\t"
                        f"{match['record_index']}\t"
                        f"{match['origin_x']},{match['origin_y']}\t"
                        f"{match['width']}x{match['height']}"
                    )
        elif args.command == "bob-render":
            _print_bob_render(
                render_bob(
                    args.path, args.palette, args.output,
                    args.record_index, args.slot,
                ),
                args.json,
            )
        elif args.command == "bob-sheet":
            _print_bob_sheet(
                render_bob_sheet(
                    args.path, args.palette, args.output, args.columns,
                ),
                args.json,
            )
        elif args.command == "are-info":
            _print_are(
                parse_are(args.path), args.json, args.entities,
                load_entity_type_names(args.catalog) if not args.json else None,
            )
        elif args.command == "entity-catalog":
            _print_entity_catalog(
                build_are_type_catalog(args.path), args.json,
                load_entity_type_names(args.catalog),
                load_dispatch_ledger(args.dispatch_ledger)
                if args.dispatch_ledger else None,
                args.groups,
            )
        elif args.command == "entity-experiment-plan":
            _print_entity_experiment_plan(
                build_entity_experiment_plan(
                    args.path, args.level,
                    load_dispatch_ledger(args.dispatch_ledger)
                    if args.dispatch_ledger else None,
                ),
                args.json,
            )
        elif args.command == "level-render":
            _print_level_summary(
                render_level(
                    args.path,
                    args.output,
                    args.assets_dir,
                    args.are,
                    not args.no_entities,
                ),
                args.json,
            )
        elif args.command == "are-patch":
            patched = patch_are_entity_data(
                args.path.read_bytes(),
                args.reference,
                args.entity_index,
                args.dx,
                args.dy,
                args.entity_type,
            )
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_bytes(patched)
            print(f"patched ARE written to {args.output}")
        elif args.command == "are-experiment":
            manifest = create_are_experiments(
                args.path,
                args.output_dir,
                args.level,
                args.reference,
                args.entity_index,
                args.type_variant,
                args.overwrite,
            )
            if args.json:
                print(json.dumps(manifest, indent=2))
            else:
                print(f"generated {len(manifest['variants'])} variants in {args.output_dir}")
                print(f"manifest: {args.output_dir / 'manifest.json'}")
                for variant in manifest["variants"]:
                    print(f"  {variant['name']}: {variant['mutation']}")
        elif args.command == "entity-variant":
            manifest = create_entity_variant(
                args.path,
                args.output_dir,
                args.level,
                args.record_offset,
                args.inert_type,
                args.overwrite,
                tuple(args.stream_cell) if args.stream_cell else None,
                target_type=args.target_type,
                runtime_position=(tuple(args.runtime_position)
                                  if args.runtime_position else None),
            )
            if args.json:
                print(json.dumps(manifest, indent=2))
            else:
                print(f"created entity variants in {args.output_dir}")
                for variant in manifest["variants"]:
                    print(f"  {variant['name']}: {variant['mutation']}")
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
