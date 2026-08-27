#!/usr/bin/env python3
"""Verify the focused protected-mode descriptor backend closure."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
LEDGER = ROOT / "research/ghidra/descriptor-backend-static-closure.json"
NOTE = ROOT / "research/notes/descriptor-backend-static-decomp.cpp"
EXPECTED_NOTE_SHA256 = "e79ddf41d31d1c8d79b1663e29edc92b40fe27a8f42aea75bd87d3c38c5cc41d"


class DescriptorClosureError(Exception):
    pass


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def load(path: Path = LEDGER) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise DescriptorClosureError(f"cannot read {path}: {exc}") from exc
    if not isinstance(value, dict) or value.get("schema") != "quiky.descriptor-backend-static-closure.v1":
        raise DescriptorClosureError("unexpected descriptor backend closure ledger")
    return value


def check_source(ledger: dict[str, Any], root: Path = ROOT) -> None:
    source = ledger.get("source")
    if not isinstance(source, dict):
        raise DescriptorClosureError("source is missing")
    executable = root / source["executable"]
    if not executable.is_file() or sha256(executable) != source["executable_sha256"]:
        raise DescriptorClosureError("executable hash drift")
    if source.get("ghidra_language") != "x86:LE:16:Protected Mode:default":
        raise DescriptorClosureError("descriptor closure must use protected-mode Ghidra")
    pipeline = source.get("pipeline", "").lower()
    if "ghidra" not in pipeline or "objdump" in pipeline:
        raise DescriptorClosureError("descriptor closure pipeline is not the repository Ghidra process")
    runner = root / source["runner"]
    if not runner.is_file():
        raise DescriptorClosureError(f"missing Ghidra runner: {runner}")
    runner_text = runner.read_text(encoding="utf-8")
    for anchor in ("DumpExternalClosure.java", "DumpFocusedDisasm.java", '"1C6E"', '"1C92"', '"5C27"', '"5CC3"', '"5DC3"'):
        if anchor not in runner_text:
            raise DescriptorClosureError(f"Ghidra runner missing target anchor: {anchor}")


def check_indexing(ledger: dict[str, Any]) -> None:
    indexing = ledger.get("shared_indexing")
    if not isinstance(indexing, dict):
        raise DescriptorClosureError("shared indexing contract is missing")
    for required in ("map_coordinates", "map_index", "map_base", "descriptor_coordinates", "descriptor_index", "descriptor_bank"):
        if not indexing.get(required):
            raise DescriptorClosureError(f"missing indexing field: {required}")
    if "X>>4" not in indexing["map_index"] or "(X>>3)&0xFFFE" not in indexing["descriptor_index"]:
        raise DescriptorClosureError("map/descriptor coordinate split drifted")
    if "0x01FF" not in indexing["descriptor_bank"] or "DS:30D4" not in indexing["descriptor_bank"]:
        raise DescriptorClosureError("descriptor bank selector contract drifted")


def check_contracts(ledger: dict[str, Any]) -> None:
    contracts = ledger.get("contracts")
    expected = {
        "01F7:1C6E": "probe_map_word_bit_4000_1C6E",
        "01F7:1C92": "probe_map_word_bit_1000_1C92",
        "01F7:5C27": "probe_descriptor_quadrant_5C27",
        "01F7:5CC3": "read_descriptor_word_5CC3",
        "01F7:5DC3": "probe_map_word_bit_0800_5DC3",
    }
    if not isinstance(contracts, list) or len(contracts) != len(expected):
        raise DescriptorClosureError("descriptor helper set is incomplete")
    seen: set[str] = set()
    for contract in contracts:
        if not isinstance(contract, dict):
            raise DescriptorClosureError("descriptor contract must be an object")
        address = contract.get("address")
        if address in seen or address not in expected:
            raise DescriptorClosureError(f"unexpected or duplicate descriptor helper: {address}")
        seen.add(address)
        if contract.get("name") != expected[address]:
            raise DescriptorClosureError(f"descriptor helper name drifted at {address}")
        for field in ("inputs", "outputs", "confidence", "evidence"):
            if field not in contract or not contract[field]:
                raise DescriptorClosureError(f"descriptor helper {address} lacks {field}")
        for field in ("player_global_reads", "player_global_writes"):
            if field not in contract:
                raise DescriptorClosureError(f"descriptor helper {address} lacks {field}")
        if contract["player_global_writes"] != []:
            raise DescriptorClosureError(f"descriptor helper {address} unexpectedly writes state")
    if seen != set(expected):
        raise DescriptorClosureError("descriptor helper set is incomplete")


def check_note(ledger: dict[str, Any], root: Path = ROOT) -> None:
    note = root / ledger["note"]["path"]
    if not note.is_file():
        raise DescriptorClosureError(f"missing static note: {note}")
    actual = sha256(note)
    expected = ledger["note"]["sha256"]
    if expected != EXPECTED_NOTE_SHA256 or actual != expected:
        raise DescriptorClosureError("descriptor static note hash is not pinned")
    text = note.read_text(encoding="utf-8")
    for anchor in (
        "probe_map_word_bit_4000_1C6E",
        "probe_map_word_bit_1000_1C92",
        "probe_descriptor_quadrant_5C27",
        "read_descriptor_word_5CC3",
        "probe_map_word_bit_0800_5DC3",
        "x>>3 rounded down",
        "selected bit clear yields ZF=1",
    ):
        if anchor not in text:
            raise DescriptorClosureError(f"static note missing anchor: {anchor}")


def verify(path: Path = LEDGER, root: Path = ROOT) -> None:
    ledger = load(path)
    check_source(ledger, root)
    check_indexing(ledger)
    check_contracts(ledger)
    check_note(ledger, root)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ledger", type=Path, default=LEDGER)
    args = parser.parse_args()
    try:
        verify(args.ledger, ROOT)
    except DescriptorClosureError as exc:
        parser.error(str(exc))
    print("OK: descriptor backend static closure")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
