#!/usr/bin/env python3
"""Verify the focused player external-state contract and export ledger."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
LEDGER = ROOT / "research/ghidra/player-external-state-closure.json"
ALLOWED_CLASSIFICATIONS = {"inline", "contract", "irrelevant", "unresolved"}
REQUIRED_ADDRESSES = {
    "01F7:0E06", "01F7:05A0", "01F7:0931", "01F7:0B56", "01F7:0E96", "01F7:0FDC", "01F7:34BC", "01F7:0FA2", "01F7:1036",
    "01F7:39FE", "01F7:9C70", "01F7:5DA1", "01F7:9CF5", "01F7:9D19",
    "01F7:9D5E", "01F7:9D82", "01F7:9DC7", "01F7:A075", "01F7:A0B2", "01F7:5DC3",
    "01F7:5937", "01F7:386F", "01F7:0442", "01F7:1C6E", "01F7:1C92",
    "01F7:0517", "01F7:04DF",
    "01F7:5C27", "01F7:5CC3", "01F7:5D38", "01F7:5D60", "01F7:16CE",
    "01F7:10B5", "01F7:1693", "01F7:3376", "01F7:6370",
    "01F7:6484", "01F7:648E", "01F7:4519",
    "01F7:38CA", "01F7:38EC", "01F7:1BD1",
    "01F7:9BEE", "01F7:9C0C", "01F7:1B07", "01F7:1B5D",
    "01F7:1749", "01F7:178D", "01F7:1798", "01F7:17A3", "01F7:1892", "01F7:5C11",
    "01F7:45AB", "01F7:470C", "01F7:6328", "01F7:1DEE", "01F7:1DCA", "01F7:1C4D", "01F7:6D5F", "01F7:6DA3", "01F7:6DB1", "01F7:6DC4", "01F7:68C0", "01F7:684A", "01F7:689F", "01F7:68AD", "01F7:4AB3", "01F7:4BA0", "01F7:4C5D", "01F7:4C8B", "01F7:4D44", "01F7:4DCE", "01F7:4EC9", "01F7:4F82", "01F7:4416-44FE", "01F7:1CDA", "01F7:1E04", "01F7:321F",
    "01F7:17AE", "01F7:1ED7", "01F7:3062",
    "01F7:1B77", "01F7:393C", "01F7:3A8A",
    "01F7:1997", "01F7:3529", "01F7:34E3", "01F7:3808",
    "01F7:199D", "01F7:19E6", "01F7:1AAA", "01F7:1AE6", "01F7:1AF5",
    "01F7:4727", "01F7:47E7",
    "01F7:8BC2", "01F7:8BE5", "01F7:8C08", "01F7:8C2B", "01F7:8C4E",
    "01F7:8C71", "01F7:8C8A", "01F7:8CA3", "01F7:8CBC", "01F7:8CD5", "01F7:8CEE", "01F7:8D07",
    "01F7:8D20", "01F7:8D31", "01F7:8E4B", "01F7:9256", "01F7:9269",
    "01D7:3FAD", "01D7:34C7", "01D7:3861", "01D7:39ED", "01D7:4BA4", "01D7:14E1",
    "01D7:4A39-4B7B",
}
CONTRACT_FIELDS = {
    "address", "name", "classification", "calling_convention",
    "inputs_outputs", "player_reads", "player_writes", "global_reads",
    "global_writes", "probe_coordinates", "scheduler_position",
    "object_effects", "feeds_back_into_simulation", "callees", "callers",
    "confidence", "evidence",
}
ADDRESS_RE = re.compile(r"^[0-9A-F]{4}:[0-9A-F]{4}(?:-[0-9A-F]{4})?$")


class ExternalClosureError(Exception):
    pass


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def load(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise ExternalClosureError("ledger top-level value must be an object")
    if value.get("schema") != "quiky.player-external-state-closure.v1":
        raise ExternalClosureError("unexpected external-state closure schema")
    return value


def check_source_hashes(ledger: dict[str, Any], root: Path) -> None:
    source = ledger.get("source")
    if not isinstance(source, dict):
        raise ExternalClosureError("source must be an object")
    executable = root / source["executable"]
    segment3 = root / source["segment3"]
    segment1 = root / source["segment1"]
    for path in (executable, segment3, segment1):
        if not path.is_file():
            raise ExternalClosureError(f"missing source artifact: {path}")
    if sha256(executable) != source["executable_sha256"]:
        raise ExternalClosureError(f"executable SHA-256 drift: {executable}")
    if sha256(segment3) != source["segment3_sha256"]:
        raise ExternalClosureError(f"segment-3 SHA-256 drift: {segment3}")
    if source.get("segment1_sha256") and sha256(segment1) != source["segment1_sha256"]:
        raise ExternalClosureError(f"segment-1 SHA-256 drift: {segment1}")


def check_contracts(ledger: dict[str, Any]) -> None:
    functions = ledger.get("functions")
    if not isinstance(functions, list) or not functions:
        raise ExternalClosureError("functions must be a non-empty array")
    seen: set[str] = set()
    by_address: dict[str, dict[str, Any]] = {}
    for index, function in enumerate(functions, 1):
        if not isinstance(function, dict):
            raise ExternalClosureError(f"functions[{index}] must be an object")
        missing = sorted(CONTRACT_FIELDS - function.keys())
        if missing:
            raise ExternalClosureError(
                f"{function.get('address', index)} lacks: {', '.join(missing)}"
            )
        address = function["address"]
        if not isinstance(address, str) or not ADDRESS_RE.fullmatch(address):
            raise ExternalClosureError(f"invalid address-qualified contract: {address!r}")
        if address in seen:
            raise ExternalClosureError(f"duplicate contract {address}")
        seen.add(address)
        by_address[address] = function
        if function["classification"] not in ALLOWED_CLASSIFICATIONS:
            raise ExternalClosureError(f"{address}: invalid classification")
        for field in (
            "player_reads", "player_writes", "global_reads", "global_writes",
            "probe_coordinates", "callees", "callers", "evidence",
        ):
            if not isinstance(function[field], list):
                raise ExternalClosureError(f"{address}.{field} must be an array")
        if not function["evidence"]:
            raise ExternalClosureError(f"{address} has no evidence")
        if function["classification"] == "unresolved":
            name = function["name"]
            if not any(token in name for token in (address.split(":", 1)[1], "address", "unknown")):
                raise ExternalClosureError(
                    f"unresolved contract {address} has a speculative semantic name: {name}"
                )

    missing = sorted(REQUIRED_ADDRESSES - seen)
    if missing:
        raise ExternalClosureError("missing required contracts: " + ", ".join(missing))

    if by_address["01F7:0E96"]["callees"][0]["address"] != "01F7:0FDC":
        raise ExternalClosureError("0E96 must retain its 0FDC tail contract")
    if by_address["01F7:0FDC"]["callees"][0]["address"] != "01F7:34BC":
        raise ExternalClosureError("0FDC must retain its visible-queue append edge")
    visible_queue = by_address["01F7:34BC"]
    if visible_queue["classification"] != "irrelevant":
        raise ExternalClosureError("34BC must remain presentation-only")
    if visible_queue["player_writes"] or visible_queue["feeds_back_into_simulation"] != "no":
        raise ExternalClosureError("34BC gained an unverified simulation side effect")
    if visible_queue["global_writes"] != ["DS:8174 += 1", "visible queue record at ES:DI"]:
        raise ExternalClosureError("34BC visible-queue write contract drifted")
    if by_address["01F7:5937"]["callees"][0]["address"] != "01F7:386F":
        raise ExternalClosureError("5937 must retain the 386F view-state dispatch")
    if by_address["01F7:386F"]["callees"][0]["address"] != "01F7:0442":
        raise ExternalClosureError("386F must retain the 0442 indirect boundary")
    for address in ("01F7:A0B2", "01F7:5937", "01F7:386F"):
        if not by_address[address]["global_writes"]:
            raise ExternalClosureError(f"{address} lost its global-write contract")
    if by_address["01F7:8D20"]["callees"][-1]["address"] != "01F7:8D31":
        raise ExternalClosureError("8D20 must retain its 8D31 state-body edge")
    if not any("DS:60D8" in item for item in by_address["01F7:8D31"]["global_writes"]):
        raise ExternalClosureError("8D31 must retain the collected-mask write")
    if "DS:89E6=FFFF" not in " ".join(by_address["01F7:9269"]["global_writes"]):
        raise ExternalClosureError("9269 must retain the cloud transition-latch write")
    if not any("DS:6468" in item for item in by_address["01F7:4727"]["global_writes"]):
        raise ExternalClosureError("4727 must retain its PRNG cursor write")
    effect = by_address["01F7:8E4B"]
    if not any("DS:8828" in item and "word" in item for item in effect["global_writes"]):
        raise ExternalClosureError("8E4B must retain its state-10 X publication")
    if not any("DS:882A" in item and "word" in item for item in effect["global_writes"]):
        raise ExternalClosureError("8E4B must retain its state-10 Y publication")
    if [callee["address"] for callee in effect["callees"][-2:]] != [
        "01F7:3376", "01F7:16CE",
    ]:
        raise ExternalClosureError("8E4B must retain its map lookup/effect-writer edge")
    progression = by_address["01D7:14E1"]
    if progression["classification"] != "contract":
        raise ExternalClosureError("14E1 must remain a contract boundary")
    if progression["player_writes"] != []:
        raise ExternalClosureError("14E1 gained an unverified player write")
    writes = " ".join(progression["global_writes"])
    for token in ("DS:612E=0x000C", "DS:880C decremented", "DS:881C/DS:881E += 0x0014", "DS:881C/DS:881E += 0x07D0", "DS:85DB=1"):
        if token not in writes:
            raise ExternalClosureError(f"14E1 lost static write contract: {token}")
    if progression["callers"][0]["address"] != "01D7:4F0D":
        raise ExternalClosureError("14E1 caller drifted from 4F0D")

    common = {
        item.get("address"): item
        for item in functions
        if isinstance(item, dict)
    }
    for address in ("01F7:1997", "01F7:3529", "01F7:34E3", "01F7:3808"):
        if address not in common:
            raise ExternalClosureError(f"common callback contract is missing: {address}")
        if common[address]["classification"] != "irrelevant":
            raise ExternalClosureError(f"common callback must remain irrelevant: {address}")
        if common[address]["player_writes"]:
            raise ExternalClosureError(f"common callback gained a player write: {address}")
        if common[address]["feeds_back_into_simulation"] != "no":
            raise ExternalClosureError(f"common callback feedback classification drifted: {address}")
    if [item.get("address") for item in common["01F7:1997"]["callees"]] != ["01F7:3529"]:
        raise ExternalClosureError("1997 wrapper edge drifted")
    if [item.get("address") for item in common["01F7:3529"]["callees"]] != ["01F7:34E3"]:
        raise ExternalClosureError("3529 callback edge drifted")
    if [item.get("address") for item in common["01F7:34E3"]["callees"]] != ["01F7:3808"]:
        raise ExternalClosureError("34E3 queue-writer edge drifted")
    if common["01F7:3808"]["callees"]:
        raise ExternalClosureError("3808 must remain a leaf")
    if common["01F7:34E3"]["global_writes"] != [
        "queue record at DS:37DE + DS:37E0*0x10", "DS:37E0 += 1",
    ]:
        raise ExternalClosureError("34E3 queue-write contract drifted")
    if common["01F7:3808"]["global_reads"] != ["DS:81CC"]:
        raise ExternalClosureError("3808 global-read contract drifted")
    if not any(item.get("address") == "01F7:1997" for item in by_address["01F7:0FA2"]["callees"]):
        raise ExternalClosureError("0FA2 must retain the default 1997 callback edge")


def check_5937_dispatch_static(ledger: dict[str, Any], root: Path) -> None:
    closure = ledger.get("5937_dispatch_static")
    if not isinstance(closure, dict):
        raise ExternalClosureError("5937_dispatch_static must be an object")
    if closure.get("status") != "direct_5937_body_and_observed_level_startup_targets_non_simulation_other_runtime_targets_unresolved":
        raise ExternalClosureError("5937 dispatch static status drifted")
    if set(closure.get("targets", [])) != {"01F7:0442", "01F7:0598"}:
        raise ExternalClosureError("5937 dispatch target set drifted")
    if closure.get("shared_body_offsets") != ["01F7:04DF", "01F7:0517"]:
        raise ExternalClosureError("5937 shared body offset set drifted")
    observed_targets = ["1247:1470", "1387:03FC", "1397:1040", "14A7:1258", "163F:0C10"]
    if closure.get("observed_runtime_targets") != observed_targets:
        raise ExternalClosureError("5937 observed runtime target set drifted")
    matrix_path = root / closure.get("observed_runtime_target_matrix", "")
    if not matrix_path.is_file():
        raise ExternalClosureError(f"missing 5937 runtime target matrix: {matrix_path}")
    try:
        matrix = json.loads(matrix_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read 5937 runtime target matrix: {exc}") from exc
    if matrix.get("schema") != "quiky.player-5937-runtime-target-level-matrix.v1":
        raise ExternalClosureError("unexpected 5937 runtime target matrix schema")
    if matrix.get("source", {}).get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("5937 runtime target matrix executable hash drift")
    if matrix.get("source", {}).get("trace_script_sha256") != "0f0c1e769b09e923d02a536679338134b533a2785e6c9b06e5cb915e8a245b6c":
        raise ExternalClosureError("5937 runtime target matrix trace-script hash drift")
    records = matrix.get("dispatch", {}).get("records")
    if not isinstance(records, list) or [item.get("record_callback", {}).get("segment", "").removeprefix("0x") + ":" + item.get("record_callback", {}).get("offset", "").removeprefix("0x") for item in records] != [
        "1247:1470", "1397:1040", "1387:03FC", "163F:0C10",
    ]:
        raise ExternalClosureError("5937 runtime target matrix callback set drifted")
    for item in records:
        if item.get("feeds_back_into_player_simulation") is not False:
            raise ExternalClosureError("5937 runtime target matrix contains an unbounded simulation target")
        if item.get("dispatched_object_record", {}).get("differences") != []:
            raise ExternalClosureError("5937 runtime target matrix object differences drifted")
        if item.get("player_record_differences") != [] or item.get("original_data_differences") != []:
            raise ExternalClosureError("5937 runtime target matrix simulation differences drifted")
    if closure.get("direct_player_or_callback_global_writes") != []:
        raise ExternalClosureError("5937 dispatch direct simulation writes are not empty")
    if closure.get("direct_read_set") != [
        "DS:85DA", "DS:60D8", "DS:60DA", "DS:881C", "DS:8822",
        "DS:880A", "DS:880C", "DS:4FF2", "DS:4FF6", "DS:4FF8", "DS:4FFA",
    ]:
        raise ExternalClosureError("5937 direct read set drifted")
    if closure.get("direct_writes")[:4] != [
        "DS:60DA", "DS:4FF2", "DS:4FF8", "DS:4FFA",
    ]:
        raise ExternalClosureError("5937 direct write set drifted")
    expected_matrix = [
        {"bit": "0x01", "ax": "0x0109", "bx": "0x0006", "cx": "0x026D"},
        {"bit": "0x02", "ax": "0x0113", "bx": "0x0006", "cx": "0x026E"},
        {"bit": "0x04", "ax": "0x011A", "bx": "0x0006", "cx": "0x026F"},
        {"bit": "0x08", "ax": "0x0121", "bx": "0x0006", "cx": "0x0270"},
        {"bit": "0x10", "ax": "0x0128", "bx": "0x0006", "cx": "0x0271"},
        {"bit": "0x20", "ax": "0x012F", "bx": "0x0006", "cx": "0x0272"},
        {"bit": "0x40", "ax": "0x0132", "bx": "0x0006", "cx": "0x0273"},
    ]
    if closure.get("changed_bit_dispatch_matrix") != expected_matrix:
        raise ExternalClosureError("5937 changed-bit dispatch matrix drifted")
    helper_path = root / closure.get("helper_static_decomp_note", "")
    if not helper_path.is_file():
        raise ExternalClosureError(f"missing 5937 helper static note: {helper_path}")
    if sha256(helper_path) != closure.get("helper_static_decomp_note_sha256"):
        raise ExternalClosureError("5937 helper static note hash drift")
    helper_text = helper_path.read_text(encoding="utf-8")
    for anchor in (
        "update_auxiliary_player_dispatch_5937",
        "kChangedBitDispatch",
        "0x0109",
        "dispatch_score_digits",
        "DS:4FF6 is not written here",
        "No player-record offset",
    ):
        if anchor not in helper_text:
            raise ExternalClosureError(f"5937 helper note missing anchor: {anchor}")
    helper_evidence = root / "research/evidence/player-dos-parity/5937-helper-static-v1.json"
    if not helper_evidence.is_file():
        raise ExternalClosureError(f"missing 5937 helper static evidence: {helper_evidence}")
    try:
        helper_payload = json.loads(helper_evidence.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read 5937 helper static evidence: {exc}") from exc
    if helper_payload.get("schema") != "quiky.player-5937-helper-static.v1":
        raise ExternalClosureError("unexpected 5937 helper evidence schema")
    if helper_payload.get("source", {}).get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("5937 helper evidence executable hash drift")
    if helper_payload.get("contract", {}).get("global_writes") != [
        "DS:60DA", "DS:4FF2", "DS:4FF8", "DS:4FFA",
    ]:
        raise ExternalClosureError("5937 helper evidence write set drifted")
    call_site = closure.get("indirect_call_site_0598")
    if not isinstance(call_site, dict) or call_site.get("address") != "01F7:0598":
        raise ExternalClosureError("5937 dispatch must retain the 0598 call-site contract")
    if call_site.get("classification") != "contract":
        raise ExternalClosureError("0598 must remain a contract boundary")
    if call_site.get("player_writes") != [] or call_site.get("global_writes") != []:
        raise ExternalClosureError("0598 direct simulation writes are not empty")
    callees = {item.get("address") for item in call_site.get("callees", [])}
    if not set(observed_targets).issubset(callees):
        raise ExternalClosureError("0598 observed level-start target set drifted")
    runtime_evidence = root / "research/evidence/player-dos-parity/5937-runtime-target-w1l1-v1.json"
    if not runtime_evidence.is_file():
        raise ExternalClosureError(f"missing 5937 runtime target evidence: {runtime_evidence}")
    try:
        runtime_payload = json.loads(runtime_evidence.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read 5937 runtime target evidence: {exc}") from exc
    if runtime_payload.get("schema") != "quiky.player-5937-runtime-target-w1l1.v1":
        raise ExternalClosureError("unexpected 5937 runtime target evidence schema")
    if runtime_payload.get("source", {}).get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("5937 runtime target executable hash drift")
    if runtime_payload.get("source", {}).get("trace_sha256") != "4b67593418dad24cb9a0b3dccb42491334bf15cac744ab4c039c97614225234d":
        raise ExternalClosureError("5937 runtime target trace hash drift")
    target = runtime_payload.get("target_contract", {})
    if target.get("address") != "14A7:1258" or target.get("feeds_back_into_player_simulation") is not False:
        raise ExternalClosureError("5937 runtime target contract drifted")
    if target.get("player_context", {}).get("record_differences") != [] or target.get("original_data_differences") != []:
        raise ExternalClosureError("5937 runtime target simulation-difference contract drifted")
    runtime_static_note = root / closure.get("runtime_target_static_note", "")
    if not runtime_static_note.is_file():
        raise ExternalClosureError(f"missing captured runtime-target static note: {runtime_static_note}")
    if sha256(runtime_static_note) != closure.get("runtime_target_static_note_sha256"):
        raise ExternalClosureError("captured runtime-target static note hash drift")
    runtime_static_evidence = root / closure.get("runtime_target_static_evidence", "")
    if not runtime_static_evidence.is_file():
        raise ExternalClosureError(f"missing captured runtime-target static evidence: {runtime_static_evidence}")
    try:
        runtime_static_payload = json.loads(runtime_static_evidence.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read captured runtime-target static evidence: {exc}") from exc
    if runtime_static_payload.get("schema") != "quiky.player-5937-runtime-target-static.v1":
        raise ExternalClosureError("unexpected captured runtime-target static evidence schema")
    if runtime_static_payload.get("source", {}).get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("captured runtime-target executable hash drift")
    static_target = runtime_static_payload.get("target", {})
    if static_target.get("address") != "14EF:0218":
        raise ExternalClosureError("captured runtime target address drifted")
    if static_target.get("classification") != "contract" or static_target.get("callees") != []:
        raise ExternalClosureError("captured runtime target contract drifted")
    static_exports = runtime_static_payload.get("static_exports", {})
    if static_exports.get("independent_project_a_equals_b") is not True:
        raise ExternalClosureError("captured runtime target independent-project check is not true")
    static_note = runtime_static_note.read_text(encoding="utf-8")
    for anchor in (
        "address_named_loaded_callback_14EF_0218",
        "RETF at 14EF:0358",
        "DS:SI-relative",
        "four port writes",
        "other DS:6D8A targets remain address-named and unresolved",
    ):
        if anchor not in static_note:
            raise ExternalClosureError(f"captured runtime-target note missing anchor: {anchor}")
    matrix_note_path = root / closure.get("runtime_target_static_matrix_note", "")
    if not matrix_note_path.is_file():
        raise ExternalClosureError(f"missing runtime-target matrix note: {matrix_note_path}")
    if sha256(matrix_note_path) != closure.get("runtime_target_static_matrix_note_sha256"):
        raise ExternalClosureError("runtime-target matrix note hash drift")
    matrix_evidence_path = root / closure.get("runtime_target_static_matrix_evidence", "")
    if not matrix_evidence_path.is_file():
        raise ExternalClosureError(f"missing runtime-target matrix evidence: {matrix_evidence_path}")
    try:
        matrix_payload = json.loads(matrix_evidence_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read runtime-target matrix evidence: {exc}") from exc
    if matrix_payload.get("schema") != "quiky.player-5937-runtime-target-static-matrix.v1":
        raise ExternalClosureError("unexpected runtime-target matrix evidence schema")
    if matrix_payload.get("source", {}).get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("runtime-target matrix executable hash drift")
    expected_runtime_targets = {
        "14EF:0218", "14F7:03C8", "14FF:04F4", "1507:0640",
        "150F:0770", "1517:0888", "151F:097C",
    }
    matrix_targets = matrix_payload.get("targets")
    if not isinstance(matrix_targets, list) or {
        item.get("address") for item in matrix_targets if isinstance(item, dict)
    } != expected_runtime_targets:
        raise ExternalClosureError("runtime-target matrix target set drifted")
    for item in matrix_targets:
        if item.get("call_count") != 0:
            raise ExternalClosureError("runtime-target matrix contains a call")
        if item.get("simulation_differences") != {"player": [], "globals": [], "object": []}:
            raise ExternalClosureError("runtime-target matrix simulation differences drifted")
    matrix_note = matrix_note_path.read_text(encoding="utf-8")
    for anchor in (
        "changed-bit 01F7:5937 dispatch matrix",
        "no calls; RETF",
        "The seven entry/return samples",
    ):
        if anchor not in matrix_note:
            raise ExternalClosureError(f"runtime-target matrix note missing anchor: {anchor}")
    note_path = root / closure.get("static_decomp_note", "")
    if not note_path.is_file():
        raise ExternalClosureError(f"missing 5937 dispatch static note: {note_path}")
    if sha256(note_path) != closure.get("static_decomp_note_sha256"):
        raise ExternalClosureError("5937 dispatch static note hash drift")
    note = note_path.read_text(encoding="utf-8")
    for anchor in (
        "address_named_dispatch_0442",
        "address_named_dispatch_entry_04df",
        "address_named_dispatch_entry_0517",
        "RETF 0x6 at 059D",
        "record +0x18/+0x1A",
        "VGA sequencer port",
    ):
        if anchor not in note:
            raise ExternalClosureError(f"5937 dispatch note missing anchor: {anchor}")


def check_animation_data_static(ledger: dict[str, Any], root: Path) -> None:
    animation = ledger.get("animation_data_static")
    if not isinstance(animation, dict):
        raise ExternalClosureError("animation_data_static must be an object")
    if animation.get("status") != "confirmed_static_runtime_ds_segment_a_b_equal":
        raise ExternalClosureError("animation data static status drifted")
    if animation.get("source_executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("animation data executable hash drift")
    if animation.get("runtime_ds_selector") != "0237":
        raise ExternalClosureError("animation data runtime DS selector drifted")
    expected = {
        "01F7:3142": [4, 0, 1, 2, 3, 4, 5, 6],
        "01F7:3156": [4, 0, 0, 0, -3],
        "01F7:3160": [8, 10, 11, 12, -1],
        "01F7:316A": [14, 0, 16, 17, 18, 18, 19, 19, 19, 18, 17, 16, 0, -1],
        "01F7:3186": [20, 13, 14, 15, -1],
        "01F7:3190": [2, 0, 1, 2, 3, 4, 5, 6],
        "01F7:31A4": [14, 20, 21, 22, 23, 24, 25, 26, 27, 28, -3],
        "01F7:31BA": [15, 30, 31, 32, 33, 33, 33, 33, 34, 34, 34, 35, 36, 37],
    }
    if animation.get("descriptors") != expected:
        raise ExternalClosureError("animation descriptor words drifted")
    evidence_path = root / "research/evidence/player-dos-parity/player-animation-tables-static-v1.json"
    if not evidence_path.is_file():
        raise ExternalClosureError(f"missing animation data evidence: {evidence_path}")
    try:
        evidence = json.loads(evidence_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read animation data evidence: {exc}") from exc
    if evidence.get("schema") != "quiky.player-animation-tables-static.v1":
        raise ExternalClosureError("unexpected animation data evidence schema")
    if evidence.get("source", {}).get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("animation evidence executable hash drift")
    if evidence.get("descriptors") != expected:
        raise ExternalClosureError("animation evidence descriptor words drifted")
    if evidence.get("comparison", {}).get("project_a_equals_project_b") is not True:
        raise ExternalClosureError("animation data independent-project check is not true")
    note_path = root / animation["static_decomp_note"]
    if not note_path.is_file():
        raise ExternalClosureError(f"missing animation data static note: {note_path}")
    note = note_path.read_text(encoding="utf-8")
    for anchor in (
        "QUIKY_SEG06",
        "animation_3142",
        "animation_3190",
        "01F7:3AB9",
        "DumpDataWords.java",
    ):
        if anchor not in note:
            raise ExternalClosureError(f"animation data note missing anchor: {anchor}")
    engine = root / "engine/src/player_update.cpp"
    engine_text = engine.read_text(encoding="utf-8")
    for token in ("kAnimation3142", "kAnimation3190", "{0x3142", "{0x3190"):
        if token not in engine_text:
            raise ExternalClosureError(f"engine animation table missing {token}")


def check_5937_record_manager_static(ledger: dict[str, Any], root: Path) -> None:
    by_address = {
        item.get("address"): item
        for item in ledger.get("functions", [])
        if isinstance(item, dict)
    }
    manager = by_address.get("01F7:05A0")
    if not isinstance(manager, dict):
        raise ExternalClosureError("05A0 record manager contract is missing")
    if manager.get("classification") != "contract":
        raise ExternalClosureError("05A0 must remain a contract boundary")
    if manager.get("player_writes") != []:
        raise ExternalClosureError("05A0 gained an unverified player write")
    if manager.get("global_writes") != ["DS:6D8E[selector] when a free record is found"]:
        raise ExternalClosureError("05A0 selector-map write contract drifted")
    if [item.get("address") for item in manager.get("callees", [])] != [
        "0227:05CD", "01F7:0931",
    ]:
        raise ExternalClosureError("05A0 callee relocation order drifted")
    releaser = by_address.get("01F7:0931")
    if not isinstance(releaser, dict) or releaser.get("global_writes") != [
        "selected record resource pointer words", "DS:6D8E[selector]=FFFF",
    ]:
        raise ExternalClosureError("0931 resource-release contract drifted")
    evidence = root / "research/evidence/player-dos-parity/5937-dispatch-table-manager-static-v1.json"
    if not evidence.is_file():
        raise ExternalClosureError(f"missing 05A0 static evidence: {evidence}")
    try:
        payload = json.loads(evidence.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read 05A0 static evidence: {exc}") from exc
    if payload.get("schema") != "quiky.5937-dispatch-table-manager-static.v1":
        raise ExternalClosureError("unexpected 05A0 static evidence schema")
    if payload.get("source", {}).get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("05A0 static evidence executable hash drift")
    note = root / "research/notes/5937-dispatch-table-manager-static-decomp.cpp"
    if not note.is_file() or sha256(note) != "9b94be43a66ce4fed00a622b997be17e469260268d57aa4521bae3e058e74490":
        raise ExternalClosureError("05A0 static note hash drift")


def check_5937_record_loader_static(ledger: dict[str, Any], root: Path) -> None:
    by_address = {
        item.get("address"): item
        for item in ledger.get("functions", [])
        if isinstance(item, dict)
    }
    loader = by_address.get("01D7:39ED")
    if not isinstance(loader, dict):
        raise ExternalClosureError("39ED dispatch-record loader contract is missing")
    if loader.get("classification") != "contract":
        raise ExternalClosureError("39ED must remain a contract boundary")
    if loader.get("player_reads") != [] or loader.get("player_writes") != []:
        raise ExternalClosureError("39ED gained an unverified player access")
    if loader.get("global_reads") != ["DS:97E2", "DS:6D8A"]:
        raise ExternalClosureError("39ED global-read contract drifted")
    if loader.get("global_writes") != [
        "DS:6D8A record fields +0x00/+0x02/+0x08/+0x0A/+0x0C/+0x10/+0x12/+0x18/+0x1A/+0x20/+0x22/+0x28",
    ]:
        raise ExternalClosureError("39ED record-write contract drifted")
    if [item.get("address") for item in loader.get("callees", [])] != [
        "01F7:05A0", "address-named resource helpers",
    ]:
        raise ExternalClosureError("39ED callee boundary drifted")
    if "01D7:39ED-3BAB" not in ledger.get("focused_static_export", {}).get("mainloop_listing_ranges", []):
        raise ExternalClosureError("39ED listing range is missing")
    evidence = root / "research/evidence/player-dos-parity/5937-dispatch-record-loader-static-v1.json"
    if not evidence.is_file():
        raise ExternalClosureError(f"missing 39ED static evidence: {evidence}")
    try:
        payload = json.loads(evidence.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read 39ED static evidence: {exc}") from exc
    if payload.get("schema") != "quiky.5937-dispatch-record-loader-static.v1":
        raise ExternalClosureError("unexpected 39ED static evidence schema")
    if payload.get("source", {}).get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("39ED static evidence executable hash drift")
    note = root / "research/notes/5937-dispatch-record-loader-static-decomp.cpp"
    if not note.is_file() or sha256(note) != "5d1e126a042850be6d06688619ef089b498e5b6bf566c07949b7afb32cd11b62":
        raise ExternalClosureError("39ED static note hash drift")
    note_text = note.read_text(encoding="utf-8")
    for anchor in (
        "DispatchRecord",
        "manage_dispatch_record_selector_05A0",
        "continue; // 3BA7: JMP 39ED",
        "record.callback_off = record.resource_a_off",
    ):
        if anchor not in note_text:
            raise ExternalClosureError(f"39ED static note missing anchor: {anchor}")


def check_bump_static(ledger: dict[str, Any], root: Path) -> None:
    """Verify the focused type-0x34 callback and its player-response helper."""
    by_address = {
        item.get("address"): item
        for item in ledger.get("functions", [])
        if isinstance(item, dict)
    }
    expected = {
        "01F7:9BEE": ["01F7:5D38"],
        "01F7:9C0C": [
            "01F7:1DCA", "01F7:1DEE", "01F7:5D60", "01F7:39FE",
            "01F7:5D38", "01F7:1B5D", "01E7:0FCF",
        ],
        "01F7:1B07": ["01F7:5D38"],
        "01F7:1B5D": ["01F7:1B07"],
    }
    for address, callees in expected.items():
        function = by_address.get(address)
        if not isinstance(function, dict):
            raise ExternalClosureError(f"missing BUMP contract: {address}")
        if function.get("classification") != "contract":
            raise ExternalClosureError(f"BUMP contract must remain a contract: {address}")
        if [item.get("address") for item in function.get("callees", [])] != callees:
            raise ExternalClosureError(f"BUMP callee order drifted: {address}")
    initializer = by_address["01F7:9BEE"]
    if initializer.get("player_writes") != []:
        raise ExternalClosureError("9BEE gained an unverified player write")
    callback = by_address["01F7:9C0C"]
    callback_writes = " ".join(callback.get("global_writes", []))
    if "DS:612E=0x0004" not in callback_writes:
        raise ExternalClosureError("9C0C lost its DS:612E contact write")
    if "DS:8950 &= 0xffcf" not in callback_writes:
        raise ExternalClosureError("9C0C lost the nested DS:8950 mask write")
    transition = by_address["01F7:1B07"]
    if transition.get("global_writes") != ["DS:8950 &= 0xffcf"]:
        raise ExternalClosureError("1B07 global-write contract drifted")
    for field in ("+0x0E", "+0x37", "+0x3A", "+0x3B", "+0x3E"):
        if field not in transition.get("player_writes", []):
            raise ExternalClosureError(f"1B07 lost player write {field}")
    response = by_address["01F7:1B5D"]
    if "+0x2B=0xFF" not in response.get("player_writes", []):
        raise ExternalClosureError("1B5D lost +0x2B response write")
    if "+0x0E -= 0x0001B000 modulo 32 bits" not in response.get("player_writes", []):
        raise ExternalClosureError("1B5D lost wrapping vertical response")
    evidence_path = root / "research/evidence/player-dos-parity/bump-static-v1.json"
    if not evidence_path.is_file():
        raise ExternalClosureError(f"missing BUMP static evidence: {evidence_path}")
    try:
        evidence = json.loads(evidence_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read BUMP static evidence: {exc}") from exc
    if evidence.get("schema") != "quiky.bump-static.v1":
        raise ExternalClosureError("unexpected BUMP static evidence schema")
    if evidence.get("source", {}).get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("BUMP static evidence executable hash drift")
    note = root / "research/notes/bump-static-decomp.cpp"
    expected_hash = "83cc933dfc49d79752bbcce15241823d2f74c4e5d4eae0fae9baba67b39886f9"
    if not note.is_file() or sha256(note) != expected_hash:
        raise ExternalClosureError("BUMP static note hash drift")
    note_text = note.read_text(encoding="utf-8")
    for anchor in (
        "initialize_bump_9BEE",
        "DS_85DA < 0x32",
        "apply_tile_transition_1B07_static",
        "apply_bump_player_response_1B5D",
        "DS_612E = 0x0004",
        "word ADD",
    ):
        if anchor not in note_text:
            raise ExternalClosureError(f"BUMP static note missing anchor: {anchor}")


def check_progression_static(ledger: dict[str, Any], root: Path) -> None:
    evidence_path = root / "research/evidence/player-dos-parity/player-progression-static-v1.json"
    if not evidence_path.is_file():
        raise ExternalClosureError(f"missing progression static evidence: {evidence_path}")
    try:
        evidence = json.loads(evidence_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read progression static evidence: {exc}") from exc
    if evidence.get("schema") != "quiky.player-progression-static.v1":
        raise ExternalClosureError("unexpected progression static evidence schema")
    source = evidence.get("source", {})
    if source.get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("progression static executable hash drift")
    if source.get("language") != "x86:LE:16:Protected Mode:default":
        raise ExternalClosureError("progression static language drift")
    if source.get("independent_project_check") != "passed":
        raise ExternalClosureError("progression static independent check missing")
    for path_key, hash_key, independent_hash_key in (
        ("decomp_mainloop_a", "decomp_mainloop_sha256", "independent_decomp_sha256"),
        ("disasm_a", "disasm_sha256", "independent_disasm_sha256"),
    ):
        path = root / source[path_key]
        if not path.is_file() or sha256(path) != source[hash_key]:
            raise ExternalClosureError(f"progression static export hash drift: {path}")
        independent = root / source[path_key].replace("-a/", "-b/")
        if not independent.is_file() or sha256(independent) != source[independent_hash_key]:
            raise ExternalClosureError(f"progression independent export hash drift: {independent}")
    extent = evidence.get("function_extent", {})
    if extent.get("address") != "01D7:14E1" or extent.get("last_instruction") != "01D7:1733" or extent.get("next_function") != "01D7:1734" or extent.get("listing_count") != 595:
        raise ExternalClosureError("14E1 function extent drifted")
    contracts = {item.get("address"): item for item in evidence.get("static_contracts", []) if isinstance(item, dict)}
    if not {"01D7:4EA0-4F0D", "01D7:14E1", "01D7:4F10-5010"}.issubset(contracts):
        raise ExternalClosureError("progression static contract set is incomplete")
    if contracts["01D7:14E1"].get("callee_order", [])[0] != "01D7:021D":
        raise ExternalClosureError("14E1 first callee drifted")
    if "DS:881C:DS:881E += 0x07D0 at 16DE" not in " ".join(contracts["01D7:14E1"].get("global_writes", [])):
        raise ExternalClosureError("14E1 all-seven score write drifted")
    relocations = {(item.get("instruction"), item.get("target")) for item in evidence.get("relocations", []) if isinstance(item, dict)}
    for pair in (("01D7:4EBA", "01E7:0D18"), ("01D7:5017", "01F7:0908"), ("01D7:5042", "01F7:321F")):
        if pair not in relocations:
            raise ExternalClosureError(f"progression relocation missing: {pair[0]} -> {pair[1]}")
    note = root / evidence["note"]["path"]
    if not note.is_file() or sha256(note) != evidence["note"]["sha256"]:
        raise ExternalClosureError("progression static note hash drift")
    note_text = note.read_text(encoding="utf-8")
    for anchor in ("completion_accounting_14E1", "completion_gate_4ea0", "01D7:16C6", "DS:612E = 0x000c", "DS:85DB=1", "01D7:1733"):
        if anchor not in note_text:
            raise ExternalClosureError(f"progression static note missing anchor: {anchor}")


def check_transition_effect_static(ledger: dict[str, Any], root: Path) -> None:
    """Verify the bounded 199D -> 0CE3 relocated effect closure."""
    path = root / "research/evidence/player-dos-parity/player-transition-effect-static-v1.json"
    if not path.is_file():
        raise ExternalClosureError(f"missing transition-effect static evidence: {path}")
    try:
        evidence = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read transition-effect static evidence: {exc}") from exc
    if evidence.get("schema") != "quiky.player-transition-effect-static.v1":
        raise ExternalClosureError("unexpected transition-effect static evidence schema")
    source = evidence.get("source", {})
    if source.get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("transition-effect executable hash drift")
    if source.get("ghidra_language") != "x86:LE:16:Protected Mode:default":
        raise ExternalClosureError("transition-effect Ghidra language drift")
    if source.get("independent_projects") != "project-a and project-b decompilation/listing outputs matched":
        raise ExternalClosureError("transition-effect independent-project check missing")
    relocations = {
        (item.get("instruction"), item.get("target"))
        for item in evidence.get("relocations", [])
        if isinstance(item, dict)
    }
    expected_relocations = {
        ("01F7:19A9", "01E7:0CE3"),
        ("01E7:0CE8", "0227:05CD"),
        ("01E7:0D03", "01E7:33D5"),
    }
    if relocations != expected_relocations:
        raise ExternalClosureError("transition-effect relocation set drifted")
    contracts = {
        item.get("address"): item
        for item in evidence.get("contracts", [])
        if isinstance(item, dict)
    }
    if set(contracts) != {"01E7:0CE3", "01E7:33D5", "0227:05CD"}:
        raise ExternalClosureError("transition-effect contract set drifted")
    if contracts["01E7:0CE3"].get("classification") != "contract":
        raise ExternalClosureError("0CE3 must remain an external contract")
    if contracts["01E7:0CE3"].get("player_writes") != []:
        raise ExternalClosureError("0CE3 gained an unverified player write")
    if contracts["01E7:33D5"].get("classification") != "irrelevant":
        raise ExternalClosureError("33D5 must remain a presentation/runtime leaf")
    if contracts["0227:05CD"].get("classification") != "irrelevant":
        raise ExternalClosureError("05CD must remain a runtime stack-guard leaf")
    if contracts["0227:05CD"].get("player_writes") != []:
        raise ExternalClosureError("05CD gained an unverified player write")
    note = root / "research/notes/player-transition-effect-static-decomp.cpp"
    if not note.is_file() or sha256(note) != evidence.get("note_sha256"):
        raise ExternalClosureError("transition-effect static note hash drift")
    note_text = note.read_text(encoding="utf-8")
    for anchor in (
        "address_named_transition_effect_01E7_0CE3",
        "address_named_effect_parameter_writer_01E7_33D5",
        "address_named_stack_probe_0227_05CD",
        "CALLF 0227:05CD",
        "FFFF:2FE9 = BX",
        "DS:504C = 0x18",
    ):
        if anchor not in note_text:
            raise ExternalClosureError(f"transition-effect note missing anchor: {anchor}")


def check_transition_writer_callback_evidence(ledger: dict[str, Any], root: Path) -> None:
    """Verify the natural damage-writer to player-transition callback trace."""
    path = root / "research/evidence/player-dos-parity/player-transition-writer-callback-v1.json"
    if not path.is_file():
        raise ExternalClosureError(f"missing transition writer/callback evidence: {path}")
    try:
        evidence = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read transition writer/callback evidence: {exc}") from exc
    if evidence.get("schema") != "quiky.player-transition-writer-callback.v1":
        raise ExternalClosureError("unexpected transition writer/callback evidence schema")
    source = evidence.get("source", {})
    if source.get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("transition writer/callback executable hash drift")
    trace = root / source.get("trace", "")
    if not trace.is_file():
        raise ExternalClosureError(f"missing transition writer/callback trace: {trace}")
    if sha256(trace) != source.get("trace_sha256"):
        raise ExternalClosureError("transition writer/callback trace hash drift")
    method = evidence.get("method", {})
    if method.get("callback") != "01F7:3FF8":
        raise ExternalClosureError("transition writer/callback focus drifted")
    if method.get("coordinate_patch") is not None:
        raise ExternalClosureError("transition writer/callback trace must be unpatched")
    if method.get("watched_addresses") != ["01F7:19E6", "01F7:4416", "01F7:44DC"]:
        raise ExternalClosureError("transition writer/callback watch set drifted")
    observation = evidence.get("observation", {})
    if observation.get("order") != ["01F7:19E6", "01F7:4416", "01F7:44DC"]:
        raise ExternalClosureError("transition writer/callback order is not closed")
    if observation.get("damage_writer", {}).get("transition_gate_before") != 0:
        raise ExternalClosureError("damage writer must begin with a zero transition gate")
    if observation.get("transition_entry", {}).get("health_after") != 0:
        raise ExternalClosureError("transition entry health result drifted")
    if observation.get("transition_entry", {}).get("lives_after") != 3:
        raise ExternalClosureError("transition entry lives result drifted")
    if observation.get("transition_tail", {}).get("transition_gate_observed_nonzero_at_entry") is not True:
        raise ExternalClosureError("transition tail lost the nonzero-gate observation")
    conclusion = evidence.get("conclusion", {})
    if conclusion.get("status") != "normal_gameplay_transition_branch_confirmed":
        raise ExternalClosureError("transition writer/callback conclusion drifted")


def check_natural_flagged_contact_evidence(ledger: dict[str, Any], root: Path) -> None:
    """Verify the unpatched W1L1 tile-40 traversal and landing trace."""
    evidence_path = root / "research/evidence/player-dos-parity/player-natural-flagged-contact-v1.json"
    if not evidence_path.is_file():
        raise ExternalClosureError(f"missing natural flagged-contact evidence: {evidence_path}")
    try:
        evidence = json.loads(evidence_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read natural flagged-contact evidence: {exc}") from exc
    if evidence.get("schema") != "quiky.player-natural-flagged-contact.v1":
        raise ExternalClosureError("unexpected natural flagged-contact evidence schema")
    source = evidence.get("source", {})
    if source.get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("natural flagged-contact executable hash drift")
    trace = root / source.get("trace", "")
    if not trace.is_file() or sha256(trace) != source.get("trace_sha256"):
        raise ExternalClosureError("natural flagged-contact trace missing or hash drifted")
    trace_data = json.loads(trace.read_text(encoding="utf-8"))
    if trace_data.get("schema") != "quiky-resource-trace-v1":
        raise ExternalClosureError("natural flagged-contact trace schema drifted")
    if trace_data.get("inputs", {}).get("executable_sha256") != source.get("executable_sha256"):
        raise ExternalClosureError("natural flagged-contact trace executable hash drift")
    if trace_data.get("inputs", {}).get("select_level") != "W1L1":
        raise ExternalClosureError("natural flagged-contact trace level drifted")
    if trace_data.get("script_sha256") != source.get("trace_script_sha256"):
        raise ExternalClosureError("natural flagged-contact trace script hash drift")
    map_path = root / source.get("map", "")
    if not map_path.is_file() or sha256(map_path) != source.get("map_sha256"):
        raise ExternalClosureError("natural flagged-contact MAP missing or hash drifted")
    control_trace = root / source.get("control_trace", "")
    if not control_trace.is_file() or sha256(control_trace) != source.get("control_trace_sha256"):
        raise ExternalClosureError("natural flagged-contact control trace missing or hash drifted")
    control_data = json.loads(control_trace.read_text(encoding="utf-8"))
    if control_data.get("schema") != "quiky-resource-trace-v1":
        raise ExternalClosureError("natural flagged-contact control trace schema drifted")
    if control_data.get("inputs", {}).get("executable_sha256") != source.get("executable_sha256"):
        raise ExternalClosureError("natural flagged-contact control executable hash drift")
    if control_data.get("inputs", {}).get("select_level") != "W1L1":
        raise ExternalClosureError("natural flagged-contact control trace level drifted")
    if control_data.get("script_sha256") != source.get("trace_script_sha256"):
        raise ExternalClosureError("natural flagged-contact control trace script hash drift")
    method = evidence.get("method", {})
    if method.get("coordinate_patch") is not None:
        raise ExternalClosureError("natural flagged-contact trace must be unpatched")
    if method.get("execute_watches") != [
        "01F7:41C1", "01F7:41CF", "01F7:3D02", "01F7:5CC3"
    ]:
        raise ExternalClosureError("natural flagged-contact watch set drifted")

    samples = trace_data.get("events", [{}])[0].get("samples", [])
    control_samples = control_data.get("events", [{}])[0].get("samples", [])

    def sample_at(frame: int) -> dict[str, Any]:
        matches = [item for item in samples if item.get("frame_index") == frame]
        if len(matches) != 1:
            raise ExternalClosureError(
                f"natural flagged-contact expected one sample at frame {frame}, found {len(matches)}"
            )
        return matches[0]

    def watched(sample: dict[str, Any], offset: int) -> dict[str, Any]:
        matches = [
            item for item in sample.get("execute_watches", [])
            if item.get("breakpoint", {}).get("segment") == 0x1F7
            and item.get("breakpoint", {}).get("offset") == offset
        ]
        if len(matches) != 1:
            raise ExternalClosureError(
                f"natural flagged-contact expected one watch {offset:04X}, found {len(matches)}"
            )
        return matches[0]

    control_matches = [item for item in control_samples if item.get("frame_index") == 659]
    if len(control_matches) != 1:
        raise ExternalClosureError(
            f"natural flagged-contact expected one control sample at frame 659, found {len(control_matches)}"
        )
    control = control_matches[0]
    control_order = [
        item.get("breakpoint", {}).get("offset")
        for item in control.get("execute_watches", [])
        if item.get("breakpoint", {}).get("segment") == 0x1F7
    ]
    if control_order != [0x41C1, 0x41CF]:
        raise ExternalClosureError("natural flagged-contact 41C1/41CF order drifted")
    for offset in (0x41C1, 0x41CF):
        registers = watched(control, offset).get("registers", {})
        if {key: registers.get(key) for key in ("eax", "ebx", "ecx", "edx", "esi", "es", "flags")} != {
            "eax": 0, "ebx": 9310, "ecx": 9310, "edx": 0,
            "esi": 751, "es": 639, "flags": 12870,
        }:
            raise ExternalClosureError(f"natural flagged-contact {offset:04X} registers drifted")

    landing = sample_at(695)
    player = landing.get("player_callback", {}).get("object", {})
    if player.get("state_size") != 120:
        raise ExternalClosureError("natural flagged-contact player record size drifted")
    if player.get("position", {}).get("x") != 1049 or player.get("position", {}).get("y") != 395:
        raise ExternalClosureError("natural flagged-contact landing position drifted")
    if player.get("player_byte_0x37") != 0 or player.get("velocity_y_fixed_signed") != 0:
        raise ExternalClosureError("natural flagged-contact grounded result drifted")
    landing_order = [
        item.get("breakpoint", {}).get("offset")
        for item in landing.get("execute_watches", [])
        if item.get("breakpoint", {}).get("segment") == 0x1F7
    ]
    if landing_order != [0x3D02, 0x5CC3]:
        raise ExternalClosureError("natural flagged-contact landing probe order drifted")
    probe = watched(landing, 0x5CC3)
    if probe.get("registers", {}).get("edx") != 16:
        raise ExternalClosureError("natural flagged-contact descriptor word drifted")
    observation = evidence.get("observations", {}).get("landing", {})
    if observation.get("descriptor_word") != "0x0010":
        raise ExternalClosureError("natural flagged-contact evidence descriptor drifted")
    if observation.get("map_cell") != {"x": 65, "y": 24, "raw_word": "0x2028", "tile_id": 40}:
        raise ExternalClosureError("natural flagged-contact MAP cell drifted")
    if evidence.get("status") != "natural_tile40_landing_and_descriptor_word_confirmed":
        raise ExternalClosureError("natural flagged-contact conclusion drifted")


def check_natural_tile41_contact_evidence(ledger: dict[str, Any], root: Path) -> None:
    """Verify the unpatched W1L1 tile-41 alignment-bit landing trace."""
    evidence_path = root / "research/evidence/player-dos-parity/player-natural-tile41-contact-v1.json"
    if not evidence_path.is_file():
        raise ExternalClosureError(f"missing natural tile-41 evidence: {evidence_path}")
    try:
        evidence = json.loads(evidence_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read natural tile-41 evidence: {exc}") from exc
    if evidence.get("schema") != "quiky.player-natural-tile41-contact.v1":
        raise ExternalClosureError("unexpected natural tile-41 evidence schema")
    source = evidence.get("source", {})
    if source.get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("natural tile-41 executable hash drift")
    trace = root / source.get("trace", "")
    if not trace.is_file() or sha256(trace) != source.get("trace_sha256"):
        raise ExternalClosureError("natural tile-41 trace missing or hash drifted")
    trace_data = json.loads(trace.read_text(encoding="utf-8"))
    if trace_data.get("schema") != "quiky-resource-trace-v1":
        raise ExternalClosureError("natural tile-41 trace schema drifted")
    if trace_data.get("inputs", {}).get("executable_sha256") != source.get("executable_sha256"):
        raise ExternalClosureError("natural tile-41 trace executable hash drift")
    if trace_data.get("inputs", {}).get("select_level") != "W1L1":
        raise ExternalClosureError("natural tile-41 trace level drifted")
    if trace_data.get("script_sha256") != source.get("trace_script_sha256"):
        raise ExternalClosureError("natural tile-41 trace script hash drift")
    map_path = root / source.get("map", "")
    if not map_path.is_file() or sha256(map_path) != source.get("map_sha256"):
        raise ExternalClosureError("natural tile-41 MAP missing or hash drifted")
    method = evidence.get("method", {})
    if method.get("coordinate_patch") is not None:
        raise ExternalClosureError("natural tile-41 trace must be unpatched")
    if method.get("execute_watches") != [
        "01F7:41C1", "01F7:41CF", "01F7:3D02", "01F7:5CC3"
    ]:
        raise ExternalClosureError("natural tile-41 watch set drifted")
    samples = json.loads(trace.read_text(encoding="utf-8")).get("events", [{}])[0].get("samples", [])

    def sample_at(frame: int) -> dict[str, Any]:
        matches = [item for item in samples if item.get("frame_index") == frame]
        if len(matches) != 1:
            raise ExternalClosureError(
                f"natural tile-41 expected one sample at frame {frame}, found {len(matches)}"
            )
        return matches[0]

    def watched(sample: dict[str, Any], offset: int) -> dict[str, Any]:
        matches = [
            item for item in sample.get("execute_watches", [])
            if item.get("breakpoint", {}).get("segment") == 0x1F7
            and item.get("breakpoint", {}).get("offset") == offset
        ]
        if len(matches) != 1:
            raise ExternalClosureError(
                f"natural tile-41 expected one watch {offset:04X}, found {len(matches)}"
            )
        return matches[0]

    ascent = sample_at(682)
    ascent_object = ascent.get("player_callback", {}).get("object", {})
    if ascent_object.get("position", {}).get("x") != 1086 or ascent_object.get("position", {}).get("y") != 319:
        raise ExternalClosureError("natural tile-41 ascent position drifted")
    if ascent_object.get("player_byte_0x37") != 0xFF or ascent_object.get("velocity_y_fixed_signed") != -73728:
        raise ExternalClosureError("natural tile-41 ascent state drifted")
    landing = sample_at(722)
    player = landing.get("player_callback", {}).get("object", {})
    if player.get("state_size") != 120:
        raise ExternalClosureError("natural tile-41 player record size drifted")
    if player.get("position", {}).get("x") != 1095 or player.get("position", {}).get("y") != 372:
        raise ExternalClosureError("natural tile-41 landing position drifted")
    if player.get("player_byte_0x37") != 0 or player.get("velocity_y_fixed_signed") != 0:
        raise ExternalClosureError("natural tile-41 grounded result drifted")
    landing_order = [
        item.get("breakpoint", {}).get("offset")
        for item in landing.get("execute_watches", [])
        if item.get("breakpoint", {}).get("segment") == 0x1F7
    ]
    if landing_order != [0x3D02, 0x5CC3]:
        raise ExternalClosureError("natural tile-41 landing probe order drifted")
    if watched(landing, 0x5CC3).get("registers", {}).get("edx") != 80:
        raise ExternalClosureError("natural tile-41 descriptor word drifted")
    observation = evidence.get("observation", {}).get("landing", {})
    if observation.get("descriptor_word") != "0x0050":
        raise ExternalClosureError("natural tile-41 evidence descriptor drifted")
    if observation.get("map_cell") != {"x": 68, "y": 23, "raw_word": "0x2029", "tile_id": 41}:
        raise ExternalClosureError("natural tile-41 MAP cell drifted")
    if evidence.get("status") != "natural_tile41_landing_and_alignment_case_confirmed":
        raise ExternalClosureError("natural tile-41 conclusion drifted")


def check_negative_mode_second_probe_evidence(ledger: dict[str, Any], root: Path) -> None:
    """Verify the protected-mode negative-path/apex attribution trace."""
    evidence_path = root / "research/evidence/player-dos-parity/player-negative-mode-second-probe-v1.json"
    if not evidence_path.is_file():
        raise ExternalClosureError(f"missing negative-mode second-probe evidence: {evidence_path}")
    try:
        evidence = json.loads(evidence_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read negative-mode second-probe evidence: {exc}") from exc
    if evidence.get("schema") != "quiky.player-negative-mode-second-probe.v1":
        raise ExternalClosureError("unexpected negative-mode second-probe evidence schema")

    source = evidence.get("source", {})
    if source.get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("negative-mode second-probe executable hash drift")
    trace = root / source.get("trace", "")
    if not trace.is_file() or sha256(trace) != source.get("trace_sha256"):
        raise ExternalClosureError("negative-mode second-probe trace missing or hash drifted")
    trace_data = json.loads(trace.read_text(encoding="utf-8"))
    if trace_data.get("schema") != "quiky-resource-trace-v1":
        raise ExternalClosureError("negative-mode second-probe trace schema drifted")
    if trace_data.get("inputs", {}).get("executable_sha256") != source.get("executable_sha256"):
        raise ExternalClosureError("negative-mode second-probe trace executable hash drift")
    if trace_data.get("inputs", {}).get("select_level") != "W1L1":
        raise ExternalClosureError("negative-mode second-probe trace level drifted")
    if trace_data.get("script_sha256") != source.get("trace_script_sha256"):
        raise ExternalClosureError("negative-mode second-probe trace script hash drift")

    method = evidence.get("method", {})
    if method.get("level") != "W1L1" or method.get("callback") != "01F7:3FF8":
        raise ExternalClosureError("negative-mode second-probe callback scope drifted")
    if method.get("coordinate_patch") is not None:
        raise ExternalClosureError("negative-mode second-probe trace must be unpatched")
    expected_watches = [
        "01F7:4323", "01F7:4363", "01F7:4366", "01F7:3986",
        "01F7:1C92", "01F7:3997", "01F7:41C1", "01F7:41CF",
    ]
    if method.get("execute_watches") != expected_watches:
        raise ExternalClosureError("negative-mode second-probe watch set drifted")

    contract = evidence.get("static_contract", {})
    expected_contract = {
        "entry": "01F7:4323 CALL 01F7:3986; JNZ 01F7:41C1",
        "integrate": "01F7:432A computes +0x0e + +0x58, clamps only the signed negative lower bound, and 01F7:4350 branches to 41C1 when the result is nonnegative",
        "second_probe": "01F7:4359 writes +0x0e and adds it to +0x06; 01F7:4363 calls 3986; 01F7:4366 JZ goes to 4384 and the opposite path goes to 41C1",
        "response": "01F7:41C1/41CF writes +0x3e=0x03e7, +0x37=1, +0x0e=0, and selects animation descriptor 0x3186 before 4384",
    }
    if contract != expected_contract:
        raise ExternalClosureError("negative-mode second-probe static contract drifted")

    events = trace_data.get("events", [])
    if len(events) != 1 or not isinstance(events[0], dict):
        raise ExternalClosureError("negative-mode second-probe trace event shape drifted")
    samples = events[0].get("samples", [])

    def sample_at(frame: int) -> dict[str, Any]:
        matches = [item for item in samples if item.get("frame_index") == frame]
        if len(matches) != 1:
            raise ExternalClosureError(
                f"negative-mode second-probe expected one sample at frame {frame}, found {len(matches)}"
            )
        return matches[0]

    def watch_order(sample: dict[str, Any]) -> list[int]:
        return [
            item.get("breakpoint", {}).get("offset")
            for item in sample.get("execute_watches", [])
            if item.get("breakpoint", {}).get("segment") == 0x1F7
        ]

    apex = sample_at(256)
    if watch_order(apex) != [0x4323, 0x3986, 0x1C92, 0x3997, 0x41C1, 0x41CF]:
        raise ExternalClosureError("negative-mode apex probe order drifted")
    apex_pre = apex.get("player_callback", {}).get("object", {})
    if apex_pre.get("position", {}).get("x") != 457 or apex_pre.get("position", {}).get("y") != 316:
        raise ExternalClosureError("negative-mode apex pre-position drifted")
    if apex_pre.get("player_byte_0x37") != 0xFF or apex_pre.get("velocity_y_fixed_signed") != -8192:
        raise ExternalClosureError("negative-mode apex pre-state drifted")
    apex_post = apex.get("player_callback", {}).get("post_object", {})
    if (
        apex_post.get("player_byte_0x37") != 1
        or apex_post.get("velocity_y_fixed_signed") != 0
        or apex_post.get("player_word_0x3e") != 999
    ):
        raise ExternalClosureError("negative-mode shared 41C1 response drifted")
    if any(offset in watch_order(apex) for offset in (0x4363, 0x4366)):
        raise ExternalClosureError("negative-mode apex unexpectedly executed the second probe")

    ascent = sample_at(248)
    if watch_order(ascent) != [0x4323, 0x3986, 0x1C92, 0x3997, 0x4363, 0x4366]:
        raise ExternalClosureError("negative-mode clear second-probe order drifted")
    ascent_pre = ascent.get("player_callback", {}).get("object", {})
    if ascent_pre.get("position", {}).get("x") != 448 or ascent_pre.get("position", {}).get("y") != 321:
        raise ExternalClosureError("negative-mode ascent pre-position drifted")
    if ascent_pre.get("player_byte_0x37") != 0xFF or ascent_pre.get("velocity_y_fixed_signed") != -73728:
        raise ExternalClosureError("negative-mode ascent pre-state drifted")
    second_probe = [
        item for item in ascent.get("execute_watches", [])
        if item.get("breakpoint", {}).get("segment") == 0x1F7
        and item.get("breakpoint", {}).get("offset") == 0x4366
    ]
    if len(second_probe) != 1 or second_probe[0].get("registers", {}).get("flags") != 12870:
        raise ExternalClosureError("negative-mode second-probe return flags drifted")

    notes = ledger.get("targeted_static_audit", {}).get("static_notes", {})
    note_rel = "research/notes/player-negative-mode-second-probe-static.cpp"
    note_path = root / note_rel
    if not note_path.is_file() or sha256(note_path) != notes.get(note_rel):
        raise ExternalClosureError("negative-mode second-probe static note hash drift")
    note = note_path.read_text(encoding="utf-8")
    for anchor in (
        "01F7:4323", "01F7:4356", "01F7:4363", "01F7:4366",
        "ordinary ascent-to-fall boundary", "not natural ceiling evidence",
    ):
        if anchor not in note:
            raise ExternalClosureError(f"negative-mode second-probe note missing anchor: {anchor}")
    if evidence.get("conclusion", {}).get("status") != "negative_mode_second_probe_and_apex_join_static_closed":
        raise ExternalClosureError("negative-mode second-probe conclusion drifted")


def check_camera_map_refresh_static(ledger: dict[str, Any], root: Path) -> None:
    evidence_path = root / "research/evidence/player-dos-parity/camera-map-refresh-static-v1.json"
    if not evidence_path.is_file():
        raise ExternalClosureError(f"missing camera/map refresh evidence: {evidence_path}")
    try:
        evidence = json.loads(evidence_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read camera/map refresh evidence: {exc}") from exc
    if evidence.get("schema") != "quiky.camera-map-refresh-static.v1":
        raise ExternalClosureError("unexpected camera/map refresh evidence schema")
    source = evidence.get("source", {})
    if source.get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("camera/map refresh executable hash drift")
    if source.get("ghidra_language") != "x86:LE:16:Protected Mode":
        raise ExternalClosureError("camera/map refresh must use protected-mode Ghidra")
    if source.get("independent_projects") != "project-a and project-b decompilation/listing outputs matched":
        raise ExternalClosureError("camera/map refresh independent export check is not passed")
    contracts = {item.get("address"): item for item in evidence.get("contracts", [])}
    expected = {"01F7:17AE", "01F7:1ED7", "01F7:3062"}
    if set(contracts) != expected:
        raise ExternalClosureError("camera/map refresh contract set drifted")
    if contracts["01F7:17AE"].get("classification") != "contract":
        raise ExternalClosureError("17AE must remain a simulation-relevant contract")
    if contracts["01F7:1ED7"].get("classification") != "contract":
        raise ExternalClosureError("1ED7 must remain a camera/stream contract")
    if contracts["01F7:3062"].get("classification") != "irrelevant":
        raise ExternalClosureError("3062 must remain presentation-only")
    if contracts["01F7:17AE"].get("global_writes") != [
        "DS:8960[0..0x7f]", "DS:895E=0", "DS:6586 + 8*n first word for n=0..0x7f",
    ]:
        raise ExternalClosureError("17AE scheduler/event reset writes drifted")
    writes = set(contracts["01F7:1ED7"].get("global_writes", []))
    for token in ("DS:81B2", "DS:81AE", "DS:81BE", "DS:81C2", "DS:81CE", "DS:81D0", "DS:81A6", "DS:81AA"):
        if token not in " ".join(writes):
            raise ExternalClosureError(f"1ED7 lost camera write contract: {token}")
    if contracts["01F7:3062"].get("player_writes") != []:
        raise ExternalClosureError("3062 gained a player write")
    if contracts["01F7:3062"].get("feeds_back_into_simulation") != "no direct player, MAP, descriptor, scheduler, or callback-global simulation feedback":
        raise ExternalClosureError("3062 simulation classification drifted")
    note_path = root / "research/notes/camera-map-refresh-static-decomp.cpp"
    if not note_path.is_file() or sha256(note_path) != "76c46536bb8dfd3cbaba2031c6446b4f55cccaa698fa4fe07fd70c9aabb479bb":
        raise ExternalClosureError("camera/map refresh static note hash drift")
    note = note_path.read_text(encoding="utf-8")
    for anchor in (
        "reset_stream_and_event_banks_17AE",
        "update_camera_scroll_1ED7",
        "copy_prepared_vga_map_3062",
        "DS:8960",
        "DS:81CE",
        "presentation-only contract",
    ):
        if anchor not in note:
            raise ExternalClosureError(f"camera/map refresh note missing anchor: {anchor}")


def check_scheduler_order(ledger: dict[str, Any]) -> None:
    order = ledger.get("static_order", {}).get("main_loop_pairs")
    expected = [
        ("01D7:44FA->01F7:0E96", "01D7:4518->01F7:0FA2"),
        ("01D7:47FC->01F7:0E96", "01D7:481A->01F7:0FA2"),
        ("01D7:4872->01F7:0E96", "01D7:4890->01F7:0FA2"),
    ]
    if not isinstance(order, list) or len(order) != len(expected):
        raise ExternalClosureError("static_order.main_loop_pairs must contain three pairs")
    for item, pair in zip(order, expected):
        if not isinstance(item, dict):
            raise ExternalClosureError("scheduler order entry must be an object")
        if (item.get("scheduler"), item.get("nonzero_state")) != pair:
            raise ExternalClosureError("scheduler relocation pair drifted")
        if item.get("order") != "0E96 before 0FA2":
            raise ExternalClosureError("scheduler order must remain 0E96 before 0FA2")


def check_targeted_static_audit(ledger: dict[str, Any]) -> None:
    audit = ledger.get("targeted_static_audit")
    if not isinstance(audit, dict):
        raise ExternalClosureError("targeted_static_audit must be an object")
    if audit.get("status") != "static_body_closed_with_runtime_boundaries_explicit":
        raise ExternalClosureError("targeted static audit status drifted")
    static_notes = audit.get("static_notes")
    if not isinstance(static_notes, dict) or not static_notes:
        raise ExternalClosureError("targeted static audit must hash its static notes")
    for relative, expected_hash in static_notes.items():
        note_path = ROOT / relative
        if not note_path.is_file():
            raise ExternalClosureError(f"missing targeted static note: {note_path}")
        if sha256(note_path) != expected_hash:
            raise ExternalClosureError(f"targeted static note hash drift: {note_path}")
    functions = audit.get("functions")
    if not isinstance(functions, list) or not functions:
        raise ExternalClosureError("targeted_static_audit.functions must be non-empty")
    required = {
        "01F7:0FA2", "01F7:0FDC", "01F7:1DEE", "01F7:9DC7/A075/A0B2",
        "01F7:1997/3529/34E3/3808",
        "01F7:9C70/9CF5/9D19/9D5E/9D82 -> 9DC7",
        "01F7:38CA/38EC -> 0E06 -> 4519",
        "01F7:6370/6484/648E -> 6328 -> 16CE", "01F7:5D38/5D60",
        "01F7:5937 -> 386F -> 0442/04DF/0517", "01F7:41C1/41CF -> 3D02/3DF2",
        "01D7:39ED -> 01F7:05A0",
        "01F7:199D/19E6 -> 01F7:1AAA/1AF5 -> 01D7:4BA4",
        "01F7:1BD1", "01F7:1C4D", "01F7:6D5F/6DA3/6DB1/6DC4", "01F7:6DC4", "01F7:68C0",
        "01F7:4727/47E7",
        "01F7:9BEE/9C0C -> 1B07/1B5D",
        "01F7:8BC2/8BE5/8C08/8C2B/8C4E/8C71/8C8A/8CA3/8CBC/8CD5/8CEE/8D07",
        "01F7:8D20/8D31", "01F7:9256/9269",
    }
    seen: set[str] = set()
    for item in functions:
        if not isinstance(item, dict):
            raise ExternalClosureError("targeted static audit function must be an object")
        address = item.get("address")
        if not isinstance(address, str) or address in seen:
            raise ExternalClosureError("targeted static audit address is missing or duplicated")
        seen.add(address)
        for field in ("name", "calling_convention", "static_fact", "player_global_effect", "confidence", "evidence"):
            if field not in item:
                raise ExternalClosureError(f"targeted static audit {address} lacks {field}")
        if not isinstance(item["evidence"], list) or not item["evidence"]:
            raise ExternalClosureError(f"targeted static audit {address} has no evidence")
    missing = sorted(required - seen)
    if missing:
        raise ExternalClosureError("targeted static audit missing: " + ", ".join(missing))
    if not isinstance(audit.get("static_stop"), str) or not audit["static_stop"]:
        raise ExternalClosureError("targeted static audit must state its stopping rule")


def check_runtime_scheduler_membership(ledger: dict[str, Any], root: Path) -> None:
    runtime = ledger.get("runtime_scheduler_membership")
    if not isinstance(runtime, dict):
        raise ExternalClosureError("runtime_scheduler_membership must be an object")
    evidence_path = root / runtime.get("trace", "")
    if not evidence_path.is_file():
        raise ExternalClosureError(f"missing scheduler membership evidence: {evidence_path}")
    try:
        evidence = json.loads(evidence_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read scheduler membership evidence: {exc}") from exc
    if evidence.get("schema") != "quiky.player-scheduler-membership.v2":
        raise ExternalClosureError("unexpected scheduler membership evidence schema")
    if sha256(evidence_path) != runtime.get("evidence_sha256", sha256(evidence_path)):
        raise ExternalClosureError("scheduler membership evidence hash drift")
    source = evidence.get("source", {})
    if source.get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("scheduler membership executable hash drift")
    expected_layout = [
        ("0x00", "phase_callback_offset"),
        ("0x02", "secondary_callback_offset"),
        ("0x04", "object_offset"),
        ("0x06", "entry_word_06"),
    ]
    export = evidence.get("static_export", {})
    if export.get("runner") != "research/tools/run_player_external_closure.py":
        raise ExternalClosureError("scheduler membership static runner drifted")
    if export.get("ghidra_language") != "x86:LE:16:Protected Mode":
        raise ExternalClosureError("scheduler membership static export is not Protected Mode Ghidra")
    export_root = root / export.get("output", "")
    decomp = export_root / "decomp-a/QUIKY_SEG03.bin.c"
    listing = export_root / "disasm-seg3-a/QUIKY_SEG03.bin.asm"
    if not decomp.is_file() or not listing.is_file():
        raise ExternalClosureError("scheduler membership static export is incomplete")
    if sha256(decomp) != export.get("decomp_sha256"):
        raise ExternalClosureError("scheduler membership static decomp hash drift")
    if sha256(listing) != export.get("listing_sha256"):
        raise ExternalClosureError("scheduler membership static listing hash drift")
    if export.get("independent_project_check") != "passed; project-a and project-b outputs matched":
        raise ExternalClosureError("scheduler membership independent Ghidra check is not passed")
    static_artifact_path = root / "research/evidence/player-dos-parity/scheduler-dispatch-static-v1.json"
    if not static_artifact_path.is_file():
        raise ExternalClosureError(f"missing scheduler static artifact: {static_artifact_path}")
    try:
        static_artifact = json.loads(static_artifact_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read scheduler static artifact: {exc}") from exc
    if static_artifact.get("schema") != "quiky.scheduler-dispatch-static.v1":
        raise ExternalClosureError("unexpected scheduler static artifact schema")
    if static_artifact.get("source", {}).get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("scheduler static artifact executable hash drift")
    if static_artifact.get("source", {}).get("output") != export.get("output"):
        raise ExternalClosureError("scheduler static artifact output drift")
    if static_artifact.get("source", {}).get("listing_sha256") != export.get("listing_sha256"):
        raise ExternalClosureError("scheduler static artifact listing hash drift")
    static_layout = static_artifact.get("entry_layout")
    if not isinstance(static_layout, list) or [
        (item.get("offset"), item.get("name"))
        for item in static_layout if isinstance(item, dict)
    ] != expected_layout:
        raise ExternalClosureError("scheduler static artifact entry layout drifted")
    entry_layout = evidence.get("entry_layout")
    if not isinstance(entry_layout, list) or [
        (item.get("offset"), item.get("name"))
        for item in entry_layout if isinstance(item, dict)
    ] != expected_layout:
        raise ExternalClosureError("scheduler membership entry layout drifted")
    listing_text = listing.read_text(encoding="utf-8")
    for token in (
        "0000:0eba  MOV AX,word ptr [SI]",
        "0000:0fba  MOV AX,word ptr [SI + 0x2]",
        "0000:0fc7  MOV DI,word ptr [SI + 0x4]",
        "0000:0fcd  CALL AX",
        "0000:1036  AND word ptr ES:[DI + 0x18],0xffff",
        "0000:104e  MOV word ptr [SI + 0x2],BX",
        "0000:1057  MOV word ptr [SI + 0x4],DI",
    ):
        if token not in listing_text:
            raise ExternalClosureError(f"scheduler static listing lost: {token}")
    observations = evidence.get("observations")
    if not isinstance(observations, list) or len(observations) < 2:
        raise ExternalClosureError("scheduler membership evidence needs two observations")
    first = observations[0]
    entry = first.get("player_entry", {})
    if (
        first.get("selected_bank_base") != "0x7766"
        or entry.get("phase_callback_offset") != "0x3ff8"
        or entry.get("secondary_callback_offset") != "0x1997"
        or entry.get("object_offset") != "0x0000"
    ):
        raise ExternalClosureError("scheduler membership first selected-bank observation drifted")
    second_entries = observations[1].get("player_entries", [])
    if not any(
        item.get("bank_base") == "0x7566"
        and item.get("phase_callback_offset") == "0x3ff8"
        and item.get("object_offset") == "0x0000"
        for item in second_entries
        if isinstance(item, dict)
    ):
        raise ExternalClosureError("scheduler membership rotated-bank observation is missing")


def check_death_recovery_static(ledger: dict[str, Any], root: Path | None = None) -> None:
    closure = ledger.get("death_recovery_static")
    if not isinstance(closure, dict):
        raise ExternalClosureError("death_recovery_static must be an object")
    if closure.get("status") != "static_writes_closed_observed_natural_owner_closed":
        raise ExternalClosureError("death/recovery static status drifted")
    if closure.get("ghidra_language") != "x86:LE:16:Protected Mode":
        raise ExternalClosureError("death/recovery export must use protected-mode Ghidra")
    if closure.get("independent_export_check") != (
        "passed; project-a and project-b decompilation/listing/data outputs matched"
    ):
        raise ExternalClosureError("death/recovery independent export check is not passed")
    if set(closure.get("segment3_targets", [])) != {
        "01F7:199D", "01F7:19E6", "01F7:1AAA", "01F7:1AE6", "01F7:1AF5",
        "01F7:1C4D", "01F7:6DC4",
    }:
        raise ExternalClosureError("death/recovery segment-3 target set drifted")
    if set(closure.get("segment1_targets", [])) != {
        "01D7:34C7", "01D7:3861",
        "01D7:4BA4", "01D7:4BAE", "01D7:4BD8", "01D7:4C43",
        "01D7:4CB8", "01D7:4EA0", "01D7:4EAA",
    }:
        raise ExternalClosureError("death/recovery segment-1 target set drifted")
    facts = closure.get("static_facts")
    if not isinstance(facts, list) or len(facts) < 5:
        raise ExternalClosureError("death/recovery static facts are incomplete")
    open_edges = closure.get("runtime_open_edges")
    if not isinstance(open_edges, list) or not open_edges:
        raise ExternalClosureError("death/recovery runtime boundaries are missing")
    relocation = closure.get("relocation")
    if relocation != {
        "source": "01F7:1B01",
        "target": "01F7:1AAA",
        "kind": "far_call",
        "evidence": "NE relocation table",
    }:
        raise ExternalClosureError("death/recovery 1B01 relocation contract drifted")
    if root is not None:
        static_note_path = root / closure.get("static_decomp_note", "")
        if not static_note_path.is_file():
            raise ExternalClosureError(
                f"missing death/recovery static decomp note: {static_note_path}"
            )
        if sha256(static_note_path) != closure.get("static_decomp_note_sha256"):
            raise ExternalClosureError("death/recovery static decomp note hash drift")
        static_note = static_note_path.read_text(encoding="utf-8")
        for anchor in (
            "player_instant_death_199D",
            "player_damage_or_death_19E6",
            "player_respawn_reinitialize_1AAA",
            "restore_health_and_checkpoint_1AF5",
            "main_loop_lifecycle_gate_4BA4",
            "4BA4-4BB5",
            "4C43-4C5D",
            "address_named_lifecycle_call_4C87",
            "01F7:1B01 -> 01F7:1AAA",
        ):
            if anchor not in static_note:
                raise ExternalClosureError(
                    f"death/recovery static decomp note missing anchor: {anchor}"
                )
        evidence_path = root / closure.get("runtime_evidence", "")
        if not evidence_path.is_file():
            raise ExternalClosureError(f"missing death/recovery runtime evidence: {evidence_path}")
        if sha256(evidence_path) != closure.get("runtime_evidence_sha256"):
            raise ExternalClosureError("death/recovery runtime evidence hash drift")
        try:
            evidence = json.loads(evidence_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            raise ExternalClosureError(f"cannot read death/recovery runtime evidence: {exc}") from exc
        if evidence.get("schema") != "quiky.player-death-recovery-lifecycle.v1":
            raise ExternalClosureError("unexpected death/recovery runtime evidence schema")
        if evidence.get("source", {}).get("executable_sha256") != ledger["source"]["executable_sha256"]:
            raise ExternalClosureError("death/recovery runtime executable hash drift")
        focused_evidence_path = root / closure.get("focused_runtime_evidence", "")
        if not focused_evidence_path.is_file():
            raise ExternalClosureError(
                f"missing focused death/recovery runtime evidence: {focused_evidence_path}"
            )
        if sha256(focused_evidence_path) != closure.get("focused_runtime_evidence_sha256"):
            raise ExternalClosureError("focused death/recovery runtime evidence hash drift")
        try:
            focused = json.loads(focused_evidence_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            raise ExternalClosureError(
                f"cannot read focused death/recovery runtime evidence: {exc}"
            ) from exc
        if focused.get("schema") != "quiky.player-death-recovery-focused.v1":
            raise ExternalClosureError("unexpected focused death/recovery evidence schema")
        if focused.get("source", {}).get("executable_sha256") != ledger["source"]["executable_sha256"]:
            raise ExternalClosureError("focused death/recovery executable hash drift")
        if focused.get("observations", {}).get("recovery_entry", {}).get("address") != "01F7:1AAA":
            raise ExternalClosureError("focused recovery entry must be 01F7:1AAA")
        if focused.get("observations", {}).get("recovery_entry", {}).get("observed_frame") != 1770:
            raise ExternalClosureError("focused recovery entry frame drifted")
        recovered = focused.get("observations", {}).get("first_recovered_callback", {})
        if recovered.get("address") != "01F7:3FF8" or recovered.get("observed_frame") != 1780:
            raise ExternalClosureError("focused first recovered callback boundary drifted")
        recovered_player = recovered.get("player_record", {})
        if recovered_player.get("position_pixels") != [1673, 368]:
            raise ExternalClosureError("focused recovered spawn position drifted")
        if recovered_player.get("mode_byte_0x37") != "0x00":
            raise ExternalClosureError("focused recovered mode drifted")
        if recovered_player.get("velocity_x_fixed") != "0x00000000" or recovered_player.get("velocity_y_fixed") != "0x00000000":
            raise ExternalClosureError("focused recovered velocity drifted")
        owner_path = root / closure.get("natural_owner_evidence", "")
        if not owner_path.is_file():
            raise ExternalClosureError(f"missing natural owner evidence: {owner_path}")
        if sha256(owner_path) != closure.get("natural_owner_evidence_sha256"):
            raise ExternalClosureError("natural owner evidence hash drift")
        owner = json.loads(owner_path.read_text(encoding="utf-8"))
        if owner.get("schema") != "quiky.player-natural-damage-owner.v1":
            raise ExternalClosureError("unexpected natural owner evidence schema")
        if owner.get("source", {}).get("executable_sha256") != ledger["source"]["executable_sha256"]:
            raise ExternalClosureError("natural owner executable hash drift")
        damage = owner.get("observations", {}).get("damage_writer", {})
        if damage.get("address") != "01F7:19E6" or damage.get("caller_callback") != "01F7:6DC4":
            raise ExternalClosureError("natural owner route must be 6DC4 -> 19E6")


def check_lifecycle_relocation_static(ledger: dict[str, Any], root: Path) -> None:
    closure = ledger.get("lifecycle_relocation_static")
    if not isinstance(closure, dict):
        raise ExternalClosureError("lifecycle_relocation_static must be an object")
    if closure.get("status") != "relocated_targets_and_camera_bridge_closed_timer_membership_open":
        raise ExternalClosureError("lifecycle relocation static status drifted")
    if closure.get("runner") != "research/tools/run_player_external_closure.py":
        raise ExternalClosureError("lifecycle relocation runner drifted")
    if closure.get("ghidra_language") != "x86:LE:16:Protected Mode":
        raise ExternalClosureError("lifecycle relocation export must use protected-mode Ghidra")
    expected_sets = {
        "segment2": {"01E7:082D", "01E7:0C71", "01E7:0CAA", "01E7:0D18"},
        "segment3": {"01F7:0908", "01F7:106A", "01F7:17AE", "01F7:17D4", "01F7:1AF5", "01F7:1ED7", "01F7:20AF", "01F7:3062", "01F7:31D1", "01F7:321F", "01F7:5BEF", "01F7:F07B", "01F7:F111"},
        "segment4": {"0207:0002", "0207:022A", "0207:08D8", "0207:17A0"},
    }
    target_sets = closure.get("target_sets")
    if not isinstance(target_sets, dict):
        raise ExternalClosureError("lifecycle relocation target_sets must be an object")
    for segment, expected in expected_sets.items():
        if set(target_sets.get(segment, [])) != expected:
            raise ExternalClosureError(f"lifecycle relocation {segment} target set drifted")
    direct = closure.get("direct_simulation_writes")
    if not isinstance(direct, list) or not all(isinstance(item, str) for item in direct):
        raise ExternalClosureError("lifecycle relocation direct writes are incomplete")
    for anchor in ("106A", "17D4", "17AE", "1AF5", "1ED7", "20AF", "31D1", "321F"):
        if not any(anchor in item for item in direct):
            raise ExternalClosureError(f"lifecycle relocation direct write missing {anchor}")
    open_edges = closure.get("runtime_open_edges")
    if not isinstance(open_edges, list) or not open_edges:
        raise ExternalClosureError("lifecycle relocation runtime boundaries are missing")
    note_path = root / closure.get("static_decomp_note", "")
    if not note_path.is_file():
        raise ExternalClosureError(f"missing lifecycle relocation static note: {note_path}")
    if sha256(note_path) != closure.get("static_decomp_note_sha256"):
        raise ExternalClosureError("lifecycle relocation static note hash drift")
    note = note_path.read_text(encoding="utf-8")
    for anchor in (
        "clear_dead_scheduler_callbacks_106A",
        "clear_are_event_queue_17D4",
        "initialize_are_event_slots_17AE",
        "restore_health_and_respawn_row_1AF5",
        "publish_camera_target_delta_20AF",
        "set_camera_origin_31D1",
        "update_camera_scroll_1ED7",
        "render_map_page_3062",
        "rebuild_map_pages_321F",
        "wait_for_timer_ticks_0207_0002",
        "bounded_transition_dispatch_0908",
        "Runtime boundaries still requiring traces",
    ):
        if anchor not in note:
            raise ExternalClosureError(f"lifecycle relocation note missing anchor: {anchor}")


def check_current_platform_player_evidence(ledger: dict[str, Any], root: Path) -> None:
    path = root / "research/evidence/player-dos-parity/platform-player-current-v1.json"
    if not path.is_file():
        raise ExternalClosureError(f"missing current platform/player evidence: {path}")
    try:
        evidence = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read current platform/player evidence: {exc}") from exc
    if evidence.get("schema") != "quiky.platform-player-current.v1":
        raise ExternalClosureError("unexpected current platform/player evidence schema")
    source = evidence.get("source", {})
    if source.get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("current platform/player executable hash drift")
    runtime = source.get("runtime_artifacts", {})
    if runtime.get("QUIKY.EXE") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("current platform/player runtime executable hash drift")
    trace_path = root / source.get("dos_trace", "")
    if not trace_path.is_file() or sha256(trace_path) != source.get("dos_trace_sha256"):
        raise ExternalClosureError("current platform/player DOS trace hash drift")
    observations = evidence.get("observations", {})
    if observations.get("scheduler_order") != ["platform_callback", "player_callback"]:
        raise ExternalClosureError("current platform/player scheduler order drifted")
    if observations.get("player_record_size") != "0x78":
        raise ExternalClosureError("current platform/player record size drifted")
    if observations.get("all_samples", {}).get("carry_consumed_before_player_return") is not True:
        raise ExternalClosureError("current platform/player carry consumption is not confirmed")


def check_current_platform_jump_detachment_evidence(ledger: dict[str, Any], root: Path) -> None:
    path = root / "research/evidence/player-dos-parity/platform-player-jump-detach-current-v1.json"
    if not path.is_file():
        raise ExternalClosureError(f"missing current platform jump-detachment evidence: {path}")
    try:
        evidence = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read current platform jump-detachment evidence: {exc}") from exc
    if evidence.get("schema") != "quiky.player-platform-jump-detach-current.v1":
        raise ExternalClosureError("unexpected current platform jump-detachment evidence schema")
    source = evidence.get("source", {})
    if source.get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("current platform jump-detachment executable hash drift")
    trace_path = root / source.get("trace", "")
    if not trace_path.is_file() or sha256(trace_path) != source.get("trace_sha256"):
        raise ExternalClosureError("current platform jump-detachment trace hash drift")
    if source.get("trace_script_sha256") != "403c2ec31102294955da6f36ae54900ae836deb303a87bb281d4a48e8c532ddf":
        raise ExternalClosureError("current platform jump-detachment trace-script hash drift")
    observations = evidence.get("observations", [])
    if not isinstance(observations, list) or len(observations) < 2:
        raise ExternalClosureError("current platform jump-detachment observations are incomplete")
    first = observations[0]
    if first.get("scheduler_order") != ["01F7:9DC7", "01F7:A075", "01F7:A0B2", "01F7:3FF8"]:
        raise ExternalClosureError("current platform jump-detachment scheduler order drifted")
    globals_before = first.get("globals_before_player", {})
    if globals_before.get("DS:5006") != "0xFFFF" or globals_before.get("DS:8816") != "0x00000001" or globals_before.get("DS:8812") != "0xFFF80001":
        raise ExternalClosureError("current platform jump-detachment carry publication drifted")
    globals_after = first.get("globals_after_player", {})
    if globals_after.get("DS:8816") != "0x00000000" or globals_after.get("DS:8812") != "0x00000000":
        raise ExternalClosureError("current platform jump-detachment carry consumption drifted")
    if first.get("player_before", {}).get("record_size") != "0x78" or first.get("player_after", {}).get("record_size") != "0x78":
        raise ExternalClosureError("current platform jump-detachment record size drifted")
    if first.get("player_before", {}).get("mode_0x37") != "0x00" or first.get("player_after", {}).get("mode_0x37") != "0xFF":
        raise ExternalClosureError("current platform jump-detachment mode transition drifted")
    second = observations[1]
    if second.get("carry_before_player", {}).get("DS:8816") != "0x00000000" or second.get("carry_before_player", {}).get("DS:8812") != "0x00000000":
        raise ExternalClosureError("current platform jump-detachment later carry state drifted")
    if second.get("player_mode_before_after") != ["0xFF", "0xFF"]:
        raise ExternalClosureError("current platform jump-detachment later airborne mode drifted")


def check_falling_leaf_static(ledger: dict[str, Any], root: Path) -> None:
    path = root / "research/evidence/player-dos-parity/falling-leaf-static-v1.json"
    if not path.is_file():
        raise ExternalClosureError(f"missing falling-leaf static evidence: {path}")
    try:
        evidence = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read falling-leaf static evidence: {exc}") from exc
    if evidence.get("schema") != "quiky.falling-leaf-static.v1":
        raise ExternalClosureError("unexpected falling-leaf static evidence schema")
    if evidence.get("source", {}).get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("falling-leaf static executable hash drift")
    callback = evidence.get("contract", {}).get("callback", {})
    if callback.get("address") != "01F7:47E7":
        raise ExternalClosureError("falling-leaf callback address drifted")
    if callback.get("player_writes") != []:
        raise ExternalClosureError("falling-leaf callback gained an unverified player write")
    if callback.get("callees") != [
        "01F7:1DCA", "01F7:1DEE", "01F7:1BD1", "01F7:5D60",
    ]:
        raise ExternalClosureError("falling-leaf callback callee order drifted")
    leaf = next(
        (item for item in ledger.get("functions", [])
         if item.get("address") == "01F7:47E7"),
        None,
    )
    if not isinstance(leaf, dict) or "research/evidence/player-dos-parity/falling-leaf-static-v1.json" not in leaf.get("evidence", []):
        raise ExternalClosureError("47E7 ledger entry is missing falling-leaf static evidence")


def check_common_callback_static(ledger: dict[str, Any], root: Path) -> None:
    path = root / "research/evidence/player-dos-parity/platform-common-callback-static-v1.json"
    if not path.is_file():
        raise ExternalClosureError(f"missing common callback static evidence: {path}")
    try:
        evidence = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExternalClosureError(f"cannot read common callback static evidence: {exc}") from exc
    if evidence.get("schema") != "quiky.platform-common-callback-static.v1":
        raise ExternalClosureError("unexpected common callback static evidence schema")
    if evidence.get("source", {}).get("executable_sha256") != ledger["source"]["executable_sha256"]:
        raise ExternalClosureError("common callback executable hash drift")
    export = evidence.get("export", {})
    if export.get("runner") != "research/tools/run_player_external_closure.py":
        raise ExternalClosureError("common callback runner drifted")
    if export.get("independent_project_a_equals_b") is not True:
        raise ExternalClosureError("common callback independent export check is not true")
    for key, expected_hash in (
        ("decomp_a", export.get("decomp_sha256")),
        ("decomp_b", export.get("decomp_sha256")),
        ("listing_a", export.get("listing_sha256")),
        ("listing_b", export.get("listing_sha256")),
        ("data_a", export.get("data_sha256")),
        ("data_b", export.get("data_sha256")),
    ):
        artifact = root / export.get(key, "")
        if not artifact.is_file():
            raise ExternalClosureError(f"missing common callback export artifact: {artifact}")
        if sha256(artifact) != expected_hash:
            raise ExternalClosureError(f"common callback export hash drift: {artifact}")
    contracts = evidence.get("contracts", {})
    if [contracts.get(address, {}).get("name") for address in ("01F7:1997", "01F7:3529", "01F7:34E3", "01F7:3808")] != [
        "address_named_common_callback_1997",
        "address_named_common_callback_3529",
        "address_named_queue_writer_34E3",
        "address_named_coordinate_helper_3808",
    ]:
        raise ExternalClosureError("common callback evidence function names drifted")
    if contracts["01F7:34E3"].get("global_writes") != [
        "queue record at DS:37DE + DS:37E0*0x10", "DS:37E0 += 1",
    ]:
        raise ExternalClosureError("common callback evidence queue writes drifted")
    if evidence.get("stopping_conclusion", "").find("does not write player state") < 0:
        raise ExternalClosureError("common callback evidence lacks simulation stop conclusion")
    for function in ledger.get("functions", []):
        if isinstance(function, dict) and function.get("address") == "01F7:1997":
            if "research/evidence/player-dos-parity/platform-common-callback-static-v1.json" not in function.get("evidence", []):
                raise ExternalClosureError("1997 ledger entry is missing common callback evidence")
            break
    else:
        raise ExternalClosureError("1997 ledger entry is missing")


def check_exports(ledger: dict[str, Any], root: Path) -> None:
    focused = ledger.get("focused_static_export")
    if not isinstance(focused, dict):
        raise ExternalClosureError("focused_static_export must be an object")
    if focused.get("runner") != "research/tools/run_player_external_closure.py":
        raise ExternalClosureError("focused static export runner drifted")
    if focused.get("language") != "x86:LE:16:Protected Mode:default":
        raise ExternalClosureError("focused static export language drifted")
    if focused.get("independent_project_check") != "passed":
        raise ExternalClosureError("focused static export was not independently checked")
    if focused.get("listing_ranges") != [
        "01F7:0E96-0F38", "01F7:0FDC-1032", "01F7:1036-1066",
        "01F7:1997-1BD1", "01F7:3529-3923", "01F7:34E3-38CA", "01F7:3808-38A9",
        "01F7:1749-17AD", "01F7:181C-18D3", "01F7:5C11-5C25",
        "01F7:1B07-1B5C", "01F7:1B5D-1BDC",
        "01F7:1BD1-1C10", "01F7:1C6E-1CB5", "01F7:5C27-5C86", "01F7:5CC3-5D22", "01F7:5DC3-5DF2",
        "01F7:1DEE-1E03", "01F7:A06F-A07E", "01F7:9DC7-9EC6", "01F7:9BEE-9C4D", "01F7:9C0C-9CAB", "01F7:A075-A0B1", "01F7:A0B2-A100",
        "01F7:3D02-3D81", "01F7:3DF2-3E71", "01F7:3FF8-44F7", "01F7:05A0-060F",
        "01F7:6D5F-6DBF", "01F7:6DC4-70D8",
        "01F7:4727-47E6", "01F7:474D-47CC", "01F7:47E7-48E6",
        "01F7:8BC2-8C21", "01F7:8BE5-8C04", "01F7:8C08-8C27",
        "01F7:8C2B-8C4A", "01F7:8C4E-8C8D", "01F7:8C71-8C90",
        "01F7:8C8A-8CA9", "01F7:8CA3-8CC2", "01F7:8CBC-8CDB",
        "01F7:8CD5-8CF4", "01F7:8CEE-8D0D", "01F7:8D07-8D26",
        "01F7:8D20-8E9F", "01F7:8D31-8E30", "01F7:8E42-8E81", "01F7:8E4B-9555",
        "01F7:9256-9295", "01F7:9269-92E8", "01F7:92A9-9348",
    ]:
        raise ExternalClosureError("focused scheduler/carry listing ranges drifted")
    if focused.get("callback_boundary_ranges") != [
        "01F7:41C1-41E5", "01F7:4416-44FE", "01F7:5937-5BED",
    ]:
        raise ExternalClosureError("focused callback-boundary listing ranges drifted")
    if focused.get("mainloop_listing_ranges") != [
        "01D7:3FAD-4430", "01D7:44F0-451F", "01D7:47F0-482F",
        "01D7:4860-489F", "01D7:4A39-4B7B", "01D7:48A0-495F",
        "01D7:39ED-3BAB", "01D7:34C7-3546", "01D7:3861-38E0",
        "01D7:4B80-4C3F", "01D7:4EA0-4F5F", "01D7:4F10-4FBF",
        "01D7:5010-507F", "01D7:14E1-1733",
    ]:
        raise ExternalClosureError("focused main-loop progression listing ranges drifted")
    if focused.get("timer_boundary_ranges") != [
        "01F7:F049-F079", "0207:0002-002B", "0207:101F-1079", "0207:10A3-10A9",
    ]:
        raise ExternalClosureError("focused timer-boundary listing ranges drifted")
    if focused.get("animation_data_ranges") != [
        "QUIKY_SEG06:3142-3150", "QUIKY_SEG06:3156-3164",
        "QUIKY_SEG06:316A-3188", "QUIKY_SEG06:3186-3194",
        "QUIKY_SEG06:3190-319E", "QUIKY_SEG06:31A4-31B8",
        "QUIKY_SEG06:31BA-31D8",
    ]:
        raise ExternalClosureError("focused animation data ranges drifted")
    exports = ledger.get("authoritative_exports")
    if not isinstance(exports, dict):
        raise ExternalClosureError("authoritative_exports must be an object")
    required = ("runner", "decomp_script", "listing_script", "data_script", "callsite_source")
    for key in required:
        path = root / exports[key]
        if not path.is_file():
            raise ExternalClosureError(f"missing export tool {key}: {path}")
    runner_text = (root / exports["runner"]).read_text(encoding="utf-8")
    if "analyzeHeadless" not in runner_text or "x86:LE:16:Protected Mode" not in runner_text:
        raise ExternalClosureError("external runner is not using the Ghidra protected-mode pipeline")
    if "objdump" in runner_text.lower():
        raise ExternalClosureError("external runner must not use objdump")
    if '"0442"' not in runner_text:
        raise ExternalClosureError("external runner must export the 0442 indirect boundary")
    for token in ('"1DCA"', '"1C4D"', '"6DC4"', '"38CA"', '"38EC"'):
        if token not in runner_text:
            raise ExternalClosureError("external runner must export the natural hazard owner closure")
    if (
        "SEG3_LISTING" not in runner_text
        or "SEG1_TARGETS" not in runner_text
        or '("3FAD", 1156)' not in runner_text
        or '"39ED"' not in runner_text
        or '("39ED", 448)' not in runner_text
        or '"9BEE"' not in runner_text
        or '"9C0C"' not in runner_text
        or '("4A39", 323)' not in runner_text
        or '("0E96", 176)' not in runner_text
        or '("05A0", 112)' not in runner_text
        or '("9BEE", 96)' not in runner_text
        or '("9C0C", 160)' not in runner_text
        or '("0FDC", 80)' not in runner_text
        or '("0FA2", 64)' not in runner_text
        or '("1036", 112)' not in runner_text
        or '("0B56", 192)' not in runner_text
        or '("1CDA", 128)' not in runner_text
        or '("1E04", 448)' not in runner_text
        or '"1997"' not in runner_text
        or '"3529"' not in runner_text
        or '"34E3"' not in runner_text
        or '"3808"' not in runner_text
        or '("1997", 128)' not in runner_text
        or '("3529", 128)' not in runner_text
        or '("34E3", 128)' not in runner_text
        or '("3808", 64)' not in runner_text
        or '("321F", 384)' not in runner_text
        or '("1749", 128)' not in runner_text
        or '("178D", 24)' not in runner_text
        or '("1798", 16)' not in runner_text
        or '("17A3", 16)' not in runner_text
        or '("1892", 256)' not in runner_text
        or '("5C11", 96)' not in runner_text
        or '("1C6E", 72)' not in runner_text
        or '("1C92", 40)' not in runner_text
        or '("3971", 40)' not in runner_text
        or '("3986", 40)' not in runner_text
        or '("5C27", 96)' not in runner_text
        or '("5CC3", 96)' not in runner_text
        or '("5DC3", 48)' not in runner_text
        or '("1DEE", 32)' not in runner_text
        or '("A06F", 16)' not in runner_text
        or '("9DC7", 256)' not in runner_text
        or '("A075", 64)' not in runner_text
        or '("A0B2", 96)' not in runner_text
        or '("3D02", 128)' not in runner_text
        or '("3DF2", 128)' not in runner_text
        or '("3FF8", 1280)' not in runner_text
        or '("6D5F", 96)' not in runner_text
        or '("6DA3", 64)' not in runner_text
        or '("6DB1", 32)' not in runner_text
        or '("6DC4", 800)' not in runner_text
        or '("4727", 192)' not in runner_text
        or '("47E7", 256)' not in runner_text
        or '("8BC2", 96)' not in runner_text
        or '("8D20", 384)' not in runner_text
        or '("8D31", 256)' not in runner_text
        or '("8E4B", 1803)' not in runner_text
        or '("9256", 64)' not in runner_text
        or '("9269", 128)' not in runner_text
        or '("1BD1", 64)' not in runner_text
        or '"1B5D"' not in runner_text
        or '"1B07"' not in runner_text
        or '("1B07", 86)' not in runner_text
        or '("1B5D", 128)' not in runner_text
        or '("41C1", 40)' not in runner_text
        or '("4416", 232)' not in runner_text
        or '("5937", 624)' not in runner_text
        or '"0517"' not in runner_text
        or '"16CE"' not in runner_text
        or '"6370"' not in runner_text
        or "DATA_RANGES" not in runner_text
        or "ANIMATION_DATA_RANGES" not in runner_text
        or '"QUIKY_SEG06.bin"' not in runner_text
        or "animation_data_dir" not in runner_text
        or "SEG4_LISTING" not in runner_text
        or '("0002", 48)' not in runner_text
        or '("101F", 96)' not in runner_text
        or '("10A3", 48)' not in runner_text
        or '("4EA0", 192)' not in runner_text
        or '("4F10", 176)' not in runner_text
        or '("5010", 112)' not in runner_text
        or '("14E1", 595)' not in runner_text
        or '"14E1"' not in runner_text
        or '("34C7", 128)' not in runner_text
        or '("3861", 128)' not in runner_text
        or "SEG2_LIFECYCLE_TARGETS" not in runner_text
        or '"33D5"' not in runner_text
        or '("33D5", 64)' not in runner_text
        or "SEG5_LIFECYCLE_TARGETS" not in runner_text
        or '"05CD"' not in runner_text
        or '("05CD", 64)' not in runner_text
        or "SEG3_LIFECYCLE_TARGETS" not in runner_text
        or "SEG4_LIFECYCLE_TARGETS" not in runner_text
        or "lifecycle_seg2_decomp_dir" not in runner_text
        or "lifecycle_seg3_decomp_dir" not in runner_text
        or "lifecycle_seg4_decomp_dir" not in runner_text
    ):
        raise ExternalClosureError(
            "external runner must export the 5937, contact-effect, and data boundaries"
        )
    for token in ('"199D"', '"19E6"', '"1AAA"', '"0B56"', '"1CDA"', '"1E04"', '"321F"', '"1AF5"', '"34C7"', '"3861"', 'SEG1_TARGETS', 'death_recovery_records', '0x1B01'):
        if token not in runner_text:
            raise ExternalClosureError(
                "external runner must export the death/recovery static closure"
            )


def verify(ledger_path: Path, root: Path) -> None:
    ledger = load(ledger_path)
    check_source_hashes(ledger, root)
    check_contracts(ledger)
    check_5937_dispatch_static(ledger, root)
    check_animation_data_static(ledger, root)
    check_5937_record_manager_static(ledger, root)
    check_5937_record_loader_static(ledger, root)
    check_bump_static(ledger, root)
    check_progression_static(ledger, root)
    check_transition_effect_static(ledger, root)
    check_transition_writer_callback_evidence(ledger, root)
    check_natural_flagged_contact_evidence(ledger, root)
    check_natural_tile41_contact_evidence(ledger, root)
    check_negative_mode_second_probe_evidence(ledger, root)
    check_camera_map_refresh_static(ledger, root)
    check_scheduler_order(ledger)
    check_targeted_static_audit(ledger)
    check_runtime_scheduler_membership(ledger, root)
    check_death_recovery_static(ledger, root)
    check_lifecycle_relocation_static(ledger, root)
    check_current_platform_player_evidence(ledger, root)
    check_current_platform_jump_detachment_evidence(ledger, root)
    check_falling_leaf_static(ledger, root)
    check_common_callback_static(ledger, root)
    check_exports(ledger, root)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ledger", type=Path, default=LEDGER)
    parser.add_argument("--root", type=Path, default=ROOT)
    args = parser.parse_args(argv)
    try:
        verify(args.ledger, args.root)
    except (ExternalClosureError, OSError, KeyError, TypeError) as exc:
        print(f"player-external-closure: {exc}", file=sys.stderr)
        return 1
    print("OK: player external-state closure ledger")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
