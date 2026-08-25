#!/usr/bin/env python3
"""Small executable-shaped model of the recovered player descriptor leaves.

This module intentionally models only the code that is mechanically recovered
from segment 3.  It does not invent a player rectangle, a floor mask, or an
object-platform rule.  Coordinates are the 16-bit integer words read from
object +0x04/+0x08; velocities are signed 16.16 words.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Mapping


def u16(value: int) -> int:
    return value & 0xFFFF


def s16(value: int) -> int:
    value &= 0xFFFF
    return value - 0x10000 if value & 0x8000 else value


def s32(value: int) -> int:
    value &= 0xFFFFFFFF
    return value - 0x100000000 if value & 0x80000000 else value


def arithmetic_shift_right_one(value: int) -> int:
    """The 32-bit ``shr`` plus sign-repair sequence used by 3D02."""
    return s32(s32(value) // 2)


def quadrant_mask(x: int, y: int) -> int:
    """Return the 5C27 low-nibble bit selected by AX=y and BX=x."""
    x_bit3 = bool(u16(x) & 0x08)
    y_bit3 = bool(u16(y) & 0x08)
    if y_bit3:
        return 0x02 if x_bit3 else 0x01
    return 0x04 if x_bit3 else 0x08


def quadrant_hit(descriptor: int, x: int, y: int) -> bool:
    """Model the carry/non-carry decision of 5C27."""
    descriptor &= 0xFFFF
    return bool(descriptor & 0x000F and descriptor & quadrant_mask(x, y))


@dataclass(frozen=True)
class DescriptorMap:
    """A synthetic MAP/descriptor table for reversible geometry tests."""

    cells: Mapping[tuple[int, int], int] = field(default_factory=dict)
    descriptors: Mapping[int, int] = field(default_factory=dict)

    def descriptor_at_pixel(self, x: int, y: int) -> int:
        # The real helpers receive 16-bit AX/BX words.  The test maps use
        # non-negative coordinates, but masking here preserves wraparound.
        cell = (u16(x) >> 4, u16(y) >> 4)
        tile_id = self.cells.get(cell, 0) & 0x01FF
        return self.descriptors.get(tile_id, 0) & 0xFFFF


@dataclass(frozen=True)
class Probe:
    x: int
    y: int
    descriptor: int
    mask: int
    hit: bool


@dataclass(frozen=True)
class SideProbeResult:
    """Result and observations for 3A1F/3DF2's ordered X±5 probes."""

    clear: bool
    probes: tuple[Probe, ...]
    side_response_written: bool = False


def side_probe_pair(query: DescriptorMap, x: int, y: int) -> SideProbeResult:
    """Probe x-5, then x+5, short-circuiting on the first hit."""
    probes: list[Probe] = []
    for probe_x in (u16(x - 5), u16(x + 5)):
        descriptor = query.descriptor_at_pixel(probe_x, y)
        mask = quadrant_mask(probe_x, y)
        hit = quadrant_hit(descriptor, probe_x, y)
        probes.append(Probe(probe_x, u16(y), descriptor, mask, hit))
        if hit:
            return SideProbeResult(False, tuple(probes))
    return SideProbeResult(True, tuple(probes))


@dataclass
class PlayerLeafState:
    x: int
    y: int
    velocity_y: int = 0
    mode: int = 0
    gate: int = 0
    vertical_response: int = 0
    side_response: int = 1

    def __post_init__(self) -> None:
        self.x = u16(self.x)
        self.y = u16(self.y)
        self.velocity_y = s32(self.velocity_y)
        self.mode &= 0xFF
        self.gate &= 0xFF
        self.vertical_response &= 0xFF
        self.side_response &= 0xFF


def probe_side_latch(query: DescriptorMap, state: PlayerLeafState) -> SideProbeResult:
    """Model 3A1F's gates and its only persistent write (+0x3B = FF)."""
    if state.gate != 0 or state.mode == 0xFF:
        return SideProbeResult(False, tuple())
    result = side_probe_pair(query, state.x, state.y)
    if result.clear:
        state.side_response = 0xFF
        return SideProbeResult(True, result.probes, True)
    return result


def snap_y_on_side_contact(query: DescriptorMap,
                           state: PlayerLeafState) -> bool:
    """Model 3DF2; returns whether the integer Y word was written."""
    if state.side_response == 0 or state.vertical_response != 0:
        return False
    result = side_probe_pair(query, state.x, state.y)
    if result.clear:
        return False
    state.y = u16(state.y & 0xFFF8)
    return True


@dataclass(frozen=True)
class DescriptorResponseResult:
    return_offset: int
    al: int | None
    descriptor: int | None
    probe_y: int
    retried: bool
    original_y: int
    final_y: int
    original_velocity_y: int
    final_velocity_y: int
    final_vertical_response: int


def resolve_descriptor_response(query: DescriptorMap,
                                state: PlayerLeafState) -> DescriptorResponseResult:
    """Model 3D02, including its unusual retry and rejection side effects."""
    original_y = state.y
    original_velocity = state.velocity_y
    if state.side_response == 0:
        return DescriptorResponseResult(
            0x3DF1, None, None, state.y, False, original_y, state.y,
            original_velocity, state.velocity_y, state.vertical_response,
        )

    state.vertical_response = 0
    descriptor = query.descriptor_at_pixel(state.x, state.y)
    retried = False
    if descriptor & 0x0030 == 0:
        state.y = u16(state.y - 8)
        retried = True
        descriptor = query.descriptor_at_pixel(state.x, state.y)
        if descriptor & 0x0030 == 0:
            state.y = u16(state.y + 8)
            return DescriptorResponseResult(
                0x3D44, original_y & 0xFF, descriptor, state.y, retried, original_y,
                state.y, original_velocity, state.velocity_y,
                state.vertical_response,
            )

    if descriptor & 0x0020:
        state.velocity_y = arithmetic_shift_right_one(state.velocity_y)
        state.vertical_response = 0xFF
        phase = state.x & 0x000F
    else:
        state.velocity_y = arithmetic_shift_right_one(-state.velocity_y)
        state.vertical_response = 1
        phase = 0x000F - (state.x & 0x000F)

    target_y = (state.y & 0xFFF0) + (phase >> 1)
    if descriptor & 0x0040 == 0:
        target_y += 8
    target_y = u16(target_y)

    if s16(original_y) < s16(target_y):
        state.vertical_response = 0
        # If the y-8 retry found the descriptor, the executable leaves that
        # retry subtraction in the object even on this rejection path.
        return DescriptorResponseResult(
            0x3DE4, 0, descriptor, state.y, retried, original_y, state.y,
            original_velocity, state.velocity_y, state.vertical_response,
        )

    if s16(original_y) != s16(target_y):
        state.y = target_y
    return DescriptorResponseResult(
        0x3DF1, 1, descriptor, state.y, retried, original_y, state.y,
        original_velocity, state.velocity_y, state.vertical_response,
    )


def side_geometry_map(left: int | None, right: int | None) -> DescriptorMap:
    """Build a two-probe map at player x=32,y=8 for matrix tests.

    The x-5 probe is in cell 1 and x+5 is in cell 2.  ``None`` means empty;
    an integer is the descriptor word for that cell.
    """
    cells: dict[tuple[int, int], int] = {}
    descriptors: dict[int, int] = {}
    for tile_id, descriptor in enumerate((left, right), start=1):
        if descriptor is not None:
            cells[(tile_id, 0)] = tile_id
            descriptors[tile_id] = descriptor
    return DescriptorMap(cells, descriptors)


__all__ = [
    "DescriptorMap", "DescriptorResponseResult", "PlayerLeafState", "Probe",
    "SideProbeResult", "arithmetic_shift_right_one", "quadrant_hit",
    "quadrant_mask", "probe_side_latch", "resolve_descriptor_response",
    "side_geometry_map", "side_probe_pair", "snap_y_on_side_contact",
]
