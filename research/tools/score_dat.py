#!/usr/bin/env python3
"""Decode Quiky's eight-entry SCORE.DAT high-score file."""

from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path
from typing import Any


DATA_SIZE = 128
RECORD_SIZE = 16
RECORD_COUNT = DATA_SIZE // RECORD_SIZE
CHECKSUM_SEED = 0x2A
KEY_BASE = 0xC8
KEY_PERIOD = 0x20


class ScoreDatError(ValueError):
    """Raised when SCORE.DAT is malformed or fails its checksum."""


def _decode_bytes(encoded: bytes) -> bytes:
    return bytes(
        value ^ ((KEY_BASE + (index % KEY_PERIOD)) & 0xFF)
        for index, value in enumerate(encoded)
    )


def decode_score_dat(data: bytes) -> dict[str, Any]:
    """Validate and decode one SCORE.DAT payload."""
    if len(data) != DATA_SIZE + 2:
        raise ScoreDatError(
            f"expected {DATA_SIZE + 2} bytes, got {len(data)}"
        )
    encoded = data[:DATA_SIZE]
    stored_checksum = struct.unpack_from("<H", data, DATA_SIZE)[0]
    computed_checksum = (CHECKSUM_SEED + sum(encoded)) & 0xFFFF
    if stored_checksum != computed_checksum:
        raise ScoreDatError(
            f"checksum mismatch: stored 0x{stored_checksum:04x}, "
            f"computed 0x{computed_checksum:04x}"
        )

    decoded = _decode_bytes(encoded)
    records = []
    for index in range(RECORD_COUNT):
        offset = index * RECORD_SIZE
        name_length = decoded[offset]
        if name_length > 8:
            raise ScoreDatError(
                f"record {index}: name length {name_length} exceeds eight bytes"
            )
        name_bytes = decoded[offset + 1 : offset + 9]
        try:
            name = name_bytes[:name_length].decode("ascii")
        except UnicodeDecodeError as exc:
            raise ScoreDatError(f"record {index}: name is not ASCII") from exc
        score = struct.unpack_from("<I", decoded, offset + 9)[0]
        auxiliary = decoded[offset + 13 : offset + 16]
        records.append({
            "index": index,
            "name_length": name_length,
            "name": name,
            "score": score,
            "auxiliary_hex": auxiliary.hex(),
            "score_aux_word": struct.unpack_from("<H", auxiliary)[0],
            "progression_byte": auxiliary[2],
        })
    return {
        "data_size": DATA_SIZE,
        "record_count": RECORD_COUNT,
        "checksum_seed": CHECKSUM_SEED,
        "stored_checksum": stored_checksum,
        "computed_checksum": computed_checksum,
        "records": records,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", type=Path)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    report = decode_score_dat(args.path.read_bytes())
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(f"checksum=0x{report['stored_checksum']:04x}")
        for record in report["records"]:
            print(
                f"{record['index']}: {record['name']!r} "
                f"score={record['score']} aux={record['auxiliary_hex']}"
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
