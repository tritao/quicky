#!/usr/bin/env python3
"""Verify the focused player external-state contract and export ledger."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
LEDGER = ROOT / "research/ghidra/player-external-state-closure.json"
ALLOWED_CLASSIFICATIONS = {"inline", "contract", "irrelevant", "unresolved"}
REQUIRED_ADDRESSES = {
    "01F7:0E06", "01F7:0E96", "01F7:0FDC", "01F7:0FA2", "01F7:1036",
    "01F7:39FE", "01F7:9DC7", "01F7:A075", "01F7:A0B2", "01F7:5DC3",
    "01F7:5937", "01F7:386F", "01F7:0442", "01F7:1C6E", "01F7:1C92",
    "01F7:5C27", "01F7:5CC3", "01F7:5D38", "01F7:5D60", "01F7:4519",
    "01F7:45AB", "01F7:470C", "01F7:6328", "01F7:1DEE", "01F7:4416-44FE",
}
CONTRACT_FIELDS = {
    "address", "name", "classification", "calling_convention",
    "inputs_outputs", "player_reads", "player_writes", "global_reads",
    "global_writes", "probe_coordinates", "scheduler_position",
    "object_effects", "feeds_back_into_simulation", "callees", "callers",
    "confidence", "evidence",
}
ADDRESS_RE = re.compile(r"^[0-9A-F]{4}:[0-9A-F]{4}(?:-[0-9A-F]{4})?$")


class ExternalClosureError(Exception):
    pass


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def load(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise ExternalClosureError("ledger top-level value must be an object")
    if value.get("schema") != "quiky.player-external-state-closure.v1":
        raise ExternalClosureError("unexpected external-state closure schema")
    return value


def check_source_hashes(ledger: dict[str, Any], root: Path) -> None:
    source = ledger.get("source")
    if not isinstance(source, dict):
        raise ExternalClosureError("source must be an object")
    executable = root / source["executable"]
    segment3 = root / source["segment3"]
    segment1 = root / source["segment1"]
    for path in (executable, segment3, segment1):
        if not path.is_file():
            raise ExternalClosureError(f"missing source artifact: {path}")
    if sha256(executable) != source["executable_sha256"]:
        raise ExternalClosureError(f"executable SHA-256 drift: {executable}")
    if sha256(segment3) != source["segment3_sha256"]:
        raise ExternalClosureError(f"segment-3 SHA-256 drift: {segment3}")


def check_contracts(ledger: dict[str, Any]) -> None:
    functions = ledger.get("functions")
    if not isinstance(functions, list) or not functions:
        raise ExternalClosureError("functions must be a non-empty array")
    seen: set[str] = set()
    by_address: dict[str, dict[str, Any]] = {}
    for index, function in enumerate(functions, 1):
        if not isinstance(function, dict):
            raise ExternalClosureError(f"functions[{index}] must be an object")
        missing = sorted(CONTRACT_FIELDS - function.keys())
        if missing:
            raise ExternalClosureError(
                f"{function.get('address', index)} lacks: {', '.join(missing)}"
            )
        address = function["address"]
        if not isinstance(address, str) or not ADDRESS_RE.fullmatch(address):
            raise ExternalClosureError(f"invalid address-qualified contract: {address!r}")
        if address in seen:
            raise ExternalClosureError(f"duplicate contract {address}")
        seen.add(address)
        by_address[address] = function
        if function["classification"] not in ALLOWED_CLASSIFICATIONS:
            raise ExternalClosureError(f"{address}: invalid classification")
        for field in (
            "player_reads", "player_writes", "global_reads", "global_writes",
            "probe_coordinates", "callees", "callers", "evidence",
        ):
            if not isinstance(function[field], list):
                raise ExternalClosureError(f"{address}.{field} must be an array")
        if not function["evidence"]:
            raise ExternalClosureError(f"{address} has no evidence")
        if function["classification"] == "unresolved":
            name = function["name"]
            if not any(token in name for token in (address.split(":", 1)[1], "address", "unknown")):
                raise ExternalClosureError(
                    f"unresolved contract {address} has a speculative semantic name: {name}"
                )

    missing = sorted(REQUIRED_ADDRESSES - seen)
    if missing:
        raise ExternalClosureError("missing required contracts: " + ", ".join(missing))

    if by_address["01F7:0E96"]["callees"][0]["address"] != "01F7:0FDC":
        raise ExternalClosureError("0E96 must retain its 0FDC tail contract")
    if by_address["01F7:5937"]["callees"][0]["address"] != "01F7:386F":
        raise ExternalClosureError("5937 must retain the 386F view-state dispatch")
    if by_address["01F7:386F"]["callees"][0]["address"] != "01F7:0442":
        raise ExternalClosureError("386F must retain the 0442 indirect boundary")
    for address in ("01F7:A0B2", "01F7:5937", "01F7:386F"):
        if not by_address[address]["global_writes"]:
            raise ExternalClosureError(f"{address} lost its global-write contract")


def check_scheduler_order(ledger: dict[str, Any]) -> None:
    order = ledger.get("static_order", {}).get("main_loop_pairs")
    expected = [
        ("01D7:44FA->01F7:0E96", "01D7:4518->01F7:0FA2"),
        ("01D7:47FC->01F7:0E96", "01D7:481A->01F7:0FA2"),
        ("01D7:4872->01F7:0E96", "01D7:4890->01F7:0FA2"),
    ]
    if not isinstance(order, list) or len(order) != len(expected):
        raise ExternalClosureError("static_order.main_loop_pairs must contain three pairs")
    for item, pair in zip(order, expected):
        if not isinstance(item, dict):
            raise ExternalClosureError("scheduler order entry must be an object")
        if (item.get("scheduler"), item.get("nonzero_state")) != pair:
            raise ExternalClosureError("scheduler relocation pair drifted")
        if item.get("order") != "0E96 before 0FA2":
            raise ExternalClosureError("scheduler order must remain 0E96 before 0FA2")


def check_exports(ledger: dict[str, Any], root: Path) -> None:
    exports = ledger.get("authoritative_exports")
    if not isinstance(exports, dict):
        raise ExternalClosureError("authoritative_exports must be an object")
    required = ("runner", "decomp_script", "listing_script", "callsite_source")
    for key in required:
        path = root / exports[key]
        if not path.is_file():
            raise ExternalClosureError(f"missing export tool {key}: {path}")
    runner_text = (root / exports["runner"]).read_text(encoding="utf-8")
    if "analyzeHeadless" not in runner_text or "x86:LE:16:Protected Mode" not in runner_text:
        raise ExternalClosureError("external runner is not using the Ghidra protected-mode pipeline")
    if "objdump" in runner_text.lower():
        raise ExternalClosureError("external runner must not use objdump")


def verify(ledger_path: Path, root: Path) -> None:
    ledger = load(ledger_path)
    check_source_hashes(ledger, root)
    check_contracts(ledger)
    check_scheduler_order(ledger)
    check_exports(ledger, root)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ledger", type=Path, default=LEDGER)
    parser.add_argument("--root", type=Path, default=ROOT)
    args = parser.parse_args(argv)
    try:
        verify(args.ledger, args.root)
    except (ExternalClosureError, OSError, KeyError, TypeError) as exc:
        print(f"player-external-closure: {exc}", file=sys.stderr)
        return 1
    print("OK: player external-state closure ledger")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
