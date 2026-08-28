import json
import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from derive_w1l1_leaf_ring import LeafRingError, derive_ring  # noqa: E402


class W1L1LeafRingTests(unittest.TestCase):
    def test_derives_startup_prefix_in_pool_order(self):
        payload = {
            "events": [{"samples": [{"pool": {"objects": [
                {"callback": 0x47E7, "position": {"x": 272},
                 "velocity_y_fixed": 90836, "sprite_slot": 703},
                {"callback": 0x47E7, "position": {"x": 288},
                 "velocity_y_fixed": 69204, "sprite_slot": 703},
                {"callback": 0x47E7, "position": {"x": 224},
                 "velocity_y_fixed": 90580, "sprite_slot": 700},
            ]}}]}]
        }
        ring, rows = derive_ring(payload)
        self.assertEqual(ring[:12], bytes.fromhex(
            "00000000000000980041019a"))
        self.assertEqual([row["x"] for row in rows], [272, 288, 224])
        self.assertEqual(len(ring), 0x100)

    def test_rejects_non_invertible_velocity(self):
        payload = {"samples": [{"pool": {"objects": [
            {"callback": 0x47E7, "position": {"x": 1},
             "velocity_y_fixed": 1, "sprite_slot": 703},
        ]}}]}
        with self.assertRaises(LeafRingError):
            derive_ring(payload)


if __name__ == "__main__":
    unittest.main()
