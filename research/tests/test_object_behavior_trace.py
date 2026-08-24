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

    def test_interaction_probe_flags_are_serialized(self):
        config = ObjectBehaviorConfig(
            record_offset=0x1606,
            entity_type=0x3D,
            samples=8,
            startup_recording=Path("startup.json"),
            trace_overlap=True,
            trace_collision=True,
            trace_platform=True,
            force_active_player_bounds=True,
            align_object_to_player=True,
            align_y_offset=-32,
            force_platform_ready=True,
            trace_bump=True,
            trace_contact=True,
            force_contact_gate=True,
            align_x_offset=-8,
        )
        payload = lua_config(config)
        self.assertTrue(payload["trace_overlap"])
        self.assertTrue(payload["trace_collision"])
        self.assertTrue(payload["trace_platform"])
        self.assertTrue(payload["force_active_player_bounds"])
        self.assertEqual(payload["align_y_offset"], -32)
        self.assertTrue(payload["force_platform_ready"])
        self.assertTrue(payload["trace_bump"])
        self.assertTrue(payload["trace_contact"])
        self.assertTrue(payload["force_contact_gate"])
        self.assertEqual(payload["align_x_offset"], -8)

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

    def test_selector_declaration_breakpoint_is_armed_before_resume(self):
        source = (Path(__file__).resolve().parents[1] / "automation" /
                  "quiky_object_behavior_trace.lua").read_text()
        self.assertIn("local first_declaration = false", source)
        self.assertIn("first_declaration = choose_level(select_level)", source)
        self.assertIn("if first_declaration then", source)
        self.assertIn("dosbox.wait_frames(1)", source)
        self.assertGreaterEqual(
            source.count("dosbox.breakpoint_set(0x01f7, 0x1e04, {once = true})"),
            2,
        )


if __name__ == "__main__":
    unittest.main()
