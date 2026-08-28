#!/usr/bin/env python3
"""Import a raw DOS/native trace into a named recorded-run directory.

This is intentionally an import step, not a replay step.  Historical trace
envelopes are decoded once here; the resulting run has an explicit input and
camera stream that a native frontend can consume without compatibility rules.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from quiky.runs import ToolError, stage_run_files
from quiky.trace import TraceError, load_trace


def input_rows(trace_path: Path) -> list[dict[str, object]]:
    try:
        trace = load_trace(trace_path)
    except TraceError as exc:
        raise ToolError(str(exc)) from exc
    stream = trace.payload.get("input_stream")
    if stream is None:
        events = trace.payload.get("events")
        if isinstance(events, list) and len(events) == 1 and isinstance(events[0], dict):
            stream = events[0].get("input_stream")
    if stream is not None:
        if isinstance(stream, dict):
            try:
                stream = [stream[key] for key in sorted(
                    stream, key=lambda value: int(value))]
            except (TypeError, ValueError, KeyError) as exc:
                raise ToolError(
                    f"{trace_path}: input_stream has invalid numeric keys") from exc
        if not isinstance(stream, list):
            raise ToolError(f"{trace_path}: input_stream must be an array")
        # Re-use the canonical validator through the staging boundary while
        # preserving the raw capture's explicit frame numbers.
        if any(not isinstance(row, dict) for row in stream):
            raise ToolError(f"{trace_path}: input_stream rows must be objects")
        return [dict(row) for row in stream]

    rows: list[dict[str, object]] = []
    for sample in trace.samples:
        flags = sample.input_flags
        camera = sample.camera
        if flags is None:
            raise ToolError(
                f"{trace_path}: sample {sample.sequence} has no input flags")
        if camera is None:
            raise ToolError(
                f"{trace_path}: sample {sample.sequence} has no camera")
        frame_index = sample.raw.get("frame_index", sample.sequence)
        if not isinstance(frame_index, int):
            raise ToolError(
                f"{trace_path}: sample {sample.sequence} has invalid frame_index")
        rows.append({
            "sequence": sample.sequence,
            "guest_frame": frame_index,
            "input_flags": flags,
            "camera": {"x": camera[0], "y": camera[1]},
        })
    if not rows:
        raise ToolError(f"{trace_path}: trace contains no replay samples")
    return rows


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dos-trace", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--name", required=True)
    parser.add_argument("--native-trace", type=Path)
    args = parser.parse_args(argv)
    try:
        stage_run_files(
            args.output,
            input_rows=input_rows(args.dos_trace),
            dos_state=args.dos_trace,
            native_state=args.native_trace,
            name=args.name,
        )
    except ToolError as exc:
        print(f"recorded-run: {exc}", file=sys.stderr)
        return 2
    print(f"OK: recorded run staged at {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
