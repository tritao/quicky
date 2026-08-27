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


CORE_FIELDS = (
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

# A complete callback capture also carries the address-qualified state read by
# 01F7:5937. Keep this as an explicit schema extension so older movement-only
# fixtures remain replayable while a dispatch-bearing fixture cannot silently
# drop any of the 5937 inputs.
DISPATCH_FIELDS = (
    "dispatch_gate_85da",
    "dispatch_word_60d8",
    "dispatch_previous_word_60da",
    "dispatch_score_low_881c",
    "dispatch_score_high_881e",
    "dispatch_lives_880a",
    "dispatch_ammo_880c",
    "dispatch_health_8822",
    "dispatch_aux_4ff2",
    "dispatch_aux_4ff6",
    "dispatch_aux_4ff8",
    "dispatch_aux_4ffa",
)

# Keep the old public name for callers/tests that intentionally request the
# core movement schema.
FIELDS = CORE_FIELDS
EXTENDED_FIELDS = CORE_FIELDS[:-1] + DISPATCH_FIELDS + (CORE_FIELDS[-1],)

# These mappings are the callback-global names promoted by the static
# contract.  A missing source value remains an explicit replay failure; the
# manifest never substitutes a value inferred from the player record.
SOURCE_GLOBALS = {
    "deferred_y": "player_vertical_adjust",
    "external_x_delta": "external_x_delta",
    "timer_clear": "timer_clear",
    "input_run_counter": "horizontal_branch_counter",
    # The trace names come from the existing capture schema; the address
    # ledger is authoritative for replay.  4FE2 is the limit word, 4FE8 is
    # the accumulator, and 4FE6 is the published auxiliary/view word.
    "horizontal_accumulator": "horizontal_limit",
    "view_state_b": "horizontal_aux",
    "view_state_a": "view_state_a",
    "horizontal_accel": "horizontal_accumulator",
    "idle_counter": "horizontal_timer",
    "action_low_copy": "horizontal_result_byte",
    "pending_event": "pending_event",
    "camera_x": "camera_x",
    "camera_y": "camera_y",
    "camera_y_limit": "camera_y_limit",
    "action_source": "action_source",
    "activation_state": "activation_state",
    "speed_cap_mode": "speed_cap_mode",
    "action_suppressor": "action_suppressor",
    "transition_mode": "transition_mode",
}

DISPATCH_SOURCE_GLOBALS = {
    field: field for field in DISPATCH_FIELDS
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
    if isinstance(globals_value, dict):
        # Static 01F7:F21B/F21C returns DS:88BC | DS:8196.  The tracer
        # publishes those words separately: keyboard_action_flags is the
        # physical/scripted keyboard word at DS:88BC and input_action_flags
        # is the auxiliary word at DS:8196.  Replaying only the latter loses
        # jump and other held-input edges when the keyboard word is nonzero.
        keyboard = globals_value.get("keyboard_action_flags")
        auxiliary = globals_value.get("input_action_flags")
        if keyboard is not None and auxiliary is not None:
            keyboard_value = integer(keyboard, "keyboard_action_flags")
            auxiliary_value = integer(auxiliary, "input_action_flags")
            return (keyboard_value | auxiliary_value) & 0xffff
        if auxiliary is not None:
            return integer(auxiliary, "input_action_flags")
        if keyboard is not None:
            return integer(keyboard, "keyboard_action_flags")
    raise ReplayManifestError(
        "sample is missing both input action words; action is not guessed from the record"
    )


def sample_globals(sample: dict[str, Any]) -> dict[str, Any]:
    source = sample.get("globals")
    if isinstance(source, dict):
        return source
    callback = sample.get("player_callback")
    if isinstance(callback, dict) and isinstance(callback.get("pre_globals"), dict):
        return callback["pre_globals"]
    return {}


def row_for(
    sample: dict[str, Any],
    fields: tuple[str, ...] = FIELDS,
    source_globals: dict[str, str] = SOURCE_GLOBALS,
) -> tuple[list[str], list[str]]:
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

    source = sample_globals(sample)
    values = [str(sequence), str(flags)]
    unmapped: list[str] = []
    for field in fields[2:-1]:
        source_name = source_globals.get(field)
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

    source_samples = samples_from_payload(payload)
    if not source_samples:
        raise ReplayManifestError("trace contains no object samples")

    # Presence of any 5937 address-qualified field promotes the entire input
    # to the extended schema. This is deliberately based on captured data,
    # never on a default value inferred from the player record.
    has_dispatch_state = any(
        any(field in sample_globals(sample) for field in DISPATCH_FIELDS)
        for sample in source_samples
    )
    fields = EXTENDED_FIELDS if has_dispatch_state else FIELDS
    source_globals = dict(SOURCE_GLOBALS)
    if has_dispatch_state:
        source_globals.update(DISPATCH_SOURCE_GLOBALS)

    rows: list[list[str]] = []
    missing: dict[str, list[str]] = {}
    for sample in source_samples:
        values, unmapped = row_for(sample, fields, source_globals)
        rows.append(values)
        if unmapped:
            missing[str(values[0])] = unmapped
    if has_dispatch_state and missing:
        raise ReplayManifestError(
            "extended 5937 replay is missing required callback-global data: "
            + json.dumps(missing, sort_keys=True)
        )
    return {
        "schema": ("quiky.player-replay-v2" if has_dispatch_state
                   else "quiky.player-replay-v1"),
        "source": str(path),
        "fields": list(fields),
        "rows": rows,
        "unresolved_fields": [
            field for field in fields[2:-1] if field not in source_globals
        ],
        "unmapped_globals": missing,
    }


def write_tsv(manifest: dict[str, Any], path: Path) -> None:
    lines = [
        "# " + manifest["schema"],
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
