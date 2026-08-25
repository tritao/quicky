#!/usr/bin/env python3
"""Mechanically audit the player callback closure ledger and exports."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import tempfile
from pathlib import Path
from typing import Any

from ghidra_ne_segments import read_segments
from ne_relocs import read_relocations

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "research/ghidra/player-callback-closure.json"
DECOMP = ROOT / "research/notes/player-static-decomp.cpp"


class ClosureError(Exception):
    pass


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def address(value: str) -> tuple[int, int]:
    selector, offset = value.split(":", 1)
    selectors = {"01D7": 1, "01E7": 2, "01F7": 3, "0207": 4,
                 "0227": 5, "0237": 6}
    selector = selector.upper()
    try:
        segment = int(selector, 10) if selector.isdigit() and len(selector) <= 2 else selectors[selector]
        return segment, int(offset, 16)
    except (KeyError, ValueError) as exc:
        raise ClosureError(f"invalid address {value}") from exc


def load(path: Path) -> dict[str, Any]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ClosureError(f"cannot read {path}: {exc}") from exc
    if payload.get("schema") != "quiky.player-callback-closure.v2":
        raise ClosureError("unsupported player callback closure schema")
    return payload


def check_hashes(payload: dict[str, Any], root: Path) -> None:
    source = payload["source"]
    executable = root / source["executable"]
    segment = root / source["segment_image"]
    if sha256(executable) != source["sha256"]:
        raise ClosureError(f"executable hash mismatch: {executable}")
    if sha256(segment) != source["segment_sha256"]:
        raise ClosureError(f"segment hash mismatch: {segment}")


def check_model(payload: dict[str, Any]) -> set[str]:
    fields = payload["typed_model"]["player_record"]
    offsets: set[str] = set()
    for field in fields:
        offset = field["offset"].upper()
        if offset in offsets:
            raise ClosureError(f"duplicate player field offset {offset}")
        offsets.add(offset)
        if not field.get("name") or not field.get("type"):
            raise ClosureError(f"incomplete typed field {offset}")
    globals_seen: set[str] = set()
    for item in payload["typed_model"]["globals"]:
        if item["address"] in globals_seen:
            raise ClosureError(f"duplicate global {item['address']}")
        globals_seen.add(item["address"])
        if not item.get("name") or not item.get("type"):
            raise ClosureError(f"incomplete global {item['address']}")
    return offsets


def check_functions(payload: dict[str, Any], segment_size: int,
                    field_offsets: set[str]) -> dict[tuple[int, int], dict[str, Any]]:
    allowed = {"inline", "contract", "irrelevant", "unresolved"}
    confidence = {"confirmed", "mechanical", "provisional", "unresolved"}
    functions: dict[tuple[int, int], dict[str, Any]] = {}
    names: set[str] = set()
    for item in payload["functions"]:
        identity = address(item["address"])
        if identity in functions:
            raise ClosureError(f"duplicate closure function {item['address']}")
        functions[identity] = item
        if item["name"] in names:
            raise ClosureError(f"duplicate closure name {item['name']}")
        names.add(item["name"])
        if item.get("classification") not in allowed:
            raise ClosureError(f"unclassified function {item['address']}")
        if item.get("confidence") not in confidence:
            raise ClosureError(f"invalid confidence for {item['name']}")
        required = ("old_symbol", "signature", "inputs", "outputs", "evidence",
                    "player_reads", "player_writes", "global_reads", "global_writes",
                    "callees")
        for key in required:
            if key not in item:
                raise ClosureError(f"{item['name']} has no {key}")
        if not item["old_symbol"] or not item["signature"] or not item["inputs"] or not item["outputs"] or not item["evidence"]:
            raise ClosureError(f"{item['name']} has incomplete contract metadata")
        for offset in item["player_reads"] + item["player_writes"]:
            if offset.upper() not in field_offsets:
                raise ClosureError(f"{item['name']} accesses untyped player offset {offset}")
        if item.get("range") is not None:
            start, end = (int(value, 16) for value in item["range"])
            if identity[0] == 3 and not (0 <= start < end <= segment_size):
                raise ClosureError(f"invalid range for {item['name']}")
    return functions


def check_edges(payload: dict[str, Any], functions: dict[tuple[int, int], dict[str, Any]]) -> None:
    for item in payload["functions"]:
        for edge in item["callees"]:
            target = address(edge["address"])
            if target not in functions:
                raise ClosureError(
                    f"{item['name']} callee {edge['address']} has no classified contract"
                )
            if edge.get("classification") not in {"inline", "contract", "irrelevant", "unresolved"}:
                raise ClosureError(f"{item['name']} callee {edge['address']} is unclassified")
            if not edge.get("flags"):
                raise ClosureError(f"{item['name']} callee {edge['address']} has no flag contract")


def check_c_source(field_offsets: set[str], source_path: Path) -> None:
    source = source_path.read_text(encoding="utf-8")
    pattern = re.compile(r"->(u8|u16|i16|i32)\(0x([0-9a-fA-F]+)")
    for kind, raw_offset in pattern.findall(source):
        offset = raw_offset.upper()
        if offset not in field_offsets:
            raise ClosureError(f"C-like source accesses untyped player offset 0x{offset}")
        if kind == "i32" and offset not in {"02", "06", "0A", "0E", "44", "48", "4C", "50", "54", "58", "5C", "60", "64"}:
            raise ClosureError(f"i32 access has no typed fixed-point field at 0x{offset}")


def check_callgraph(payload: dict[str, Any], path: Path,
                    functions: dict[tuple[int, int], dict[str, Any]]) -> None:
    graph = json.loads(path.read_text(encoding="utf-8"))
    known = set(functions)
    expected: set[tuple[tuple[int, int], tuple[int, int], tuple[int, int]]] = set()
    for item in payload["functions"]:
        source = address(item["address"])
        for callee in item["callees"]:
            target = address(callee["address"])
            for raw_site in callee["site"].split(","):
                expected.add((source, (source[0], int(raw_site, 16)), target))

    actual: set[tuple[tuple[int, int], tuple[int, int], tuple[int, int]]] = set()
    for edge in graph.get("edges", []):
        source = address(edge["source"])
        call_site = address(edge["call_site"])
        target = address(edge["target"])
        if source not in known:
            raise ClosureError(f"exported call edge source {edge['source']} is outside closure")
        if target not in known:
            raise ClosureError(
                f"exported call edge {edge['source']} -> {edge['target']} lacks a contract"
            )
        actual.add((source, call_site, target))
    if actual != expected:
        missing = sorted(expected - actual)
        extra = sorted(actual - expected)
        raise ClosureError(f"call graph drift; missing={missing[:3]} extra={extra[:3]}")


def segment_bytes(executable: Path, number: int) -> bytes:
    blob = executable.read_bytes()
    try:
        segment = read_segments(executable)[number - 1]
    except IndexError as exc:
        raise ClosureError(f"executable has no segment {number}") from exc
    start = int(segment["file_offset"])
    length = int(segment["file_length"])
    return blob[start:start + length]


def expected_call_edges(payload: dict[str, Any]) -> list[tuple[tuple[int, int], int, tuple[int, int], str]]:
    edges: list[tuple[tuple[int, int], int, tuple[int, int], str]] = []
    for item in payload["functions"]:
        source = address(item["address"])
        if item.get("range") is None:
            continue
        for callee in item["callees"]:
            target = address(callee["address"])
            for raw_site in callee["site"].split(","):
                edges.append((source, int(raw_site, 16), target, callee["name"]))
    return edges


def classify_expected_call(executable: Path,
                           source: tuple[int, int], site: int,
                           target: tuple[int, int]) -> str:
    """Classify a ledger call from raw bytes and the independent NE ledger.

    Historical ledger rows use three nearby conventions for far-call sites:
    the opcode, its relocation operand, and one byte before the opcode.  The
    relocation record remains the authority; the small window only normalizes
    that representation and never supplies a target itself.
    """
    code = segment_bytes(executable, source[0])
    relocations = read_relocations(executable)
    for record in relocations:
        if record["segment"] != source[0] or record["source_type"] != 0x03:
            continue
        if record["target_segment"] != target[0] or record["target_offset"] != target[1]:
            continue
        instruction = record["instruction"]
        relocation_source = record["source"]
        if instruction is None:
            continue
        if min(abs(instruction - site), abs(relocation_source - site)) <= 1:
            return "far"

    if source[0] == target[0] and 0 <= site + 2 < len(code) and code[site] == 0xE8:
        displacement = int.from_bytes(code[site + 1:site + 3], "little", signed=True)
        if ((site + 3 + displacement) & 0xFFFF) == target[1]:
            return "near"
    return "unresolved"


def check_independent_callgraph(payload: dict[str, Any], executable: Path,
                                path: Path) -> None:
    """Check Ghidra near edges while recovering far edges from NE relocations."""
    graph = json.loads(path.read_text(encoding="utf-8"))
    if graph.get("schema") != "quiky.player-callback-ghidra-callgraph.v2":
        raise ClosureError("unsupported Ghidra-derived call graph schema")

    functions = {address(item["address"]): item for item in payload["functions"]}
    expected_near: set[tuple[tuple[int, int], tuple[int, int], tuple[int, int]]] = set()
    expected_far = 0
    expected_unresolved: list[str] = []
    for source, site, target, name in expected_call_edges(payload):
        kind = classify_expected_call(executable, source, site, target)
        if kind == "near":
            expected_near.add((source, (source[0], site), target))
        elif kind == "far":
            expected_far += 1
        else:
            expected_unresolved.append(f"{source[0]}:{site:04X} -> {target[0]}:{target[1]:04X} ({name})")
    if expected_unresolved:
        raise ClosureError("ledger call is neither a direct near call nor an NE far relocation: " +
                           ", ".join(expected_unresolved[:3]))

    actual_near: set[tuple[tuple[int, int], tuple[int, int], tuple[int, int]]] = set()
    for edge in graph.get("edges", []):
        if edge.get("call_kind") != "near":
            continue
        source = address(edge["source"])
        call_site = address(edge["call_site"])
        target = address(edge["target"])
        if source not in functions:
            raise ClosureError(f"Ghidra call source {edge['source']} is outside closure")
        actual_near.add((source, call_site, target))
    if actual_near != expected_near:
        missing = sorted(expected_near - actual_near)
        extra = sorted(actual_near - expected_near)
        raise ClosureError(f"Ghidra near-call drift; missing={missing[:3]} extra={extra[:3]}")

    for call in graph.get("unresolved_calls", []):
        if call.get("classification") not in {"far-or-indirect", "near"}:
            raise ClosureError("Ghidra unresolved call has invalid classification")
        if address(call["source"]) not in functions:
            raise ClosureError(f"unresolved call source {call['source']} is outside closure")
    print(f"OK: Ghidra near-call graph ({len(actual_near)} edges)")
    print(f"OK: NE relocation far-call contract ({expected_far} edges)")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=MANIFEST)
    parser.add_argument("--segment", type=Path,
                        default=ROOT / "research/build/player-callback-baseline/segments/QUIKY_SEG03.bin")
    parser.add_argument("--decomp", type=Path, default=DECOMP)
    parser.add_argument("--executable", type=Path, default=ROOT / "game/QUIKY.EXE")
    parser.add_argument("--callgraph", type=Path)
    parser.add_argument("--ghidra-callgraph", type=Path)
    args = parser.parse_args()
    try:
        payload = load(args.manifest)
        check_hashes(payload, ROOT)
        fields = check_model(payload)
        functions = check_functions(payload, args.segment.stat().st_size, fields)
        check_edges(payload, functions)
        check_c_source(fields, args.decomp)
        if args.callgraph:
            check_callgraph(payload, args.callgraph, functions)
        if args.ghidra_callgraph:
            check_independent_callgraph(payload, args.executable, args.ghidra_callgraph)
    except (ClosureError, OSError, json.JSONDecodeError) as exc:
        print(f"FAIL: {exc}")
        return 1
    print(f"OK: {len(functions)} classified closure functions")
    print(f"OK: {len(fields)} typed player offsets")
    print(f"OK: {len(payload['address_labels'])} address-qualified data labels")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
