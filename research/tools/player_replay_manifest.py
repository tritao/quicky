#!/usr/bin/env python3
"""Convert a captured DOS player trace into a complete-record replay manifest.

The manifest is deliberately line-oriented because ``quiky-player-trace`` is
also used in small build environments without a JSON library. Every row
contains the captured callback pre-state and action word. Globals with a
confirmed name mapping are restored; ``?`` preserves an unmapped value as an
explicit replay boundary.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Iterable


FIELDS = (
    "sequence",
    "action_flags",
    "deferred_y",
    "external_x_delta",
    "timer_clear",
    "input_run_counter",
    "horizontal_accumulator",
    "view_state_a",
    "view_state_b",
    "horizontal_accel",
    "idle_counter",
    "action_low_copy",
    "pending_event",
    "camera_x",
    "camera_y",
    "camera_y_limit",
    "action_source",
    "activation_state",
    "speed_cap_mode",
    "action_suppressor",
    "transition_mode",
    "pre_record_hex",
)

# These are the only source-name mappings promoted here. The remaining
# callback globals retain '?' until a trace or static contract identifies
# their source field. See PlayerCallbackGlobals in player_update.h.
SOURCE_GLOBALS = {
    "deferred_y": "player_vertical_adjust",
    "input_run_counter": "horizontal_branch_counter",
    # The trace names come from the existing capture schema; the address
    # ledger is authoritative for replay.  4FE2 is the limit word, 4FE8 is
    # the accumulator, and 4FE6 is the published auxiliary/view word.
    "horizontal_accumulator": "horizontal_limit",
    "view_state_b": "horizontal_aux",
    "horizontal_accel": "horizontal_accumulator",
    "idle_counter": "horizontal_timer",
    "action_low_copy": "horizontal_result_byte",
    "camera_x": "camera_x",
    "camera_y": "camera_y",
}


class ReplayManifestError(Exception):
    pass


def samples_from_payload(payload: dict[str, Any]) -> list[dict[str, Any]]:
    events = payload.get("events")
    if isinstance(events, list):
        if len(events) != 1 or not isinstance(events[0], dict):
            raise ReplayManifestError("events must contain one object")
        payload = events[0]
    samples = payload.get("samples")
    if not isinstance(samples, list):
        raise ReplayManifestError("trace has no samples array")
    return [sample for sample in samples if isinstance(sample, dict)]


def integer(value: Any, label: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise ReplayManifestError(f"{label} must be an integer")
    return value


def callback_record(sample: dict[str, Any], which: str) -> str:
    callback = sample.get("player_callback")
    if not isinstance(callback, dict):
        raise ReplayManifestError("sample has no player_callback object")
    object_name = "pre_object" if which == "pre" else "post_object"
    obj = callback.get(object_name)
    if isinstance(obj, dict) and isinstance(obj.get("state_hex"), str):
        return obj["state_hex"]
    direct = callback.get(f"{which}_record_hex")
    if isinstance(direct, str):
        return direct
    raise ReplayManifestError(f"sample is missing complete {which} record")


def action_flags(sample: dict[str, Any]) -> int:
    globals_value = sample.get("globals")
    if isinstance(globals_value, dict) and "input_action_flags" in globals_value:
        return integer(globals_value["input_action_flags"], "input_action_flags")
    raise ReplayManifestError(
        "sample is missing globals.input_action_flags; action is not guessed from the record"
    )


def row_for(sample: dict[str, Any]) -> tuple[list[str], list[str]]:
    sequence = integer(sample.get("sequence"), "sequence")
    flags = action_flags(sample)
    if not 0 <= flags <= 0xFFFF:
        raise ReplayManifestError("input_action_flags is outside uint16 range")
    record = callback_record(sample, "pre").lower()
    if len(record) != 0x78 * 2:
        raise ReplayManifestError("pre player record is not exactly 0x78 bytes")
    try:
        bytes.fromhex(record)
    except ValueError as exc:
        raise ReplayManifestError("pre player record is not hexadecimal") from exc

    source = sample.get("globals")
    if not isinstance(source, dict):
        source = {}
    values = [str(sequence), str(flags)]
    unmapped: list[str] = []
    for field in FIELDS[2:-1]:
        source_name = SOURCE_GLOBALS.get(field)
        if source_name is None:
            values.append("?")
            continue
        if source_name not in source:
            values.append("?")
            unmapped.append(f"{field}<-{source_name}")
        else:
            values.append(str(integer(source[source_name], source_name)))
    values.append(record)
    return values, unmapped


def build_manifest(path: Path) -> dict[str, Any]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ReplayManifestError(f"cannot read trace: {exc}") from exc
    if not isinstance(payload, dict):
        raise ReplayManifestError("trace top-level value must be an object")

    rows: list[list[str]] = []
    missing: dict[str, list[str]] = {}
    for sample in samples_from_payload(payload):
        values, unmapped = row_for(sample)
        rows.append(values)
        if unmapped:
            missing[str(values[0])] = unmapped
    if not rows:
        raise ReplayManifestError("trace contains no object samples")
    return {
        "schema": "quiky.player-replay-v1",
        "source": str(path),
        "fields": list(FIELDS),
        "rows": rows,
        "unresolved_fields": [
            field for field in FIELDS[2:-1] if field not in SOURCE_GLOBALS
        ],
        "unmapped_globals": missing,
    }


def write_tsv(manifest: dict[str, Any], path: Path) -> None:
    lines = [
        "# quiky.player-replay-v1",
        "# " + " ".join(manifest["fields"]),
        "# unresolved_global_fields " + " ".join(manifest["unresolved_fields"]),
    ]
    for row in manifest["rows"]:
        lines.append(" ".join(row))
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(list(argv) if argv is not None else None)
    try:
        manifest = build_manifest(args.original)
        write_tsv(manifest, args.output)
    except ReplayManifestError as exc:
        print(f"player-replay-manifest: {exc}", file=sys.stderr)
        return 2
    print(json.dumps({
        "rows": len(manifest["rows"]),
        "unresolved_fields": manifest["unresolved_fields"],
        "unmapped_globals": manifest["unmapped_globals"],
        "output": str(args.output),
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
