#!/usr/bin/env python3
"""Validate the focused player callback branch/write audit.

The audit is intentionally separate from the broader closure ledger: this
check makes the five under-specified fields and the 3D02/3DF2 caller order
mechanically reviewable without pretending that the runtime implementation is
already complete.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any


class AuditError(Exception):
    """Raised when the focused audit is incomplete or inconsistent."""


TARGET_FIELDS = {"+0x37", "+0x3A", "+0x3B", "+0x3E", "+0x40"}
REQUIRED_LABELS = {
    "callback_entry",
    "transition_gate",
    "input_acquisition",
    "action_frame_counter_reset",
    "action_frame_counter_increment",
    "positive_mode_entry",
    "grounded_contact",
    "ordinary_mode_entry",
    "ordinary_correction",
    "jump_initiation",
    "negative_mode_entry",
    "common_tail",
    "transition_block",
}


def read_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise AuditError(f"{path}: cannot read JSON: {exc}") from exc
    if not isinstance(value, dict):
        raise AuditError(f"{path}: top-level value must be an object")
    return value


def check_closure_offsets(closure: dict[str, Any]) -> None:
    typed = closure.get("typed_model", {}).get("player_record", [])
    typed_offsets = {str(item.get("offset", "")).upper() for item in typed}
    expected = {field[3:].upper() for field in TARGET_FIELDS}
    missing = sorted(expected - typed_offsets)
    if missing:
        raise AuditError("typed player model lacks target offsets: " + ", ".join(missing))


def check_branch_order(audit: dict[str, Any]) -> None:
    branches = audit.get("branch_order")
    if not isinstance(branches, list) or not branches:
        raise AuditError("branch_order must be a non-empty array")
    labels: list[str] = []
    addresses: list[str] = []
    for index, branch in enumerate(branches, 1):
        if not isinstance(branch, dict):
            raise AuditError(f"branch_order[{index}] must be an object")
        label = branch.get("label")
        address = branch.get("address")
        evidence = branch.get("evidence")
        if not isinstance(label, str) or not label:
            raise AuditError(f"branch_order[{index}] has no label")
        if not isinstance(address, str) or ":" not in address:
            raise AuditError(f"branch_order[{index}] has no qualified address")
        if not isinstance(evidence, list) or not evidence:
            raise AuditError(f"branch_order[{index}] has no evidence")
        labels.append(label)
        addresses.append(address)
    duplicate_labels = sorted({item for item in labels if labels.count(item) > 1})
    duplicate_addresses = sorted({item for item in addresses if addresses.count(item) > 1})
    if duplicate_labels:
        raise AuditError("duplicate branch labels: " + ", ".join(duplicate_labels))
    if duplicate_addresses:
        raise AuditError("duplicate branch addresses: " + ", ".join(duplicate_addresses))
    missing = sorted(REQUIRED_LABELS - set(labels))
    if missing:
        raise AuditError("required branch labels missing: " + ", ".join(missing))


def check_field_writes(audit: dict[str, Any]) -> None:
    writes = audit.get("field_writes")
    if not isinstance(writes, dict):
        raise AuditError("field_writes must be an object")
    if set(writes) != TARGET_FIELDS:
        raise AuditError(
            "field_writes must cover exactly " + ", ".join(sorted(TARGET_FIELDS))
        )
    for field in sorted(TARGET_FIELDS):
        records = writes[field]
        if not isinstance(records, list) or not records:
            raise AuditError(f"{field}: no write records")
        for index, record in enumerate(records, 1):
            if not isinstance(record, dict):
                raise AuditError(f"{field}[{index}] must be an object")
            for required in ("address", "value", "condition", "confidence", "evidence"):
                if required not in record:
                    raise AuditError(f"{field}[{index}] lacks {required}")
            if not isinstance(record["evidence"], list) or not record["evidence"]:
                raise AuditError(f"{field}[{index}] lacks evidence")


def check_caller_order(audit: dict[str, Any]) -> None:
    paths = audit.get("caller_order")
    if not isinstance(paths, list) or not paths:
        raise AuditError("caller_order must be a non-empty array")
    names = {str(item.get("path")) for item in paths if isinstance(item, dict)}
    for required in ("positive_mode_41E8", "grounded_contact_427F",
                     "ordinary_correction_42C9", "negative_mode_4323"):
        if required not in names:
            raise AuditError(f"caller_order lacks {required}")
    ordinary = next(item for item in paths if item.get("path") == "ordinary_correction_42C9")
    order = ordinary.get("order", [])
    if not (isinstance(order, list) and order[:2] == [
            "42C9 snap_player_y_on_side_contact",
            "42CC apply_descriptor_vertical_correction"]):
        raise AuditError("ordinary correction must call 3DF2 before 3D02")
    grounded = next(item for item in paths if item.get("path") == "grounded_contact_427F")
    order = grounded.get("order", [])
    if not (isinstance(order, list) and order[:2] == [
            "4286 apply_descriptor_vertical_correction",
            "4289 snap_player_y_on_side_contact"]):
        raise AuditError("grounded contact must call 3D02 before 3DF2")
    positive = next(item for item in paths if item.get("path") == "positive_mode_41E8")
    if "3DF2" in " ".join(positive.get("order", [])):
        raise AuditError("positive no-contact path must not invent a 3DF2 call")


def check_dynamic_hashes(audit: dict[str, Any], root: Path, require: bool) -> list[str]:
    warnings: list[str] = []
    checks = audit.get("dynamic_checks", [])
    for check in checks:
        if not isinstance(check, dict):
            raise AuditError("dynamic_checks contains a non-object")
        trace = check.get("trace")
        expected = check.get("sha256")
        if not isinstance(trace, str) or not isinstance(expected, str):
            raise AuditError("dynamic check must name a trace and sha256")
        path = root / trace
        if not path.is_file():
            message = f"optional trace unavailable: {trace}"
            if require:
                raise AuditError(message)
            warnings.append(message)
            continue
        actual = hashlib.sha256(path.read_bytes()).hexdigest()
        if actual != expected:
            raise AuditError(f"{trace}: sha256 drift (expected {expected}, got {actual})")
    return warnings


def verify(audit_path: Path, closure_path: Path, require_traces: bool = False) -> list[str]:
    audit = read_json(audit_path)
    closure = read_json(closure_path)
    if audit.get("schema") != "quiky.player-callback-focused-audit.v1":
        raise AuditError("unexpected focused audit schema")
    if closure.get("schema") != "quiky.player-callback-closure.v2":
        raise AuditError("unexpected closure ledger schema")
    check_closure_offsets(closure)
    check_branch_order(audit)
    check_field_writes(audit)
    check_caller_order(audit)
    return check_dynamic_hashes(audit, audit_path.parents[2], require_traces)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--audit", type=Path,
                        default=Path("research/ghidra/player-callback-focused-audit.json"))
    parser.add_argument("--closure", type=Path,
                        default=Path("research/ghidra/player-callback-closure.json"))
    parser.add_argument("--require-traces", action="store_true",
                        help="fail if an optional ignored build trace is unavailable")
    args = parser.parse_args(argv)
    try:
        warnings = verify(args.audit, args.closure, args.require_traces)
    except AuditError as exc:
        print(f"player-callback-focused-audit: {exc}", file=sys.stderr)
        return 1
    print("OK: focused callback branch/write audit")
    for warning in warnings:
        print(f"WARNING: {warning}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
