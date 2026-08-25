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
import sys
from dataclasses import dataclass
from pathlib import Path
import struct

from quiky_ne import SEGMENT_SELECTORS, read_ne, read_ne_bytes

try:
    from capstone import CS_ARCH_X86, CS_MODE_16, Cs
except ImportError as exc:  # pragma: no cover - diagnostic for fresh checkouts
    raise SystemExit("ne_target_decompile.py requires the capstone package") from exc


@dataclass(frozen=True)
class Segment:
    number: int
    file_offset: int
    length: int
    relocations: tuple[tuple[int, int, int, int, int], ...]


def ne_segments(blob: bytes) -> tuple[int, int, list[Segment]]:
    """Return ``(NE header offset, alignment shift, segment metadata)``.

    Keep this compatibility-shaped function for callers of the original
    Capstone tool, but source all parsing from the shared NE model.
    """
    image = read_ne_bytes(blob)
    segments = [
        Segment(
            segment.number,
            segment.file_offset if segment.file_length else 0,
            segment.file_length,
            tuple(
                (
                    record.source_type,
                    record.flags,
                    record.source,
                    record.target_segment,
                    record.target_offset,
                )
                for record in image.relocations(segment.number)
            ),
        )
        for segment in image.segments
    ]
    return image.ne_offset, image.segment_shift, segments


def selector(segment: int) -> str:
    runtime_selector = SEGMENT_SELECTORS.get(segment)
    return f"{runtime_selector:04X}" if runtime_selector is not None else f"seg{segment}"


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
