"""Adapters for the historical Quiky trace envelopes.

Raw evidence is deliberately left untouched.  ``NormalizedTrace`` adapts the
two shapes currently in circulation—top-level ``samples`` and DOSBox's
single-event ``events[0].samples``—and exposes common callback/state fields.
Individual comparators can still apply their domain-specific canonicalization
to ``NormalizedSample.raw`` where the evidence contract requires it.
"""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Mapping


class TraceError(Exception):
    """Raised when a trace does not match a supported historical envelope."""


def _read_payload(path: Path) -> dict[str, Any]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise TraceError(f"{path}: cannot read JSON: {exc}") from exc
    if not isinstance(payload, dict):
        raise TraceError(f"{path}: expected a JSON object")
    return payload


def load_payload(path: Path) -> dict[str, Any]:
    """Load a raw JSON trace payload without selecting an envelope shape."""

    return _read_payload(path)


def extract_samples(payload: Mapping[str, Any]) -> tuple[list[dict[str, Any]], str]:
    """Return raw samples and the envelope shape used by a payload.

    A single event wrapper is the only historical wrapper accepted here.  This
    keeps malformed multi-event captures fail-closed instead of silently
    comparing the wrong event.
    """

    events = payload.get("events")
    if isinstance(events, list):
        if len(events) != 1 or not isinstance(events[0], dict):
            raise TraceError("events must contain one object")
        event = events[0]
        samples = event.get("samples")
        if not isinstance(samples, list):
            raise TraceError("event has no samples[]")
        shape = "events[0].samples"
    else:
        samples = payload.get("samples")
        if not isinstance(samples, list):
            raise TraceError("trace has no samples[]")
        shape = "samples"
    if any(not isinstance(sample, dict) for sample in samples):
        raise TraceError(f"{shape}: every sample must be an object")
    return list(samples), shape


def _callback(raw: Mapping[str, Any]) -> dict[str, Any] | None:
    value = raw.get("player_callback")
    return value if isinstance(value, dict) else None


def _record(raw: Mapping[str, Any], which: str) -> str | None:
    """Find a callback record across the known trace generations."""

    callback = _callback(raw)
    if raw.get("_record_hex") is not None:
        value = raw.get("_record_hex")
        if isinstance(value, str):
            return value
    if callback is not None:
        object_name = "pre_object" if which == "pre" else "post_object"
        obj = callback.get(object_name)
        if isinstance(obj, dict) and isinstance(obj.get("state_hex"), str):
            return obj["state_hex"]
        for key in (
            f"{which}_record_hex",
            f"{which}_player_record_hex",
            f"{which}_state_hex",
        ):
            if isinstance(callback.get(key), str):
                return callback[key]
    for key in (
        f"{which}_record_hex",
        f"{which}_player_record_hex",
        f"{which}_state_hex",
    ):
        if isinstance(raw.get(key), str):
            return raw[key]
    player = raw.get("player")
    if isinstance(player, dict):
        for key in (
            f"{which}_record_hex",
            f"{which}_state_hex",
            f"{which}_record",
        ):
            value = player.get(key)
            if isinstance(value, str):
                return value
    return None


def _input_flags(raw: Mapping[str, Any]) -> int | None:
    callback = _callback(raw)
    if callback is not None and isinstance(callback.get("input_flags"), int):
        return callback["input_flags"] & 0xFFFF
    if isinstance(raw.get("input_flags"), int):
        return raw["input_flags"] & 0xFFFF
    globals_value = raw.get("globals")
    if isinstance(globals_value, dict):
        keyboard = globals_value.get("keyboard_action_flags")
        auxiliary = globals_value.get("input_action_flags")
        if isinstance(keyboard, int) and isinstance(auxiliary, int):
            return (keyboard | auxiliary) & 0xFFFF
        if isinstance(auxiliary, int):
            return auxiliary & 0xFFFF
        if isinstance(keyboard, int):
            return keyboard & 0xFFFF
    return None


def _camera(raw: Mapping[str, Any]) -> tuple[int, int] | None:
    value = raw.get("camera")
    if isinstance(value, dict) and isinstance(value.get("x"), int) and isinstance(value.get("y"), int):
        return value["x"], value["y"]
    globals_value = raw.get("globals")
    if isinstance(globals_value, dict):
        x, y = globals_value.get("camera_x"), globals_value.get("camera_y")
        if isinstance(x, int) and isinstance(y, int):
            return x, y
    return None


@dataclass(frozen=True)
class NormalizedSample:
    """One sample with common fields adapted and raw evidence preserved."""

    sequence: int
    raw: dict[str, Any]
    callback: dict[str, Any] | None
    pre_record_hex: str | None
    post_record_hex: str | None
    input_flags: int | None
    camera: tuple[int, int] | None

    @property
    def globals(self) -> dict[str, Any] | None:
        value = self.raw.get("globals")
        return value if isinstance(value, dict) else None

    def get(self, key: str, default: Any = None) -> Any:
        """Provide mapping-like access for incremental wrapper migrations."""

        return self.raw.get(key, default)


@dataclass(frozen=True)
class NormalizedTrace:
    """A trace envelope plus its normalized samples."""

    path: Path | None
    payload: dict[str, Any]
    shape: str
    samples: tuple[NormalizedSample, ...]

    @property
    def by_sequence(self) -> dict[int, NormalizedSample]:
        result: dict[int, NormalizedSample] = {}
        for sample in self.samples:
            if sample.sequence in result:
                raise TraceError(f"duplicate sample sequence {sample.sequence}")
            result[sample.sequence] = sample
        return result


def normalize_sample(raw: dict[str, Any], *, label: str = "trace") -> NormalizedSample:
    sequence = raw.get("sequence")
    if not isinstance(sequence, int):
        raise TraceError(f"{label}: every sample needs integer sequence")
    return NormalizedSample(
        sequence=sequence,
        raw=raw,
        callback=_callback(raw),
        pre_record_hex=_record(raw, "pre"),
        post_record_hex=_record(raw, "post"),
        input_flags=_input_flags(raw),
        camera=_camera(raw),
    )


def normalize_payload(payload: dict[str, Any], *, label: str = "trace") -> NormalizedTrace:
    """Normalize an already-decoded payload for callers with custom I/O."""

    raw_samples, shape = extract_samples(payload)
    samples = tuple(
        normalize_sample(sample, label=label) for sample in raw_samples
    )
    result = NormalizedTrace(None, payload, shape, samples)
    result.by_sequence
    return result


def load_trace(path: Path) -> NormalizedTrace:
    """Load and normalize a trace without changing its on-disk representation."""

    payload = _read_payload(path)
    result = normalize_payload(payload, label=str(path))
    return NormalizedTrace(path, result.payload, result.shape, result.samples)
