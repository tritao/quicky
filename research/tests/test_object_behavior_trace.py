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
            sprite_init_offset=0x6DB0,
        )
        payload = lua_config(config)
        self.assertEqual(payload["record_offset"], 0x177A)
        self.assertEqual(payload["entity_type"], 0x01)
        self.assertEqual(payload["camera_x"], 700)
        self.assertEqual(payload["camera_y"], 150)
        self.assertEqual(payload["sprite_init_offset"], 0x6DB0)
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

    def test_direct_callback_mode_and_inert_capture_are_present(self):
        source = (Path(__file__).resolve().parents[1] / "automation" /
                  "quiky_object_behavior_trace.lua").read_text()
        self.assertIn("sprite_init_offset", source)
        self.assertIn("initialized_object.update_callback", source)
        self.assertIn("if expected_type == 0 then", source)


if __name__ == "__main__":
    unittest.main()
