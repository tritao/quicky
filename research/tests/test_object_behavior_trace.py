import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
ROOT = Path(__file__).resolve().parents[2] / "research"
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

    def test_authored_stream_controls_are_serialized(self):
        config = ObjectBehaviorConfig(
            record_offset=0x1782,
            entity_type=0x79,
            samples=1,
            startup_recording=Path("startup.json"),
            initial_camera_x=576,
            initial_camera_y=192,
            prestream_input_key="KBD_right",
            prestream_input_frames=300,
        )
        payload = lua_config(config)
        self.assertEqual(payload["initial_camera_x"], 576)
        self.assertEqual(payload["initial_camera_y"], 192)
        self.assertEqual(payload["prestream_input_key"], "KBD_right")
        self.assertEqual(payload["prestream_input_frames"], 300)

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
            force_completion_wait_release=True,
            puzzle_probe_frames=24,
        )
        payload = lua_config(config)
        self.assertEqual(payload["force_tile_mask"], 0x3F)
        self.assertTrue(payload["trace_puzzle_completion"])
        self.assertTrue(payload["force_completion_outer_state"])
        self.assertTrue(payload["force_completion_wait_release"])
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

    def test_platform_compact_mode_is_serialized(self):
        config = ObjectBehaviorConfig(
            record_offset=0x1898,
            entity_type=0x3D,
            samples=60,
            startup_recording=Path("startup.json"),
            trace_platform_player=True,
            platform_trace_compact=True,
        )
        payload = lua_config(config)
        self.assertTrue(payload["trace_platform_player"])
        self.assertTrue(payload["platform_trace_compact"])

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

    def test_normalizes_nested_selector_handoff_hits(self):
        trace = normalize_behavior_trace({
            "samples": [],
            "selector_handoff_trace": {
                "selector_handoff_hits": {
                    "2": {"sequence": 2},
                    "1": {"sequence": 1},
                },
            },
        })
        self.assertEqual(
            [hit["sequence"] for hit in
             trace["selector_handoff_trace"]["selector_handoff_hits"]],
            [1, 2],
        )

    def test_selector_handoff_diagnostic_publishes_consumed_declaration(self):
        source = (ROOT / "automation" / "quiky_object_behavior_trace.lua").read_text(
            encoding="utf-8"
        )
        self.assertIn(
            "publish the trace now instead of returning to",
            source,
        )
        self.assertIn(
            "dosbox.output.behavior_trace = selector_handoff_trace",
            source,
        )

    def test_launched_trace_records_runtime_artifact_hashes(self):
        source = (Path(__file__).resolve().parents[1] / "tools" /
                  "object_behavior_trace.py").read_text(encoding="utf-8")
        self.assertIn('"runtime_artifacts": runtime_artifacts', source)
        self.assertIn('"QUIKY.EXE": sha256(runtime_dir / "QUIKY.EXE")', source)
        self.assertIn('"NESTLE.DAT": sha256(runtime_dir / "NESTLE.DAT")', source)

    def test_direct_callback_mode_and_inert_capture_are_present(self):
        source = (Path(__file__).resolve().parents[1] / "automation" /
                  "quiky_object_behavior_trace.lua").read_text()
        self.assertIn("sprite_init_offset", source)
        self.assertIn("initialized_object.update_callback", source)
        self.assertIn("if expected_type == 0 then", source)

    def test_selector_declaration_breakpoint_is_armed_before_resume(self):
        source = (Path(__file__).resolve().parents[1] / "automation" /
                  "quiky_object_behavior_trace.lua").read_text()
        self.assertIn("local launch = wait_hit(\"selector Space dispatch\")", source)
        self.assertIn("dosbox.output.checkpoints.launch = launch", source)
        self.assertIn("4B18 is the selector dispatch boundary", source)
        self.assertIn("force_level_loop_ready", source)
        self.assertIn("trace_level_loop_timer", source)
        self.assertIn("trace_selector_handoff", source)
        self.assertIn("platform_trace_compact", source)
        self.assertIn("platform-compact", source)
        self.assertIn("01f7, 0xf049", source)
        self.assertIn("local first_declaration = nil", source)
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
        self.assertIn("force_completion_wait_release", source)
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
