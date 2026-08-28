#!/usr/bin/env python3
"""Compatibility wrapper for the shared player parity comparator.

The historical command line and import names remain stable. Implementation
now lives in :mod:`quiky.parity`, where it shares the trace envelope adapter
with the session comparator.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

from quiky.parity import (
    GLOBAL_FIELD_MAP,
    ParityError,
    canonical_effects,
    canonical_factory,
    canonical_global_write,
    canonical_globals,
    canonical_input,
    canonical_probes,
    compare_player,
    player_parity_coverage,
    validate_record,
)
from quiky.trace import TraceError, extract_samples, load_trace


def load_payload(path: Path) -> dict[str, Any]:
    """Retain the old helper while validating through the shared adapter."""

    try:
        payload = load_trace(path).payload
    except TraceError as exc:
        raise ParityError(str(exc)) from exc
    events = payload.get("events")
    if isinstance(events, list):
        return events[0]
    return payload


def sample_map(payload: dict[str, Any], label: str) -> dict[int, dict[str, Any]]:
    try:
        samples, _ = extract_samples(payload)
    except TraceError as exc:
        raise ParityError(str(exc)) from exc
    result: dict[int, dict[str, Any]] = {}
    for sample in samples:
        sequence = sample.get("sequence")
        if not isinstance(sequence, int):
            raise ParityError(f"{label}: every sample needs integer sequence")
        if sequence in result:
            raise ParityError(f"{label}: duplicate sample sequence {sequence}")
        result[sequence] = sample
    return result


def callback(sample: dict[str, Any]) -> dict[str, Any] | None:
    value = sample.get("player_callback")
    return value if isinstance(value, dict) else None


def record_hex(sample: dict[str, Any], which: str) -> str | None:
    callback_value = callback(sample)
    if callback_value is not None:
        obj = callback_value.get("pre_object" if which == "pre" else "post_object")
        if isinstance(obj, dict) and isinstance(obj.get("state_hex"), str):
            return obj["state_hex"]
        direct = callback_value.get(f"{which}_record_hex")
        if isinstance(direct, str):
            return direct
    direct = sample.get(f"{which}_record_hex")
    return direct if isinstance(direct, str) else None


def compare(original: Path, candidate: Path,
            require_complete: bool = False,
            required_fields: list[str] | None = None
            ) -> list[dict[str, Any]]:
    return compare_player(original, candidate,
                          require_complete=require_complete,
                          required_fields=required_fields)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--max-report", type=int, default=8)
    parser.add_argument(
        "--require-complete", action="store_true",
        help="fail when either trace omits a comparable callback field",
    )
    parser.add_argument(
        "--require-field", action="append", default=[],
        choices=("input_flags", "probes", "global_writes",
                 "factory_objects", "effects"),
        help="fail when either trace omits this comparable field; repeatable",
    )
    parser.add_argument(
        "--coverage-report", type=Path,
        help="write machine-readable field coverage for both traces",
    )
    args = parser.parse_args(argv)
    try:
        if args.coverage_report is not None:
            report = {
                "schema": "quiky.player-parity-report.v1",
                "required_fields": sorted(set(args.require_field) | (
                    {"input_flags", "probes", "global_writes",
                     "factory_objects", "effects"}
                    if args.require_complete else set()
                )),
                "original": player_parity_coverage(args.original),
                "candidate": player_parity_coverage(args.candidate),
            }
            args.coverage_report.parent.mkdir(parents=True, exist_ok=True)
            args.coverage_report.write_text(
                json.dumps(report, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
        mismatches = compare(
            args.original,
            args.candidate,
            require_complete=args.require_complete,
            required_fields=args.require_field,
        )
    except ParityError as exc:
        print(f"player-parity: {exc}", file=sys.stderr)
        return 2
    if mismatches:
        print(f"MISMATCH callbacks={len(mismatches)}")
        for item in mismatches[:args.max_report]:
            print(json.dumps(item, sort_keys=True))
        return 1
    print("OK: player callback parity")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
