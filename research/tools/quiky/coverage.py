"""Runtime callback coverage aggregation for trace artifacts."""

from __future__ import annotations

from collections import Counter
from pathlib import Path
from typing import Any, Iterable

from .trace import NormalizedSample, load_trace


COVERAGE_SCHEMA = "quiky.callback-coverage-v1"


def _offset(value: Any) -> int | None:
    if isinstance(value, dict):
        value = value.get("offset")
    return value if isinstance(value, int) else None


def _append(counter: Counter[int], values: Any) -> None:
    if not isinstance(values, list):
        return
    for item in values:
        value = item
        if isinstance(item, dict):
            value = item.get("phase_callback_offset", item.get("callback"))
        offset = _offset(value)
        if offset is not None and offset not in (0, 0xFFFF):
            counter[offset] += 1


def sample_callback_coverage(sample: NormalizedSample) -> Counter[int]:
    """Count callback PCs visible in one normalized sample."""

    counter: Counter[int] = Counter()
    raw = sample.raw
    _append(counter, raw.get("scheduler_callbacks"))
    scheduler = raw.get("scheduler")
    if isinstance(scheduler, dict):
        _append(counter, scheduler.get("entries"))
    pool = raw.get("pool")
    if isinstance(pool, dict):
        _append(counter, pool.get("objects"))
    _append(counter, raw.get("entities"))
    for key in ("callback", "phase_callback_offset"):
        value = raw.get(key)
        offset = _offset(value)
        if offset is not None and offset not in (0, 0xFFFF):
            counter[offset] += 1
    return counter


def coverage_for_trace(path: Path) -> dict[str, Any]:
    trace = load_trace(path)
    counts: Counter[int] = Counter()
    for sample in trace.samples:
        counts.update(sample_callback_coverage(sample))
    return {
        "schema": COVERAGE_SCHEMA,
        "source": str(path),
        "samples": len(trace.samples),
        "callbacks": [
            {"offset": offset, "count": counts[offset]}
            for offset in sorted(counts)
        ],
    }


def merge_coverage(reports: Iterable[dict[str, Any]]) -> dict[str, Any]:
    counts: Counter[int] = Counter()
    sources = []
    for report in reports:
        if report.get("schema") != COVERAGE_SCHEMA:
            raise ValueError(f"unsupported coverage schema: {report.get('schema')!r}")
        if report.get("source") is not None:
            sources.append(report["source"])
        for item in report.get("callbacks", []):
            if isinstance(item, dict) and isinstance(item.get("offset"), int):
                counts[item["offset"]] += int(item.get("count", 0))
    return {
        "schema": COVERAGE_SCHEMA,
        "sources": sources,
        "callbacks": [
            {"offset": offset, "count": counts[offset]}
            for offset in sorted(counts)
        ],
    }


def missing_callbacks(report: dict[str, Any], known: Iterable[int]) -> list[int]:
    observed = {
        item["offset"] for item in report.get("callbacks", [])
        if isinstance(item, dict) and isinstance(item.get("offset"), int)
    }
    return sorted(set(known) - observed)
