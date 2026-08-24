#!/usr/bin/env python3
"""Summarize the ordered player collision-helper calls in quikytrace ledgers.

The guest probe records one event for each helper while the player callback is
paused.  This report keeps that order (and the registers at each boundary)
instead of collapsing the calls into a single ``collision_helpers`` set.  The
ordered path is the useful input for the eventual compatible collision model.
"""

from __future__ import annotations

import argparse
import csv
import json
import sys
from pathlib import Path
from typing import Any, TextIO


class CollisionReportError(Exception):
    """Raised when a collision trace is incomplete or malformed."""


HELPER_NAMES = {
    0x648E: "player_collision_648e",
    0x6484: "player_collision_6484",
    0x3A8A: "vertical_collision_3a8a",
    0x3A1F: "property_probe_3a1f",
    0x3DF2: "descriptor_probe_3df2",
}


def _as_int(value: Any, field: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise CollisionReportError(f"{field} must be an integer")
    return value


def _trace_payload(path: Path) -> dict[str, Any]:
    try:
        ledger = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise CollisionReportError(f"{path}: cannot read JSON trace: {exc}") from exc
    events = ledger.get("events")
    if isinstance(events, list):
        if len(events) != 1 or not isinstance(events[0], dict):
            raise CollisionReportError(f"{path}: expected one trace event")
        return events[0]
    if isinstance(ledger.get("samples"), list):
        return ledger
    raise CollisionReportError(f"{path}: no samples array")


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


def _u16(registers: dict[str, Any], name: str) -> int | None:
    value = registers.get(name)
    return value & 0xFFFF if isinstance(value, int) else None


def _tail(object_state: Any) -> dict[str, int | None]:
    if not isinstance(object_state, dict):
        return {f"object_0x{offset:02x}": None for offset in range(0x36, 0x3C)}
    return {
        f"object_0x{offset:02x}": (
            object_state.get(f"player_byte_0x{offset:02x}")
            if isinstance(object_state.get(f"player_byte_0x{offset:02x}"), int)
            else None
        )
        for offset in range(0x36, 0x3C)
    }


def collision_rows(paths: list[Path], labels: list[str] | None = None) -> list[dict[str, Any]]:
    """Flatten collision events to event-level rows in observed order."""
    if labels is not None and len(labels) != len(paths):
        raise CollisionReportError("labels must match the number of trace paths")
    rows: list[dict[str, Any]] = []
    for index, path in enumerate(paths):
        scenario = labels[index] if labels is not None else path.stem
        trace = _trace_payload(path)
        samples = trace.get("samples")
        if not isinstance(samples, list):
            raise CollisionReportError(f"{path}: samples must be an array")
        for sample in samples:
            if not isinstance(sample, dict):
                raise CollisionReportError(f"{path}: sample must be an object")
            sequence = _as_int(sample.get("sequence"), "sample.sequence")
            frame_index = sample.get("frame_index", 0)
            if not isinstance(frame_index, int):
                raise CollisionReportError(f"{path}: sample.frame_index must be an integer")
            player_x, player_y = _player_position(sample)
            events = sample.get("collisions")
            if events is None:
                event = sample.get("collision")
                events = [event] if isinstance(event, dict) else []
            if not isinstance(events, list):
                raise CollisionReportError(f"{path}: collisions must be an array")
            for order, event in enumerate(events):
                if not isinstance(event, dict):
                    raise CollisionReportError(f"{path}: collision event must be an object")
                helper = _as_int(event.get("helper_offset"), "collision.helper_offset")
                registers = event.get("registers", {})
                if not isinstance(registers, dict):
                    raise CollisionReportError(f"{path}: collision.registers must be an object")
                object_state = event.get("object")
                return_state = event.get("return_breakpoint")
                return_registers = (return_state.get("registers", {})
                                   if isinstance(return_state, dict) else {})
                if not isinstance(return_registers, dict):
                    raise CollisionReportError(
                        f"{path}: collision.return_breakpoint.registers must be an object"
                    )
                map_property = event.get("map_property")
                map_lookup = (map_property.get("map_lookup", {})
                              if isinstance(map_property, dict) else {})
                if not isinstance(map_lookup, dict):
                    raise CollisionReportError(
                        f"{path}: collision.map_property.map_lookup must be an object"
                    )
                row = {
                    "trace": str(path),
                    "scenario": scenario,
                    "sequence": sequence,
                    "frame_index": frame_index,
                    "helper_order": order,
                    "helper_offset": helper,
                    "helper_name": HELPER_NAMES.get(helper, f"unknown_{helper:04x}"),
                    "player_x": player_x,
                    "player_y": player_y,
                    "eax": _u16(registers, "eax"),
                    "ebx": _u16(registers, "ebx"),
                    "ecx": _u16(registers, "ecx"),
                    "edx": _u16(registers, "edx"),
                    "event_index": event.get("event_index"),
                    "return_offset": (return_state.get("offset")
                                       if isinstance(return_state, dict) else None),
                    "return_ax": _u16(return_registers, "eax"),
                    "return_dx": _u16(return_registers, "edx"),
                    "return_flags": (return_registers.get("flags")
                                      if isinstance(return_registers.get("flags"), int)
                                      else None),
                    "map_x": (map_lookup.get("x")
                              if isinstance(map_lookup.get("x"), int) else None),
                    "map_y": (map_lookup.get("y")
                              if isinstance(map_lookup.get("y"), int) else None),
                    "map_cell_word": (map_lookup.get("cell_word")
                                       if isinstance(map_lookup.get("cell_word"), int)
                                       else None),
                    "map_tile_id": (map_lookup.get("tile_id")
                                     if isinstance(map_lookup.get("tile_id"), int)
                                     else None),
                    "descriptor_word": (map_property.get("descriptor_word")
                                         if isinstance(map_property, dict) and
                                         isinstance(map_property.get("descriptor_word"), int)
                                         else None),
                }
                row.update(_tail(object_state))
                rows.append(row)
    return rows


def _hex(value: Any, width: int = 4) -> str:
    return "-" if value is None else f"0x{value:0{width}x}"


def render_report(rows: list[dict[str, Any]], stream: TextIO) -> None:
    samples = sorted({(row["scenario"], row["sequence"]) for row in rows})
    print(f"collision_rows={len(rows)}", file=stream)
    print(f"collision_samples={len(samples)}", file=stream)
    print("scenario sequence frame player_x player_y helper_path", file=stream)
    for scenario, sequence in samples:
        sample_rows = [row for row in rows
                       if row["scenario"] == scenario and row["sequence"] == sequence]
        path = ">".join(row["helper_name"] for row in sample_rows)
        first = sample_rows[0]
        x = first["player_x"] if first["player_x"] is not None else "-"
        y = first["player_y"] if first["player_y"] is not None else "-"
        print(f"{scenario} {sequence} {first['frame_index']} {x} {y} {path}", file=stream)
    helpers = sorted({row["helper_offset"] for row in rows})
    print("helpers=" + ",".join(_hex(value) for value in helpers) if helpers
          else "helpers=-", file=stream)


CSV_FIELDS = [
    "trace", "scenario", "sequence", "frame_index", "helper_order",
    "helper_offset", "helper_name", "player_x", "player_y",
    "eax", "ebx", "ecx", "edx", "event_index",
    "return_offset", "return_ax", "return_dx", "return_flags",
    "map_x", "map_y", "map_cell_word", "map_tile_id", "descriptor_word",
    "object_0x36", "object_0x37", "object_0x38", "object_0x39",
    "object_0x3a", "object_0x3b",
]


def write_csv(rows: list[dict[str, Any]], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=CSV_FIELDS, lineterminator="\n")
        writer.writeheader()
        writer.writerows({key: row.get(key) for key in CSV_FIELDS} for row in rows)


def write_json(rows: list[dict[str, Any]], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps({"schema": "quiky-player-collision-v1", "rows": rows},
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
        rows = collision_rows(args.traces, args.labels)
        render_report(rows, sys.stdout)
        if args.csv_output is not None:
            write_csv(rows, args.csv_output)
            print(f"wrote collision CSV to {args.csv_output}")
        if args.json_output is not None:
            write_json(rows, args.json_output)
            print(f"wrote collision JSON to {args.json_output}")
    except CollisionReportError as exc:
        print(f"player-collision-report: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
