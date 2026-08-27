#!/usr/bin/env python3
"""Verify the focused 01F7:0E06 caller matrix."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
LEDGER = ROOT / "research/ghidra/player-factory-static-closure.json"
NOTE = ROOT / "research/notes/player-factory-static-decomp.cpp"
EXPECTED_NOTE_SHA256 = "c658daf5805f7960a25414a0b8e7c5c7b5c97a3a4ffca90d58479a7f6de80d0e"


class FactoryClosureError(Exception):
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
        raise FactoryClosureError(f"cannot read {path}: {exc}") from exc
    if not isinstance(value, dict) or value.get("schema") != "quiky.player-factory-static-closure.v1":
        raise FactoryClosureError("unexpected player factory closure ledger")
    return value


def check_source(ledger: dict[str, Any], root: Path = ROOT) -> None:
    source = ledger.get("source")
    if not isinstance(source, dict):
        raise FactoryClosureError("source is missing")
    executable = root / source["executable"]
    if not executable.is_file() or sha256(executable) != source["executable_sha256"]:
        raise FactoryClosureError("executable hash drift")
    if source.get("ghidra_language") != "x86:LE:16:Protected Mode:default":
        raise FactoryClosureError("factory closure must use protected-mode Ghidra")
    pipeline = source.get("pipeline", "").lower()
    if "ghidra" not in pipeline or "ne_relocs.py" not in pipeline or "objdump" in pipeline:
        raise FactoryClosureError("factory closure pipeline is not the repository Ghidra process")
    runner = root / source["runner"]
    if not runner.is_file():
        raise FactoryClosureError(f"missing Ghidra runner: {runner}")
    runner_text = runner.read_text(encoding="utf-8")
    for anchor in (
        "DumpExternalClosure.java",
        "DumpFocusedDisasm.java",
        '"0E06"',
        '"0B56"',
        '"1E04"',
        '"38EC"',
        '"16CE"',
        '"6370"',
        '"648E"',
    ):
        if anchor not in runner_text:
            raise FactoryClosureError(f"Ghidra runner missing target anchor: {anchor}")


def check_allocator(ledger: dict[str, Any]) -> None:
    allocator = ledger.get("allocator")
    if not isinstance(allocator, dict) or allocator.get("address") != "01F7:0E06":
        raise FactoryClosureError("allocator contract is missing")
    if allocator.get("record_stride") != "0x78" or allocator.get("scan_count") != 64:
        raise FactoryClosureError("allocator pool geometry drifted")
    if allocator.get("callees") != ["01F7:1036"]:
        raise FactoryClosureError("allocator registration edge drifted")
    if allocator.get("return_flags") != "no caller-consumed flags proven; allocation failure enters the VGA error loop":
        raise FactoryClosureError("allocator return contract drifted")
    expected_writes = {
        "+0x18=AX", "+0x1C=0x1997", "+0x28=1", "+0x17=1",
        "+0x12=0xFFFF", "+0x1A=0xFFFF", "+0x14=0",
    }
    if set(allocator.get("common_writes", [])) != expected_writes:
        raise FactoryClosureError("allocator common writes drifted")


def relocation_instructions(root: Path) -> set[int]:
    command = [
        "python3", str(root / "research/tools/ne_relocs.py"),
        str(root / "game/QUIKY.EXE"), "--json",
    ]
    try:
        raw = subprocess.check_output(command, cwd=root, text=True)
        records = json.loads(raw)
    except (OSError, subprocess.CalledProcessError, json.JSONDecodeError) as exc:
        raise FactoryClosureError(f"cannot audit NE relocations: {exc}") from exc
    return {
        item["instruction"]
        for item in records
        if item.get("segment") == 3
        and item.get("target_segment") == 3
        and item.get("target_offset") == 0x0E06
    }


def check_calls(ledger: dict[str, Any], root: Path = ROOT) -> None:
    calls = ledger.get("reachable_factory_calls")
    if not isinstance(calls, list) or len(calls) != 6:
        raise FactoryClosureError("focused factory call matrix must contain six edges")
    expected = {
        "01F7:0B6F": ("01F7:0B56", "0x3F27", "0x0000"),
        "01F7:1717": ("01F7:16CE", "0x10B5", "0x11B2"),
        "01F7:1E89": ("01F7:1E04", "runtime: DS:81D2 + 4*type", "0x0000"),
        "01F7:390E": ("01F7:38EC", "0x4519", "0x0000"),
        "01F7:6432": ("01F7:6370", "0x6328", "0x0000"),
        "01F7:654E": ("01F7:648E", "0x6328", "0x0000"),
    }
    seen: set[str] = set()
    for call in calls:
        if not isinstance(call, dict):
            raise FactoryClosureError("factory call entry must be an object")
        address = call.get("address")
        if address in seen or address not in expected:
            raise FactoryClosureError(f"unexpected or duplicate factory edge: {address}")
        seen.add(address)
        caller, callback_ax, callback_dx = expected[address]
        if (call.get("caller"), call.get("callback_ax"), call.get("callback_dx")) != (caller, callback_ax, callback_dx):
            raise FactoryClosureError(f"factory register contract drifted at {address}")
        for field in ("post_allocation_writes", "player_global_writes", "next_callback", "simulation_feedback", "confidence", "evidence"):
            if field not in call or not call[field]:
                raise FactoryClosureError(f"factory edge {address} lacks {field}")
        if not all(isinstance(item, str) for item in call["evidence"]):
            raise FactoryClosureError(f"factory edge {address} evidence is malformed")
    if seen != set(expected):
        raise FactoryClosureError("focused factory edge set is incomplete")

    actual = relocation_instructions(root)
    required = {0x0B6F, 0x1717, 0x1E89, 0x390E, 0x6432, 0x654E}
    if not required.issubset(actual):
        missing = ", ".join(f"{value:04X}" for value in sorted(required - actual))
        raise FactoryClosureError(f"NE relocation audit missing focused 0E06 edges: {missing}")


def check_note(ledger: dict[str, Any], root: Path = ROOT) -> None:
    note = root / ledger["note"]["path"]
    if not note.is_file():
        raise FactoryClosureError(f"missing static note: {note}")
    actual = sha256(note)
    expected = ledger["note"]["sha256"]
    if expected != EXPECTED_NOTE_SHA256 or actual != expected:
        raise FactoryClosureError("player factory static note hash is not pinned")
    text = note.read_text(encoding="utf-8")
    for anchor in (
        "object_pool_factory_0E06",
        "01F7:0B6F",
        "01F7:1717",
        "01F7:1E89",
        "01F7:390E",
        "01F7:6432",
        "01F7:654E",
        "DX is preserved by the body",
        "01F7:6484",
        "01F7:648E",
    ):
        if anchor not in text:
            raise FactoryClosureError(f"static note missing anchor: {anchor}")


def verify(path: Path = LEDGER, root: Path = ROOT) -> None:
    ledger = load(path)
    check_source(ledger, root)
    check_allocator(ledger)
    check_calls(ledger, root)
    check_note(ledger, root)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ledger", type=Path, default=LEDGER)
    args = parser.parse_args()
    try:
        verify(args.ledger, ROOT)
    except FactoryClosureError as exc:
        parser.error(str(exc))
    print("OK: player 0E06 factory caller closure")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
