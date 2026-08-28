#!/usr/bin/env python3
"""Validate and summarize the focused 01F7:16CE MAP-writer trace."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any


class ContactMapWriterError(Exception):
    pass


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def load(path: Path) -> dict[str, Any]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ContactMapWriterError(f"cannot read {path}: {exc}") from exc
    if not isinstance(payload, dict):
        raise ContactMapWriterError("trace top-level value must be an object")
    if not isinstance(payload.get("events"), list):
        raise ContactMapWriterError("trace has no events array")
    return payload


def writer_rows(payload: dict[str, Any]) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for event_index, event in enumerate(payload["events"]):
        if not isinstance(event, dict):
            raise ContactMapWriterError(f"events[{event_index}] is not an object")
        for sample_index, sample in enumerate(event.get("samples", [])):
            if not isinstance(sample, dict):
                raise ContactMapWriterError(
                    f"events[{event_index}].samples[{sample_index}] is not an object"
                )
            for watch in sample.get("execute_watches", []):
                if not isinstance(watch, dict):
                    raise ContactMapWriterError("execute watch is not an object")
                if watch.get("offset") != 0x16CE:
                    continue
                writer = watch.get("map_writer")
                if not isinstance(writer, dict):
                    raise ContactMapWriterError(
                        "01F7:16CE watch has no map_writer snapshot"
                    )
                required = (
                    "coordinates", "effect_selector", "effect_word",
                    "map_selector", "cell_offset", "before_word",
                    "after_word", "map_write_applied",
                )
                missing = [key for key in required if key not in writer]
                if missing:
                    raise ContactMapWriterError(
                        "01F7:16CE map_writer is missing " + ", ".join(missing)
                    )
                coordinates = writer["coordinates"]
                if not isinstance(coordinates, dict) or not {
                    "x", "y"
                } <= coordinates.keys():
                    raise ContactMapWriterError(
                        "01F7:16CE coordinates are incomplete"
                    )
                before = writer["before_word"]
                after = writer["after_word"]
                if bool(writer["map_write_applied"]) != (before != after):
                    raise ContactMapWriterError(
                        "01F7:16CE map_write_applied disagrees with MAP words"
                    )
                rows.append({
                    "event_index": event_index,
                    "sample_index": sample_index,
                    "sequence": sample.get("sequence"),
                    "frame_index": sample.get("frame_index"),
                    "coordinates": {
                        "x": coordinates["x"], "y": coordinates["y"]
                    },
                    "effect_selector": writer["effect_selector"],
                    "effect_word": writer["effect_word"],
                    "map_selector": writer["map_selector"],
                    "cell_offset": writer["cell_offset"],
                    "before_word": before,
                    "after_word": after,
                    "before_tile_id": writer.get("before_tile_id"),
                    "after_tile_id": writer.get("after_tile_id"),
                    "map_write_applied": bool(writer["map_write_applied"]),
                })
    if not rows:
        raise ContactMapWriterError("trace contains no 01F7:16CE writer event")
    return rows


def build_report(trace_path: Path, command: str | None = None) -> dict[str, Any]:
    payload = load(trace_path)
    rows = writer_rows(payload)
    inputs = payload.get("inputs", {})
    changed = next((row for row in rows if row["map_write_applied"]), None)
    if changed is None:
        raise ContactMapWriterError(
            "trace contains no 01F7:16CE MAP mutation to audit"
        )
    changed_summary = (
        "The first observed contact-child update changes the selected MAP "
        f"low-nine-bit tile from 0x{changed['before_tile_id']:03X} to "
        f"0x{changed['after_tile_id']:03X}."
    )
    return {
        "schema": "quiky.player-contact-map-writer.v1",
        "purpose": (
            "Focused runtime evidence for the statically recovered "
            "6328 -> 16CE contact-child feedback edge."
        ),
        "source": {
            "executable": inputs.get("executable", "game/QUIKY.EXE"),
            "executable_sha256": inputs.get("executable_sha256"),
            "archive": inputs.get("archive", "game/NESTLE.DAT"),
            "archive_sha256": inputs.get("archive_sha256"),
            "trace": str(trace_path),
            "trace_sha256": sha256(trace_path),
            "trace_script": payload.get("script"),
            "trace_script_sha256": payload.get("script_sha256"),
        },
        "method": {
            "level": inputs.get("select_level"),
            "watch": "01F7:6328 object callback plus 01F7:16CE execute watch",
            "map_patch": "W1L1 cell (8,25) low-nine-bit tile 5, persistent",
            "command": command,
        },
        "observations": {
            "writer_events": len(rows),
            "changed_events": sum(row["map_write_applied"] for row in rows),
            "unchanged_events": sum(
                not row["map_write_applied"] for row in rows
            ),
            "rows": rows,
        },
        "interpretation": {
            "confirmed": [
                "01F7:16CE receives cell X/Y in AX/BX and the replacement word in DX.",
                changed_summary,
                "A repeated contact-child update writes the same 0x1F2 value and is observationally a no-op.",
                "The controlled child path therefore has a real MAP mutation, even though 6328 itself has no player-record write.",
            ],
            "does_not_prove": [
                "that the patched contact tile is a retail W1L1 descriptor class",
                "that the resulting MAP tile changes a later solid/descriptor probe",
                "natural contact frequency or unrelated 16CE callers",
            ],
        },
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--command")
    args = parser.parse_args(argv)
    try:
        report = build_report(args.trace, args.command)
        rendered = json.dumps(report, indent=2) + "\n"
        if args.output:
            args.output.write_text(rendered, encoding="utf-8")
        else:
            print(rendered, end="")
    except (ContactMapWriterError, OSError) as exc:
        parser.error(str(exc))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
