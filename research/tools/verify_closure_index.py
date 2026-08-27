#!/usr/bin/env python3
"""Verify the canonical research-closure index and source identity."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
INDEX = ROOT / "research/ghidra/closure-index.json"


class ClosureIndexError(Exception):
    pass


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def load(path: Path = INDEX) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ClosureIndexError(f"cannot read {path}: {exc}") from exc
    if not isinstance(value, dict) or value.get("schema") != "quiky.research-closure-index.v1":
        raise ClosureIndexError("unexpected closure-index schema")
    return value


def verify(index_path: Path = INDEX, root: Path = ROOT) -> None:
    index = load(index_path)
    source = index.get("source")
    if not isinstance(source, dict):
        raise ClosureIndexError("source contract is missing")
    executable = root / str(source.get("executable", ""))
    if not executable.is_file():
        raise ClosureIndexError(f"missing executable: {executable}")
    if sha256(executable) != source.get("sha256"):
        raise ClosureIndexError("executable hash drifted")
    pipeline = str(source.get("pipeline", ""))
    if "ghidra" not in pipeline.lower() or "objdump" in pipeline.lower():
        raise ClosureIndexError("closure index must identify the Ghidra-only pipeline")

    closures = index.get("closures")
    if not isinstance(closures, list) or not closures:
        raise ClosureIndexError("closures are missing")
    ids: set[str] = set()
    for closure in closures:
        if not isinstance(closure, dict):
            raise ClosureIndexError("closure entry must be an object")
        closure_id = closure.get("id")
        if not isinstance(closure_id, str) or not closure_id or closure_id in ids:
            raise ClosureIndexError(f"duplicate/invalid closure id: {closure_id}")
        ids.add(closure_id)
        for key in ("scope", "ledger", "verifier", "status"):
            if not isinstance(closure.get(key), str) or not closure[key]:
                raise ClosureIndexError(f"{closure_id} lacks {key}")
        ledger = root / closure["ledger"]
        verifier = root / closure["verifier"]
        if not ledger.is_file():
            raise ClosureIndexError(f"{closure_id} ledger is missing: {ledger}")
        if not verifier.is_file():
            raise ClosureIndexError(f"{closure_id} verifier is missing: {verifier}")
        try:
            payload = json.loads(ledger.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            raise ClosureIndexError(f"{closure_id} ledger is unreadable: {exc}") from exc
        if not isinstance(payload, dict) or not payload.get("schema"):
            raise ClosureIndexError(f"{closure_id} ledger has no schema")
        for note in closure.get("notes", []):
            note_path = root / note
            if not note_path.is_file():
                raise ClosureIndexError(f"{closure_id} note is missing: {note_path}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--index", type=Path, default=INDEX)
    parser.add_argument("--root", type=Path, default=ROOT)
    args = parser.parse_args()
    verify(args.index, args.root)
    print("OK: canonical research closure index verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
