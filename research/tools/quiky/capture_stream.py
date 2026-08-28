"""Crash-recoverable framed CBOR streams for interactive captures."""

from __future__ import annotations

import struct
from pathlib import Path
from typing import Any, BinaryIO, Iterator

MAGIC = b"QCAP\x01"


def encode(value: Any) -> bytes:
    def head(major: int, argument: int) -> bytes:
        if argument < 24:
            return bytes([(major << 5) | argument])
        widths = ((0xff, 24, ">B"), (0xffff, 25, ">H"),
                  (0xffffffff, 26, ">I"), (0xffffffffffffffff, 27, ">Q"))
        for maximum, info, fmt in widths:
            if argument <= maximum:
                return bytes([(major << 5) | info]) + struct.pack(fmt, argument)
        raise ValueError("CBOR integer is too large")

    if value is None:
        return b"\xf6"
    if value is False:
        return b"\xf4"
    if value is True:
        return b"\xf5"
    if isinstance(value, int):
        return head(0, value) if value >= 0 else head(1, -1 - value)
    if isinstance(value, float):
        return b"\xfb" + struct.pack(">d", value)
    if isinstance(value, bytes):
        return head(2, len(value)) + value
    if isinstance(value, str):
        raw = value.encode("utf-8")
        return head(3, len(raw)) + raw
    if isinstance(value, list):
        return head(4, len(value)) + b"".join(encode(item) for item in value)
    if isinstance(value, dict):
        entries = [(encode(key), encode(item)) for key, item in value.items()]
        entries.sort(key=lambda item: (len(item[0]), item[0]))
        return head(5, len(entries)) + b"".join(
            key + item for key, item in entries)
    raise TypeError(f"unsupported CBOR value: {type(value).__name__}")


def _decode(raw: bytes, offset: int = 0) -> tuple[Any, int]:
    if offset >= len(raw):
        raise ValueError("truncated CBOR value")
    initial, offset = raw[offset], offset + 1
    major, info = initial >> 5, initial & 31
    if info < 24:
        argument = info
    elif info in (24, 25, 26, 27):
        widths = {24: (1, ">B"), 25: (2, ">H"),
                  26: (4, ">I"), 27: (8, ">Q")}
        width, fmt = widths[info]
        if offset + width > len(raw):
            raise ValueError("truncated CBOR argument")
        argument = struct.unpack_from(fmt, raw, offset)[0]
        offset += width
    else:
        raise ValueError("indefinite CBOR values are unsupported")
    if major == 0:
        return argument, offset
    if major == 1:
        return -1 - argument, offset
    if major in (2, 3):
        end = offset + argument
        if end > len(raw):
            raise ValueError("truncated CBOR string")
        value = raw[offset:end]
        return (value if major == 2 else value.decode("utf-8")), end
    if major == 4:
        values = []
        for _ in range(argument):
            value, offset = _decode(raw, offset)
            values.append(value)
        return values, offset
    if major == 5:
        values = {}
        for _ in range(argument):
            key, offset = _decode(raw, offset)
            value, offset = _decode(raw, offset)
            values[key] = value
        return values, offset
    if initial == 0xf4:
        return False, offset
    if initial == 0xf5:
        return True, offset
    if initial == 0xf6:
        return None, offset
    if initial == 0xfb:
        if offset + 8 > len(raw):
            raise ValueError("truncated CBOR float")
        return struct.unpack_from(">d", raw, offset)[0], offset + 8
    raise ValueError("unsupported CBOR value")


def decode(raw: bytes) -> Any:
    value, offset = _decode(raw)
    if offset != len(raw):
        raise ValueError("trailing CBOR data")
    return value


def append(stream: BinaryIO, value: Any) -> None:
    if stream.tell() == 0:
        stream.write(MAGIC)
    payload = encode(value)
    stream.write(struct.pack(">I", len(payload)))
    stream.write(payload)


def read(path: Path, *, tolerate_truncated_tail: bool = False) -> Iterator[Any]:
    with path.open("rb") as stream:
        if stream.read(len(MAGIC)) != MAGIC:
            raise ValueError("capture stream has an invalid header")
        while True:
            header = stream.read(4)
            if not header:
                return
            if len(header) != 4:
                if tolerate_truncated_tail:
                    return
                raise ValueError("capture stream has a truncated frame header")
            size = struct.unpack(">I", header)[0]
            payload = stream.read(size)
            if len(payload) != size:
                if tolerate_truncated_tail:
                    return
                raise ValueError("capture stream has a truncated frame")
            yield decode(payload)
