import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from object_behavior_compare import compare_payload  # noqa: E402


def descriptor_sample(sequence, before, after, sprite_before=0xD6,
                      sprite_after=0xD6):
    return {
        "sequence": sequence,
        "callback": {"offset": 0x882F, "related_hits": [], "helper_calls": []},
        "object_before": {
            "sprite_slot": sprite_before, "descriptor": before,
            "update_callback": 0x882F,
        },
        "object_after": {
            "sprite_slot": sprite_after, "descriptor": after,
            "update_callback": 0x882F,
        },
        "termination": {"visibility_gate_hit": False},
    }


class ObjectBehaviorCompareTests(unittest.TestCase):
    def test_descriptor_timer_contract(self):
        descriptor = {
            "mode": 1, "reload_delay": 3, "timer": 3,
            "sequence_base": 0x1002, "sequence_cursor": 0x1002,
            "sequence_words": [0x10, 0x11, 0xFFFE, 0x12],
        }
        after = dict(descriptor, timer=2)
        result = compare_payload({
            "config": {"entity_type": 0x33},
            "samples": [descriptor_sample(1, descriptor, after)],
        })
        self.assertTrue(result["passed"], result)
        self.assertEqual(result["descriptor"]["samples_checked"], 1)

    def test_descriptor_relative_loop_contract(self):
        before = {
            "mode": 1, "reload_delay": 3, "timer": 0,
            "sequence_base": 0x1002, "sequence_cursor": 0x1004,
            "sequence_words": [0x10, 0x11, 0xFFFE, 0x12],
        }
        after = dict(before, timer=3, sequence_cursor=0x1002)
        result = compare_payload({
            "config": {"entity_type": 0x33},
            "samples": [descriptor_sample(1, before, after,
                                            sprite_before=0x11,
                                            sprite_after=0x10)],
        })
        self.assertTrue(result["passed"], result)

    def test_type34_active_gate_and_action_chain(self):
        sample = {
            "sequence": 1,
            "callback": {
                "offset": 0x9C0C,
                "related_hits": [{"offset": 0x9C29}],
                "helper_calls": [{"offset": 0x1B5D}, {"offset": 0x0FCF}],
            },
            "object_before": {"position": {"x": 128, "y": 404}},
            "object_after": {"position": {"x": 128, "y": 404}},
            "bounds_object_before": {"position": {"x": 128, "y": 400}},
            "globals_before": {"proximity_gate": 0x31, "action_word": 0},
            "globals_after": {"proximity_gate": 0x31, "action_word": 4},
            "termination": {"visibility_gate_hit": False},
        }
        result = compare_payload({
            "config": {"entity_type": 0x34, "probe_position_x": 128,
                       "probe_bounds_byte_37": 1},
            "samples": [sample],
        })
        self.assertTrue(result["passed"], result)

    def test_type34_inactive_gate_does_not_change_action(self):
        sample = {
            "sequence": 1,
            "callback": {"offset": 0x9C0C, "related_hits": [], "helper_calls": []},
            "object_before": {"position": {"x": 128, "y": 404}},
            "object_after": {"position": {"x": 128, "y": 404}},
            "globals_before": {"proximity_gate": 0x32, "action_word": 0},
            "globals_after": {"proximity_gate": 0x32, "action_word": 0},
            "termination": {"visibility_gate_hit": False},
        }
        result = compare_payload({
            "config": {"entity_type": 0x34, "probe_position_x": 128,
                       "probe_bounds_byte_37": 1},
            "samples": [sample],
        })
        self.assertTrue(result["passed"], result)


if __name__ == "__main__":
    unittest.main()
