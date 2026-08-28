#!/usr/bin/env python3
"""Derive a replayable W1L1 leaf PRNG ring prefix from a DOS pool trace.

The captured W1L1 pool lists leaf objects in the publication order used by
the stream pass.  Each 01F7:4727 initializer consumes one selector byte and
one velocity byte.  The six dedicated event declarations precede the leaves
in the startup stream, so the default output reserves six leading bytes for
that earlier consumption and emits a complete 256-byte ring suitable for
``quiky-parity-replay --leaf-prng-ring-hex``.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


LEAF_CALLBACK = 0x47E7
LEAF_BASE_VELOCITY = 0x13000
LEAF_STEP = 300
LEAF_SCALE = 0x80
RING_SIZE = 0x100


class LeafRingError(ValueError):
    """Raised when a trace does not contain an invertible leaf state."""


def _ordered_samples(payload: dict[str, Any]) -> list[dict[str, Any]]:
    samples = payload.get("samples")
    if not isinstance(samples, list):
        events = payload.get("events")
        if isinstance(events, list):
            for event in events:
                if isinstance(event, dict) and isinstance(event.get("samples"), list):
                    samples = event["samples"]
                    break
    if not isinstance(samples, list) or not samples:
        raise LeafRingError("trace contains no samples")
    result = [sample for sample in samples if isinstance(sample, dict)]
    if not result:
        raise LeafRingError("trace contains no object samples")
    return result


def _callback_offset(value: Any) -> int | None:
    if isinstance(value, dict):
        value = value.get("offset")
    return value if isinstance(value, int) else None


def _pool_leaves(sample: dict[str, Any]) -> list[dict[str, Any]]:
    pool = sample.get("pool")
    if not isinstance(pool, dict) or not isinstance(pool.get("objects"), list):
        raise LeafRingError("first sample has no DOS pool object list")
    leaves = []
    for item in pool["objects"]:
        if not isinstance(item, dict):
            continue
        if _callback_offset(item.get("callback")) == LEAF_CALLBACK:
            leaves.append(item)
    if not leaves:
        raise LeafRingError("first sample contains no 01F7:47E7 leaf objects")
    return leaves


def derive_ring(payload: dict[str, Any], leading_bytes: bytes = bytes(6)) -> tuple[bytes, list[dict[str, Any]]]:
    """Return ``(ring, derivation_rows)`` from the first DOS sample.

    The first observed leaf callback has already applied the callback's
    ``velocity -= 300`` step, so the initializer velocity is recovered by
    adding 300 before inverting the signed-byte fixed-point formula.
    """

    if len(leading_bytes) > RING_SIZE:
        raise LeafRingError("leading PRNG prefix is larger than 256 bytes")
    leaves = _pool_leaves(_ordered_samples(payload)[0])
    ring = bytearray(leading_bytes)
    rows: list[dict[str, Any]] = []
    for index, item in enumerate(leaves):
        velocity = item.get("velocity_y_fixed")
        if not isinstance(velocity, int):
            raise LeafRingError(f"leaf {index} has no integer velocity_y_fixed")
        initializer_velocity = velocity + LEAF_STEP
        numerator = LEAF_BASE_VELOCITY - initializer_velocity
        if numerator % LEAF_SCALE != 0:
            raise LeafRingError(
                f"leaf {index} velocity does not invert to a signed PRNG byte"
            )
        signed_velocity_byte = numerator // LEAF_SCALE
        if not -128 <= signed_velocity_byte <= 127:
            raise LeafRingError(f"leaf {index} velocity byte is outside int8 range")

        sprite_slot = item.get("sprite_slot")
        if sprite_slot == 700:
            selector_byte = 1
        elif sprite_slot == 703:
            selector_byte = 0
        else:
            raise LeafRingError(
                f"leaf {index} first-frame sprite slot {sprite_slot!r} is not 700/703"
            )
        ring.extend((selector_byte, signed_velocity_byte & 0xFF))
        rows.append({
            "index": index,
            "x": item.get("position", {}).get("x")
            if isinstance(item.get("position"), dict) else None,
            "selector_byte": f"0x{selector_byte:02x}",
            "velocity_byte": f"0x{signed_velocity_byte & 0xff:02x}",
            "signed_velocity_byte": signed_velocity_byte,
        })

    if len(ring) > RING_SIZE:
        raise LeafRingError("derived leaf prefix exceeds the 256-byte ring")
    ring.extend(bytes(RING_SIZE - len(ring)))
    return bytes(ring), rows


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument(
        "--leading-hex", default="000000000000",
        help="bytes consumed before the leaves, as hex (default: six zero bytes)",
    )
    parser.add_argument("--json", action="store_true", dest="as_json")
    args = parser.parse_args(argv)
    try:
        try:
            leading = bytes.fromhex(args.leading_hex)
        except ValueError as exc:
            raise LeafRingError("--leading-hex must contain valid hex") from exc
        payload = json.loads(args.trace.read_text(encoding="utf-8"))
        ring, rows = derive_ring(payload, leading)
    except (OSError, json.JSONDecodeError, LeafRingError) as exc:
        print(f"derive-w1l1-leaf-ring: {exc}", file=sys.stderr)
        return 2

    if args.as_json:
        print(json.dumps({
            "leading_hex": args.leading_hex,
            "rows": rows,
            "ring_hex": ring.hex(),
        }, indent=2, sort_keys=True))
    else:
        print(ring.hex())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
