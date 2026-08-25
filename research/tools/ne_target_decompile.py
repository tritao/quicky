#!/usr/bin/env python3
"""Disassemble selected QUIKY.EXE NE segments with relocations resolved.

The executable is a 16-bit NE image.  Capstone can decode the segment bytes,
but far calls are stored with zero operands and are fixed up by the loader
from the segment relocation table.  This tool applies those internal NE
relocations in memory before disassembly and annotates the resulting calls
with the runtime selector/offset used by the debugger traces.
"""

from __future__ import annotations

import argparse
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

try:
    from capstone import CS_ARCH_X86, CS_MODE_16, Cs
except ImportError as exc:  # pragma: no cover - diagnostic for fresh checkouts
    raise SystemExit("ne_target_decompile.py requires the capstone package") from exc


RUNTIME_SELECTORS = {
    1: "01D7",
    2: "01E7",
    3: "01F7",
    4: "0207",
    5: "0227",
    6: "0237",
}


@dataclass(frozen=True)
class Segment:
    number: int
    file_offset: int
    length: int
    relocations: tuple[tuple[int, int, int, int, int], ...]


def ne_segments(blob: bytes) -> tuple[int, int, list[Segment]]:
    """Return (NE header offset, alignment shift, segment metadata)."""
    if blob[0:2] != b"MZ":
        raise ValueError("not an MZ executable")
    ne_offset = struct.unpack_from("<I", blob, 0x3C)[0]
    if blob[ne_offset:ne_offset + 2] != b"NE":
        raise ValueError("MZ executable has no NE header")
    segment_count = struct.unpack_from("<H", blob, ne_offset + 0x1C)[0]
    segment_table = struct.unpack_from("<H", blob, ne_offset + 0x22)[0]
    resource_table = struct.unpack_from("<H", blob, ne_offset + 0x24)[0]
    align_shift = struct.unpack_from("<H", blob, ne_offset + 0x32)[0]
    segment_table_abs = ne_offset + segment_table
    segments: list[Segment] = []
    for number in range(1, segment_count + 1):
        entry = segment_table_abs + (number - 1) * 8
        sector, length, _flags, _min_alloc = struct.unpack_from("<HHHH", blob, entry)
        # A zero-length segment is legal (QUIKY has one at the end of its
        # table) and has no image or relocation records.
        if length == 0:
            segments.append(Segment(number, 0, 0, ()))
            continue
        file_offset = sector << align_shift
        data_end = file_offset + length
        if data_end > len(blob):
            raise ValueError(f"segment {number} extends past end of file")
        # Relocations are appended to the physical segment image.  The NE
        # segment table length excludes them, so the count is immediately
        # after the segment bytes and records are 8 bytes each.
        reloc_count = struct.unpack_from("<H", blob, data_end)[0]
        reloc_start = data_end + 2
        reloc_end = reloc_start + reloc_count * 8
        if reloc_end > len(blob):
            raise ValueError(f"segment {number} relocation table is truncated")
        relocs = tuple(
            struct.unpack_from("<BBHHH", blob, reloc_start + i * 8)
            for i in range(reloc_count)
        )
        segments.append(Segment(number, file_offset, length, relocs))
    return ne_offset, align_shift, segments


def selector(segment: int) -> str:
    return RUNTIME_SELECTORS.get(segment, f"seg{segment}")


def disassemble(blob: bytes, segment: Segment, start: int, end: int,
                calls_only: bool = False) -> list[str]:
    if not 0 <= start < end <= segment.length:
        raise ValueError(f"range {start:#x}:{end:#x} is outside segment {segment.number}")
    code = bytearray(blob[segment.file_offset:segment.file_offset + segment.length])
    # Relocation sources point at the first byte of the far-call operand,
    # normally one byte after the instruction address (opcode 9Ah).
    far_targets: dict[int, tuple[int, int]] = {}
    for rel_type, rel_flags, source, target_segment, target_offset in segment.relocations:
        # 0x03 is an internal far call.  The relocation source points at the
        # two-byte offset operand; the following word is the segment operand.
        if rel_type == 0x03 and rel_flags == 0 and source + 4 <= len(code):
            struct.pack_into("<HH", code, source, target_offset, target_segment)
            far_targets[source] = (target_segment, target_offset)

    md = Cs(CS_ARCH_X86, CS_MODE_16)
    md.detail = False
    output: list[str] = []
    for insn in md.disasm(bytes(code[start:end]), start):
        source = insn.address
        text = f"{selector(segment.number)}:{source:04X}  {insn.mnemonic:<7} {insn.op_str}".rstrip()
        relocation = next(
            (target for operand, target in far_targets.items()
             if insn.address <= operand < insn.address + insn.size),
            None,
        )
        if relocation is not None:
            target_segment, target_offset = relocation
            text += f"    ; -> {selector(target_segment)}:{target_offset:04X}"
        # Calls-only is intentionally limited to direct call instructions,
        # including near calls that Capstone renders as an absolute target.
        if calls_only and insn.mnemonic not in {"call", "lcall"}:
            continue
        output.append(text)
    return output


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--segment", type=int, required=True)
    parser.add_argument("--start", type=lambda value: int(value, 0), required=True)
    parser.add_argument("--end", type=lambda value: int(value, 0), required=True)
    parser.add_argument("--calls-only", action="store_true")
    return parser


def main() -> int:
    args = build_parser().parse_args()
    blob = args.executable.read_bytes()
    _ne_offset, _shift, segments = ne_segments(blob)
    try:
        segment = segments[args.segment - 1]
    except IndexError as exc:
        raise SystemExit(f"segment must be between 1 and {len(segments)}") from exc
    try:
        lines = disassemble(blob, segment, args.start, args.end, args.calls_only)
    except ValueError as exc:
        raise SystemExit(str(exc)) from exc
    print(f"# {args.executable} segment {args.segment} ({selector(args.segment)}) "
          f"range {args.start:#x}:{args.end:#x}")
    for line in lines:
        print(line)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
