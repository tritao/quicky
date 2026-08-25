#!/usr/bin/env python3
"""Compare one engine SFX trace with the live macro-dispatch ledger."""

from __future__ import annotations

import argparse
import json
import re
import wave
from pathlib import Path


MACRO_RE = re.compile(
    r"TFMX macro tick=(?P<tick>\d+) offset=(?P<offset>\d+) "
    r"step=(?P<step>\d+) command=(?P<command>\d+) "
    r"args=(?P<bb>\d+):(?P<cd>\d+):(?P<ee>\d+)"
)
STATE_RE = re.compile(
    r"TFMX sfx-state tick=(?P<tick>\d+) voice=(?P<voice>\d+) "
    r"active=(?P<active>\d+) dma=(?P<dma>\d+) volume=(?P<volume>-?\d+) "
    r"period=(?P<period>-?\d+) out=(?P<out>-?\d+) "
    r"macro-stopped=(?P<stopped>\d+) step=(?P<step>\d+) "
    r"wait=(?P<wait>\d+) effects=(?P<effects>\d+) "
    r"sample=(?P<sample>\d+)/(?P<length>\d+) "
    r"playback=(?P<playback>\d+)/(?P<playback_length>\d+)"
)


def command_argument(raw: bytes, command: int) -> int:
    if command == 2:
        return int.from_bytes(raw[1:4], "big")
    return int.from_bytes(raw[2:4], "big")


def tfx_commands(tfx: bytes, effect_id: int) -> tuple[int, list[tuple[int, int, int]]]:
    macro_table = int.from_bytes(tfx[0x1D8:0x1DC], "little")
    pointer = int.from_bytes(
        tfx[macro_table + effect_id * 4:macro_table + effect_id * 4 + 4],
        "big",
    )
    commands = []
    for step in range(128):
        raw = tfx[pointer + step * 4:pointer + step * 4 + 4]
        if len(raw) != 4:
            raise ValueError(f"macro step {step} lies outside the TFX")
        command = raw[0] & 0x3F
        commands.append((step, command, command_argument(raw, command)))
        if command == 7:
            return pointer, commands
    raise ValueError("macro has no terminal command 7")


def live_commands(trace: dict) -> list[tuple[int, int, int]]:
    dispatch = trace["trace"]["events"]["1"]["low_level"]["macro_dispatch"]
    rows = [value for value in dispatch.values() if "command_id" in value]
    rows.sort(key=lambda value: value["sequence"])
    return [
        (value["macro_step"], value["command_id"], value["command_argument_word"])
        for value in rows
    ]


def engine_commands(log: str) -> list[dict[str, int]]:
    result = []
    for line in log.splitlines():
        match = MACRO_RE.search(line)
        if not match:
            continue
        row = {key: int(value) for key, value in match.groupdict().items()}
        row["argument"] = (
            (row["bb"] << 16) | (row["cd"] << 8) | row["ee"]
            if row["command"] == 2
            else (row["cd"] << 8) | row["ee"]
        )
        result.append(row)
    return result


def engine_states(log: str, voice: int) -> list[dict[str, int]]:
    result = []
    for line in log.splitlines():
        match = STATE_RE.search(line)
        if not match:
            continue
        row = {key: int(value) for key, value in match.groupdict().items()}
        if row["voice"] == voice:
            result.append(row)
    return result


def wave_info(path: Path) -> str:
    with wave.open(str(path), "rb") as stream:
        return (
            f"{stream.getframerate()} Hz, {stream.getnchannels()} channel(s), "
            f"{stream.getsampwidth() * 8}-bit PCM"
        )


def format_stream(rows: list[tuple[int, int, int]]) -> str:
    return " ".join(f"{command:02x}:{argument:04x}" for _, command, argument in rows)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tfx", type=Path, required=True)
    parser.add_argument("--live", type=Path, required=True)
    parser.add_argument("--engine-log", type=Path, required=True)
    parser.add_argument("--engine-wav", type=Path)
    parser.add_argument("--effect-id", type=int, default=2)
    parser.add_argument("--voice", type=int, default=3)
    args = parser.parse_args()

    tfx_pointer, expected = tfx_commands(args.tfx.read_bytes(), args.effect_id)
    live = live_commands(json.loads(args.live.read_text(encoding="utf-8")))
    engine = engine_commands(args.engine_log.read_text(encoding="utf-8"))
    states = engine_states(args.engine_log.read_text(encoding="utf-8"), args.voice)

    print(f"effect {args.effect_id}: TFX macro 0x{tfx_pointer:04x}")
    print(f"TFX:   {format_stream(expected)}")
    print(f"Live:  {format_stream(live)}")
    print(f"live_match={live == expected}")
    print(
        "Engine: "
        + " ".join(
            f"tick{row['tick']}:{row['command']:02x}:{row['argument']:04x}"
            for row in engine
        )
    )
    grouped = {}
    for row in engine:
        grouped.setdefault(row["tick"], []).append(row["command"])
    print(
        "engine_command_groups="
        + " ".join(
            f"tick{tick}=" + ",".join(f"{command:02x}" for command in commands)
            for tick, commands in sorted(grouped.items())
        )
    )
    print(
        "engine_states="
        + " ".join(
            f"tick{row['tick']}:{row['sample']:x}/{row['length']:x}:"
            f"{row['playback']:x}/{row['playback_length']:x}:v{row['volume']}"
            for row in states
        )
    )
    if args.engine_wav:
        print(f"engine_wav={wave_info(args.engine_wav)}")
    print("guest_device=12000 Hz, 1 channel, 8-bit unsigned PCM")
    return 0 if live == expected else 1


if __name__ == "__main__":
    raise SystemExit(main())
