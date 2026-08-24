#!/usr/bin/env python3
"""Summarize MAP descriptor queries from player property traces.

The player Lua probe records the raw MAP cell and the descriptor lookup at
01F7:5C27/5CC3.  This report keeps the archive's upper MAP field beside the
masked tile ID and descriptor word so the two representations cannot be
confused during later collision analysis.
"""

from __future__ import annotations

import argparse
import csv
import json
import sys
from pathlib import Path
from typing import Any, TextIO


class PropertyReportError(Exception):
    """Raised when a property trace is incomplete or malformed."""


def _as_int(value: Any, field: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise PropertyReportError(f"{field} must be an integer")
    return value


def _payload(path: Path) -> dict[str, Any]:
    try:
        ledger = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise PropertyReportError(f"{path}: cannot read JSON trace: {exc}") from exc
    events = ledger.get("events")
    if isinstance(events, list):
        if len(events) != 1 or not isinstance(events[0], dict):
            raise PropertyReportError(f"{path}: expected one trace event")
        return events[0]
    if isinstance(ledger.get("samples"), list):
        return ledger
    raise PropertyReportError(f"{path}: no samples array")


def _player_position(sample: dict[str, Any]) -> tuple[int | None, int | None]:
    """Join DS:881A to the pool when the trace includes the pool snapshot."""
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


def property_rows(paths: list[Path], labels: list[str] | None = None) -> list[dict[str, Any]]:
    """Return one normalized row for every recorded MAP property query."""
    if labels is not None and len(labels) != len(paths):
        raise PropertyReportError("labels must match the number of trace paths")
    rows: list[dict[str, Any]] = []
    for index, path in enumerate(paths):
        scenario = labels[index] if labels is not None else path.stem
        trace = _payload(path)
        samples = trace.get("samples")
        if not isinstance(samples, list):
            raise PropertyReportError(f"{path}: samples must be an array")
        for sample in samples:
            if not isinstance(sample, dict):
                raise PropertyReportError(f"{path}: sample must be an object")
            prop = sample.get("map_property")
            if not isinstance(prop, dict):
                continue
            coordinates = prop.get("coordinates")
            lookup = prop.get("map_lookup")
            if not isinstance(coordinates, dict) or not isinstance(lookup, dict):
                raise PropertyReportError(f"{path}: property row lacks coordinates/lookup")
            x = _as_int(coordinates.get("x"), "coordinates.x")
            y = _as_int(coordinates.get("y"), "coordinates.y")
            raw_cell = prop.get("raw_cell_word", lookup.get("cell_word"))
            tile_id = prop.get("tile_id", prop.get("descriptor_tile_id",
                                                       lookup.get("tile_id")))
            helper_offset = _as_int(prop.get("helper_offset"),
                                    "map_property.helper_offset")
            is_quadrant_query = helper_offset == 0x5C27
            row = {
                "trace": str(path),
                "scenario": scenario,
                "sequence": _as_int(sample.get("sequence"), "sample.sequence"),
                "helper_offset": helper_offset,
                "x": x,
                "y": y,
                "ax_bit_3": coordinates.get("ax_bit_3", (y >> 3) & 1),
                "bx_bit_3": coordinates.get("bx_bit_3", (x >> 3) & 1),
                "raw_cell_word": raw_cell,
                "map_property_field": prop.get("map_property_field"),
                "tile_id": tile_id,
                "descriptor_offset": prop.get("descriptor_offset"),
                "descriptor_word": prop.get("descriptor_word"),
                "descriptor_low_nibble": prop.get("descriptor_low_nibble"),
                "quadrant_flag_mask": prop.get("quadrant_flag_mask")
                    if is_quadrant_query else None,
                "quadrant_bits": prop.get("quadrant_bits")
                    if is_quadrant_query else None,
                "descriptor_flag_set": prop.get("descriptor_flag_set")
                    if is_quadrant_query else None,
            }
            row["player_x"], row["player_y"] = _player_position(sample)
            rows.append(row)
    return rows


def _hex(value: Any, width: int = 4) -> str:
    return "-" if value is None else f"0x{value:0{width}x}"


def render_report(rows: list[dict[str, Any]], stream: TextIO) -> None:
    """Render a stable, reviewable text summary."""
    helpers = sorted({row["helper_offset"] for row in rows})
    scenarios = sorted({row["scenario"] for row in rows})
    print(f"property_rows={len(rows)}", file=stream)
    print("helpers=" + ",".join(_hex(value) for value in helpers), file=stream)
    print("scenarios=" + ",".join(scenarios), file=stream)
    print(
        "scenario sequence helper x y player_x player_y raw_cell property tile descriptor mask bits",
        file=stream,
    )
    for row in rows:
        print(
            f"{row['scenario']} {row['sequence']} {_hex(row['helper_offset'])} "
            f"{row['x']} {row['y']} {row['player_x'] if row['player_x'] is not None else '-'} "
            f"{row['player_y'] if row['player_y'] is not None else '-'} "
            f"{_hex(row['raw_cell_word'])} {row['map_property_field'] if row['map_property_field'] is not None else '-'} "
            f"{_hex(row['tile_id'], 3)} {_hex(row['descriptor_word'])} "
            f"{_hex(row['quadrant_flag_mask'], 2)} {_hex(row['quadrant_bits'], 2)}",
            file=stream,
        )


CSV_FIELDS = [
    "trace", "scenario", "sequence", "helper_offset", "x", "y",
    "player_x", "player_y", "ax_bit_3", "bx_bit_3", "raw_cell_word",
    "map_property_field", "tile_id", "descriptor_offset", "descriptor_word",
    "descriptor_low_nibble", "quadrant_flag_mask", "quadrant_bits",
    "descriptor_flag_set",
]


def write_csv(rows: list[dict[str, Any]], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=CSV_FIELDS, lineterminator="\n")
        writer.writeheader()
        writer.writerows({key: row.get(key) for key in CSV_FIELDS} for row in rows)


def write_json(rows: list[dict[str, Any]], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = {"schema": "quiky-player-property-v1", "rows": rows}
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("traces", nargs="+", type=Path)
    parser.add_argument("--label", action="append", dest="labels",
                        help="scenario label, repeated once per trace")
    parser.add_argument("--csv-output", type=Path)
    parser.add_argument("--json-output", type=Path)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        rows = property_rows(args.traces, args.labels)
        render_report(rows, sys.stdout)
        if args.csv_output is not None:
            write_csv(rows, args.csv_output)
            print(f"wrote property CSV to {args.csv_output}")
        if args.json_output is not None:
            write_json(rows, args.json_output)
            print(f"wrote property JSON to {args.json_output}")
    except PropertyReportError as exc:
        print(f"player-property-report: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
