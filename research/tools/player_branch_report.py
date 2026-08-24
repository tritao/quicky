#!/usr/bin/env python3
"""Report descriptor-mask branches observed in the 01F7:3D02 path."""

from __future__ import annotations

import argparse
import csv
import json
import sys
from pathlib import Path
from typing import Any, TextIO


class BranchReportError(Exception):
    """Raised when a collision-branch trace is malformed."""


BRANCH_NAMES = {
    0x3D02: "entry",
    0x3D1E: "test_dx_30_first",
    0x3D36: "test_dx_30_retry",
    0x3D40: "test_dx_30_restore",
    0x3D45: "test_dx_20",
    0x3DD0: "test_dx_40",
    0x3DE4: "return_reject",
    0x3D44: "return_clear",
    0x3DF1: "return_al",
}


def _as_int(value: Any, field: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise BranchReportError(f"{field} must be an integer")
    return value


def _payload(path: Path) -> dict[str, Any]:
    try:
        ledger = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise BranchReportError(f"{path}: cannot read JSON trace: {exc}") from exc
    events = ledger.get("events")
    if isinstance(events, list):
        if len(events) != 1 or not isinstance(events[0], dict):
            raise BranchReportError(f"{path}: expected one trace event")
        return events[0]
    if isinstance(ledger.get("samples"), list):
        return ledger
    raise BranchReportError(f"{path}: no samples array")


def _player_position(sample: dict[str, Any]) -> tuple[int | None, int | None]:
    globals_state = sample.get("globals")
    pool = sample.get("pool")
    if not isinstance(globals_state, dict) or not isinstance(pool, dict):
        return None, None
    target = globals_state.get("player_object_offset")
    objects = pool.get("objects")
    if not isinstance(target, int) or not isinstance(objects, list):
        return None, None
    matches = [item for item in objects
               if isinstance(item, dict) and item.get("offset") == target]
    if len(matches) != 1:
        return None, None
    position = matches[0].get("position")
    if not isinstance(position, dict):
        return None, None
    x, y = position.get("x"), position.get("y")
    return (x if isinstance(x, int) else None,
            y if isinstance(y, int) else None)


def branch_rows(paths: list[Path], labels: list[str] | None = None) -> list[dict[str, Any]]:
    """Flatten each sample's branch event sequence into event-level rows."""
    if labels is not None and len(labels) != len(paths):
        raise BranchReportError("labels must match the number of trace paths")
    rows: list[dict[str, Any]] = []
    for index, path in enumerate(paths):
        scenario = labels[index] if labels is not None else path.stem
        trace = _payload(path)
        samples = trace.get("samples")
        if not isinstance(samples, list):
            raise BranchReportError(f"{path}: samples must be an array")
        for sample in samples:
            if not isinstance(sample, dict):
                raise BranchReportError(f"{path}: sample must be an object")
            events = sample.get("branch_events", [])
            if not isinstance(events, list):
                raise BranchReportError(f"{path}: branch_events must be an array")
            player_x, player_y = _player_position(sample)
            sequence = _as_int(sample.get("sequence"), "sample.sequence")
            branch_return = sample.get("branch_return")
            return_offset = (branch_return.get("offset")
                             if isinstance(branch_return, dict) else None)
            return_registers = (branch_return.get("registers", {})
                                if isinstance(branch_return, dict) else {})
            return_al = ((return_registers.get("eax", 0) & 0xff)
                         if return_offset in (0x3DE4, 0x3DF1) and
                         isinstance(return_registers, dict) else None)
            for event in events:
                if not isinstance(event, dict):
                    raise BranchReportError(f"{path}: branch event must be an object")
                offset = _as_int(event.get("offset"), "branch_event.offset")
                registers = event.get("registers", {})
                dx = event.get("dx")
                if dx is None and isinstance(registers, dict):
                    dx = registers.get("edx", 0) & 0xffff
                dx = _as_int(dx, "branch_event.dx")
                object_state = event.get("object")
                object_byte_3a = (object_state.get("player_byte_0x3a")
                                  if isinstance(object_state, dict) else None)
                rows.append({
                    "trace": str(path),
                    "scenario": scenario,
                    "sequence": sequence,
                    "player_x": player_x,
                    "player_y": player_y,
                    "offset": offset,
                    "name": BRANCH_NAMES.get(offset, f"unknown_{offset:04x}"),
                    "dx": dx,
                    "dx_mask_0x30": event.get("dx_mask_0x30", dx & 0x30),
                    "dx_mask_0x20": event.get("dx_mask_0x20", dx & 0x20),
                    "dx_mask_0x40": event.get("dx_mask_0x40", dx & 0x40),
                    "object_byte_0x3a": object_byte_3a,
                    "return_offset": return_offset,
                    "return_al": return_al,
                })
    return rows


def _hex(value: Any, width: int = 4) -> str:
    return "-" if value is None else f"0x{value:0{width}x}"


def render_report(rows: list[dict[str, Any]], stream: TextIO) -> None:
    samples = sorted({(row["scenario"], row["sequence"]) for row in rows})
    print(f"branch_rows={len(rows)}", file=stream)
    print(f"branch_samples={len(samples)}", file=stream)
    print(
        "scenario sequence player_x player_y path dx_masks return_al object_0x3a",
        file=stream,
    )
    for scenario, sequence in samples:
        sample_rows = [row for row in rows
                       if row["scenario"] == scenario and row["sequence"] == sequence]
        path = ">".join(row["name"] for row in sample_rows)
        first = sample_rows[0]
        masks = ",".join(
            f"{_hex(row['dx'], 4)}/{_hex(row['dx_mask_0x30'], 2)}/"
            f"{_hex(row['dx_mask_0x20'], 2)}/{_hex(row['dx_mask_0x40'], 2)}"
            for row in sample_rows
        )
        last = sample_rows[-1]
        print(
            f"{scenario} {sequence} {first['player_x'] if first['player_x'] is not None else '-'} "
            f"{first['player_y'] if first['player_y'] is not None else '-'} {path} "
            f"{masks} {_hex(last['return_al'], 2)} {_hex(last['object_byte_0x3a'], 2)}",
            file=stream,
        )


CSV_FIELDS = [
    "trace", "scenario", "sequence", "player_x", "player_y", "offset", "name",
    "dx", "dx_mask_0x30", "dx_mask_0x20", "dx_mask_0x40", "object_byte_0x3a",
    "return_offset", "return_al",
]


def write_csv(rows: list[dict[str, Any]], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=CSV_FIELDS, lineterminator="\n")
        writer.writeheader()
        writer.writerows({key: row.get(key) for key in CSV_FIELDS} for row in rows)


def write_json(rows: list[dict[str, Any]], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps({"schema": "quiky-player-branch-v1", "rows": rows},
                               indent=2) + "\n", encoding="utf-8")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("traces", nargs="+", type=Path)
    parser.add_argument("--label", action="append", dest="labels")
    parser.add_argument("--csv-output", type=Path)
    parser.add_argument("--json-output", type=Path)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        rows = branch_rows(args.traces, args.labels)
        render_report(rows, sys.stdout)
        if args.csv_output is not None:
            write_csv(rows, args.csv_output)
            print(f"wrote branch CSV to {args.csv_output}")
        if args.json_output is not None:
            write_json(rows, args.json_output)
            print(f"wrote branch JSON to {args.json_output}")
    except BranchReportError as exc:
        print(f"player-branch-report: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
