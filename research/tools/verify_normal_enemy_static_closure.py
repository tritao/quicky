#!/usr/bin/env python3
"""Verify the focused Ghidra export contract for normal enemy callbacks."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
LEDGER = ROOT / "research/ghidra/normal-enemy-static-closure.json"
NOTE = ROOT / "research/notes/normal-enemy-static-decomp.cpp"


class NormalEnemyStaticClosureError(Exception):
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
        raise NormalEnemyStaticClosureError(f"cannot read {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise NormalEnemyStaticClosureError("ledger must be an object")
    if value.get("schema") != "quiky.normal-enemy-static-closure.v1":
        raise NormalEnemyStaticClosureError("unexpected ledger schema")
    return value


def check_source(ledger: dict[str, Any], root: Path = ROOT) -> None:
    source = ledger.get("source")
    if not isinstance(source, dict):
        raise NormalEnemyStaticClosureError("source must be an object")
    for key, hash_key in (
        ("executable", "executable_sha256"),
        ("segment3", "segment3_sha256"),
    ):
        path = root / source[key]
        if not path.is_file():
            raise NormalEnemyStaticClosureError(f"missing source: {path}")
        if sha256(path) != source[hash_key]:
            raise NormalEnemyStaticClosureError(f"source hash drift: {path}")
    if source.get("ghidra_language") != "x86:LE:16:Protected Mode":
        raise NormalEnemyStaticClosureError("wrong Ghidra language")
    if "objdump" in source.get("pipeline", "").lower():
        raise NormalEnemyStaticClosureError("objdump is not an allowed source")


def check_contracts(ledger: dict[str, Any], root: Path = ROOT) -> None:
    expected = {
        "pengo": ("70D9", "715E"),
        "krabbe": ("771D", "778C"),
        "fisch": ("7AE3", "7B71"),
        "schnee": ("6651", "66E1"),
        "fliege": ("7E78", "7EF8"),
        "spinne": ("840D", "8472"),
        "buggy": ("500C", "5071"),
        "ufo_enemy": ("5EAC", "5F28"),
    }
    families = ledger.get("families")
    if not isinstance(families, list):
        raise NormalEnemyStaticClosureError("families must be an array")
    by_id: dict[str, dict[str, Any]] = {}
    for family in families:
        if not isinstance(family, dict):
            raise NormalEnemyStaticClosureError("family must be an object")
        family_id = family.get("id")
        if family_id in by_id:
            raise NormalEnemyStaticClosureError(f"duplicate family {family_id}")
        by_id[family_id] = family
        if family_id not in expected:
            raise NormalEnemyStaticClosureError(f"unexpected family {family_id}")
        initializer = family.get("initializer")
        callback = family.get("callback")
        if not isinstance(initializer, dict) or not isinstance(callback, dict):
            raise NormalEnemyStaticClosureError(f"{family_id}: missing initializer/callback")
        if initializer.get("address", "").split(":")[-1] != expected[family_id][0]:
            raise NormalEnemyStaticClosureError(f"{family_id}: initializer address drift")
        if callback.get("address", "").split(":")[-1].split("-")[0] != expected[family_id][1]:
            raise NormalEnemyStaticClosureError(f"{family_id}: callback address drift")
        for label, contract in (("initializer", initializer), ("callback", callback)):
            required_fields = ("address", "name", "player_writes", "global_writes", "callees", "evidence")
            if label == "initializer":
                required_fields += ("object_writes",)
            for field in required_fields:
                if field not in contract:
                    raise NormalEnemyStaticClosureError(f"{family_id}.{label} lacks {field}")
            list_fields = ("player_writes", "global_writes", "callees", "evidence")
            if label == "initializer":
                list_fields += ("object_writes",)
            for field in list_fields:
                if not isinstance(contract[field], list):
                    raise NormalEnemyStaticClosureError(f"{family_id}.{label}.{field} must be an array")
            if not contract["evidence"]:
                raise NormalEnemyStaticClosureError(f"{family_id}.{label} has no evidence")
            if contract["player_writes"]:
                raise NormalEnemyStaticClosureError(f"{family_id}.{label} has an unproven player write")
    if set(by_id) != set(expected):
        raise NormalEnemyStaticClosureError("normal-enemy family set drifted")

    common = ledger.get("common_contract")
    if not isinstance(common, dict) or not common.get("shared_edges"):
        raise NormalEnemyStaticClosureError("shared contracts are missing")
    shared = {item.get("address"): item for item in common["shared_edges"]}
    for address in ("01F7:1DCA", "01F7:1DEE", "01F7:1B77", "01F7:1C4D/1C6E"):
        if address not in shared:
            raise NormalEnemyStaticClosureError(f"missing shared edge {address}")
    if any(shared[address].get("player_writes") for address in shared):
        raise NormalEnemyStaticClosureError("shared contract gained a player write")

    tails = ledger.get("contact_tails")
    if not isinstance(tails, list) or not tails:
        raise NormalEnemyStaticClosureError("contact tails are missing")
    for tail in tails:
        if not isinstance(tail, dict) or not tail.get("evidence"):
            raise NormalEnemyStaticClosureError("invalid contact-tail contract")
        if tail.get("player_writes") != []:
            raise NormalEnemyStaticClosureError(f"{tail.get('address')} has a player write")
    required_tails = {"01F7:70C9", "01F7:6D4F", "01F7:53E7/5911", "01F7:6318", "01F7:83FD/87C1", "01F7:7AD3"}
    if {tail.get("address") for tail in tails} != required_tails:
        raise NormalEnemyStaticClosureError("contact-tail set drifted")

    if not NOTE.is_file():
        raise NormalEnemyStaticClosureError(f"missing static note: {NOTE}")
    note_hash = ledger.get("note_sha256")
    if note_hash and sha256(NOTE) != note_hash:
        raise NormalEnemyStaticClosureError("static note hash drift")


def check_runner(ledger: dict[str, Any], root: Path = ROOT) -> None:
    runner_path = root / ledger["export"]["runner"]
    if not runner_path.is_file():
        raise NormalEnemyStaticClosureError(f"missing runner: {runner_path}")
    runner = runner_path.read_text(encoding="utf-8")
    for token in (
        "analyzeHeadless",
        "x86:LE:16:Protected Mode",
        '"70D9"', '"715E"', '"771D"', '"778C"',
        '"7AE3"', '"7B71"', '"6651"', '"66E1"',
        '"7E78"', '"7EF8"', '"840D"', '"8472"',
        '"500C"', '"5071"', '"5EAC"', '"5F28"',
        '("70D9", 640)', '("715E", 640)', '("771D", 640)',
        '("778C", 640)', '("7AE3", 640)', '("7B71", 640)',
        '("6651", 560)', '("66E1", 560)', '("7E78", 640)',
        '("7EF8", 640)', '("840D", 640)', '("8472", 640)',
        '("500C", 640)', '("5071", 640)', '("5EAC", 640)',
        '("5F28", 640)', '("70C9", 48)', '("7AD3", 48)',
    ):
        if token not in runner:
            raise NormalEnemyStaticClosureError(f"runner missing {token}")
    if "objdump" in runner.lower():
        raise NormalEnemyStaticClosureError("runner must not use objdump")


def check_export(export: Path) -> None:
    paths = (
        export / "decomp-a/QUIKY_SEG03.bin.c",
        export / "decomp-b/QUIKY_SEG03.bin.c",
        export / "disasm-seg3-a/QUIKY_SEG03.bin.asm",
        export / "disasm-seg3-b/QUIKY_SEG03.bin.asm",
    )
    if any(not path.is_file() for path in paths):
        missing = [str(path) for path in paths if not path.is_file()]
        raise NormalEnemyStaticClosureError("missing generated export: " + ", ".join(missing))
    if paths[0].read_bytes() != paths[1].read_bytes():
        raise NormalEnemyStaticClosureError("A/B decompilation drifted")
    if paths[2].read_bytes() != paths[3].read_bytes():
        raise NormalEnemyStaticClosureError("A/B instruction listing drifted")
    listing = paths[2].read_text(encoding="utf-8")
    for address in ("70D9", "715E", "771D", "778C", "7AE3", "7B71", "6651", "66E1", "7E78", "7EF8", "840D", "8472", "500C", "5071", "5EAC", "5F28", "70C9", "7AD3"):
        if f"; ---- {address} " not in listing:
            raise NormalEnemyStaticClosureError(f"listing missing {address}")


def verify(ledger_path: Path = LEDGER, root: Path = ROOT, export: Path | None = None) -> None:
    ledger = load(ledger_path)
    check_source(ledger, root)
    check_contracts(ledger, root)
    check_runner(ledger, root)
    if export is not None:
        check_export(export)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ledger", type=Path, default=LEDGER)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--export", type=Path)
    args = parser.parse_args()
    verify(args.ledger, args.root, args.export)
    print("OK: focused normal-enemy Ghidra closure contracts and runner verified")
    if args.export:
        print(f"OK: independent export matched at {args.export}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
