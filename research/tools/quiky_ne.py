#!/usr/bin/env python3
"""Shared NE image and runtime-address model for QUIKY research tools.

The Ghidra workflow imports each file-backed NE segment as an independent
raw image.  Keeping the executable parsing and selector mapping here makes
that representation explicit and prevents individual tools from growing
slightly different versions of the same NE assumptions.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
import struct
from typing import Iterator


SEGMENT_SELECTORS: dict[int, int] = {
    1: 0x01D7,
    2: 0x01E7,
    3: 0x01F7,
    4: 0x0207,
    5: 0x0227,
    6: 0x0237,
}
SELECTOR_SEGMENTS: dict[int, int] = {
    selector: segment for segment, selector in SEGMENT_SELECTORS.items()
}


def selector_for_segment(segment: int) -> int:
    """Return the runtime selector for a known QUIKY segment."""

    try:
        return SEGMENT_SELECTORS[segment]
    except KeyError as exc:
        raise ValueError(f"unknown QUIKY segment {segment}") from exc


def segment_for_selector(selector: int | str) -> int:
    """Return the raw-image segment number for a runtime selector."""

    if isinstance(selector, str):
        selector = int(selector, 16)
    try:
        return SELECTOR_SEGMENTS[selector]
    except KeyError as exc:
        raise ValueError(f"unknown QUIKY runtime selector {selector:04X}") from exc


@dataclass(frozen=True)
class Address:
    """A segment-relative address in the separate raw-segment model."""

    segment: int
    offset: int

    @property
    def selector(self) -> int:
        return selector_for_segment(self.segment)

    def as_segment_offset(self) -> str:
        return f"{self.segment}:{self.offset:04X}"

    def as_runtime(self) -> str:
        return f"{self.selector:04X}:{self.offset:04X}"


def parse_address(value: str) -> Address:
    """Parse ``01F7:3FF8`` or the internal ``3:3FF8`` spelling."""

    try:
        segment_text, offset_text = value.strip().split(":", 1)
        if not segment_text or not offset_text:
            raise ValueError
        if segment_text.lower().startswith("0x"):
            segment_text = segment_text[2:]
        if len(segment_text) <= 2 and segment_text.isdecimal():
            segment = int(segment_text, 10)
        else:
            segment = segment_for_selector(segment_text)
        offset = int(offset_text, 16)
    except (ValueError, TypeError) as exc:
        raise ValueError(f"invalid QUIKY address {value!r}") from exc
    if segment < 1 or offset < 0 or offset > 0xFFFF:
        raise ValueError(f"invalid QUIKY address {value!r}")
    # Resolve the selector here too, so a segment-qualified address cannot
    # silently refer to an unknown runtime segment.
    selector_for_segment(segment)
    return Address(segment, offset)


@dataclass(frozen=True)
class NeSegment:
    """Metadata for one entry in the NE segment table."""

    number: int
    file_offset: int
    file_length: int
    memory_length: int
    flags: int
    min_alloc: int
    runtime_selector: int | None

    @property
    def filename(self) -> str:
        return f"QUIKY_SEG{self.number:02d}.bin"


@dataclass(frozen=True)
class NeRelocation:
    """One raw NE relocation record."""

    segment: int
    source: int
    instruction: int | None
    source_type: int
    flags: int
    target_segment: int
    target_offset: int
    target_selector: int | None

    def as_dict(self) -> dict[str, int | None]:
        return {
            "segment": self.segment,
            "source": self.source,
            "instruction": self.instruction,
            "source_type": self.source_type,
            "flags": self.flags,
            "target_segment": self.target_segment,
            "target_offset": self.target_offset,
            "target_selector": self.target_selector,
        }


@dataclass
class NeImage:
    """Parsed NE executable with raw and allocated segment views."""

    path: Path
    data: bytes
    ne_offset: int
    segment_shift: int
    segments: tuple[NeSegment, ...]
    _relocation_cache: dict[int, tuple[NeRelocation, ...]] = field(
        default_factory=dict, init=False, repr=False
    )

    def segment(self, number: int) -> NeSegment:
        if not 1 <= number <= len(self.segments):
            raise ValueError(f"segment must be between 1 and {len(self.segments)}")
        return self.segments[number - 1]

    def raw_bytes(self, number: int) -> bytes:
        segment = self.segment(number)
        start = segment.file_offset
        return self.data[start:start + segment.file_length]

    def memory_bytes(self, number: int) -> bytes:
        """Return the allocated segment image, including zero-filled tail."""

        segment = self.segment(number)
        payload = self.raw_bytes(number)
        if len(payload) < segment.memory_length:
            payload += b"\0" * (segment.memory_length - len(payload))
        return payload

    def segment_dicts(self) -> list[dict[str, int | str | None]]:
        return [
            {
                "number": segment.number,
                "file_offset": segment.file_offset,
                "file_length": segment.file_length,
                "memory_length": segment.memory_length,
                "flags": segment.flags,
                "min_alloc": segment.min_alloc,
                "runtime_selector": (
                    f"{segment.runtime_selector:04X}"
                    if segment.runtime_selector is not None else None
                ),
                "filename": segment.filename,
            }
            for segment in self.segments
        ]

    def relocations(self, number: int | None = None) -> list[NeRelocation]:
        """Read relocations, optionally restricted to one segment."""

        numbers = range(1, len(self.segments) + 1) if number is None else (number,)
        result: list[NeRelocation] = []
        for segment_number in numbers:
            if segment_number in self._relocation_cache:
                result.extend(self._relocation_cache[segment_number])
                continue
            segment = self.segment(segment_number)
            if segment.file_length == 0:
                records: tuple[NeRelocation, ...] = ()
            else:
                raw_end = segment.file_offset + segment.file_length
                if raw_end + 2 > len(self.data):
                    raise ValueError(
                        f"segment {segment.number} relocation table is truncated"
                    )
                count = struct.unpack_from("<H", self.data, raw_end)[0]
                table_end = raw_end + 2 + count * 8
                if table_end > len(self.data):
                    raise ValueError(
                        f"segment {segment.number} relocation table is truncated"
                    )
                parsed: list[NeRelocation] = []
                for index in range(count):
                    record_offset = raw_end + 2 + index * 8
                    source_type, flags, source, target_segment, target_offset = (
                        struct.unpack_from("<BBHHH", self.data, record_offset)
                    )
                    instruction = None
                    if source and source - 1 < segment.file_length:
                        if self.data[segment.file_offset + source - 1] == 0x9A:
                            instruction = source - 1
                    parsed.append(NeRelocation(
                        segment=segment.number,
                        source=source,
                        instruction=instruction,
                        source_type=source_type,
                        flags=flags,
                        target_segment=target_segment,
                        target_offset=target_offset,
                        target_selector=SEGMENT_SELECTORS.get(target_segment),
                    ))
                records = tuple(parsed)
            self._relocation_cache[segment_number] = records
            result.extend(records)
        return result

    def relocation_dicts(self, number: int | None = None) -> list[dict[str, int | None]]:
        return [record.as_dict() for record in self.relocations(number)]


def read_ne_bytes(data: bytes, path: Path = Path("<memory>")) -> NeImage:
    """Parse an NE byte string and return its shared image model."""

    data = bytes(data)
    if len(data) < 0x40 or data[:2] != b"MZ":
        raise ValueError(f"not an MZ executable: {path}")
    ne_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if ne_offset + 0x34 > len(data) or data[ne_offset:ne_offset + 2] != b"NE":
        raise ValueError(f"MZ executable does not contain an NE header: {path}")

    count = struct.unpack_from("<H", data, ne_offset + 0x1C)[0]
    table_relative = struct.unpack_from("<H", data, ne_offset + 0x22)[0]
    segment_shift = struct.unpack_from("<H", data, ne_offset + 0x32)[0]
    table = ne_offset + table_relative
    table_end = table + count * 8
    if table < 0 or table_end > len(data):
        raise ValueError(f"NE segment table is truncated: {path}")

    segments: list[NeSegment] = []
    for number in range(1, count + 1):
        offset, length, flags, min_alloc = struct.unpack_from(
            "<HHHH", data, table + (number - 1) * 8
        )
        file_offset = offset << segment_shift
        file_length = 0 if length == 0 else length
        if file_length and file_offset + file_length > len(data):
            raise ValueError(f"segment {number} extends past end of file")
        memory_length = max(file_length, min_alloc) or 0x10000
        segments.append(NeSegment(
            number=number,
            file_offset=file_offset,
            file_length=file_length,
            memory_length=memory_length,
            flags=flags,
            min_alloc=min_alloc,
            runtime_selector=SEGMENT_SELECTORS.get(number),
        ))
    return NeImage(path, data, ne_offset, segment_shift, tuple(segments))


def read_ne(path: Path) -> NeImage:
    """Parse an NE executable and return its shared image model."""

    path = Path(path)
    return read_ne_bytes(path.read_bytes(), path)


def iter_relocations(path: Path) -> Iterator[NeRelocation]:
    """Yield all relocation records from an executable."""

    yield from read_ne(path).relocations()
