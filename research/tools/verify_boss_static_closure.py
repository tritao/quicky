#!/usr/bin/env python3
"""Verify the bounded Ghidra pooled-boss closure and its retained export."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
LEDGER = ROOT / "research/ghidra/boss-static-closure.json"
NOTE = ROOT / "research/notes/boss-static-decomp.cpp"


class BossStaticClosureError(Exception):
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
        raise BossStaticClosureError(f"cannot read {path}: {exc}") from exc
    if not isinstance(value, dict) or value.get("schema") != "quiky.boss-static-closure.v1":
        raise BossStaticClosureError("unexpected boss closure ledger")
    return value


def check_source(ledger: dict[str, Any], root: Path = ROOT) -> None:
    source = ledger.get("source")
    if not isinstance(source, dict):
        raise BossStaticClosureError("source must be an object")
    for path_key, hash_key in (("executable", "executable_sha256"), ("segment1", "segment1_sha256"), ("segment3", "segment3_sha256")):
        path = root / source[path_key]
        if not path.is_file():
            raise BossStaticClosureError(f"missing source: {path}")
        if sha256(path) != source[hash_key]:
            raise BossStaticClosureError(f"source hash drift: {path}")
    if source.get("ghidra_language") != "x86:LE:16:Protected Mode":
        raise BossStaticClosureError("wrong Ghidra language")
    if "objdump" in source.get("pipeline", "").lower():
        raise BossStaticClosureError("objdump is not an allowed source")


def check_contract(contract: dict[str, Any], label: str) -> None:
    for field in ("address", "name", "player_writes", "global_writes", "evidence"):
        if field not in contract:
            raise BossStaticClosureError(f"{label} lacks {field}")
    for field in ("player_writes", "global_writes", "evidence"):
        if not isinstance(contract[field], list):
            raise BossStaticClosureError(f"{label}.{field} must be an array")
    if not contract["evidence"]:
        raise BossStaticClosureError(f"{label} has no evidence")


def check_contracts(ledger: dict[str, Any], root: Path = ROOT) -> None:
    pool = ledger.get("pool_contract")
    if not isinstance(pool, dict) or pool.get("stride") != "0x78":
        raise BossStaticClosureError("pool contract is missing or drifted")
    if pool.get("player_writes") != []:
        raise BossStaticClosureError("pool contract gained a player write")
    shared = ledger.get("shared_contracts")
    if not isinstance(shared, list) or not shared:
        raise BossStaticClosureError("shared contracts are missing")
    for contract in shared:
        check_contract(contract, f"shared {contract.get('address')}")
        if contract["player_writes"]:
            raise BossStaticClosureError(f"shared {contract.get('address')} has a player write")

    expected = {
        # The callback compares the pre-increment counter; consumed hits are
        # one greater than these strict machine thresholds.
        "W1": ("B142", "B25D", "B33B", 4),
        "W2": ("B9F3", "BB0E", "BBEC", 5),
        "W3": ("C28A", "C328", "C40B", 4),
        "W4": ("CC68", "CDA3", "CE81", 5),
        "W5": ("D2F6", "D55A", "D63D", 3),
    }
    worlds = ledger.get("worlds")
    if not isinstance(worlds, list) or {w.get("world") for w in worlds} != set(expected):
        raise BossStaticClosureError("world set drifted")
    for world in worlds:
        wid = world["world"]
        constructor = world.get("constructor")
        damage = world.get("damage_callback")
        main = world.get("main_callback")
        if not all(isinstance(item, dict) for item in (constructor, damage, main)):
            raise BossStaticClosureError(f"{wid}: missing core contract")
        for label, contract in (("constructor", constructor), ("damage", damage), ("main", main)):
            check_contract(contract, f"{wid} {label}")
            if contract["player_writes"]:
                raise BossStaticClosureError(f"{wid} {label} has an unproven player write")
        for contract, index in ((constructor, 0), (damage, 1), (main, 2)):
            if contract["address"].split(":")[-1] != expected[wid][index]:
                raise BossStaticClosureError(f"{wid}: {contract['address']} address drifted")
        if damage.get("threshold") is None:
            raise BossStaticClosureError(f"{wid}: damage threshold missing")
        if str(expected[wid][3]) not in damage["threshold"]:
            raise BossStaticClosureError(f"{wid}: damage threshold drifted")
        if not isinstance(world.get("child_callbacks"), list) or not world["child_callbacks"]:
            raise BossStaticClosureError(f"{wid}: child callback closure missing")

    for edge in ledger.get("completion_edges", []):
        check_contract(edge, f"completion {edge.get('address')}")
        if edge["player_writes"]:
            raise BossStaticClosureError(f"completion {edge.get('address')} has a player write")
    if not ledger.get("unresolved"):
        raise BossStaticClosureError("unresolved boundary report is missing")
    if not NOTE.is_file():
        raise BossStaticClosureError(f"missing static note: {NOTE}")
    note_hash = ledger.get("note_sha256")
    if note_hash and sha256(NOTE) != note_hash:
        raise BossStaticClosureError("static note hash drift")


def check_runner(ledger: dict[str, Any], root: Path = ROOT) -> None:
    runner_path = root / ledger["export"]["runner"]
    runner = runner_path.read_text(encoding="utf-8") if runner_path.is_file() else ""
    if not runner:
        raise BossStaticClosureError(f"missing runner: {runner_path}")
    for token in ("analyzeHeadless", "x86:LE:16:Protected Mode", "DumpBossDecomp.java", "DumpFocusedDisasm.java", "--pad-to-memory", "B142", "B25D", "B33B", "B9F3", "BB0E", "BBEC", "C28A", "C328", "C40B", "CC68", "CDA3", "CE81", "D2F6", "D55A", "D63D"):
        if token not in runner:
            raise BossStaticClosureError(f"runner missing {token}")
    if "objdump" in runner.lower():
        raise BossStaticClosureError("runner must not use objdump")


def check_export(export: Path, ledger: dict[str, Any]) -> None:
    manifest_path = export / "manifest.json"
    if not manifest_path.is_file():
        raise BossStaticClosureError(f"missing export manifest: {manifest_path}")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("source_executable_sha256") != ledger["source"]["executable_sha256"]:
        raise BossStaticClosureError("export executable hash drifted")
    required = ("seg3_decomp", "seg1_decomp", "seg2_decomp", "seg5_decomp", "seg3_listing", "seg1_listing")
    for key in required:
        rel = manifest.get("exports", {}).get(key)
        if not rel:
            raise BossStaticClosureError(f"manifest lacks {key}")
        a = export / rel
        if not a.is_file() or sha256(a) != manifest.get("sha256", {}).get(key):
            raise BossStaticClosureError(f"export hash missing/drifted: {key}")
        b = export / str(rel).replace("-a/", "-b/")
        if not b.is_file() or a.read_bytes() != b.read_bytes():
            raise BossStaticClosureError(f"A/B export drifted: {key}")
        if b"DECOMPILATION FAILED" in a.read_bytes() or b"MISSING" in a.read_bytes():
            raise BossStaticClosureError(f"failed decompilation marker in {key}")
    listing = (export / manifest["exports"]["seg3_listing"]).read_text(encoding="utf-8")
    for address in ("B142", "B25D", "B33B", "B9F3", "BB0E", "BBEC", "C28A", "C328", "C40B", "CC68", "CDA3", "CE81", "D2F6", "D55A", "D63D"):
        if f"; ---- {address} " not in listing:
            raise BossStaticClosureError(f"listing missing {address}")


def verify(ledger_path: Path = LEDGER, root: Path = ROOT, export: Path | None = None) -> None:
    ledger = load(ledger_path)
    check_source(ledger, root)
    check_contracts(ledger, root)
    check_runner(ledger, root)
    if export is not None:
        check_export(export, ledger)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ledger", type=Path, default=LEDGER)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--export", type=Path)
    args = parser.parse_args()
    verify(args.ledger, args.root, args.export)
    print("OK: focused pooled-boss Ghidra closure contracts and runner verified")
    if args.export:
        print(f"OK: retained independent export matched at {args.export}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
