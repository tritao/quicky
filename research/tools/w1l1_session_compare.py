#!/usr/bin/env python3
"""Compatibility wrapper for the normalized W1L1 session comparator."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

from quiky.parity import (
    SessionTraceError,
    active_objects as _active_objects,
    callback_offsets,
    compare_active_objects,
    compare_session,
    scheduler_offsets as _scheduler_offsets,
    validate_record as _validate_record,
    _effects,
    _global_writes,
    _session_camera,
    _session_input,
    _session_probes,
)
from quiky.trace import TraceError, extract_samples, load_trace, normalize_sample


def load_payload(path: Path) -> dict[str, Any]:
    try:
        return load_trace(path).payload
    except TraceError as exc:
        raise SessionTraceError(str(exc)) from exc


def expected_samples(payload: dict[str, Any]) -> list[dict[str, Any]]:
    try:
        return extract_samples(payload)[0]
    except TraceError as exc:
        raise SessionTraceError(str(exc)) from exc


def sample_map(samples: list[Any], label: str) -> dict[int, dict[str, Any]]:
    result: dict[int, dict[str, Any]] = {}
    for sample in samples:
        if not isinstance(sample, dict) or not isinstance(sample.get("sequence"), int):
            raise SessionTraceError(f"{label}: each sample needs integer sequence")
        sequence = sample["sequence"]
        if sequence in result:
            raise SessionTraceError(f"{label}: duplicate sequence {sequence}")
        result[sequence] = sample
    return result


def callback(sample: dict[str, Any]) -> dict[str, Any]:
    value = sample.get("player_callback")
    return value if isinstance(value, dict) else {}


def record(sample: dict[str, Any], which: str) -> str | None:
    normalized = normalize_sample(sample, label="sample")
    return normalized.pre_record_hex if which == "pre" else normalized.post_record_hex


def valid_record(value: str | None, label: str) -> str:
    try:
        return _validate_record(value, label, error_type=SessionTraceError)
    except SessionTraceError as exc:
        # Preserve the old session module's wording for direct callers while
        # the shared comparator uses the more explicit common wording.
        message = str(exc).replace(
            "missing complete player record", "missing player record"
        ).replace(
            "player record is not exactly", "record is not exactly"
        ).replace(
            "player record is not hexadecimal", "record is not hexadecimal"
        )
        raise SessionTraceError(message) from exc


def _normalized(sample: dict[str, Any]):
    return normalize_sample(sample, label="sample")


def input_flags(sample: dict[str, Any]) -> int | None:
    return _session_input(_normalized(sample))


def camera(sample: dict[str, Any]) -> tuple[int, int] | None:
    return _session_camera(_normalized(sample))


def probes(sample: dict[str, Any]) -> list[Any] | None:
    return _session_probes(_normalized(sample))


def global_writes(sample: dict[str, Any]) -> list[Any] | None:
    return _global_writes(_normalized(sample))


def effects(sample: dict[str, Any]) -> list[Any] | None:
    return _effects(_normalized(sample))


def scheduler_offsets(sample: dict[str, Any]) -> list[int] | None:
    return _scheduler_offsets(_normalized(sample))


def active_objects(sample: dict[str, Any]) -> list[dict[str, Any]] | None:
    return _active_objects(_normalized(sample))


def compare(original: Path, candidate: Path) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    return compare_session(original, candidate)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--max-report", type=int, default=8)
    parser.add_argument(
        "--require-complete", action="store_true",
        help="fail when either trace omits a comparable field",
    )
    args = parser.parse_args(argv)
    try:
        mismatches, coverage = compare(args.original, args.candidate)
    except SessionTraceError as exc:
        print(f"w1l1-session: {exc}", file=sys.stderr)
        return 2
    if mismatches:
        print(f"MISMATCH fields={len(mismatches)} coverage_gaps={len(coverage)}")
        for item in mismatches[:args.max_report]:
            print(json.dumps(item, sort_keys=True))
        if coverage:
            print("COVERAGE GAPS")
            for item in coverage[:args.max_report]:
                print(json.dumps(item, sort_keys=True))
        return 1
    if coverage and args.require_complete:
        print(f"INCOMPLETE fields=0 coverage_gaps={len(coverage)}")
        for item in coverage[:args.max_report]:
            print(json.dumps(item, sort_keys=True))
        return 2
    print(f"OK: W1L1 session parity fields; coverage_gaps={len(coverage)}")
    for item in coverage[:args.max_report]:
        print(json.dumps(item, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
