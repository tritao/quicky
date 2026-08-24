#!/usr/bin/env python3
"""Run an isolated baseline/inert ARE-entity experiment."""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path

from quikyctl import (
    QuikyError, create_entity_variant, select_entity_representative,
)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--level", default="W1L1.ARE")
    parser.add_argument("--record-offset",
                        type=lambda value: int(value, 0))
    parser.add_argument("--type", dest="entity_type", required=True,
                        type=lambda value: int(value, 0))
    parser.add_argument("--inert-type", type=lambda value: int(value, 0), default=0)
    parser.add_argument("--delay-frames", type=int, default=300)
    parser.add_argument("--overwrite", action="store_true")
    parser.add_argument("--dry-run", action="store_true",
                        help="print the selected representative without running DOSBox")
    args = parser.parse_args(argv)

    repo_root = Path(__file__).resolve().parents[2]
    try:
        selected = select_entity_representative(
            args.archive, args.entity_type, args.level,
        )
        record_offset = (
            args.record_offset if args.record_offset is not None
            else selected.record_offset
        )
        if args.record_offset is not None and record_offset != selected.record_offset:
            # Explicit offsets remain supported, but create_entity_variant performs
            # the authoritative type and bounds validation below.
            selected_source = "explicit"
        else:
            selected_source = "automatic"
        selection = {
            "source": selected_source,
            "level": args.level,
            "type": args.entity_type,
            "record_offset": record_offset,
            "nearest_catalog_placement": {
                "record_offset": selected.record_offset,
                "world_x": selected.world_x, "world_y": selected.world_y,
                "reference": selected.reference,
            },
            "anchor": {"world_x": 768, "world_y": 224},
        }
        if args.dry_run:
            print(json.dumps(selection, indent=2))
            return 0
        manifest = create_entity_variant(
            args.archive,
            args.output_dir,
            args.level,
            record_offset,
            args.inert_type,
            args.overwrite,
            (12, 3) if selected_source == "automatic" else None,
        )
    except (OSError, QuikyError) as exc:
        print(f"quikyentity: {exc}", file=sys.stderr)
        return 2

    trace_tool = repo_root / "research/tools/quikytrace.py"
    results = []
    for variant, expected_type in zip(
        manifest["variants"], (args.entity_type, args.inert_type)
    ):
        name = variant["name"]
        state = args.output_dir / f"{name}-state.json"
        screenshot = args.output_dir / f"{name}.png"
        command = [
            sys.executable, str(trace_tool), "--launch", "--headless",
            "--runtime-dir", variant["directory"],
            "--output", str(state),
            "--entity-record-offset", hex(record_offset),
            "--entity-type", hex(expected_type),
            "--screenshot", str(screenshot),
            "--screenshot-delay-frames", str(args.delay_frames),
        ]
        subprocess.run(command, cwd=repo_root, check=True)
        results.append({"name": name, "state": str(state),
                        "screenshot": str(screenshot)})

    difference = args.output_dir / "difference.png"
    changed_pixels = None
    compare = shutil.which("compare")
    if compare:
        completed = subprocess.run(
            [compare, "-metric", "AE", results[0]["screenshot"],
             results[1]["screenshot"], str(difference)],
            text=True, capture_output=True,
        )
        if completed.returncode not in (0, 1):
            raise RuntimeError(completed.stderr.strip())
        changed_pixels = int(completed.stderr.strip() or "0")

    experiment = {
        **manifest,
        "selection": selection,
        "capture_delay_frames": args.delay_frames,
        "results": results,
        "difference": str(difference) if compare else None,
        "changed_pixels": changed_pixels,
        "interpretation": None,
    }
    experiment_path = args.output_dir / "experiment.json"
    experiment_path.write_text(
        json.dumps(experiment, indent=2) + "\n", encoding="utf-8"
    )
    print(f"experiment: {experiment_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
