import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from player_collision_kernel import (  # noqa: E402
    DescriptorMap,
    PlayerLeafState,
    probe_side_latch,
    quadrant_hit,
    quadrant_mask,
    resolve_descriptor_response,
    side_geometry_map,
    side_probe_pair,
    snap_y_on_side_contact,
)


class PlayerCollisionKernelTests(unittest.TestCase):
    def test_quadrant_bit_order(self):
        self.assertEqual(quadrant_mask(0, 0), 0x08)       # top-left
        self.assertEqual(quadrant_mask(8, 0), 0x04)       # top-right
        self.assertEqual(quadrant_mask(0, 8), 0x01)       # bottom-left
        self.assertEqual(quadrant_mask(8, 8), 0x02)       # bottom-right
        self.assertTrue(quadrant_hit(0x08, 0, 0))
        self.assertFalse(quadrant_hit(0x08, 8, 0))

    def test_side_probe_order_and_short_circuit(self):
        left_wall = side_probe_pair(side_geometry_map(0x0F, None), 32, 8)
        self.assertFalse(left_wall.clear)
        self.assertEqual([probe.x for probe in left_wall.probes], [27])

        right_wall = side_probe_pair(side_geometry_map(None, 0x0F), 32, 8)
        self.assertFalse(right_wall.clear)
        self.assertEqual([probe.x for probe in right_wall.probes], [27, 37])

        gap = side_probe_pair(side_geometry_map(None, None), 32, 8)
        self.assertTrue(gap.clear)
        self.assertEqual([probe.x for probe in gap.probes], [27, 37])

    def test_ledge_corner_and_one_tile_gap_are_probe_patterns(self):
        # A ledge and an inside corner are not separate leaf algorithms: they
        # are the same ordered pair with one or both probes occupied.
        ledge = side_probe_pair(side_geometry_map(0x0F, None), 32, 8)
        inside_corner = side_probe_pair(side_geometry_map(0x0F, 0x0F), 32, 8)
        outside_corner = side_probe_pair(side_geometry_map(None, 0x0F), 32, 8)
        one_tile_gap = side_probe_pair(side_geometry_map(None, None), 32, 8)
        self.assertFalse(ledge.clear)
        self.assertFalse(inside_corner.clear)
        self.assertFalse(outside_corner.clear)
        self.assertTrue(one_tile_gap.clear)
        self.assertEqual(len(inside_corner.probes), 1)
        self.assertEqual(len(outside_corner.probes), 2)

    def test_3a1f_latches_only_after_two_clear_probes(self):
        state = PlayerLeafState(32, 8, side_response=1)
        result = probe_side_latch(side_geometry_map(None, None), state)
        self.assertTrue(result.clear)
        self.assertTrue(result.side_response_written)
        self.assertEqual(state.side_response, 0xFF)

        state = PlayerLeafState(32, 8, side_response=1)
        result = probe_side_latch(side_geometry_map(0x0F, None), state)
        self.assertFalse(result.clear)
        self.assertFalse(result.side_response_written)
        self.assertEqual(state.side_response, 1)

    def test_3df2_snaps_y_not_x(self):
        state = PlayerLeafState(32, 13, vertical_response=0, side_response=0xFF)
        self.assertTrue(snap_y_on_side_contact(side_geometry_map(0x0F, None), state))
        self.assertEqual(state.x, 32)
        self.assertEqual(state.y, 8)

        state = PlayerLeafState(32, 13, vertical_response=0, side_response=0xFF)
        self.assertFalse(snap_y_on_side_contact(side_geometry_map(None, None), state))
        self.assertEqual(state.y, 13)

    def test_3d02_flag_matrix(self):
        query = DescriptorMap({(2, 25): 1}, {1: 0x60})
        state = PlayerLeafState(32, 400, velocity_y=0x18000, side_response=1)
        result = resolve_descriptor_response(query, state)
        self.assertEqual(result.return_offset, 0x3DF1)
        self.assertEqual(result.al, 1)
        self.assertEqual(result.final_vertical_response, 0xFF)
        self.assertEqual(result.final_velocity_y, 0xC000)

        query = DescriptorMap({}, {})
        state = PlayerLeafState(32, 401, velocity_y=0x18000, side_response=1)
        result = resolve_descriptor_response(query, state)
        self.assertEqual(result.return_offset, 0x3D44)
        self.assertEqual(result.al, 401 & 0xFF)

        # 0x20 alone selects the response branch but lacks 0x40, so this
        # target rejects the same placement after applying the half-velocity.
        query = DescriptorMap({(0, 9): 1}, {1: 0x20})
        state = PlayerLeafState(0, 144, velocity_y=0x18000, side_response=1)
        result = resolve_descriptor_response(query, state)
        self.assertEqual(result.return_offset, 0x3DE4)
        self.assertEqual(result.al, 0)
        self.assertEqual(result.final_vertical_response, 0)
        self.assertEqual(result.final_velocity_y, 0xC000)

    def test_3d02_retry_uses_the_retried_y_for_correction(self):
        # Current cell is empty; y-8 contains a response descriptor.  The
        # accepted path keeps the retry subtraction while computing its target.
        query = DescriptorMap({(2, 24): 1}, {1: 0x20})
        state = PlayerLeafState(32, 400, velocity_y=0, side_response=1)
        result = resolve_descriptor_response(query, state)
        self.assertTrue(result.retried)
        self.assertEqual(result.return_offset, 0x3DF1)
        self.assertEqual(result.original_y, 400)
        self.assertEqual(result.final_y, 392)


if __name__ == "__main__":
    unittest.main()
