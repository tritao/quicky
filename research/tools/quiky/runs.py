"""Canonical recorded-run directories."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import Any, Iterable

from .common import ToolError, file_fingerprint, read_json, write_json
from .state import (PROFILES, compare_state, label_lifecycle, load_state_jsonl,
                    save_state_jsonl)

RUN_SCHEMA = "quiky.recorded-run-v5"
RUN_FILES = ("input.jsonl", "expected-state.jsonl", "actual-state.jsonl")


def validate_fingerprint(value: Any, *, label: str) -> dict[str, Any]:
    """Validate the one fingerprint shape used throughout run manifests."""
    if (not isinstance(value, dict) or set(value) != {"path", "size", "sha256"} or
            not isinstance(value["path"], str) or not value["path"] or
            not isinstance(value["size"], int) or value["size"] < 0 or
            not isinstance(value["sha256"], str) or len(value["sha256"]) != 64 or
            any(character not in "0123456789abcdef" for character in value["sha256"])):
        raise ToolError(f"{label} fingerprint is invalid")
    return value


def validate_input_row(value: Any, *, label: str = "input row") -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ToolError(f"{label}: expected object")
    sequence, flags = value.get("sequence"), value.get("input_flags")
    guest_frame = value.get("guest_frame", sequence)
    if not isinstance(sequence, int) or sequence < 1:
        raise ToolError(f"{label}: sequence must be a positive integer")
    if not isinstance(guest_frame, int) or guest_frame < 0:
        raise ToolError(f"{label}: guest_frame must be a non-negative integer")
    if not isinstance(flags, int) or not 0 <= flags <= 0xffff:
        raise ToolError(f"{label}: input_flags must be a uint16")
    camera = value.get("camera")
    if camera is not None and (not isinstance(camera, dict) or
                               not isinstance(camera.get("x"), int) or
                               not isinstance(camera.get("y"), int)):
        raise ToolError(f"{label}: camera must contain integer x and y")
    unknown = set(value).difference({"sequence", "guest_frame", "input_flags", "camera"})
    if unknown:
        raise ToolError(f"{label}: unknown fields: {', '.join(sorted(unknown))}")
    return {"sequence": sequence, "guest_frame": guest_frame,
            "input_flags": flags,
            **({"camera": {"x": camera["x"], "y": camera["y"]}}
               if camera is not None else {})}


def validate_input_rows(rows: Iterable[dict[str, Any]]) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []
    previous = 0
    for index, row in enumerate(rows, 1):
        normalized = validate_input_row(row, label=f"input row {index}")
        if normalized["sequence"] <= previous:
            raise ToolError("input rows must have strictly increasing sequence")
        previous = normalized["sequence"]
        result.append(normalized)
    return result


def load_input_jsonl(path: Path) -> list[dict[str, Any]]:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise ToolError(f"{path}: cannot read input stream: {exc}") from exc
    rows: list[dict[str, Any]] = []
    for index, line in enumerate(lines, 1):
        if not line.strip():
            raise ToolError(f"{path}: blank line at {index}")
        try:
            rows.append(json.loads(line))
        except json.JSONDecodeError as exc:
            raise ToolError(f"{path}: invalid JSON at line {index}: {exc}") from exc
    return validate_input_rows(rows)


def save_input_jsonl(path: Path, rows: Iterable[dict[str, Any]]) -> None:
    normalized = validate_input_rows(rows)
    if not normalized:
        raise ToolError("input stream is empty")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(json.dumps(row, sort_keys=True, separators=(",", ":")) + "\n"
                            for row in normalized), encoding="utf-8")


def new_run_manifest(name: str, *, profile: str,
                     provenance: dict[str, Any] | None = None) -> dict[str, Any]:
    if not name:
        raise ToolError("run name must be non-empty")
    if profile not in PROFILES:
        raise ToolError(f"unknown parity profile {profile!r}")
    return {"schema": RUN_SCHEMA, "format_version": 5, "name": name,
            "profile": profile, "provenance": dict(provenance or {}),
            "replay": None, "files": {}}


def validate_manifest(manifest: dict[str, Any]) -> dict[str, Any]:
    if not isinstance(manifest, dict) or manifest.get("schema") != RUN_SCHEMA:
        raise ToolError(f"manifest must use {RUN_SCHEMA}")
    if manifest.get("format_version") != 5:
        raise ToolError("manifest format_version must be 5")
    if not isinstance(manifest.get("name"), str) or not manifest["name"]:
        raise ToolError("manifest name must be non-empty")
    if manifest.get("profile") not in PROFILES:
        raise ToolError("manifest has invalid parity profile")
    if not isinstance(manifest.get("provenance"), dict):
        raise ToolError("manifest provenance must be an object")
    for name, fingerprint in manifest["provenance"].items():
        validate_fingerprint(fingerprint, label=f"manifest provenance {name}")
    if not isinstance(manifest.get("files"), dict):
        raise ToolError("manifest files must be an object")
    replay = manifest.get("replay")
    if replay is not None:
        required = {"archive", "map", "player_bob", "leaf_prng"}
        if not isinstance(replay, dict) or set(replay) != required:
            raise ToolError("manifest replay configuration is invalid")
        archive = replay["archive"]
        validate_fingerprint(archive, label="manifest replay archive")
        if not isinstance(replay["map"], str) or not replay["map"]:
            raise ToolError("manifest replay map is invalid")
        if replay["player_bob"] is not None and (
                not isinstance(replay["player_bob"], str) or not replay["player_bob"]):
            raise ToolError("manifest replay player_bob is invalid")
        leaf = replay["leaf_prng"]
        if leaf is not None and (not isinstance(leaf, dict) or
                                 set(leaf) != {"index", "ring_hex"} or
                                 not isinstance(leaf.get("index"), int) or
                                 not 0 <= leaf["index"] <= 0xff or
                                 not isinstance(leaf.get("ring_hex"), str) or
                                 len(leaf["ring_hex"]) != 0x200 or
                                 any(character not in "0123456789abcdef"
                                     for character in leaf["ring_hex"])):
            raise ToolError("manifest replay leaf PRNG state is invalid")
    unknown = set(manifest).difference(
        {"schema", "format_version", "name", "profile", "provenance",
         "replay", "files"})
    if unknown:
        raise ToolError(f"manifest has unknown fields: {', '.join(sorted(unknown))}")
    return manifest


def load_manifest(path: Path) -> dict[str, Any]:
    try:
        return validate_manifest(read_json(path))
    except ToolError as exc:
        raise ToolError(f"{path}: {exc}") from exc


def save_manifest(path: Path, manifest: dict[str, Any]) -> None:
    validate_manifest(manifest)
    write_json(path, manifest)


def _fingerprints(directory: Path) -> dict[str, Any]:
    """Fingerprint parity inputs, never regenerable verification reports."""
    result = {}
    for name in RUN_FILES:
        if not (directory / name).is_file():
            continue
        fingerprint = file_fingerprint(directory / name)
        fingerprint["path"] = name
        result[name] = fingerprint
    return result


def stage_run_files(directory: Path, *, name: str, profile: str,
                    input_rows: Iterable[dict[str, Any]], expected_state: Path,
                    actual_state: Path | None = None,
                    provenance: dict[str, Any] | None = None) -> dict[str, Any]:
    if directory.exists() and any(directory.iterdir()):
        raise ToolError(f"run directory is not empty: {directory}")
    directory.mkdir(parents=True, exist_ok=True)
    save_input_jsonl(directory / "input.jsonl", input_rows)
    for target, source in (("expected-state.jsonl", expected_state),
                           ("actual-state.jsonl", actual_state)):
        if source is not None:
            load_state_jsonl(source)
            shutil.copyfile(source, directory / target)
    manifest = new_run_manifest(name, profile=profile, provenance=provenance)
    manifest["files"] = _fingerprints(directory)
    save_manifest(directory / "manifest.json", manifest)
    return manifest


def validate_run_directory(directory: Path) -> dict[str, Any]:
    manifest = load_manifest(directory / "manifest.json")
    for name, fingerprint in manifest["files"].items():
        if name not in RUN_FILES:
            raise ToolError(f"manifest file entry is invalid: {name}")
        validate_fingerprint(fingerprint, label=f"manifest file {name}")
        path = directory / name
        if not path.is_file():
            raise ToolError(f"run is missing canonical file: {name}")
        actual_fingerprint = file_fingerprint(path)
        if (actual_fingerprint["sha256"] != fingerprint["sha256"] or
                actual_fingerprint["size"] != fingerprint["size"]):
            raise ToolError(f"run file digest mismatch: {name}")
    for required in ("input.jsonl", "expected-state.jsonl"):
        if required not in manifest["files"]:
            raise ToolError(f"manifest is missing required file: {required}")
    load_input_jsonl(directory / "input.jsonl")
    load_state_jsonl(directory / "expected-state.jsonl")
    if "actual-state.jsonl" in manifest["files"]:
        load_state_jsonl(directory / "actual-state.jsonl")
    return manifest


def verify_run_directory(directory: Path) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    manifest = validate_run_directory(directory)
    actual = directory / "actual-state.jsonl"
    if "actual-state.jsonl" not in manifest["files"]:
        raise ToolError("run parity requires actual-state.jsonl")
    mismatches, coverage = compare_state(
        directory / "expected-state.jsonl", actual, manifest["profile"])
    return mismatches, coverage


def _replay_recipe(*, archive: Path, map_resource: str,
                   player_bob: str | None, leaf_prng_index: int | None,
                   leaf_prng_ring_hex: str | None) -> dict[str, Any]:
    fingerprint = file_fingerprint(archive)
    fingerprint["path"] = str(archive)
    return {
        "archive": fingerprint,
        "map": map_resource,
        "player_bob": player_bob,
        "leaf_prng": ({"index": leaf_prng_index,
                       "ring_hex": leaf_prng_ring_hex.lower()}
                      if leaf_prng_index is not None else None),
    }


def replay_run(directory: Path, *, binary: Path, archive: Path | None = None,
               map_resource: str | None = None, player_bob: str | None = None,
               leaf_prng_index: int | None = None,
               leaf_prng_ring_hex: str | None = None) -> None:
    """Replay a run and publish state plus recipe only after successful capture."""
    manifest = validate_run_directory(directory)
    recipe = manifest.get("replay")
    configuring = (map_resource is not None or player_bob is not None or
                   leaf_prng_index is not None or leaf_prng_ring_hex is not None)
    if recipe is None or configuring:
        if archive is None or map_resource is None:
            raise ToolError("replay configuration requires --archive and --map")
        if (leaf_prng_index is None) != (leaf_prng_ring_hex is None):
            raise ToolError("leaf PRNG index and ring must be supplied together")
        recipe = _replay_recipe(
            archive=archive, map_resource=map_resource, player_bob=player_bob,
            leaf_prng_index=leaf_prng_index,
            leaf_prng_ring_hex=leaf_prng_ring_hex)
    replay_archive = archive if archive is not None else Path(recipe["archive"]["path"])
    actual_archive = file_fingerprint(replay_archive)
    if (actual_archive["sha256"] != recipe["archive"]["sha256"] or
            actual_archive["size"] != recipe["archive"]["size"]):
        raise ToolError(f"replay archive fingerprint mismatch: {replay_archive}")

    with tempfile.TemporaryDirectory(prefix="quiky-run-replay-") as temp:
        root = Path(temp)
        raw, state = root / "native-trace.json", root / "actual-state.jsonl"
        command = [str(binary), str(replay_archive), recipe["map"], str(raw),
                   "--input-jsonl", str(directory / "input.jsonl")]
        if recipe["player_bob"] is not None:
            command.extend(("--player-bob", recipe["player_bob"]))
        if recipe["leaf_prng"] is not None:
            command.extend(("--leaf-prng-index", str(recipe["leaf_prng"]["index"]),
                            "--leaf-prng-ring-hex", recipe["leaf_prng"]["ring_hex"]))
        completed = subprocess.run(command, text=True, capture_output=True)
        if completed.returncode:
            detail = completed.stderr.strip() or completed.stdout.strip()
            raise ToolError(f"native replay failed: {detail}")
        rows = load_state_jsonl(raw)
        if manifest["profile"] == "lifecycle":
            label_lifecycle(rows)
        save_state_jsonl(state, rows)

        staged_state = directory / ".actual-state.jsonl.tmp"
        staged_manifest = directory / ".manifest.json.tmp"
        try:
            shutil.copyfile(state, staged_state)
            manifest["replay"] = recipe
            manifest["files"]["actual-state.jsonl"] = file_fingerprint(staged_state)
            manifest["files"]["actual-state.jsonl"]["path"] = "actual-state.jsonl"
            save_manifest(staged_manifest, manifest)
            os.replace(staged_state, directory / "actual-state.jsonl")
            os.replace(staged_manifest, directory / "manifest.json")
        finally:
            staged_state.unlink(missing_ok=True)
            staged_manifest.unlink(missing_ok=True)
