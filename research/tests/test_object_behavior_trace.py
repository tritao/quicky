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

    def test_reload_probe_options_are_serialized(self):
        config = ObjectBehaviorConfig(
            record_offset=0x1838,
            entity_type=0x6F,
            samples=1,
            startup_recording=Path("startup.json"),
            select_level="W1L1",
            reload_after_collect=True,
            reload_level="W1L1",
            reload_wait_frames=12,
        )
        payload = lua_config(config)
        self.assertTrue(payload["reload_after_collect"])
        self.assertEqual(payload["reload_level"], "W1L1")
        self.assertEqual(payload["reload_wait_frames"], 12)

    def test_puzzle_completion_probe_options_are_serialized(self):
        config = ObjectBehaviorConfig(
            record_offset=0x17F2,
            entity_type=0x7F,
            samples=2,
            startup_recording=Path("startup.json"),
            force_tile_mask=0x3F,
            trace_puzzle_completion=True,
            force_completion_outer_state=True,
            puzzle_probe_frames=24,
        )
        payload = lua_config(config)
        self.assertEqual(payload["force_tile_mask"], 0x3F)
        self.assertTrue(payload["trace_puzzle_completion"])
        self.assertTrue(payload["force_completion_outer_state"])
        self.assertEqual(payload["puzzle_probe_frames"], 24)

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
            force_bump_player_state=True,
            force_cloud_player_state=True,
            trace_cloud_consumers=True,
            cloud_consumer_offset=0x4087,
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
        self.assertTrue(payload["force_bump_player_state"])
        self.assertTrue(payload["force_cloud_player_state"])
        self.assertTrue(payload["trace_cloud_consumers"])
        self.assertEqual(payload["cloud_consumer_offset"], 0x4087)
        self.assertEqual(payload["align_y_offset"], -32)
        self.assertTrue(payload["force_platform_ready"])
        self.assertTrue(payload["trace_bump"])
        self.assertTrue(payload["trace_contact"])
        self.assertTrue(payload["force_contact_gate"])
        self.assertEqual(payload["align_x_offset"], -8)

    def test_effect_table_probe_options_are_serialized(self):
        config = ObjectBehaviorConfig(
            record_offset=0x1838,
            entity_type=0x6F,
            samples=1,
            startup_recording=Path("startup.json"),
            trace_effect_table=True,
            effect_table_attempts=32,
        )
        payload = lua_config(config)
        self.assertTrue(payload["trace_effect_table"])
        self.assertEqual(payload["effect_table_attempts"], 32)

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

    def test_normalizes_effect_table_events(self):
        trace = normalize_behavior_trace({
            "samples": [],
            "effect_table_probe": {
                "events": {
                    "2": {"sequence": 2},
                    "1": {"sequence": 1},
                },
            },
        })
        self.assertEqual(
            [event["sequence"] for event in trace["effect_table_probe"]["events"]],
            [1, 2],
        )

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

    def test_reload_probe_has_reconstruction_capture(self):
        source = (Path(__file__).resolve().parents[1] / "automation" /
                  "quiky_object_behavior_trace.lua").read_text()
        self.assertIn("reload_after_collect", source)
        self.assertIn("reload object factory return", source)
        self.assertIn("reconstructed = false", source)

    def test_puzzle_probe_captures_post_collection_state(self):
        source = (Path(__file__).resolve().parents[1] / "automation" /
                  "quiky_object_behavior_trace.lua").read_text()
        self.assertIn("force_tile_mask", source)
        self.assertIn("puzzle_completion_probe", source)
        self.assertIn("puzzle_probe_frames", source)
        self.assertIn("force_completion_outer_state", source)
        self.assertIn("0x10cb", source)
        self.assertIn("stage_writer_points", source)
        self.assertIn("stage_gate_hit_count", source)

    def test_cloud_probe_has_player_state_control(self):
        source = (Path(__file__).resolve().parents[1] / "automation" /
                  "quiky_object_behavior_trace.lua").read_text()
        self.assertIn("force_cloud_player_state", source)
        self.assertIn("apply_cloud_player_state", source)

    def test_cloud_consumer_probe_has_reader_barrier(self):
        source = (Path(__file__).resolve().parents[1] / "automation" /
                  "quiky_object_behavior_trace.lua").read_text()
        self.assertIn("cloud_consumer_probe", source)
        self.assertIn("0x4087", source)
        self.assertIn("0x4406", source)
        self.assertIn("cloud_consumer_offset", source)

    def test_cloud_outer_renderer_probe_is_serialized(self):
        config = ObjectBehaviorConfig(
            record_offset=0x180A,
            entity_type=0x28,
            samples=2,
            startup_recording=Path("startup.json"),
            trace_cloud_outer_renderer=True,
        )
        payload = lua_config(config)
        self.assertTrue(payload["trace_cloud_outer_renderer"])
        source = (Path(__file__).resolve().parents[1] / "automation" /
                  "quiky_object_behavior_trace.lua").read_text()
        self.assertIn("cloud_outer_renderer_probe", source)
        self.assertIn("01D7:4EA0", source)


if __name__ == "__main__":
    unittest.main()
