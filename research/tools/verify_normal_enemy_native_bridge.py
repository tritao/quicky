#!/usr/bin/env python3
"""Verify the native normal-enemy bridge against the static closure ledger."""

from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
STATIC_LEDGER = ROOT / "research/ghidra/normal-enemy-static-closure.json"
BRIDGE_NOTE = ROOT / "research/notes/normal-enemy-native-bridge.cpp"
LEVEL_SOURCE = ROOT / "engine/src/level.cpp"
LEVEL_HEADER = ROOT / "engine/include/quiky/level.h"


EXPECTED = {
    "wurm2": ("0x01", "0x02", "0x6dc4"),
    "biene": ("0x03", "0x04", "0x68c0"),
    "fisch": ("0x05", "0x06", "0x7b71"),
    "krabbe": ("0x07", "0x08", "0x778c"),
    "pengo": ("0x09", "0x0a", "0x715e"),
    "schnee": ("0x0b", "0x0c", "0x66e1"),
    "fliege": ("0x15", "0x16", "0x7ef8"),
    "spinne": ("0x17", "0x18", "0x8472"),
    "buggy": ("0x19", "0x1a", "0x5071"),
    "ufo_enemy": ("0x1b", "0x1c", "0x5f28"),
}
STATIC_EXPECTED = {
    family_id: values for family_id, values in EXPECTED.items()
    if family_id not in {"wurm2", "biene"}
}


class BridgeError(RuntimeError):
    pass


def verify() -> None:
    try:
        ledger = json.loads(STATIC_LEDGER.read_text(encoding="utf-8"))
        note = BRIDGE_NOTE.read_text(encoding="utf-8")
        source = LEVEL_SOURCE.read_text(encoding="utf-8")
        header = LEVEL_HEADER.read_text(encoding="utf-8")
    except (OSError, json.JSONDecodeError) as exc:
        raise BridgeError(str(exc)) from exc

    if ledger.get("schema") != "quiky.normal-enemy-static-closure.v1":
        raise BridgeError("unexpected static closure schema")
    families = ledger.get("families")
    if not isinstance(families, list):
        raise BridgeError("static family list is missing")

    by_id = {family.get("id"): family for family in families}
    if set(by_id) != set(STATIC_EXPECTED):
        raise BridgeError("native/static normal-enemy family sets differ")

    for family_id, (left_type, right_type, callback) in STATIC_EXPECTED.items():
        family = by_id[family_id]
        types = [str(value).lower() for value in family.get("types", [])]
        if types != [left_type, right_type]:
            raise BridgeError(f"{family_id}: static type pair drifted")
        static_callback = str(family["callback"]["address"]).split(":")[-1]
        static_callback = static_callback.split("-")[0].lower()
        if static_callback != callback[2:]:
            raise BridgeError(f"{family_id}: static callback drifted")
        if callback not in source.lower():
            raise BridgeError(f"{family_id}: native callback identity missing")
        note_forms = (callback, "01f7:" + callback[2:])
        if not any(form in note.lower() for form in note_forms):
            raise BridgeError(f"{family_id}: bridge note callback missing")

    for family_id, (_, _, callback) in EXPECTED.items():
        note_forms = (callback, "01f7:" + callback[2:])
        if callback not in source.lower() or not any(
            form in note.lower() for form in note_forms
        ):
            raise BridgeError(f"{family_id}: native bridge identity missing")

    for token in (
        "normalEnemyYOffset",
        "updateNormalEnemy",
        "enemyPhase34",
        "enemySineOrProbe39",
        "enemyVerticalState36",
        "enemyTransitionState3d",
        "enemySourceOrKind2c",
        "enemyAux3e",
        "enemyVerticalOffset40",
        "enemyOriginY36",
        "enemySavedVelocity3a",
        "enemySavedDirection44",
    ):
        if token not in source and token not in header:
            raise BridgeError(f"native bridge token missing: {token}")

    if "0x4ba0" not in source.lower() or "0x4ab3" not in source.lower():
        raise BridgeError("contact response split is missing")
    if "01f7:7ae3" not in note.lower():
        raise BridgeError("FISCH initializer evidence missing")

    # WURM2 is the first family whose native map branch is descriptor-backed
    # instead of using the generic raw-MAP shortcut.  Keep its recovered
    # 1C4D/5C27 order and the second-probe JZ polarity mechanically checked.
    if "1c4d -> 5c27" not in note.lower():
        raise BridgeError("WURM2 descriptor-probe contract missing")
    for token in ("hasDescriptorTable", "blocksProbeConfirmed", "orientedX",
                  "objectY - 0x28", "sideX", "return !world.blocksProbeConfirmed"):
        if token not in source:
            raise BridgeError(f"WURM2 descriptor-probe token missing: {token}")


if __name__ == "__main__":
    verify()
    print("OK: native normal-enemy bridge matches static family closure")
