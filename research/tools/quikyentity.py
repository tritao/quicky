#!/usr/bin/env python3
"""Run an isolated baseline/inert ARE-entity experiment."""

from __future__ import annotations

import argparse
import json
import struct
import subprocess
import sys
import zlib
from pathlib import Path

from quikyctl import (
    QuikyError, _write_rgb_png, create_entity_variant,
    select_entity_representative,
)


def _read_png_rgb(path: Path) -> tuple[int, int, bytes]:
    raw = path.read_bytes()
    if raw[:8] != b"\x89PNG\r\n\x1a\n":
        raise QuikyError(f"{path} is not a PNG")
    cursor = 8
    width = height = bit_depth = color_type = None
    compressed = bytearray()
    while cursor + 8 <= len(raw):
        size = struct.unpack_from(">I", raw, cursor)[0]
        kind = raw[cursor + 4 : cursor + 8]
        payload = raw[cursor + 8 : cursor + 8 + size]
        cursor += 12 + size
        if kind == b"IHDR":
            width, height, bit_depth, color_type, compression, filtering, interlace = struct.unpack(
                ">IIBBBBB", payload
            )
            if (bit_depth != 8 or color_type not in (2, 6) or
                    compression != 0 or filtering != 0 or interlace != 0):
                raise QuikyError(f"unsupported PNG format in {path}")
        elif kind == b"IDAT":
            compressed.extend(payload)
        elif kind == b"IEND":
            break
    if width is None or height is None:
        raise QuikyError(f"PNG {path} has no IHDR")
    channels = 4 if color_type == 6 else 3
    stride = width * channels
    decoded = zlib.decompress(bytes(compressed))
    if len(decoded) != height * (stride + 1):
        raise QuikyError(f"PNG {path} has an invalid scanline length")
    rows: list[bytes] = []
    previous = bytearray(stride)
    cursor = 0
    for _ in range(height):
        filter_type = decoded[cursor]
        cursor += 1
        source = decoded[cursor : cursor + stride]
        cursor += stride
        row = bytearray(stride)
        for index, value in enumerate(source):
            left = row[index - channels] if index >= channels else 0
            above = previous[index]
            upper_left = previous[index - channels] if index >= channels else 0
            if filter_type == 0:
                result = value
            elif filter_type == 1:
                result = (value + left) & 0xFF
            elif filter_type == 2:
                result = (value + above) & 0xFF
            elif filter_type == 3:
                result = (value + ((left + above) // 2)) & 0xFF
            elif filter_type == 4:
                estimate = left + above - upper_left
                distances = (abs(estimate - left), abs(estimate - above),
                             abs(estimate - upper_left))
                predictor = (left, above, upper_left)[distances.index(min(distances))]
                result = (value + predictor) & 0xFF
            else:
                raise QuikyError(f"PNG {path} uses unknown filter {filter_type}")
            row[index] = result
        rows.append(bytes(row))
        previous = row
    if channels == 3:
        return width, height, b"".join(rows)
    return width, height, b"".join(
        row[index] for row in rows for index in range(0, len(row), 4)
    )


def analyze_frame_differences(
    baseline_paths: list[Path], inert_paths: list[Path], output_dir: Path,
) -> dict[str, object]:
    if len(baseline_paths) != len(inert_paths) or not baseline_paths:
        raise QuikyError("baseline and inert frame counts must match and be non-zero")
    output_dir.mkdir(parents=True, exist_ok=True)
    masks: list[bytearray] = []
    dimensions: tuple[int, int] | None = None
    changed_by_frame: list[int] = []
    difference_paths: list[str] = []
    for index, (baseline, inert) in enumerate(zip(baseline_paths, inert_paths)):
        width, height, left = _read_png_rgb(baseline)
        other_width, other_height, right = _read_png_rgb(inert)
        if (width, height) != (other_width, other_height):
            raise QuikyError("baseline and inert PNG dimensions differ")
        if dimensions is None:
            dimensions = (width, height)
        elif dimensions != (width, height):
            raise QuikyError("captured PNG dimensions change between frames")
        mask = bytearray(
            255 if left[pos : pos + 3] != right[pos : pos + 3] else 0
            for pos in range(0, len(left), 3)
        )
        masks.append(mask)
        changed = sum(value != 0 for value in mask)
        changed_by_frame.append(changed)
        rgb = bytes(channel for value in mask for channel in (value, value, value))
        path = output_dir / ("difference.png" if len(baseline_paths) == 1
                             else f"difference-{index:03d}.png")
        _write_rgb_png(path, width, height, rgb)
        difference_paths.append(str(path))
    assert dimensions is not None
    width, height = dimensions
    union = bytearray(len(masks[0]))
    intersection = bytearray([255]) * len(masks[0])
    for mask in masks:
        for pos, value in enumerate(mask):
            union[pos] = max(union[pos], value)
            intersection[pos] = min(intersection[pos], value)
    def write_aggregate(name: str, mask: bytearray) -> str:
        path = output_dir / name
        _write_rgb_png(path, width, height,
                       bytes(channel for value in mask for channel in (value, value, value)))
        return str(path)
    changed_positions = [pos for pos, value in enumerate(union) if value]
    if changed_positions:
        xs = [pos % width for pos in changed_positions]
        ys = [pos // width for pos in changed_positions]
        bbox = [min(xs), min(ys), max(xs) - min(xs) + 1, max(ys) - min(ys) + 1]
    else:
        bbox = None
    return {
        "frame_count": len(masks),
        "changed_pixels_by_frame": changed_by_frame,
        "union_pixels": sum(value != 0 for value in union),
        "intersection_pixels": sum(value != 0 for value in intersection),
        "union_bbox": bbox,
        "difference_paths": difference_paths,
        "union_path": write_aggregate("diff-union.png", union),
        "intersection_path": write_aggregate("diff-intersection.png", intersection),
    }


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
    parser.add_argument("--capture-frames", type=int, default=1)
    parser.add_argument("--frame-step", type=int, default=30)
    parser.add_argument("--sprite-init-offset", type=lambda value: int(value, 0),
                        default=0,
                        help="post-initializer breakpoint for sprite-slot capture")
    parser.add_argument("--overwrite", action="store_true")
    parser.add_argument("--dry-run", action="store_true",
                        help="print the selected representative without running DOSBox")
    args = parser.parse_args(argv)
    if args.capture_frames < 1 or args.frame_step < 0:
        parser.error("--capture-frames must be positive and --frame-step non-negative")

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
            "--capture-frames", str(args.capture_frames),
            "--frame-step", str(args.frame_step),
        ]
        if expected_type == args.entity_type and args.sprite_init_offset:
            command.extend(["--sprite-init-offset", hex(args.sprite_init_offset)])
        subprocess.run(command, cwd=repo_root, check=True)
        screenshot_paths = [
            screenshot if args.capture_frames == 1 else screenshot.with_name(
                f"{screenshot.stem}-frame-{index:03d}{screenshot.suffix}"
            ) for index in range(args.capture_frames)
        ]
        results.append({"name": name, "state": str(state),
                        "screenshot": str(screenshot),
                        "screenshots": [str(path) for path in screenshot_paths]})

    analysis = analyze_frame_differences(
        [Path(path) for path in results[0]["screenshots"]],
        [Path(path) for path in results[1]["screenshots"]],
        args.output_dir,
    )

    experiment = {
        **manifest,
        "selection": selection,
        "capture_delay_frames": args.delay_frames,
        "capture_frames": args.capture_frames,
        "frame_step": args.frame_step,
        "results": results,
        "difference": analysis["difference_paths"][0],
        "changed_pixels": analysis["changed_pixels_by_frame"][0],
        "frame_analysis": analysis,
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
