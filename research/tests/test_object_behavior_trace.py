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
        self.assertEqual(payload["movement_key"], "KBD_right")
        self.assertNotIn("player_callback_offset", payload)

    def test_normalizes_lua_numeric_tables(self):
        trace = normalize_behavior_trace({
            "samples": {
                "2": {"sequence": 2, "changed_bytes": {
                    "2": {"offset": 4}, "1": {"offset": 1},
                }},
                "1": {"sequence": 1, "changed_bytes": {}},
            },
        })
        self.assertEqual([sample["sequence"] for sample in trace["samples"]], [1, 2])
        self.assertEqual(trace["samples"][1]["changed_bytes"][0]["offset"], 1)
        self.assertEqual(trace["samples"][1]["changed_bytes"][1]["offset"], 4)

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
