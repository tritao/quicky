#!/usr/bin/env python3
"""Extract the MAP-tile descriptor initializers from QUIKY.EXE.

The executable contains one initializer per world.  Each initializer fills
512 four-byte records at the table published through DS:6582.  This small
decoder intentionally only recognizes the compiler's repeated instruction
patterns; it does not pretend to be a general 8086 decompiler.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from collections import Counter
from pathlib import Path
from typing import Any


INITIALIZERS = {
    "W1": (0x1734, 0x19E4),
    "W2": (0x19E4, 0x1BF1),
    "W3": (0x1BF1, 0x28ED),
    "W4": (0x28ED, 0x2D9F),
    "W5": (0x2D9F, 0x3020),
}
TABLE_ENTRIES = 512
TABLE_STRIDE = 4


def _u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def _find_init_loop(data: bytes, start: int, end: int) -> dict[str, int]:
    """Find the repeated field-0 identity write and loop bound."""
    pattern = bytes.fromhex("8b 56 fe 8b 46 fe c1 e0 02 c4 3e 82 65")
    hit = data.find(pattern, start, end)
    if hit < 0:
        raise ValueError(f"initializer 0x{start:04x}: identity loop not found")
    # The loop terminator is cmp [bp-2], 0x1ff.
    terminator = bytes.fromhex("81 7e fe ff 01")
    loop_end = data.find(terminator, hit, end)
    if loop_end < 0:
        raise ValueError(f"initializer 0x{start:04x}: loop terminator not found")
    return {
        "identity_write": hit + len(pattern),
        "loop_terminator": loop_end,
        "entry_count": TABLE_ENTRIES,
        "stride": TABLE_STRIDE,
    }


def _compare_candidates(data: bytes, start: int, stop: int) -> list[tuple[int, int]]:
    """Return (offset, immediate) cmp ax, imm instructions in a region."""
    result = []
    cursor = start
    while cursor + 3 <= stop:
        if data[cursor] == 0x3D:
            # A false 0x3d can occur in a near-jump displacement.  The real
            # compare is followed by a short conditional branch; filtering
            # here keeps the decoder bounded without pretending to disassemble
            # the whole function.
            if data[cursor + 3] in range(0x70, 0x80):
                result.append((cursor, _u16(data, cursor + 1)))
                # Skip the two-byte immediate so a literal 0x3d in the low
                # or high byte is not mistaken for another cmp opcode.
                cursor += 3
                continue
        cursor += 1
    return result


def _decode_flag_stores(data: bytes, start: int, loop_end: int) -> tuple[
    list[dict[str, int]], list[dict[str, int]], list[dict[str, int]]
]:
    """Decode ladder branches, preserving the first match for duplicate IDs."""
    store = bytes.fromhex("26 c7 45 02")
    assignments: dict[int, dict[str, int]] = {}
    ranges: list[dict[str, int]] = []
    duplicates: list[dict[str, int]] = []

    def assign(tile_id: int, item: dict[str, int]) -> None:
        """Keep the first compare-ladder match; later duplicates are unreachable."""
        prior = assignments.get(tile_id)
        if prior is None:
            assignments[tile_id] = item
        elif prior["flags"] != item["flags"]:
            duplicates.append({
                "tile_id": tile_id,
                "first_flags": prior["flags"],
                "first_store": prior["store"],
                "duplicate_flags": item["flags"],
                "duplicate_store": item["store"],
            })

    cursor = start
    while True:
        hit = data.find(store, cursor, loop_end)
        if hit < 0:
            break
        flags = _u16(data, hit + 4)
        # A generated branch has one of these forms:
        #   cmp ax, imm; jne; ... store
        #   cmp ax, low; jb; cmp ax, high; ja; ... store
        # Search only back to the previous store; this isolates one case in
        # the compiler's long compare ladder.
        previous_store = data.rfind(store, start, hit)
        # The compare for this case follows the previous case's store.  Do
        # not skip the first few bytes after that store: short branches can
        # begin immediately after the previous case's jump instruction.
        region_start = max(start, previous_store + len(store))
        compares = _compare_candidates(data, region_start, hit)
        if not compares:
            raise ValueError(f"initializer 0x{start:04x}: flag store at 0x{hit:04x} has no cmp")
        last_offset, last_value = compares[-1]
        branch = data[last_offset + 3] if last_offset + 3 < len(data) else None
        if branch == 0x75:  # jne: one exact tile ID
            assign(last_value, {"tile_id": last_value, "flags": flags,
                                "store": hit, "match": "exact"})
        elif branch in (0x77, 0x76, 0x7C):  # ja/jbe/jl used for upper range
            lower = None
            if len(compares) >= 2:
                prior_offset, prior_value = compares[-2]
                prior_branch = data[prior_offset + 3]
                if prior_branch in (0x72, 0x76, 0x7C):
                    lower = prior_value
            if lower is None:
                raise ValueError(
                    f"initializer 0x{start:04x}: range store at 0x{hit:04x} has no lower bound"
                )
            for tile_id in range(lower, last_value + 1):
                assign(tile_id, {
                    "tile_id": tile_id, "flags": flags, "store": hit,
                    "match": "range",
                })
            ranges.append({"first": lower, "last": last_value, "flags": flags,
                           "store": hit})
        else:
            raise ValueError(
                f"initializer 0x{start:04x}: unsupported branch 0x{branch:02x} "
                f"before store at 0x{hit:04x}"
            )
        cursor = hit + len(store) + 6

    records = []
    for tile_id in range(TABLE_ENTRIES):
        item = assignments.get(tile_id, {"tile_id": tile_id, "flags": 0,
                                         "store": None, "match": "default"})
        records.append({"tile_id": tile_id, "record_offset": tile_id * TABLE_STRIDE,
                        "tile_index": tile_id, "flags": item["flags"],
                        "source_store": item["store"], "match": item["match"]})
    return records, ranges, duplicates


def decode_initializer(data: bytes, world: str) -> dict[str, Any]:
    start, end = INITIALIZERS[world]
    loop = _find_init_loop(data, start, end)
    records, ranges, duplicates = _decode_flag_stores(data, start, loop["loop_terminator"])
    return {
        "world": world,
        "function": {"start": start, "end": end},
        "table": {
            "global": "DS:6582",
            "stride_global": "DS:30D4",
            "allocation_bytes": 0x800,
            "entry_count": TABLE_ENTRIES,
            "record_size": TABLE_STRIDE,
            "fields": [
                {"offset": 0, "size": 2, "name": "tile_index",
                 "producer": "loop writes tile_id",
                 "consumers": ["01F7:20C8", "01F7:2CB2"],
                 "meaning": "renderer resource index; shifted left 8 for the 0x100-byte tile image block"},
                {"offset": 2, "size": 2, "name": "flags",
                 "producer": "world-specific compare ladder",
                 "consumers": ["01F7:5C27", "01F7:5CC3"],
                 "meaning": "low-nibble occupancy and 3D02 collision flags"},
            ],
        },
        "loop": loop,
        "ranges": ranges,
        "duplicate_cases": duplicates,
        "records": records,
        "flag_histogram": dict(sorted(Counter(r["flags"] for r in records).items())),
    }


def build_report(executable: Path) -> dict[str, Any]:
    data = executable.read_bytes()
    # Segment 1 is file-backed at 0x0f00 in this NE image.  Keep this derived
    # from the documented header instead of hard-coding the executable offset.
    ne = struct.unpack_from("<I", data, 0x3C)[0]
    shift = _u16(data, ne + 0x32)
    segment_table = ne + 0x40
    sector_offset, length, _flags, _alloc = struct.unpack_from(
        "<HHHH", data, segment_table
    )
    segment = data[sector_offset << shift : (sector_offset << shift) + length]
    return {
        "schema": "quiky-runtime-descriptor-static-v1",
        "executable": str(executable),
        "executable_sha256": hashlib.sha256(data).hexdigest(),
        "segment1_file_offset": sector_offset << shift,
        "segment1_length": length,
        "mapping": {
            "tile_id_mask": "0x01ff",
            "address_formula": "DS:6582 + (cell & 0x01ff) * DS:30D4",
            "descriptor_word_offset": 2,
            "confirmed_runtime_stride": 4,
        },
        "flag_consumers": {
            "low_nibble_query": {
                "entry": "01F7:5C27",
                "descriptor_read": "record + 2 -> DX",
                "direct_relocation_call_count": 78,
                "semantics": "tests descriptor & 0x000f and returns quadrant result in flags",
                "quadrant_bit_map": {
                    "AX_bit3=1,BX_bit3=1": "descriptor bit 0x0002",
                    "AX_bit3=1,BX_bit3=0": "descriptor bit 0x0001",
                    "AX_bit3=0,BX_bit3=1": "descriptor bit 0x0004",
                    "AX_bit3=0,BX_bit3=0": "descriptor bit 0x0008",
                },
                "coordinate_bits": "AX is the 16-pixel row coordinate; BX selects the 8-pixel-aligned cell and its bit 3 chooses the subquadrant",
            },
            "full_word_query": {
                "entry": "01F7:5CC3",
                "descriptor_read": "record + 2 -> DX",
                "direct_callers": ["01F7:3D19", "01F7:3D31"],
                "semantics": "3D02 consumes DX & 0x30, 0x20, and 0x40; 5C27 also leaves the same descriptor word in DX for transition callers",
                "transition_dx_consumers": [
                    "01F7:447B", "01F7:448C", "01F7:44A0", "01F7:44B1"
                ],
            },
            "unresolved_bits": {
                "bit_10": "suppresses 3D02's y-minus-8 retry; no standalone gameplay name proven",
                "bit_20": "suppresses that retry and selects vertical-response polarity/state",
                "bit_40": "selects the eight-pixel Y alignment path",
            },
            "collision_state_latch": {
                "field": "player object +0x3a",
                "producer": "01F7:3D02 sets 0x01 or 0xff from descriptor bit 0x20; clears it on reject",
                "consumer": "01F7:3DF2 tests it only as zero/nonzero before the integer-Y snap",
                "semantics": "transient accepted vertical-response branch latch, not a persistent surface type",
            },
        },
        "map_read_audit": {
            "identified_readers": [
                {
                    "entry": "01F7:3376",
                    "routine": "map_tile_id_lookup_16px",
                    "operation": "raw MAP word & 0x01FF",
                    "role": "returns the tile ID",
                },
                {
                    "entry": "01F7:5C27",
                    "routine": "map_tile_descriptor_query_5C27",
                    "operation": "raw MAP word & 0x01FF",
                    "role": "indexes the descriptor table and tests its low nibble",
                },
                {
                    "entry": "01F7:5CC3",
                    "routine": "map_tile_descriptor_query_5CC3",
                    "operation": "raw MAP word & 0x01FF",
                    "role": "indexes the descriptor table and returns record +2",
                },
                {
                    "entry": "01F7:20C8",
                    "routine": "render_map_column",
                    "operation": "MAP word & 0x01FF",
                    "role": "indexes the descriptor +0 tile_index, then the 0x100-byte image block",
                },
                {
                    "entry": "01F7:2CB2",
                    "routine": "render_map_strip",
                    "operation": "MAP word & 0x01FF",
                    "role": "indexes the descriptor +0 tile_index, then the 0x100-byte image block",
                },
            ],
            "upper_bits": {
                "status": "no identified gameplay reader interprets MAP bits 9..15",
                "evidence": "the raw-segment MAP-pointer reference/decompiler audit finds masks in all five readers; writers preserve or replace the field but do not consume it",
                "caveat": "this is a static audit boundary, not proof against an indirect/runtime-generated reader",
            },
        },
        "map_writer_call_form_audit": {
            "source": "research/tools/AuditQuikyCallForms.java",
            "segment": "01F7",
            "indirect_call_forms": [
                {"entry": "01F7:040F", "operand": "[BP + -0x12]"},
                {"entry": "01F7:0598", "operand": "[BP + -0x12]"},
            ],
            "target_entries": ["01F7:16CE", "01F7:339A", "01F7:340A", "01F7:5C9D"],
            "target_writer_indirect_hits": [],
            "interpretation": "The analyzed segment-3 instruction listing has two indirect CALL forms, both through a local BP stack slot; neither resolves to a MAP writer. The 16CE calls remain the 23 NE-relocated far calls, while 339A, 340A, and 5C9D have no static call form.",
        },
        "construction": {
            "static_decomp": {
                "source": "/home/joao/dev/quiky-ghidra-decomp-descriptor-20260826-b/QUIKY_SEG02.bin.c",
                "entry": "01E7:382B",
                "operations": [
                    "allocates 0x800 bytes and publishes the far pointer as DS:6582:DS:6584",
                    "zero-fills the published 0x800-byte descriptor table",
                    "initializes DS:8952 and DS:8954 to zero before resource setup",
                ],
                "separate_buffer": "allocates another 0x800-byte buffer at DS:6D86:DS:6D88; this is not the descriptor table",
            },
            "table_allocation": {
                "routine": "01E7:382B",
                "allocation_call": "01E7:3874",
                "allocation_bytes": 0x800,
                "publish_base": "01E7:387C -> DS:6582",
                "publish_selector": "01E7:387F -> DS:6584",
                "zero_fill": "01E7:3883..01E7:388E",
            },
            "world_dispatch": {
                "routine": "01D7:3808",
                "selector_global": "DS:85D8",
                "world_initializers": {
                    "1": "01D7:1734",
                    "2": "01D7:19E4",
                    "3": "01D7:1BF1",
                    "4": "01D7:28ED",
                    "5": "01D7:2D9F",
                },
                "static_source": "/home/joao/dev/quiky-ghidra-decomp-descriptor1-20260826-b/QUIKY_SEG01.bin.c",
                "post_initializer": "zero-fills a separate 0x800-byte DS:6D86:DS:6D88 buffer",
            },
        },
        "loader_callsite_decomp": {
            "primary": {
                "entry": "01D7:365B",
                "callsite": "01D7:4009",
                "role": "initial level MAP construction before level-specific ICO/BOB asset setup",
            },
            "secondary": {
                "entry": "01D7:3861",
                "scheduler": "01D7:48B5",
                "gate": "4BA4 requires DS:89EA != 0 and DS:880A > 0; 4BD8 repeats DS:880A > 0",
                "selectors": ["0x0002", "0x0005", "0x0008", "0x000b", "0x000e"],
                "call_sites": ["01D7:4BF1", "01D7:4BFB", "01D7:4C05", "01D7:4C0F", "01D7:4C19"],
                "role": "third-level W1L3/W2L3/W3L3/W4L3/W5L3 transition/reload path",
            },
            "static_source": "/home/joao/dev/quiky-ghidra-decomp-transition-20260825-d/QUIKY_SEG01.bin.c",
        },
        "scheduler_gate_decomp": {
            "global": "DS:89EA",
            "setters": [
                {"entry": "01F7:199D", "instruction": "01F7:19A3", "value": "0xffff",
                 "role": "boundary/death reset; decrements DS:880A"},
                {"entry": "01F7:19E6", "instruction": "01F7:1A3D", "value": "0xffff",
                 "role": "overlap reset after DS:8822 reaches zero; decrements DS:880A"},
            ],
            "clearer": {"entry": "01F7:1AE6", "value": 0,
                        "role": "player initialization/state reset"},
            "direct_callers": {
                "01F7:199D": ["01F7:43D0"],
                "01F7:19E6": ["01F7:1BC4", "01F7:3AB3"],
            },
            "static_source": "/home/joao/dev/quiky-ghidra-decomp-gate-20260825-a/QUIKY_SEG03.bin.c",
        },
        "initializers": {
            world: decode_initializer(segment, world)
            for world in INITIALIZERS
        },
        "map_mutations": [
            {
                "routine": "load_map_resource_primary",
                "entry": "01D7:365B",
                "copy_writes": ["01D7:376A", "01D7:377F"],
                "mutation_read": "01D7:37AD",
                "mutation": "OR byte [MAP + 2*cell + 1], 0x10",
                "mutation_or_instruction": "01D7:37B0",
                "mutation_write": "01D7:37C0",
                "coverage": "first row; DS:657E / 2 cells",
                "word_delta": "0x1000 when bit was clear",
            },
            {
                "routine": "load_map_resource_secondary",
                "entry": "01D7:3861",
                "copy_writes": ["01D7:3901", "01D7:3917"],
                "mutation_read": "01D7:394C",
                "mutation": "OR byte [MAP + 2*cell + 1], 0x10",
                "mutation_or_instruction": "01D7:394F",
                "mutation_write": "01D7:3960",
                "coverage": "first row; DS:657E / 2 cells",
                "word_delta": "0x1000 when bit was clear",
            },
            {
                "routine": "map_effect_tile_rewrite",
                "entry": "01F7:16CE",
                "writes": ["01F7:1706", "01F7:170E"],
                "mutation": "MAP word = (MAP word & 0xFE00) | (DX & 0x01FF)",
                "guard": "skips the MAP write when the incoming DX has bit 0x8000",
                "coverage": "one coordinate-selected cell",
                "direct_relocation_call_count": 23,
                "direct_callers": [
                    "01F7:1892", "01F7:1944", "01F7:6359",
                    "01F7:8EE5", "01F7:8F11", "01F7:8F3D", "01F7:8F69",
                    "01F7:8F95", "01F7:8FC4", "01F7:8FF0", "01F7:901C",
                    "01F7:9048", "01F7:9074", "01F7:90A3", "01F7:90CF",
                    "01F7:90FB", "01F7:9127", "01F7:9153", "01F7:9182",
                    "01F7:91AE", "01F7:91DA", "01F7:9206", "01F7:9232",
                ],
                "status": "static callers include two transient-event paths, one short animation path, and the 8E4B tile-effect state machine",
            },
            {
                "routine": "map_low_id_normalizer",
                "entry": "01F7:33BF",
                "writes": ["01F7:33FA", "01F7:33FF"],
                "mutation": "for IDs 2..4, MAP word = (MAP word & 0xFE00) | ID",
                "coverage": "all DS:657E / 2 * DS:6580 cells",
                "direct_callers": ["01D7:37CB", "01D7:396D"],
                "status": "idempotent for a well-formed 16-bit MAP word; runs after both loaders",
            },
            {
                "routine": "map_low_id_writer",
                "entry": "01F7:339A",
                "writes": ["01F7:33B4", "01F7:33B9"],
                "mutation": "MAP word = (MAP word & 0xFE00) | CX (caller supplies low-ID bits)",
                "coverage": "one coordinate-selected cell",
                "direct_relocation_call_count": 0,
                "relocation_record_count": 0,
                "embedded_target_offset_occurrences": 0,
                "status": "file-backed entry; no NE relocation or literal target-offset reference found",
            },
            {
                "routine": "map_property_writer",
                "entry": "01F7:340A",
                "writes": ["01F7:3424", "01F7:3429"],
                "mutation": "MAP word = (MAP word & 0x01FF) | CX (caller supplies upper-property bits)",
                "coverage": "one coordinate-selected cell",
                "direct_relocation_call_count": 0,
                "relocation_record_count": 0,
                "embedded_target_offset_occurrences": 0,
                "status": "file-backed entry; no NE relocation or literal target-offset reference found",
            },
            {
                "routine": "map_cell_word_store_candidate",
                "entry": "01F7:5C9D",
                "write": "FS:[MAP + (y >> 4) * DS:657E + ((x >> 3) & 0xFFFE)] = CX",
                "direct_relocation_call_count": 0,
                "relocation_record_count": 0,
                "embedded_target_offset_occurrences": 0,
                "status": "controlled runtime write confirmed; no NE relocation or literal target-offset reference found; normal gameplay caller remains open",
            },
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = build_report(args.executable)
    rendered = json.dumps(report, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
