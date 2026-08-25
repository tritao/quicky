#!/usr/bin/env python3
"""Compare a DOSBox frame with a deterministic recreation frame.

The DOSBox automation endpoint normally produces an RGB PNG while the native
probe writes an indexed BMP. Keeping the decoders here dependency-free makes
the comparison usable from the research checkout and preserves both exact
RGB error and a region-of-interest view for isolated object probes.
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence


RGB = tuple[int, int, int]


@dataclass(frozen=True)
class Image:
    width: int
    height: int
    pixels: tuple[RGB, ...]

    def at(self, x: int, y: int) -> RGB:
        return self.pixels[y * self.width + x]


def _u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def _u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def _paeth(left: int, above: int, upper_left: int) -> int:
    estimate = left + above - upper_left
    distance_left = abs(estimate - left)
    distance_above = abs(estimate - above)
    distance_upper_left = abs(estimate - upper_left)
    if distance_left <= distance_above and distance_left <= distance_upper_left:
        return left
    if distance_above <= distance_upper_left:
        return above
    return upper_left


def _unfilter(rows: bytes, width: int, height: int, bytes_per_pixel: int,
              row_bytes: int) -> list[bytes]:
    stride = row_bytes + 1
    if len(rows) != stride * height:
        raise ValueError("PNG decompressed data has an unexpected size")
    result: list[bytes] = []
    previous = bytes(row_bytes)
    for row_index in range(height):
        filter_type = rows[row_index * stride]
        encoded = rows[row_index * stride + 1:(row_index + 1) * stride]
        decoded = bytearray(row_bytes)
        for index, value in enumerate(encoded):
            left = decoded[index - bytes_per_pixel] if index >= bytes_per_pixel else 0
            above = previous[index]
            upper_left = (previous[index - bytes_per_pixel]
                          if index >= bytes_per_pixel else 0)
            if filter_type == 0:
                decoded[index] = value
            elif filter_type == 1:
                decoded[index] = (value + left) & 0xff
            elif filter_type == 2:
                decoded[index] = (value + above) & 0xff
            elif filter_type == 3:
                decoded[index] = (value + ((left + above) // 2)) & 0xff
            elif filter_type == 4:
                decoded[index] = (value + _paeth(left, above, upper_left)) & 0xff
            else:
                raise ValueError(f"unsupported PNG filter type {filter_type}")
        result.append(bytes(decoded))
        previous = bytes(decoded)
    return result


def read_png(path: Path) -> Image:
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: not a PNG")
    width = height = depth = color_type = interlace = None
    idat = bytearray()
    palette: list[RGB] = []
    transparency: bytes | None = None
    offset = 8
    while offset + 12 <= len(data):
        length = struct.unpack_from(">I", data, offset)[0]
        kind = data[offset + 4:offset + 8]
        payload = data[offset + 8:offset + 8 + length]
        offset += 12 + length
        if kind == b"IHDR":
            width, height, depth, color_type, compression, filtering, interlace = \
                struct.unpack(">IIBBBBB", payload)
            if compression != 0 or filtering != 0:
                raise ValueError(f"{path}: unsupported PNG compression/filter method")
        elif kind == b"PLTE":
            palette = [tuple(payload[i:i + 3])  # type: ignore[misc]
                       for i in range(0, len(payload), 3)]
        elif kind == b"tRNS":
            transparency = payload
        elif kind == b"IDAT":
            idat.extend(payload)
        elif kind == b"IEND":
            break
    if width is None or height is None or depth != 8 or interlace != 0:
        raise ValueError(f"{path}: requires non-interlaced 8-bit PNG")
    if color_type not in (0, 2, 3, 4, 6):
        raise ValueError(f"{path}: unsupported PNG color type {color_type}")
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[color_type]
    rows = _unfilter(zlib.decompress(bytes(idat)), width, height, channels,
                     width * channels)
    pixels: list[RGB] = []
    for row in rows:
        for x in range(width):
            values = row[x * channels:(x + 1) * channels]
            if color_type == 0:
                pixels.append((values[0], values[0], values[0]))
            elif color_type == 2:
                pixels.append((values[0], values[1], values[2]))
            elif color_type == 3:
                if values[0] >= len(palette):
                    raise ValueError(f"{path}: palette index outside PLTE")
                pixels.append(palette[values[0]])
            elif color_type == 4:
                pixels.append((values[0], values[0], values[0]))
            else:
                pixels.append((values[0], values[1], values[2]))
    return Image(width, height, tuple(pixels))


def read_bmp(path: Path) -> Image:
    data = path.read_bytes()
    if data[:2] != b"BM" or len(data) < 54:
        raise ValueError(f"{path}: not a BMP")
    pixel_offset = _u32(data, 10)
    header_size = _u32(data, 14)
    if header_size < 40:
        raise ValueError(f"{path}: unsupported BMP header")
    width = struct.unpack_from("<i", data, 18)[0]
    height_raw = struct.unpack_from("<i", data, 22)[0]
    planes = _u16(data, 26)
    bits = _u16(data, 28)
    compression = _u32(data, 30)
    if width <= 0 or height_raw == 0 or planes != 1 or compression != 0:
        raise ValueError(f"{path}: unsupported BMP layout")
    top_down = height_raw < 0
    height = abs(height_raw)
    palette: list[RGB] = []
    palette_entries = _u32(data, 46) if header_size >= 40 else 0
    if bits == 8:
        if palette_entries == 0:
            palette_entries = 256
        palette_offset = 14 + header_size
        for index in range(palette_entries):
            blue, green, red, _ = struct.unpack_from("<BBBB", data,
                                                      palette_offset + index * 4)
            palette.append((red, green, blue))
    if bits not in (8, 24, 32):
        raise ValueError(f"{path}: supports only 8/24/32-bit BMP")
    bytes_per_pixel = bits // 8
    row_stride = ((width * bits + 31) // 32) * 4
    pixels: list[RGB] = []
    for output_y in range(height):
        stored_y = output_y if top_down else height - 1 - output_y
        row_offset = pixel_offset + stored_y * row_stride
        for x in range(width):
            value_offset = row_offset + x * bytes_per_pixel
            if bits == 8:
                palette_index = data[value_offset]
                if palette_index >= len(palette):
                    raise ValueError(f"{path}: palette index outside table")
                pixels.append(palette[palette_index])
            else:
                blue, green, red = data[value_offset:value_offset + 3]
                pixels.append((red, green, blue))
    return Image(width, height, tuple(pixels))


def read_image(path: Path) -> Image:
    suffix = path.suffix.lower()
    if suffix == ".png":
        return read_png(path)
    if suffix == ".bmp":
        return read_bmp(path)
    raise ValueError(f"{path}: use a .png or .bmp image")


def compare_pixels(left: Image, right: Image,
                   region: tuple[int, int, int, int] | None = None,
                   tolerance: int = 0) -> dict[str, object]:
    if (left.width, left.height) != (right.width, right.height):
        raise ValueError("frames have different dimensions")
    if region is None:
        x0, y0, width, height = 0, 0, left.width, left.height
    else:
        x0, y0, width, height = region
        if x0 < 0 or y0 < 0 or width <= 0 or height <= 0 or \
                x0 + width > left.width or y0 + height > left.height:
            raise ValueError("comparison region lies outside the frames")

    compared = 0
    mismatch = 0
    total_error = 0
    max_error = 0
    bbox: list[int] | None = None
    pair_counts: dict[tuple[RGB, RGB], int] = {}
    for y in range(y0, y0 + height):
        for x in range(x0, x0 + width):
            actual = left.at(x, y)
            expected = right.at(x, y)
            deltas = tuple(abs(actual[index] - expected[index]) for index in range(3))
            pixel_error = max(deltas)
            compared += 1
            total_error += sum(deltas)
            max_error = max(max_error, pixel_error)
            if pixel_error > tolerance:
                mismatch += 1
                pair = (actual, expected)
                pair_counts[pair] = pair_counts.get(pair, 0) + 1
                if bbox is None:
                    bbox = [x, y, x, y]
                else:
                    bbox[0] = min(bbox[0], x)
                    bbox[1] = min(bbox[1], y)
                    bbox[2] = max(bbox[2], x)
                    bbox[3] = max(bbox[3], y)
    top_pairs = sorted(pair_counts.items(), key=lambda item: -item[1])[:10]
    return {
        "width": left.width,
        "height": left.height,
        "region": [x0, y0, width, height],
        "pixels_compared": compared,
        "matching_pixels": compared - mismatch,
        "mismatched_pixels": mismatch,
        "match_ratio": (compared - mismatch) / compared if compared else 1.0,
        "mean_absolute_channel_error": total_error / (compared * 3)
        if compared else 0.0,
        "max_channel_error": max_error,
        "mismatch_bbox": bbox,
        "top_mismatch_pairs": [
            {"actual": list(actual), "expected": list(expected), "count": count}
            for (actual, expected), count in top_pairs
        ],
    }


def write_diff(path: Path, actual: Image, expected: Image,
               tolerance: int = 0) -> None:
    if (actual.width, actual.height) != (expected.width, expected.height):
        raise ValueError("frames have different dimensions")
    payload = bytearray()
    for y in range(actual.height):
        for x in range(actual.width):
            left = actual.at(x, y)
            right = expected.at(x, y)
            error = max(abs(left[index] - right[index]) for index in range(3))
            if error <= tolerance:
                payload.extend((0, 0, 0))
            else:
                payload.extend((min(255, 32 + error * 4), 0, 0))
    header = f"P6\n{actual.width} {actual.height}\n255\n".encode("ascii")
    path.write_bytes(header + payload)


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("actual", type=Path,
                        help="recreation PNG/BMP")
    parser.add_argument("expected", type=Path,
                        help="DOSBox PNG/BMP")
    parser.add_argument("--region", nargs=4, type=int, metavar=("X", "Y", "W", "H"),
                        help="compare only a frame region")
    parser.add_argument("--tolerance", type=int, default=0,
                        help="ignore pixels whose largest channel error is at most N")
    parser.add_argument("--json", type=Path,
                        help="also write the report as JSON")
    parser.add_argument("--diff", type=Path,
                        help="write a red-on-black PPM difference image")
    parser.add_argument("--strict", action="store_true",
                        help="return failure when any pixel exceeds tolerance")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = _parser().parse_args(argv)
    if args.tolerance < 0:
        raise SystemExit("--tolerance must be non-negative")
    actual = read_image(args.actual)
    expected = read_image(args.expected)
    region = tuple(args.region) if args.region is not None else None
    report = compare_pixels(actual, expected, region, args.tolerance)
    if args.diff is not None:
        write_diff(args.diff, actual, expected, args.tolerance)
    rendered = json.dumps(report, indent=2, sort_keys=True)
    print(rendered)
    if args.json is not None:
        args.json.write_text(rendered + "\n", encoding="utf-8")
    if args.strict and report["mismatched_pixels"]:
        return 1
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, struct.error, zlib.error) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(2)
