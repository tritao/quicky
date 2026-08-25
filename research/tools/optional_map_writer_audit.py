#!/usr/bin/env python3
"""Assemble the optional runtime-generated MAP-writer caller audit."""

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


def raw_summary(path: Path) -> dict:
    report = json.loads(path.read_text(encoding="utf-8"))
    result = report.get("result", {})
    events = sorted(result.get("events", {}).values(),
                    key=lambda event: event.get("sequence", 0))
    writer_events = [event for event in events
                     if "writer" in event.get("name", "").lower()
                     or event.get("writer")]
    return {
        "level": report.get("level"),
        "source": str(path),
        "sha256": sha256(path),
        "script_sha256": report.get("script_sha256"),
        "event_count": len(events),
        "event_names": [event.get("name", "") for event in events],
        "writer_events": writer_events,
        "hits": result.get("hits", []),
        "timeout": any(event.get("timeout") for event in events),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--event-trace", type=Path, required=True)
    parser.add_argument("--evidence", type=Path,
                        default=Path("research/notes/descriptor-construction-evidence.json"))
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    evidence = json.loads(args.evidence.read_text(encoding="utf-8"))
    static = evidence["map_writer_call_form_audit"]
    runtime = {
        "event_focused_w1l3": raw_summary(args.event_trace),
        "normal_all_writer_probes": evidence.get("normal_effect_writer_probes", []),
        "bounded_helper_probes": {
            "w1l1": evidence.get("writer_focus_probe", {}),
            "w1l1_extended": evidence.get("writer_focus_extended_probe", {}),
            "w1l3": evidence.get("writer_focus_w1l3_probe", {}),
            "w1l3_controlled": evidence.get("writer_focus_w1l3_controlled_probe", {}),
            "other_world_l1": evidence.get("writer_focus_other_world_l1_probes", []),
        },
        "controlled_16ce_state_machine": evidence.get("state_machine_effect_probes", {}),
    }
    result = {
        "schema": "quiky-optional-map-writer-caller-audit-v1",
        "static_call_form_audit": static,
        "runtime": runtime,
        "acceptance": {
            "required": "identify a reachable runtime caller for 339A, 340A, or 5C9D, or document bounded negative evidence and retain 16CE's proven controlled caller",
            "current_result": "339A/340A/5C9D remain statically unresolved and did not fire in the event-focused or bounded normal windows; 16CE is proven only through its static event/state-machine callers and controlled probes",
            "stop_rule": "do not invent a late-game attribution without a runtime hit carrying a recoverable return address and MAP coordinates",
        },
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(args.output), "sha256": sha256(args.output),
                      "event_trace": str(args.event_trace)}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
