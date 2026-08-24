#!/usr/bin/env python3
"""Summarize live descriptor-table and MAP census traces."""

from __future__ import annotations

import argparse
import csv
import json
import sys
from pathlib import Path
from typing import Any, TextIO


class CensusReportError(Exception):
    """Raised when a descriptor census is incomplete or malformed."""


def _payload(path: Path) -> dict[str, Any]:
    try:
        ledger = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise CensusReportError(f"{path}: cannot read JSON trace: {exc}") from exc
    events = ledger.get("events")
    if isinstance(events, list):
        if len(events) != 1 or not isinstance(events[0], dict):
            raise CensusReportError(f"{path}: expected one trace event")
        return events[0]
    if isinstance(ledger.get("descriptor_census"), dict):
        return ledger
    raise CensusReportError(f"{path}: no descriptor_census object")


def _int(value: Any, field: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise CensusReportError(f"{field} must be an integer")
    return value


def _arrays(value: Any, field: str) -> list[dict[str, Any]]:
    if not isinstance(value, list):
        raise CensusReportError(f"{field} must be an array")
    if not all(isinstance(item, dict) for item in value):
        raise CensusReportError(f"{field} entries must be objects")
    return value


def census_data(paths: list[Path], labels: list[str] | None = None) -> list[dict[str, Any]]:
    """Flatten candidate MAP cells, retaining the loaded table metadata."""
    if labels is not None and len(labels) != len(paths):
        raise CensusReportError("labels must match the number of trace paths")
    rows: list[dict[str, Any]] = []
    for index, path in enumerate(paths):
        scenario = labels[index] if labels is not None else path.stem
        trace = _payload(path)
        census = trace.get("descriptor_census")
        if not isinstance(census, dict):
            raise CensusReportError(f"{path}: descriptor_census must be an object")
        table = census.get("descriptor_table")
        loaded_map = census.get("map")
        if not isinstance(table, dict) or not isinstance(loaded_map, dict):
            raise CensusReportError(f"{path}: census lacks table or map data")
        entries = _arrays(table.get("entries", []), "descriptor_table.entries")
        by_tile = {
            _int(entry.get("tile_id"), "descriptor.tile_id"): entry
            for entry in entries
            if entry.get("word") is not None
        }
        cells = _arrays(loaded_map.get("cells", []), "map.cells")
        for cell in cells:
            descriptor = cell.get("descriptor")
            if descriptor is None or (descriptor & 0x70) == 0:
                continue
            tile_id = _int(cell.get("tile_id"), "map.cell.tile_id")
            table_entry = by_tile.get(tile_id, {})
            rows.append({
                "trace": str(path),
                "scenario": scenario,
                "x": _int(cell.get("x"), "map.cell.x"),
                "y": _int(cell.get("y"), "map.cell.y"),
                "world_x": _int(cell.get("world_x"), "map.cell.world_x"),
                "world_y": _int(cell.get("world_y"), "map.cell.world_y"),
                "cell": _int(cell.get("cell"), "map.cell.cell"),
                "property": _int(cell.get("property"), "map.cell.property"),
                "tile_id": tile_id,
                "descriptor": _int(descriptor, "map.cell.descriptor"),
                "descriptor_offset": table_entry.get("offset"),
                "mask_0x20": descriptor & 0x20,
                "mask_0x40": descriptor & 0x40,
                "mask_0x30": descriptor & 0x30,
            })
    return rows


def render_report(rows: list[dict[str, Any]], stream: TextIO) -> None:
    print(f"descriptor_candidates={len(rows)}", file=stream)
    print("scenario x y world_x world_y tile descriptor masks", file=stream)
    for row in rows:
        print(
            f"{row['scenario']} {row['x']} {row['y']} {row['world_x']} "
            f"{row['world_y']} 0x{row['tile_id']:03x} 0x{row['descriptor']:04x} "
            f"0x{row['mask_0x30']:02x}/0x{row['mask_0x20']:02x}/0x{row['mask_0x40']:02x}",
            file=stream,
        )


CSV_FIELDS = [
    "trace", "scenario", "x", "y", "world_x", "world_y", "cell",
    "property", "tile_id", "descriptor", "descriptor_offset",
    "mask_0x20", "mask_0x40", "mask_0x30",
]


def write_csv(rows: list[dict[str, Any]], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=CSV_FIELDS, lineterminator="\n")
        writer.writeheader()
        writer.writerows({key: row.get(key) for key in CSV_FIELDS} for row in rows)


def write_json(rows: list[dict[str, Any]], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps({
        "schema": "quiky-player-descriptor-census-v1",
        "rows": rows,
    }, indent=2) + "\n", encoding="utf-8")


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
        rows = census_data(args.traces, args.labels)
        try:
            render_report(rows, sys.stdout)
        except BrokenPipeError:
            return 0
        if args.csv_output is not None:
            write_csv(rows, args.csv_output)
            print(f"wrote descriptor census CSV to {args.csv_output}")
        if args.json_output is not None:
            write_json(rows, args.json_output)
            print(f"wrote descriptor census JSON to {args.json_output}")
    except CensusReportError as exc:
        print(f"descriptor-census-report: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
