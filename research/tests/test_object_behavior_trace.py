import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from object_behavior_trace import (  # noqa: E402
    ObjectBehaviorConfig,
    lua_config,
    normalize_behavior_trace,
)


class ObjectBehaviorTraceTests(unittest.TestCase):
    def test_config_isolated_from_player_options(self):
        config = ObjectBehaviorConfig(
            record_offset=0x177A,
            entity_type=0x01,
            samples=4,
            startup_recording=Path("startup.json"),
            camera_x=700,
            camera_y=150,
            reactivate_camera_x=816,
            reactivate_camera_y=272,
            capture_pool=False,
            helper_trace=True,
            probe_position_x=760,
            probe_position_y=256,
            probe_proximity_state=0x32,
            probe_bounds_byte_37=1,
            probe_descriptor_delay=10,
            probe_descriptor_timer=0,
            probe_descriptor_table=0x3506,
            probe_descriptor_cursor=0x3508,
            probe_descriptor_mode=0xff,
            probe_type33_direction=1,
            probe_type33_phase=-1,
            probe_type33_phase_timer=2,
            probe_type33_transition=0,
            probe_type33_state=0,
            probe_type33_state_counter=0,
            probe_type33_velocity=0,
            probe_type33_travel_counter=0,
            probe_type33_animation_counter=0,
            probe_type33_target_x=740,
            probe_type33_target_y=380,
            probe_type33_target_capacity=1,
            probe_type33_target_cursor=0,
            movement_key="KBD_right",
        )
        payload = lua_config(config)
        self.assertEqual(payload["record_offset"], 0x177A)
        self.assertEqual(payload["entity_type"], 0x01)
        self.assertEqual(payload["camera_x"], 700)
        self.assertEqual(payload["camera_y"], 150)
        self.assertEqual(payload["reactivate_camera_x"], 816)
        self.assertEqual(payload["reactivate_camera_y"], 272)
        self.assertFalse(payload["capture_pool"])
        self.assertTrue(payload["helper_trace"])
        self.assertEqual(payload["probe_position_x"], 760)
        self.assertEqual(payload["probe_position_y"], 256)
        self.assertEqual(payload["probe_proximity_state"], 0x32)
        self.assertEqual(payload["probe_bounds_byte_37"], 1)
        self.assertEqual(payload["probe_descriptor_delay"], 10)
        self.assertEqual(payload["probe_descriptor_timer"], 0)
        self.assertEqual(payload["probe_descriptor_table"], 0x3506)
        self.assertEqual(payload["probe_descriptor_cursor"], 0x3508)
        self.assertEqual(payload["probe_descriptor_mode"], 0xff)
        self.assertEqual(payload["probe_type33_direction"], 1)
        self.assertEqual(payload["probe_type33_phase"], -1)
        self.assertEqual(payload["probe_type33_velocity"], 0)
        self.assertEqual(payload["probe_type33_target_x"], 740)
        self.assertEqual(payload["probe_type33_target_y"], 380)
        self.assertEqual(payload["probe_type33_target_capacity"], 1)
        self.assertEqual(payload["movement_key"], "KBD_right")
        self.assertNotIn("player_callback_offset", payload)

    def test_normalizes_lua_numeric_tables(self):
        trace = normalize_behavior_trace({
            "samples": {
                "2": {"sequence": 2, "changed_bytes": {
                    "2": {"offset": 4}, "1": {"offset": 1},
                }, "callback": {
                    "related_hits": {"2": {"offset": 0x1dee}},
                    "helper_calls": {"1": {"offset": 0x5d38}},
                }},
                "1": {"sequence": 1, "changed_bytes": {}},
            },
        })
        self.assertEqual([sample["sequence"] for sample in trace["samples"]], [1, 2])
        self.assertEqual(trace["samples"][1]["changed_bytes"][0]["offset"], 1)
        self.assertEqual(trace["samples"][1]["changed_bytes"][1]["offset"], 4)
        self.assertEqual(trace["samples"][1]["callback"]["related_hits"][0]["offset"], 0x1dee)
        self.assertEqual(trace["samples"][1]["callback"]["helper_calls"][0]["offset"], 0x5d38)

    def test_normalizes_descriptor_sequence_words(self):
        trace = normalize_behavior_trace({
            "samples": [{
                "object_before": {"descriptor": {
                    "sequence_words": {"2": 0xfffc, "1": 0x00d6},
                }},
                "object_after": {"descriptor": {
                    "sequence_words": {"1": 0x00d6},
                }},
                "changed_bytes": {},
            }],
        })
        self.assertEqual(
            trace["samples"][0]["object_before"]["descriptor"]["sequence_words"],
            [0x00d6, 0xfffc],
        )

    def test_normalizes_followup_pool_tables(self):
        trace = normalize_behavior_trace({
            "samples": [{
                "changed_bytes": {},
                "pool_before": {"banks": {
                    "1": {"entries": {"1": {"object_offset": 120}}},
                }},
            }],
            "followup_passes": {
                "1": {
                    "entries": {"1": {"offset": 0x0eee}},
                    "pool": {"banks": {
                        "1": {"entries": {"1": {"object_offset": 120}}},
                    }},
                    "end_pool": {"banks": {}},
                },
            },
        })
        self.assertEqual(len(trace["followup_passes"]), 1)
        followup = trace["followup_passes"][0]
        self.assertEqual(followup["entries"][0]["offset"], 0x0eee)
        self.assertEqual(
            followup["pool"]["banks"][0]["entries"][0]["object_offset"],
            120,
        )
        self.assertEqual(
            trace["samples"][0]["pool_before"]["banks"][0]["entries"][0]["object_offset"],
            120,
        )

    def test_normalizes_reactivation_tables(self):
        trace = normalize_behavior_trace({
            "samples": [],
            "reactivation": {
                "stream_entries": {"1": {"offset": 0x1cda}},
                "initialized_pool": {"banks": {
                    "1": {"entries": {"1": {"object_offset": 120}}},
                }},
            },
        })
        self.assertEqual(trace["reactivation"]["stream_entries"][0]["offset"], 0x1cda)
        self.assertEqual(
            trace["reactivation"]["initialized_pool"]["banks"][0]["entries"][0]["object_offset"],
            120,
        )


if __name__ == "__main__":
    unittest.main()
