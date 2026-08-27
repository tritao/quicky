"""Dependency-light visual parity API."""

from __future__ import annotations

from pathlib import Path
from typing import Any

# The established decoder remains the implementation of the image contract;
# this module gives future run tooling a stable import location.
from scene_frame_compare import (  # type: ignore[import-not-found]
    Image,
    compare_pixels,
    read_bmp,
    read_image,
    read_png,
    write_diff,
)


def compare_frames(
    actual: Path,
    expected: Path,
    *,
    region: tuple[int, int, int, int] | None = None,
    tolerance: int = 0,
    diff: Path | None = None,
) -> dict[str, Any]:
    """Compare two PNG/BMP frames and optionally emit the PPM overlay."""

    left = read_image(actual)
    right = read_image(expected)
    report = compare_pixels(left, right, region, tolerance)
    if diff is not None:
        diff.parent.mkdir(parents=True, exist_ok=True)
        write_diff(diff, left, right, tolerance)
    return report
