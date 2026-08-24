#!/usr/bin/env python3
"""Validate and summarize the entity behavior-family research matrix."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


DIMENSIONS = (
    "initializer_fields",
    "update_callback_state_machine",
    "movement_ai",
    "collision_shape_interaction",
    "animation_table",
    "death_removal",
    "cross_world_variants",
)
STATUSES = {"confirmed", "partial", "unresolved", "not_applicable"}


class MatrixError(ValueError):
    """Raised when a behavior-family matrix violates its contract."""


def load_matrix(path: Path) -> dict[str, Any]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise MatrixError(f"cannot read matrix {path}: {exc}") from exc
    validate_matrix(payload)
    return payload


def validate_matrix(payload: Any) -> None:
    if not isinstance(payload, dict):
        raise MatrixError("matrix root must be an object")
    if payload.get("schema") != "quiky-entity-behavior-families-v1":
        raise MatrixError("matrix has the wrong schema")
    if tuple(payload.get("dimensions", ())) != DIMENSIONS:
        raise MatrixError("matrix dimensions do not match the required seven fields")
    families = payload.get("families")
    if not isinstance(families, list) or not families:
        raise MatrixError("matrix families must be a non-empty array")
    seen_family_ids: set[str] = set()
    seen_types: set[str] = set()
    for family in families:
        if not isinstance(family, dict):
            raise MatrixError("each family must be an object")
        family_id = family.get("id")
        if not isinstance(family_id, str) or not family_id:
            raise MatrixError("each family needs a non-empty id")
        if family_id in seen_family_ids:
            raise MatrixError(f"duplicate family id {family_id}")
        seen_family_ids.add(family_id)
        types = family.get("types")
        if not isinstance(types, list) or not types:
            raise MatrixError(f"family {family_id} needs at least one type")
        for entity_type in types:
            if (not isinstance(entity_type, str) or len(entity_type) != 4 or
                    not entity_type.startswith("0x")):
                raise MatrixError(f"family {family_id} has an invalid type {entity_type!r}")
            try:
                int(entity_type, 16)
            except ValueError as exc:
                raise MatrixError(f"family {family_id} has an invalid type {entity_type!r}") from exc
            if entity_type in seen_types:
                raise MatrixError(f"entity type {entity_type} appears in multiple families")
            seen_types.add(entity_type)
        for dimension in DIMENSIONS:
            evidence = family.get(dimension)
            if not isinstance(evidence, dict):
                raise MatrixError(f"family {family_id} is missing {dimension}")
            status = evidence.get("status")
            if status not in STATUSES:
                raise MatrixError(f"family {family_id} has invalid {dimension} status {status!r}")
            known = evidence.get("known")
            if not isinstance(known, str) or not known.strip():
                raise MatrixError(f"family {family_id} needs evidence text for {dimension}")
        refs = family.get("evidence_refs")
        if not isinstance(refs, list) or not refs:
            raise MatrixError(f"family {family_id} needs at least one evidence reference")
        questions = family.get("open_questions")
        if not isinstance(questions, list):
            raise MatrixError(f"family {family_id} open_questions must be an array")


def summarize(payload: dict[str, Any]) -> dict[str, Any]:
    families = payload["families"]
    counts = {dimension: {status: 0 for status in sorted(STATUSES)}
              for dimension in DIMENSIONS}
    for family in families:
        for dimension in DIMENSIONS:
            counts[dimension][family[dimension]["status"]] += 1
    return {
        "schema": payload["schema"],
        "family_count": len(families),
        "type_count": sum(len(family["types"]) for family in families),
        "families_with_open_questions": sum(bool(family["open_questions"]) for family in families),
        "dimension_status_counts": counts,
    }


def _print_report(payload: dict[str, Any]) -> None:
    summary = summarize(payload)
    print(f"families: {summary['family_count']}")
    print(f"types:    {summary['type_count']}")
    print("family\t types\t initializer\t update\t movement\t collision\t animation\t removal\t worlds")
    for family in payload["families"]:
        statuses = [family[dimension]["status"] for dimension in DIMENSIONS]
        print(
            f"{family['id']}\t{','.join(family['types'])}\t" +
            "\t".join(statuses) + "\t" + ",".join(family.get("worlds", []))
        )
    print("\ndimension status counts:")
    for dimension in DIMENSIONS:
        values = summary["dimension_status_counts"][dimension]
        print(f"{dimension}: " + ", ".join(f"{status}={values[status]}" for status in sorted(STATUSES)))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("matrix", type=Path)
    parser.add_argument("--json", action="store_true", dest="as_json")
    args = parser.parse_args(argv)
    try:
        payload = load_matrix(args.matrix)
    except MatrixError as exc:
        print(f"entity-behavior-report: {exc}", file=sys.stderr)
        return 2
    if args.as_json:
        print(json.dumps(summarize(payload), indent=2))
    else:
        _print_report(payload)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
