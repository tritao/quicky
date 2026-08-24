#!/usr/bin/env python3
"""Build a static/runtime lifecycle matrix for Quiky ARE object types."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any, Iterable


WRITE_RE = re.compile(
    r"^\s*([0-9a-f]+):\s+.*?\s+(.+?)\s*$", re.IGNORECASE
)
CALLBACK_WRITE_RE = re.compile(
    r"movw\s+\$0x([0-9a-f]+),%es:0x(18|1a)\(%di\)", re.IGNORECASE
)
DESTINATION_FIELD_RE = re.compile(
    r"%es:0x(18|1a)\(%di\)\s*$", re.IGNORECASE
)


def parse_disassembly_writes(source: str) -> list[dict[str, Any]]:
    """Extract direct writes to the pooled object's callback/source fields."""
    writes: list[dict[str, Any]] = []
    for line in source.splitlines():
        match = WRITE_RE.match(line)
        if not match:
            continue
        instruction = match.group(2)
        # The AT&T listing contains both loads and stores.  A field is a
        # direct write only when it is the final operand (the destination);
        # e.g. ``mov %es:0x18(%di),%bx`` is a load and must be ignored.
        field_match = DESTINATION_FIELD_RE.search(instruction)
        if not field_match:
            continue
        field_offset = int(field_match.group(1), 16)
        callback_match = CALLBACK_WRITE_RE.search(instruction)
        value = int(callback_match.group(1), 16) if callback_match else None
        if field_offset == 0x18:
            action = "clear" if value == 0 else (
                "install_callback" if value is not None else "modify_callback"
            )
        else:
            action = "set_source" if value is not None else "modify_source"
        writes.append({
            "address": match.group(1).lower(),
            "field": f"object+0x{field_offset:02x}",
            "value": value,
            "action": action,
            "instruction": instruction,
        })
    return writes


def parse_disassembly_file(path: Path) -> list[dict[str, Any]]:
    return parse_disassembly_writes(path.read_text(encoding="utf-8"))


def anchored_lifecycle_sites() -> list[dict[str, Any]]:
    """Return the segment:offset sites already resolved by static/runtime work."""
    return [
        {
            "site": "01F7:0E06",
            "field": "object+0x18",
            "action": "install_dispatch_callback",
            "evidence": "factory decompilation and runtime factory return",
        },
        {
            "site": "01F7:0E06",
            "field": "object+0x1A",
            "action": "set_source_declaration",
            "evidence": "factory decompilation and source-aware pool scan",
        },
        {
            "site": "01F7:0E96",
            "field": "object+0x18",
            "action": "clear_if_source_pointer_is_FFFF",
            "evidence": "scheduler decompilation at segment-3 target 0x106A",
        },
        {
            "site": "01F7:1DEE",
            "field": "object+0x18",
            "action": "clear_on_camera_rejection",
            "evidence": "static gate path and controlled camera trace",
        },
        {
            "site": "01F7:1DEE",
            "field": "FS:[object+0x1A+1]",
            "action": "clear_processed_marker_high_byte",
            "evidence": "static gate path and source-marker trace",
        },
        {
            "site": "01F7:8E4B",
            "field": "object+0x18",
            "action": "clear_at_state_10",
            "evidence": "state-machine disassembly and runtime state trace",
        },
        {
            "site": "01F7:1036",
            "field": "scheduler_banks",
            "action": "register_nonzero_callback_in_next_bank",
            "evidence": "scheduler table trace and static registration path",
        },
    ]


def _as_int(value: Any) -> int | None:
    if isinstance(value, bool):
        return int(value)
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        match = re.search(r"([0-9a-f]+)$", value, re.IGNORECASE)
        if match:
            return int(match.group(1), 16)
    return None


def load_catalog(path: Path) -> list[dict[str, Any]]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    entries = payload.get("entries", [])
    if not isinstance(entries, list):
        raise ValueError("entity catalog has no entries array")
    result = []
    for entry in entries:
        evidence = entry.get("evidence", {})
        result.append({
            "type": entry.get("type"),
            "name": entry.get("name", ""),
            "confidence": entry.get("confidence", ""),
            "level": evidence.get("level", ""),
            "record_offset": evidence.get("record_offset"),
            "update_callback": evidence.get("update_callback", ""),
            "object_class": evidence.get("object_class"),
            "factory": evidence.get("factory", ""),
            "note": evidence.get("note", ""),
        })
    return result


def _hex_word(raw_hex: Any, offset: int) -> int | None:
    if not isinstance(raw_hex, str):
        return None
    raw_hex = raw_hex.removeprefix("hex:")
    start = offset * 2
    chunk = raw_hex[start:start + 4]
    if len(chunk) != 4:
        return None
    try:
        return int.from_bytes(bytes.fromhex(chunk), "little")
    except ValueError:
        return None


def _movement_summary(entity: dict[str, Any]) -> dict[str, Any]:
    frames = entity.get("frames", [])
    if not isinstance(frames, list):
        frames = []
    marker_values: list[int] = []
    matching_slots: list[list[int]] = []
    active_callbacks: list[list[int]] = []
    scheduled_banks: list[list[int]] = []
    selected_callbacks: list[int | None] = []
    for frame in frames:
        lifecycle = frame.get("lifecycle", {})
        source = lifecycle.get("source", {})
        marker = source.get("marker_word")
        marker_values.append(marker if isinstance(marker, int) else -1)
        pool = lifecycle.get("pool", {})
        matches = pool.get("source_matches", [])
        slots = []
        callbacks = []
        for match in matches if isinstance(matches, list) else []:
            if isinstance(match, dict):
                if isinstance(match.get("index"), int):
                    slots.append(match["index"])
                if match.get("active") and isinstance(match.get("update_callback"), int):
                    callbacks.append(match["update_callback"])
        matching_slots.append(slots)
        active_callbacks.append(callbacks)
        banks = lifecycle.get("scheduler", {}).get("banks", [])
        scheduled = []
        for bank in banks if isinstance(banks, list) else []:
            if isinstance(bank, dict) and bank.get("entries"):
                scheduled.append(bank.get("bank", -1))
        scheduled_banks.append(scheduled)
        callback = _hex_word(frame.get("object_state_hex"), 0x18)
        selected_callbacks.append(callback)

    marker_cleared = any(
        before != -1 and after != -1 and (before >> 8) != 0 and (after >> 8) == 0
        for before, after in zip(marker_values, marker_values[1:])
    )
    marker_restored = any(
        before != -1 and after != -1 and (before >> 8) == 0 and (after >> 8) != 0
        for before, after in zip(marker_values, marker_values[1:])
    )
    first_slot = next((slots[0] for slots in matching_slots if slots), None)
    clear_index = next(
        (index for index, (before, after) in enumerate(
            zip(marker_values, marker_values[1:]))
         if before != -1 and after != -1 and (before >> 8) != 0 and
         (after >> 8) == 0),
        None,
    )
    restore_index = None
    if clear_index is not None:
        restore_index = next(
            (index for index in range(clear_index + 1, len(marker_values) - 1)
             if marker_values[index] != -1 and
             (marker_values[index] >> 8) == 0 and
             marker_values[index + 1] != -1 and
             (marker_values[index + 1] >> 8) != 0),
            None,
        )
    restored_slot = (
        next((slot for slot in matching_slots[restore_index + 1]
              if slot is not None), None)
        if restore_index is not None and restore_index + 1 < len(matching_slots)
        else None
    )
    callback_cleared_while_processed = any(
        before is not None and before != 0 and after == 0 and
        marker != -1 and marker >> 8
        for before, after, marker in zip(
            selected_callbacks, selected_callbacks[1:], marker_values[1:]
        )
    )
    if marker_cleared and marker_restored:
        category = "visibility_culled_then_reactivated"
    elif marker_cleared:
        category = "visibility_culled"
    elif callback_cleared_while_processed:
        category = "self_terminated_or_state_ended"
    elif (matching_slots and all(slots for slots in matching_slots) and
          all(callbacks for callbacks in active_callbacks)):
        category = "persistent_in_window"
    else:
        category = "unclassified"
    return {
        "type": entity.get("type"),
        "record_offset": entity.get("record_offset"),
        "source": entity.get("source", {}),
        "frame_count": len(frames),
        "marker_values": marker_values,
        "matching_pool_slots": matching_slots,
        "active_callbacks": active_callbacks,
        "scheduled_banks": scheduled_banks,
        "selected_callbacks": selected_callbacks,
        "first_pool_slot": first_slot,
        "reactivated_pool_slot": restored_slot if marker_restored else None,
        "marker_clear_index": clear_index,
        "marker_restore_index": restore_index,
        "marker_cleared": marker_cleared,
        "marker_restored": marker_restored,
        "callback_cleared_while_source_processed": callback_cleared_while_processed,
        "category": category,
    }


def _behavior_summary(entity: dict[str, Any]) -> dict[str, Any]:
    samples = entity.get("samples", [])
    if isinstance(samples, dict):
        samples = [samples[key] for key in sorted(samples, key=lambda item: int(item))]
    if not isinstance(samples, list):
        samples = []
    callbacks_before = []
    callbacks_after = []
    marker_before = []
    marker_after = []
    related_sites = []
    for sample in samples:
        before = sample.get("object_before", {})
        after = sample.get("object_after", {})
        callbacks_before.append(before.get("update_callback"))
        callbacks_after.append(after.get("update_callback"))
        source_before = sample.get("source_before") or {}
        source_after = sample.get("source_after") or {}
        marker_before.append(source_before.get("marker_word"))
        marker_after.append(source_after.get("marker_word"))
        related = (sample.get("callback") or {}).get("related_hits", [])
        if isinstance(related, dict):
            related = list(related.values())
        for hit in related if isinstance(related, list) else []:
            if isinstance(hit, dict) and isinstance(hit.get("offset"), int):
                related_sites.append(hit["offset"])

    callback_cleared = any(
        before not in (None, 0) and after == 0
        for before, after in zip(callbacks_before, callbacks_after)
    )
    callback_survived = any(callback not in (None, 0) for callback in callbacks_after)
    marker_processed = any(
        marker is not None and (marker >> 8) != 0
        for marker in marker_after
    )
    marker_cleared = any(
        before is not None and after is not None and
        (before >> 8) != 0 and (after >> 8) == 0
        for before, after in zip(marker_before, marker_after)
    )
    if callback_cleared and marker_processed:
        category = "self_terminated_or_state_ended"
    elif callback_survived and marker_processed:
        category = "persistent_in_window"
    elif 0x1DEE in related_sites or marker_cleared:
        category = "visibility_culled"
    elif callback_cleared:
        category = "callback_ended_without_source_evidence"
    else:
        category = "object_behavior_observed"
    return {
        "type": entity.get("type"),
        "record_offset": entity.get("record_offset"),
        "sample_count": len(samples),
        "callbacks_before": callbacks_before,
        "callbacks_after": callbacks_after,
        "source_markers_before": marker_before,
        "source_markers_after": marker_after,
        "related_sites": sorted(set(related_sites)),
        "callback_cleared": callback_cleared,
        "callback_survived": callback_survived,
        "source_marker_cleared": marker_cleared,
        "category": category,
    }


def _entity_summary(entity: dict[str, Any]) -> dict[str, Any]:
    callback = entity.get("update_callback")
    callback_offset = callback.get("offset") if isinstance(callback, dict) else None
    lifetime_samples = entity.get("lifetime_samples", [])
    if not isinstance(lifetime_samples, list):
        lifetime_samples = []
    return {
        "type": entity.get("type"),
        "record_offset": entity.get("record_offset"),
        "category": "factory_snapshot_with_lifetime_samples",
        "object_class": entity.get("object_class"),
        "object_offset": (entity.get("object") or {}).get("offset"),
        "update_callback": callback_offset,
        "object_callback_field": _hex_word(entity.get("object_state_hex"), 0x18),
        "initialized_position": entity.get("initialized_position"),
        "lifetime_sample_count": len(lifetime_samples),
        "lifetime_object_offsets": [
            (sample.get("object") or {}).get("offset")
            for sample in lifetime_samples if isinstance(sample, dict)
        ],
    }


def summarize_trace(path: Path) -> dict[str, Any]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    events = payload.get("events", [])
    event = events[0] if isinstance(events, list) and events else {}
    if payload.get("trace_kind") == "object-movement":
        summary = _movement_summary(event)
    elif payload.get("trace_kind") == "object-behavior":
        summary = _behavior_summary(event)
    elif payload.get("trace_kind") == "entity":
        summary = _entity_summary(event)
    else:
        summary = {
            "type": event.get("type"),
            "record_offset": event.get("record_offset"),
            "category": "dedicated_or_object_behavior_trace",
        }
    summary["trace"] = str(path)
    return summary


def build_matrix(catalog: Iterable[dict[str, Any]], traces: Iterable[Path],
                 disassembly: Path | None) -> dict[str, Any]:
    trace_summaries = [summarize_trace(path) for path in traces]
    by_type = {item.get("type"): item for item in trace_summaries}
    types = []
    for item in catalog:
        row = dict(item)
        row["runtime"] = by_type.get(item.get("type"))
        types.append(row)
    result: dict[str, Any] = {
        "schema": "quiky-object-lifecycle-matrix-v1",
        "static_sites": anchored_lifecycle_sites(),
        "types": types,
        "runtime_traces": trace_summaries,
    }
    if disassembly is not None:
        result["direct_disassembly_writes"] = parse_disassembly_file(disassembly)
        result["disassembly_source"] = str(disassembly)
    return result


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, required=True)
    parser.add_argument("--disassembly", type=Path)
    parser.add_argument("--trace", type=Path, action="append", default=[])
    parser.add_argument("--output", type=Path, required=True)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    matrix = build_matrix(
        load_catalog(args.catalog), args.trace, args.disassembly,
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(matrix, indent=2) + "\n", encoding="utf-8")
    print(f"wrote lifecycle matrix to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
