#!/usr/bin/env python3
"""Correlate the player object in a quikytrace player-pool ledger.

The guest probe records DS:881A as ``player_object_offset``.  This tool joins
that offset to the pool snapshots and deliberately keeps the result separate
from engine assumptions: the report shows the callback/slot evidence, while
CSV export requires explicit grounded/facing values because the current probe
has not proven those fields yet.
"""

from __future__ import annotations

import argparse
import csv
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, TextIO


class TraceReportError(Exception):
    """Raised when a player trace is incomplete or inconsistent."""


@dataclass(frozen=True)
class PlayerSample:
    sequence: int
    frame_index: int
    selector: int
    offset: int
    object: dict[str, Any]
    globals: dict[str, Any]
    callback: dict[str, Any] | None = None
    collision: dict[str, Any] | None = None
    map_lookup: dict[str, Any] | None = None
    collisions: list[dict[str, Any]] | None = None
    map_lookups: list[dict[str, Any]] | None = None


def _as_int(value: Any, field: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise TraceReportError(f"{field} must be an integer")
    return value


def _trace_payload(path: Path) -> dict[str, Any]:
    try:
        ledger = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise TraceReportError(f"{path}: cannot read JSON trace: {exc}") from exc
    if isinstance(ledger.get("events"), list):
        events = ledger["events"]
        if len(events) != 1 or not isinstance(events[0], dict):
            raise TraceReportError(f"{path}: expected exactly one player trace event")
        return events[0]
    if isinstance(ledger.get("samples"), list):
        return ledger
    raise TraceReportError(f"{path}: no player trace event or samples array")


def _objects(sample: dict[str, Any]) -> Iterable[dict[str, Any]]:
    pool = sample.get("pool")
    if not isinstance(pool, dict):
        raise TraceReportError("sample has no pool object")
    objects = pool.get("objects")
    if not isinstance(objects, list):
        raise TraceReportError("sample pool has no objects array")
    for object_state in objects:
        if not isinstance(object_state, dict):
            raise TraceReportError("sample pool contains a non-object entry")
        yield object_state


def correlate_samples(trace: dict[str, Any], offset: int | None = None) -> list[PlayerSample]:
    """Join each sample's DS:881A value to its pool object."""
    samples = trace.get("samples")
    if not isinstance(samples, list) or not samples:
        raise TraceReportError("player trace has no samples")

    result: list[PlayerSample] = []
    for sample in samples:
        if not isinstance(sample, dict):
            raise TraceReportError("player trace contains a non-object sample")
        globals_state = sample.get("globals")
        pool = sample.get("pool")
        if not isinstance(globals_state, dict) or not isinstance(pool, dict):
            raise TraceReportError("sample is missing globals or pool")
        target_offset = offset
        if target_offset is None:
            raw_offset = globals_state.get("player_object_offset")
            if raw_offset is None:
                raise TraceReportError(
                    "sample has no player_object_offset; pass --offset only for a reviewed trace"
                )
            target_offset = _as_int(raw_offset, "player_object_offset")
        matches = [
            object_state for object_state in _objects(sample)
            if object_state.get("offset") == target_offset
        ]
        if len(matches) != 1:
            sequence = sample.get("sequence", "?")
            raise TraceReportError(
                f"sample {sequence}: expected one object at offset "
                f"0x{target_offset:04x}, found {len(matches)}"
            )
        selector = _as_int(pool.get("selector"), "pool.selector")
        sequence = _as_int(sample.get("sequence"), "sample.sequence")
        frame_index = sample.get("frame_index", sequence - 1)
        frame_index = _as_int(frame_index, "sample.frame_index")
        result.append(PlayerSample(
            sequence=sequence,
            frame_index=frame_index,
            selector=selector,
            offset=target_offset,
            object=matches[0],
            globals=globals_state,
            callback=sample.get("player_callback")
            if isinstance(sample.get("player_callback"), dict) else None,
            collision=sample.get("collision") if isinstance(sample.get("collision"), dict) else None,
            map_lookup=sample.get("map_lookup") if isinstance(sample.get("map_lookup"), dict) else None,
            collisions=sample.get("collisions") if isinstance(sample.get("collisions"), list) else None,
            map_lookups=sample.get("map_lookups") if isinstance(sample.get("map_lookups"), list) else None,
        ))
    return result


def _hex_values(samples: Iterable[PlayerSample], field: str) -> str:
    values = []
    for sample in samples:
        value = sample.object.get(field)
        if isinstance(value, int) and value not in values:
            values.append(value)
    return ",".join(f"0x{value:04x}" for value in values) or "-"


def _position(sample: PlayerSample, axis: str) -> int:
    position = sample.object.get("position")
    if not isinstance(position, dict):
        raise TraceReportError(f"sample {sample.sequence}: object has no position")
    return _as_int(position.get(axis), f"position.{axis}")


def _fixed_position(sample: PlayerSample, axis: str) -> int:
    position = sample.object.get("position")
    if not isinstance(position, dict):
        raise TraceReportError(f"sample {sample.sequence}: object has no position")
    return _as_int(position.get(f"{axis}_fixed"), f"position.{axis}_fixed")


def render_report(samples: list[PlayerSample], stream: TextIO) -> None:
    first, last = samples[0], samples[-1]
    selector = first.selector
    offset = first.offset
    selector_consistent = all(sample.selector == selector for sample in samples)
    offset_consistent = all(sample.offset == offset for sample in samples)
    print(f"trace_samples={len(samples)}", file=stream)
    print(f"player_identity=0x{selector:04x}:0x{offset:04x}", file=stream)
    print(f"identity_consistent={int(selector_consistent and offset_consistent)}", file=stream)
    print(
        f"position_first={_position(first, 'x')},{_position(first, 'y')} "
        f"position_last={_position(last, 'x')},{_position(last, 'y')} "
        f"delta={_position(last, 'x') - _position(first, 'x')},"
        f"{_position(last, 'y') - _position(first, 'y')}",
        file=stream,
    )
    print(f"callbacks={_hex_values(samples, 'callback')}", file=stream)
    print(f"sprite_slots={_hex_values(samples, 'sprite_slot')}", file=stream)
    print(f"kinds={_hex_values(samples, 'kind')}", file=stream)
    record_sizes = sorted({
        callback.get("record_size")
        for sample in samples
        for callback in [sample.callback]
        if isinstance(callback, dict) and isinstance(callback.get("record_size"), int)
    })
    print(
        "callback_record_sizes=" + ",".join(str(value) for value in record_sizes)
        if record_sizes else "callback_record_sizes=-",
        file=stream,
    )
    callback_rows = []
    for sample in samples:
        callback = sample.callback
        if not isinstance(callback, dict):
            continue
        writes = callback.get("writes")
        if not isinstance(writes, list):
            writes = []
        offsets = [
            item.get("offset") for item in writes
            if isinstance(item, dict) and isinstance(item.get("offset"), int)
        ]
        callback_rows.append(
            f"{sample.frame_index}:" + ",".join(str(offset) for offset in offsets)
        )
    print(
        "callback_write_offsets=" + ";".join(callback_rows)
        if callback_rows else "callback_write_offsets=-",
        file=stream,
    )
    tail_states = []
    for sample in samples:
        state = tuple(
            sample.object.get(f"player_byte_0x{offset:02x}", "?")
            for offset in range(0x36, 0x3c)
        ) + (sample.object.get("player_word_0x3e", "?"),)
        if state not in tail_states:
            tail_states.append(state)
    print(
        "player_tail_states=" + ";".join(
            ",".join(f"{value:02x}" if isinstance(value, int) else str(value)
                      for value in state)
            for state in tail_states
        ) if tail_states else "player_tail_states=-",
        file=stream,
    )
    collision_values = []
    map_values = []
    for sample in samples:
        collision_states = sample.collisions or (
            [sample.collision] if isinstance(sample.collision, dict) else []
        )
        for collision in collision_states:
            if not isinstance(collision, dict):
                continue
            helper = collision.get("helper_offset")
            if isinstance(helper, int) and helper not in collision_values:
                collision_values.append(helper)
        map_states = sample.map_lookups or (
            [sample.map_lookup] if isinstance(sample.map_lookup, dict) else []
        )
        for lookup in map_states:
            if not isinstance(lookup, dict):
                continue
            tile_id = lookup.get("tile_id")
            if isinstance(tile_id, int) and tile_id not in map_values:
                map_values.append(tile_id)
    print(
        "collision_helpers=" + (",").join(
            f"0x{value:04x}" for value in collision_values
        ) if collision_values else "collision_helpers=-",
        file=stream,
    )
    print(
        "map_tile_ids=" + (",").join(
            f"0x{value:03x}" for value in map_values
        ) if map_values else "map_tile_ids=-",
        file=stream,
    )
    print(
        "camera_values=" + ",".join(
            f"{sample.globals.get('camera_x', '?')}:{sample.globals.get('camera_y', '?')}"
            for sample in samples
        ),
        file=stream,
    )
    print(
        "input_flags=" + ",".join(
            str(sample.globals.get("input_action_flags", "?"))
            for sample in samples
        ),
        file=stream,
    )
    puzzle_masks = []
    for sample in samples:
        value = sample.globals.get("puzzle_mask_60d8")
        if isinstance(value, int) and value not in puzzle_masks:
            puzzle_masks.append(value)
    print(
        "puzzle_masks_60d8=" + ",".join(f"0x{value:04x}" for value in puzzle_masks)
        if puzzle_masks else "puzzle_masks_60d8=-",
        file=stream,
    )


def _trunc_divide(value: int, divisor: int) -> int:
    if value >= 0:
        return value // divisor
    return -((-value) // divisor)


def write_calibration_csv(
    samples: list[PlayerSample], path: Path, frames_between: int,
    grounded: int, facing: str,
) -> None:
    if frames_between < 1:
        raise TraceReportError("frames_between must be positive for CSV export")
    if grounded not in (0, 1):
        raise TraceReportError("grounded must be 0 or 1")
    if facing not in ("left", "right"):
        raise TraceReportError("facing must be left or right")
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        output.write(
            f"# player_identity=0x{samples[0].selector:04x}:0x{samples[0].offset:04x}\n"
            f"# velocity_interval_frames={frames_between}\n"
            "# grounded/facing are explicit provisional values; the guest probe does not prove their fields yet\n"
        )
        writer = csv.writer(output, lineterminator="\n")
        writer.writerow(["phase", "step", "x", "y", "vx_raw", "vy_raw", "grounded", "facing"])
        for index, sample in enumerate(samples):
            if index == 0:
                velocity_x = velocity_y = 0
            else:
                velocity_x = _trunc_divide(
                    _fixed_position(sample, "x") - _fixed_position(samples[index - 1], "x"),
                    frames_between,
                )
                velocity_y = _trunc_divide(
                    _fixed_position(sample, "y") - _fixed_position(samples[index - 1], "y"),
                    frames_between,
                )
            writer.writerow([
                "dosbox", sample.sequence, _position(sample, "x"), _position(sample, "y"),
                velocity_x, velocity_y, grounded, facing,
            ])


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("--offset", type=lambda value: int(value, 0),
                        help="reviewed object offset when DS:881A is absent")
    parser.add_argument("--csv-output", type=Path,
                        help="also write the correlated positions/velocities as calibration CSV")
    parser.add_argument("--frames-between", type=int, default=1,
                        help="guest frames between pool samples for velocity derivation")
    parser.add_argument("--grounded", type=int,
                        help="explicit provisional grounded value for CSV export (0 or 1)")
    parser.add_argument("--facing", choices=("left", "right"),
                        help="explicit provisional facing value for CSV export")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        samples = correlate_samples(_trace_payload(args.trace), args.offset)
        render_report(samples, sys.stdout)
        if args.csv_output is not None:
            if args.grounded is None or args.facing is None:
                raise TraceReportError(
                    "--csv-output requires both --grounded and --facing; "
                    "those fields are not yet proven by the guest trace"
                )
            write_calibration_csv(
                samples, args.csv_output, args.frames_between,
                args.grounded, args.facing,
            )
            print(f"wrote calibration CSV to {args.csv_output}")
    except TraceReportError as exc:
        print(f"player-trace-report: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
