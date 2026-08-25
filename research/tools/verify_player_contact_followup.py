#!/usr/bin/env python3
"""Verify the focused player-contact contract and evidence ledger."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
LEDGER = ROOT / "research/ghidra/player-contact-followup.json"
SOURCE = ROOT / "research/notes/player-static-decomp.cpp"


class FollowupError(Exception):
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
        raise FollowupError(f"cannot read {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise FollowupError("ledger top-level value must be an object")
    if value.get("schema") != "quiky.player-contact-followup.v1":
        raise FollowupError("unexpected follow-up schema")
    return value


def check_source_hashes(ledger: dict[str, Any], root: Path) -> None:
    source = ledger["source"]
    for key in ("executable", "segment_image"):
        path = root / source[key]
        if not path.is_file():
            raise FollowupError(f"missing source artifact: {path}")
        actual = sha256(path)
        expected = source["sha256" if key == "executable" else "segment_sha256"]
        if actual != expected:
            raise FollowupError(f"{path}: sha256 drift")


REQUIRED_CONTRACTS = {
    "01F7:3D02", "01F7:3DF2", "01F7:3A1F", "01F7:3971", "01F7:3986",
    "01F7:5937", "01F7:0E06", "01F7:F21B/F21C", "01F7:A075", "01F7:A0B2",
}


def check_contracts(ledger: dict[str, Any]) -> None:
    contracts = ledger.get("contracts")
    if not isinstance(contracts, list):
        raise FollowupError("contracts must be an array")
    seen: set[str] = set()
    for index, contract in enumerate(contracts, 1):
        if not isinstance(contract, dict):
            raise FollowupError(f"contracts[{index}] must be an object")
        address = contract.get("address")
        if not isinstance(address, str) or not address:
            raise FollowupError(f"contracts[{index}] has no address")
        if address in seen:
            raise FollowupError(f"duplicate contract {address}")
        seen.add(address)
        for field in ("name", "classification", "inputs", "outputs", "confidence", "evidence", "callees"):
            if field not in contract:
                raise FollowupError(f"{address} lacks {field}")
        if not contract["evidence"]:
            raise FollowupError(f"{address} has no evidence")
        if not isinstance(contract["player_writes"], list):
            raise FollowupError(f"{address}.player_writes must be an array")
        if not isinstance(contract["global_writes"], list):
            raise FollowupError(f"{address}.global_writes must be an array")
    missing = sorted(REQUIRED_CONTRACTS - seen)
    if missing:
        raise FollowupError("missing required contracts: " + ", ".join(missing))


def check_contact_order(ledger: dict[str, Any]) -> None:
    semantics = ledger.get("contact_semantics", {})
    paths = semantics.get("callback_order")
    if not isinstance(paths, list):
        raise FollowupError("contact_semantics.callback_order must be an array")
    by_path = {item.get("path"): item for item in paths if isinstance(item, dict)}
    for name in ("grounded_contact_427F", "ordinary_mode_42B4", "negative_mode_4323", "positive_mode_41E8", "common_tail_4384"):
        if name not in by_path:
            raise FollowupError(f"missing callback path {name}")
    grounded = by_path["grounded_contact_427F"]["order"]
    if grounded[:2] != ["3D02", "3DF2"]:
        raise FollowupError("grounded path must call 3D02 before 3DF2")
    ordinary = by_path["ordinary_mode_42B4"]["order"]
    if ordinary[:3] != ["3A1F", "3DF2", "3D02"]:
        raise FollowupError("ordinary correction must call 3A1F/3DF2/3D02")
    negative = by_path["negative_mode_4323"]
    if "3D02" in negative.get("not_called", []) and "3DF2" in negative.get("not_called", []):
        pass
    else:
        raise FollowupError("negative path must explicitly exclude 3D02 and 3DF2")
    landing = semantics.get("landing", {})
    expected_landing = {"0x07", "0x08", "0x0F", "0x10", "0x12", "0x1E", "0x20", "0x22", "0x24", "0x36", "0x37", "0x3E", "0x45", "0x46"}
    if set(landing.get("observed_callback_write_offsets", [])) != expected_landing:
        raise FollowupError("landing callback write-offset set drifted")


def check_traces(ledger: dict[str, Any], root: Path, require: bool) -> list[str]:
    warnings: list[str] = []
    checks = ledger.get("dynamic_checks")
    if not isinstance(checks, list) or not checks:
        raise FollowupError("dynamic_checks must be a non-empty array")
    for check in checks:
        if not isinstance(check, dict):
            raise FollowupError("dynamic check must be an object")
        trace = check.get("trace")
        expected = check.get("sha256")
        if not isinstance(trace, str) or not isinstance(expected, str):
            raise FollowupError("dynamic check needs trace and sha256")
        path = root / trace
        if not path.is_file():
            message = f"optional trace unavailable: {trace}"
            if require:
                raise FollowupError(message)
            warnings.append(message)
            continue
        if sha256(path) != expected:
            raise FollowupError(f"{trace}: sha256 drift")
    return warnings


def check_source(source_path: Path) -> None:
    text = source_path.read_text(encoding="utf-8")
    if "object_pool_factory_0E06" not in text:
        raise FollowupError("authoritative C-like source still uses a guessed 0E06 name")
    if "initialize_contact_object" in text:
        raise FollowupError("old guessed 0E06 name remains in authoritative source")
    if re.search(r"p->vy\([^\n]*p->vx\(\)", text):
        raise FollowupError("descriptor correction still uses X velocity for the Y response")


def verify(ledger_path: Path, root: Path, require_traces: bool) -> list[str]:
    ledger = load(ledger_path)
    external = ledger.get("external_state_ledger")
    if not isinstance(external, str) or not (root / external).is_file():
        raise FollowupError("external_state_ledger link is missing or unreadable")
    check_source_hashes(ledger, root)
    check_contracts(ledger)
    check_contact_order(ledger)
    source_path = root / ledger["authoritative_c_source"]
    if not source_path.is_file():
        raise FollowupError(f"missing authoritative source: {source_path}")
    check_source(source_path)
    return check_traces(ledger, root, require_traces)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ledger", type=Path, default=LEDGER)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--require-traces", action="store_true")
    args = parser.parse_args(argv)
    try:
        warnings = verify(args.ledger, args.root, args.require_traces)
    except (FollowupError, OSError) as exc:
        print(f"player-contact-followup: {exc}", file=sys.stderr)
        return 1
    print("OK: player contact follow-up ledger")
    for warning in warnings:
        print("WARNING:", warning)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
