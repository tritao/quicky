import copy
import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from verify_w1l2_native_trace import verify  # noqa: E402


def wurm2_entity(x_fixed: int, velocity: int, timer: int) -> dict:
    return {
        "id": 93,
        "type": 2,
        "callback": {"offset": 0x6DC4},
        "x_fixed": x_fixed,
        "y_fixed": 0,
        "velocity_x_fixed": velocity,
        "velocity_y_fixed": 0,
        "enemy_phase_timer": 0,
        "enemy_timer": timer,
        "enemy_state": 0,
        "enemy_orientation": 1,
        "enemy_patrol_direction": 1,
        "enemy_transition_timer": 0,
        "enemy_phase34": 0,
        "enemy_sine_or_probe39": 0,
        "enemy_vertical_state36": 0,
        "enemy_transition_state3d": 0,
        "enemy_source_or_kind2c": -1,
        "enemy_aux3e": 0,
        "enemy_vertical_offset40": 0,
        "enemy_origin_y36": 0,
        "enemy_saved_velocity3a": 0,
        "enemy_saved_direction44": 0,
        "map_blocked": 0,
        "enemy_animation_delay": 14,
        "enemy_animation_sequence": 0x33EE,
        "bump_animation_delay20": 0,
        "bump_animation_cursor24": 0,
    }


class W1L2NativeTraceTests(unittest.TestCase):
    def test_closed_wurm2_step_and_phase_order(self):
        first = {
            "player_dependency_order": [
                {"phase": 1, "callback": {"offset": 0x6DC4}},
                {"phase": 2, "callback": {"offset": 0x3FF8}},
            ],
            "entities": [wurm2_entity(0, 0x15000, 0x14)],
        }
        second = copy.deepcopy(first)
        second["entities"][0].update({
            "x_fixed": 0x15000,
            "velocity_x_fixed": 0x15000,
            "enemy_timer": 0x13,
        })
        result = verify({"samples": [first, second]})
        self.assertEqual(result["samples_checked"], 2)
        self.assertEqual(result["wurm2_motion_steps"], 1)

    def test_positive_latch_uses_contact_response_path(self):
        first = {
            "player_dependency_order": [
                {"phase": 1, "callback": {"offset": 0x6DC4}},
                {"phase": 2, "callback": {"offset": 0x3FF8}},
            ],
            "entities": [wurm2_entity(0, 0x1000, 0x14)],
        }
        second = copy.deepcopy(first)
        second["entities"][0].update({
            "x_fixed": 0x1400,
            "velocity_x_fixed": 0x1400,
            "map_blocked": 1,
        })
        result = verify({"samples": [first, second]})
        self.assertEqual(result["wurm2_motion_steps"], 1)


if __name__ == "__main__":
    unittest.main()
