#!/usr/bin/env python3
"""Normalize natural W1L3--W5L3 lifecycle traces for the optional audit."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def summarize(path: Path) -> dict:
    report = json.loads(path.read_text(encoding="utf-8"))
    result = report.get("result", {})
    events = sorted(result.get("events", {}).values(),
                    key=lambda event: event.get("sequence", 0))
    names = [event.get("name", "") for event in events]
    secondary_names = [name for name in names
                       if "3861" in name or "secondary" in name.lower()]
    return {
        "level": report.get("level"),
        "source": str(path),
        "sha256": sha256(path),
        "script_sha256": report.get("script_sha256"),
        "executable_sha256": report.get("executable_sha256"),
        "archive_sha256": report.get("archive_sha256"),
        "event_count": len(events),
        "event_names": names,
        "terminal_event": names[-1] if names else None,
        "secondary_loader_events": secondary_names,
        "timer_wait_barrier": {
            "entered": "timer_wait_entry" in names,
            "cleared": "timer_wait_clear_flag" in names,
            "tested": "timer_wait_test_flag" in names,
            "yielded": "timer_wait_yield" in names,
            "transition_wait_entered": "transition_wait_clear" in names,
            "transition_wait_tested": "transition_wait_test" in names,
        },
        "natural_run_status": (
            "secondary_loader_reached" if secondary_names else
            "stopped_at_timer_transition_barrier"
        ),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", action="append", dest="inputs", type=Path,
                        required=True, help="one raw lifecycle trace (repeatable)")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    summaries = [summarize(path) for path in args.inputs]
    levels = [summary["level"] for summary in summaries]
    scripts = sorted({summary["script_sha256"] for summary in summaries})
    executables = sorted({summary["executable_sha256"] for summary in summaries})
    archives = sorted({summary["archive_sha256"] for summary in summaries})
    all_barrier = all(summary["natural_run_status"] ==
                      "stopped_at_timer_transition_barrier"
                      for summary in summaries)
    result = {
        "schema": "quiky-optional-secondary-lifecycle-matrix-v1",
        "levels": levels,
        "traces": summaries,
        "common_inputs": {
            "script_sha256": scripts,
            "executable_sha256": executables,
            "archive_sha256": archives,
        },
        "acceptance": {
            "required": "natural W1L3-W5L3 runs either reach 01D7:3861 or reproducibly stop at a shared transition barrier",
            "current_result": (
                "all five levels stop at the timer/transition barrier without a secondary-loader event"
                if all_barrier else
                "mixed result; inspect per-level traces"
            ),
            "stop_rule": "do not infer a gameplay trigger from an uninstrumented run that never reaches 01D7:3861",
        },
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(args.output), "sha256": sha256(args.output),
                      "levels": levels, "all_barrier": all_barrier}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
