#!/usr/bin/env python3
"""Verify the focused static contract for BIENE's startup runtime table."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
LEDGER = ROOT / "research/ghidra/biene-runtime-table-static-closure.json"


class BieneRuntimeTableError(Exception):
    pass


def load(path: Path = LEDGER) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise BieneRuntimeTableError(f"cannot read {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise BieneRuntimeTableError("ledger must be an object")
    if value.get("schema") != "quiky.biene-runtime-table-static-closure.v1":
        raise BieneRuntimeTableError("unexpected ledger schema")
    return value


def check_contracts(ledger: dict[str, Any]) -> None:
    source = ledger.get("source")
    if not isinstance(source, dict):
        raise BieneRuntimeTableError("source is missing")
    if source.get("ghidra_language") != "x86:LE:16:Protected Mode":
        raise BieneRuntimeTableError("wrong Ghidra language")
    if "objdump" in str(source.get("rerun", "")).lower():
        raise BieneRuntimeTableError("objdump is not an allowed pipeline")

    builder = ledger.get("builder")
    if not isinstance(builder, dict) or builder.get("address") != "01F7:0A43":
        raise BieneRuntimeTableError("startup builder contract is missing")
    storage = ledger.get("storage")
    if not isinstance(storage, dict):
        raise BieneRuntimeTableError("storage contract is missing")
    if storage.get("address") != "01F7:7974" or storage.get("bytes") != 2048:
        raise BieneRuntimeTableError("runtime-table storage contract drifted")
    if storage.get("raw_image_is_not_runtime_table") is not True:
        raise BieneRuntimeTableError("raw DS:7974 image must remain non-runtime data")

    loops = ledger.get("loops")
    if not isinstance(loops, list):
        raise BieneRuntimeTableError("loop contracts are missing")
    ranges = {(item.get("start"), item.get("end"), item.get("write"))
              for item in loops if isinstance(item, dict)}
    expected = {
        ("0x0000", "0x00ff", "DS:646C[index]"),
        ("0x0000", "0x03e7", "DS:7974[index]"),
        ("0x03e8", "0x07ff", "DS:7974[index]"),
    }
    if not expected.issubset(ranges):
        raise BieneRuntimeTableError("startup loop ranges drifted")

    coefficients = ledger.get("coefficient_block")
    expected_words = [
        "0x9d58", "0x9f39", "0xd73f", "0x4360", "0x309d", "0x3092",
        "0xaa67", "0x283f", "0xd732", "0xb66e", "0x1d2a", "0x38ef",
        "0x0d74", "0x00d0", "0xd00d", "0x887a", "0x8888", "0x0888",
        "0xab7e", "0xaaaa", "0xaaaa",
    ]
    if not isinstance(coefficients, dict) or coefficients.get("address") != "0227:1646":
        raise BieneRuntimeTableError("coefficient block contract is missing")
    if coefficients.get("words") != expected_words:
        raise BieneRuntimeTableError("coefficient block drifted")

    helpers = ledger.get("helper_contracts")
    if not isinstance(helpers, list):
        raise BieneRuntimeTableError("helper contracts are missing")
    addresses = {item.get("address") for item in helpers if isinstance(item, dict)}
    for required in ("0227:19EE", "0227:19B6", "0227:1959",
                     "0227:1B7E", "0227:18F1/190A"):
        if required not in addresses:
            raise BieneRuntimeTableError(f"missing helper contract {required}")
    unresolved = ledger.get("unresolved")
    if not isinstance(unresolved, list) or not unresolved:
        raise BieneRuntimeTableError("unresolved boundary was lost")


def check_runner(ledger: dict[str, Any], root: Path = ROOT) -> None:
    runner_path = root / str(ledger["source"]["rerun"].split()[1])
    # The command stores the runner in the source metadata only as a command;
    # resolve the repository path explicitly to keep this check unambiguous.
    runner_path = root / "research/tools/run_player_external_closure.py"
    if not runner_path.is_file():
        raise BieneRuntimeTableError(f"missing runner: {runner_path}")
    runner = runner_path.read_text(encoding="utf-8")
    for token in ('("1646", 21)', '("19EC", 8)',
                  "DumpDataWords.java", "analyzeHeadless",
                  "x86:LE:16:Protected Mode"):
        if token not in runner:
            raise BieneRuntimeTableError(f"runner missing {token}")
    if "objdump" in runner.lower():
        raise BieneRuntimeTableError("runner must not use objdump")


def check_export(ledger: dict[str, Any], export: Path) -> None:
    data_a = export / "biene-data-a/runtime-table-constants.txt"
    data_b = export / "biene-data-b/runtime-table-constants.txt"
    for path in (data_a, data_b):
        if not path.is_file():
            raise BieneRuntimeTableError(f"missing export: {path}")
    if data_a.read_bytes() != data_b.read_bytes():
        raise BieneRuntimeTableError("A/B coefficient export drifted")
    text = data_a.read_text(encoding="utf-8")
    for address, word in (("1646", "9D58"), ("166E", "AAAA"),
                          ("19EC", "8405")):
        if f"{address}: {word}" not in text:
            raise BieneRuntimeTableError(f"export missing {address}: {word}")


def verify(ledger_path: Path = LEDGER, root: Path = ROOT,
           export: Path | None = None) -> None:
    ledger = load(ledger_path)
    check_contracts(ledger)
    check_runner(ledger, root)
    if export is not None:
        check_export(ledger, export)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ledger", type=Path, default=LEDGER)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--export", type=Path)
    args = parser.parse_args()
    verify(args.ledger, args.root, args.export)
    print("OK: BIENE runtime-table static closure verified")
    if args.export:
        print(f"OK: independent coefficient export matched at {args.export}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
