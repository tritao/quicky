import argparse
import struct
import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from quikytrace import (  # noqa: E402
    EntityTraceConfig,
    ExecuteWatch,
    InputPhase,
    ObjectFocus,
    MemoryPatch,
    PlayerTraceConfig,
    TraceError,
    StateMachineTraceConfig,
    decode_lookup_call,
    decode_resource_state,
    compose_player_trace_source,
    entity_trace_lua_config,
    lua_literal,
    normalize_entity_trace,
    normalize_player_trace,
    player_trace_lua_config,
    parse_memory_patch,
    parse_input_phase,
    parse_execute_watch,
    parse_object_focus,
    trace_player_lua,
    trace_entity_lua,
    trace_resources_lua,
)


class QuikyTraceTests(unittest.TestCase):
    def test_player_source_composes_shared_helpers_before_probe(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        script = Path(__file__).resolve().parents[1] / "automation/quiky_player_trace.lua"
        source = compose_player_trace_source(
            script, PlayerTraceConfig(startup_recording=recording)
        )
        self.assertLess(source.index("QUIKY_TRACE_COMMON ="),
                        source.index("local common = assert(QUIKY_TRACE_COMMON"))
        self.assertLess(source.index("QUIKY_PATCH_WATCH ="),
                        source.index("local patch_watch = assert(QUIKY_PATCH_WATCH"))
        self.assertIn('TRACE_CONFIG = ', source)

    def test_memory_patch_parser_and_serialization(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        patches = (
            parse_memory_patch("player:0x3e:u16=0x1234"),
            parse_memory_patch("selector:0x27f:0x3a:u8=255"),
        )
        self.assertEqual(patches[0], MemoryPatch("player", 0x3E, 2, 0x1234))
        self.assertEqual(patches[1], MemoryPatch("selector", 0x3A, 1, 255, 0x27F))
        payload = player_trace_lua_config(PlayerTraceConfig(
            startup_recording=recording, patches=patches,
        ))
        self.assertEqual(payload["patches"][0]["width"], 2)
        self.assertEqual(payload["patches"][1]["selector"], 0x27F)

    def test_memory_patch_parser_rejects_invalid_or_overflowing_specs(self):
        for value in ("bogus", "cs:1:u8=0", "ds:0:u8=256", "player:0x10000:u8=0"):
            with self.subTest(value=value), self.assertRaises(argparse.ArgumentTypeError):
                parse_memory_patch(value)

    def test_input_phase_parser_and_serialization(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        phases = (
            parse_input_phase("KBD_right+KBD_up:12"),
            parse_input_phase("WAIT:3"),
        )
        self.assertEqual(phases[0], InputPhase(("KBD_right", "KBD_up"), 12))
        self.assertEqual(phases[1], InputPhase((), 3))
        payload = player_trace_lua_config(PlayerTraceConfig(
            startup_recording=recording, input_phases=phases,
        ))
        self.assertEqual(payload["input_phases"], [
            {"keys": ["KBD_right", "KBD_up"], "frames": 12},
            {"keys": [], "frames": 3},
        ])

    def test_input_phase_parser_rejects_invalid_specs(self):
        for value in ("KBD_right", "KBD_right:-1", "A+B+C+D:1", ":3"):
            with self.subTest(value=value), self.assertRaises(argparse.ArgumentTypeError):
                parse_input_phase(value)

    def test_execute_watch_parser_and_serialization(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        watch = parse_execute_watch("0x1f7:0x3df2")
        self.assertEqual(watch, ExecuteWatch(0x1F7, 0x3DF2))
        payload = player_trace_lua_config(PlayerTraceConfig(
            startup_recording=recording, execute_watches=(watch,),
        ))
        self.assertEqual(payload["execute_watches"], [
            {"segment": 0x1F7, "offset": 0x3DF2},
        ])

    def test_execute_watch_parser_rejects_invalid_specs(self):
        for value in ("0x1f7", "bad:1", "0x10000:0"):
            with self.subTest(value=value), self.assertRaises(argparse.ArgumentTypeError):
                parse_execute_watch(value)

    def test_object_focus_parser_and_serialization(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        focus = parse_object_focus("0xa234:0x1234")
        self.assertEqual(focus, ObjectFocus(0xA234, 0x1234))
        payload = player_trace_lua_config(PlayerTraceConfig(
            startup_recording=recording, object_focus=focus, factory_focus=True,
        ))
        self.assertEqual(payload["object_focus"], {
            "callback_offset": 0xA234, "object_offset": 0x1234,
        })
        self.assertTrue(payload["factory_focus"])

    def test_object_focus_parser_rejects_invalid_specs(self):
        for value in ("bad", "1:2:3", "0x10000"):
            with self.subTest(value=value), self.assertRaises(argparse.ArgumentTypeError):
                parse_object_focus(value)

    def test_map_cell_patch_parser_and_serialization(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        patch = parse_memory_patch("map:3:4:u16=0x160")
        self.assertEqual(
            patch, MemoryPatch("map", 0, 2, 0x160, None, 3, 4)
        )
        payload = player_trace_lua_config(PlayerTraceConfig(
            startup_recording=recording, patches=(patch,),
        ))
        self.assertEqual(payload["patches"][0]["map_x"], 3)
        self.assertEqual(payload["patches"][0]["map_y"], 4)

    def test_player_trace_normalizes_breakpoint_owner_arrays(self):
        trace = normalize_player_trace({"samples": {"1": {
            "breakpoint_owners": {"2": "watch", "1": "callback"},
            "execute_watch": {"owners": {"1": "watch"}},
            "related_breakpoints": {"1": {
                "owners": {"1": "return"},
            }},
        }}})
        sample = trace["samples"][0]
        self.assertEqual(sample["breakpoint_owners"], ["callback", "watch"])
        self.assertEqual(sample["execute_watch"]["owners"], ["watch"])
        self.assertEqual(sample["related_breakpoints"][0]["owners"], ["return"])

    def test_player_trace_normalizes_factory_window(self):
        trace = normalize_player_trace({"samples": {"1": {
            "factory_event": {
                "entry": {"owners": {"1": "factory-entry"}},
                "tail": {"owners": {"1": "factory-tail"}},
                "before_pool": {"objects": {}, "kind_0x64": {}},
                "after_pool": {"objects": {"1": {"offset": 4}}, "kind_0x64": {}},
                "created_objects": {"1": {"offset": 4}},
            },
        }}})
        event = trace["samples"][0]["factory_event"]
        self.assertEqual(event["entry"]["owners"], ["factory-entry"])
        self.assertEqual(event["tail"]["owners"], ["factory-tail"])
        self.assertEqual(event["created_objects"], [{"offset": 4}])

    def test_decode_lookup_call(self):
        stack = struct.pack("<HHHH", 0x36D0, 0x01D7, 0x1234, 0x0237)
        self.assertEqual(decode_lookup_call(stack), {
            "return_offset": 0x36D0, "return_segment": 0x01D7,
            "path_offset": 0x1234, "path_segment": 0x0237,
        })

    def test_decode_resource_state(self):
        raw = struct.pack("<III", 0x2F55A5, 0x2F3BD3, 0x19D2)
        self.assertEqual(decode_resource_state(raw), {
            "start": 0x2F3BD3, "end": 0x2F55A5, "size": 0x19D2,
        })

    def test_decoders_reject_truncated_data(self):
        with self.assertRaises(TraceError):
            decode_lookup_call(b"short")
        with self.assertRaises(TraceError):
            decode_resource_state(bytes(11))

    def test_lua_trace_loader_returns_events_in_sequence(self):
        class FakeApi:
            loaded_source = ""

            def request(self, method, path, text_body=None):
                self.loaded_source = text_body
                return {"status": "loaded"}

            def post(self, path):
                return {"status": "started"}

            def get(self, path):
                return {
                    "state": "completed",
                    "output": {"events": {"2": {"sequence": 2}, "1": {"sequence": 1}}},
                }

        api = FakeApi()
        script = Path(__file__).resolve().parents[1] / "automation/quiky_resource_trace.lua"
        events = trace_resources_lua(api, script, 2, 1, 0.01, True, False, 30)
        self.assertEqual([event["sequence"] for event in events], [1, 2])
        self.assertIn("TRACE_COUNT=2", api.loaded_source)
        self.assertIn("TRACE_PREPARE_W1L3=true", api.loaded_source)
        self.assertIn("TRACE_NAVIGATE_W1L3=false", api.loaded_source)
        self.assertIn("TRACE_NAVIGATE_LEVEL=", api.loaded_source)
        self.assertIn('TRACE_SELECT_LEVEL=""', api.loaded_source)
        self.assertIn("TRACE_TAIL_COUNT=0", api.loaded_source)

    def test_navigation_trace_quotes_level_and_replays_startup(self):
        class FakeApi:
            loaded_source = ""
            status_calls = 0
            replayed = False

            def request(self, method, path, text_body=None):
                self.loaded_source = text_body
                return {"status": "loaded"}

            def post(self, path, body=None):
                if path == "/api/v1/input/sequence":
                    self.replayed = True
                return {"status": "started"}

            def get(self, path):
                self.status_calls += 1
                if not self.replayed:
                    return {"state": "running", "output": {"awaiting_startup_replay": True}}
                return {"state": "completed", "output": {"events": {"1": {"sequence": 1}}}}

        api = FakeApi()
        script = Path(__file__).resolve().parents[1] / "automation/quiky_resource_trace.lua"
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        events = trace_resources_lua(
            api, script, 1, 1, 0.01, False, False, 30, recording,
            "W4L1", None, 0,
        )
        self.assertEqual([event["sequence"] for event in events], [1])
        self.assertIn('TRACE_NAVIGATE_LEVEL="W4L1"', api.loaded_source)
        self.assertTrue(api.replayed)

    def test_entity_trace_config_is_nested_and_lua_safe(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = EntityTraceConfig(
            record_offset=0x1792,
            entity_type=0x1F,
            startup_recording=recording,
            timeout=2.5,
            state_machine=StateMachineTraceConfig(
                samples=3, camera_x=0, camera_y=0, position_x=0,
                position_y=0, force_emission=True, patch_map_run=True,
                force_state=3, warmup_frames=12, map_patch_y_offset=16,
            ),
            select_level='W4L1',
        )
        payload = entity_trace_lua_config(config)
        self.assertEqual(payload["timeout_ms"], 2500)
        self.assertEqual(payload["state_machine"]["camera_x"], 0)
        source = "TRACE_CONFIG = " + lua_literal(payload)
        self.assertIn('["state_machine"]', source)
        self.assertIn('["patch_map_run"]=true', source)
        self.assertIn('["force_state"]=3', source)
        self.assertIn('["warmup_frames"]=12', source)
        self.assertIn('["map_patch_y_offset"]=16', source)
        self.assertNotIn("TRACE_STATE_MACHINE_", source)

    def test_entity_trace_loader_uses_structured_config(self):
        class FakeApi:
            loaded_source = ""
            replayed = False

            def request(self, method, path, text_body=None):
                self.loaded_source = text_body
                return {"status": "loaded"}

            def post(self, path, body=None):
                if path == "/api/v1/input/sequence":
                    self.replayed = True
                return {"status": "started"}

            def get(self, path):
                if not self.replayed:
                    return {"state": "running", "output": {"awaiting_startup_replay": True}}
                return {"state": "completed", "output": {"entity": {
                    "trace_schema_version": 1,
                    "state_machine_samples": {"1": {"nested_calls": {}}},
                }}}

        api = FakeApi()
        script = Path(__file__).resolve().parents[1] / "automation/quiky_entity_trace.lua"
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        entity, screenshots = trace_entity_lua(
            api, script, EntityTraceConfig(
                record_offset=0x1792, entity_type=0x2B,
                startup_recording=recording, timeout=1, poll_interval=0.01,
            )
        )
        self.assertEqual(screenshots, [])
        self.assertEqual(entity["trace_schema_version"], 1)
        self.assertEqual(entity["state_machine_samples"], {"1": {"nested_calls": {}}})
        self.assertIn("TRACE_CONFIG = ", api.loaded_source)
        self.assertIn('["record_offset"]=6034', api.loaded_source)
        self.assertNotIn("TRACE_RECORD_OFFSET=", api.loaded_source)
        self.assertTrue(api.replayed)

    def test_entity_trace_normalization_preserves_schema(self):
        entity = normalize_entity_trace({
            "trace_schema_version": 7,
            "state_machine_samples": [],
        })
        self.assertEqual(entity["trace_schema_version"], 7)
        self.assertEqual(entity["frames"], [])

    def test_entity_source_scan_is_opt_in(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = EntityTraceConfig(
            record_offset=0x1792, entity_type=0x2B,
            startup_recording=recording, source_scan=True,
        )
        payload = entity_trace_lua_config(config)
        self.assertTrue(payload["source_scan"])

    def test_entity_movement_camera_lock_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = EntityTraceConfig(
            record_offset=0x1792, entity_type=0x2B,
            startup_recording=recording, movement_key="KBD_right",
            movement_frames=30, movement_camera_x=500, movement_camera_y=100,
        )
        payload = entity_trace_lua_config(config)
        self.assertEqual(payload["movement_camera_x"], 500)
        self.assertEqual(payload["movement_camera_y"], 100)

    def test_player_trace_loader_uses_structured_config(self):
        class FakeApi:
            loaded_source = ""
            replayed = False

            def request(self, method, path, text_body=None):
                self.loaded_source = text_body
                return {"status": "loaded"}

            def post(self, path, body=None):
                if path == "/api/v1/input/sequence":
                    self.replayed = True
                return {"status": "started"}

            def get(self, path):
                if not self.replayed:
                    return {"state": "running", "output": {"awaiting_startup_replay": True}}
                return {"state": "completed", "output": {"player_trace": {
                    "trace_schema_version": 1,
                    "samples": {"1": {
                        "pool": {"objects": {}, "kind_0x64": {}},
                        "scheduler": {"entries": {"1": {"index": 0}}},
                        "map_properties": {"1": {"helper_offset": 0x5C27}},
                        "branch_events": {"1": {"offset": 0x3D1E}},
                    }},
                    "final_pool": {"objects": {}, "kind_0x64": {}},
                }}}

        api = FakeApi()
        script = Path(__file__).resolve().parents[1] / "automation/quiky_player_trace.lua"
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, timeout=1, poll_interval=0.01,
            samples=2, frames_between=4, select_level="W1L1",
        )
        trace, screenshots = trace_player_lua(api, script, config)
        self.assertEqual(screenshots, [])
        self.assertEqual(player_trace_lua_config(config)["frames_between"], 4)
        self.assertEqual(player_trace_lua_config(config)["input_frames"], 0)
        self.assertEqual(player_trace_lua_config(config)["input_samples"], 0)
        self.assertEqual(player_trace_lua_config(config)["focus_callback_offset"], 0x3FF8)
        self.assertFalse(player_trace_lua_config(config)["capture_player_record"])
        self.assertFalse(player_trace_lua_config(config)["collision_focus"])
        self.assertFalse(player_trace_lua_config(config)["map_focus"])
        self.assertFalse(player_trace_lua_config(config)["property_focus"])
        self.assertIsNone(player_trace_lua_config(config)["property_helper_offset"])
        normalized = normalize_player_trace(trace)
        self.assertEqual(normalized["samples"], [{
            "pool": {"objects": [], "kind_0x64": []},
            "scheduler": {"entries": [{"index": 0}]},
            "map_properties": [{"helper_offset": 0x5C27}],
            "branch_events": [{"offset": 0x3D1E}],
        }])
        self.assertNotIn("related_breakpoints", normalized["samples"][0])
        self.assertIn("TRACE_CONFIG = ", api.loaded_source)
        self.assertIn('["samples"]=2', api.loaded_source)
        self.assertNotIn("TRACE_PLAYER_", api.loaded_source)
        self.assertTrue(api.replayed)

    def test_player_property_helper_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, property_focus=True,
            property_helper_offset=0x5C27,
        )
        payload = player_trace_lua_config(config)
        self.assertTrue(payload["property_focus"])
        self.assertEqual(payload["property_helper_offset"], 0x5C27)

    def test_player_full_record_capture_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            focus_callback=True,
            frames_between=1,
            capture_player_record=True,
        )
        payload = player_trace_lua_config(config)
        self.assertTrue(payload["capture_player_record"])

    def test_player_secondary_input_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            input_key="KBD_right", input_key_secondary="KBD_up",
            secondary_pulse_frames=3, secondary_start_sample=2,
            secondary_end_sample=4,
        )
        payload = player_trace_lua_config(config)
        self.assertEqual(payload["input_key_secondary"], "KBD_up")
        self.assertEqual(payload["secondary_pulse_frames"], 3)
        self.assertEqual(payload["secondary_start_sample"], 2)
        self.assertEqual(payload["secondary_end_sample"], 4)

    def test_player_input_switch_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            input_key="KBD_right", input_key_switch="KBD_left",
            input_switch_sample=4,
        )
        payload = player_trace_lua_config(config)
        self.assertEqual(payload["input_key_switch"], "KBD_left")
        self.assertEqual(payload["input_switch_sample"], 4)

    def test_player_branch_focus_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, branch_focus=True,
        )
        self.assertTrue(player_trace_lua_config(config)["branch_focus"])

    def test_player_branch_patch_tile_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, branch_focus=True,
            branch_patch_tile=0x160,
        )
        self.assertEqual(
            player_trace_lua_config(config)["branch_patch_tile"], 0x160
        )

    def test_player_collision_patch_tile_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, collision_focus=True,
            collision_patch_tile=0x2A,
            collision_patch_side="right",
        )
        self.assertEqual(
            player_trace_lua_config(config)["collision_patch_tile"], 0x2A
        )
        self.assertEqual(
            player_trace_lua_config(config)["collision_patch_side"], "right"
        )

    def test_player_branch_patch_flags_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, branch_focus=True,
            branch_patch_flags=0x20,
        )
        self.assertEqual(
            player_trace_lua_config(config)["branch_patch_flags"], 0x20
        )

    def test_player_trace_uses_selector_safe_map_reads(self):
        script = Path(__file__).resolve().parents[1] / "automation/quiky_player_trace.lua"
        source = script.read_text(encoding="utf-8")
        self.assertIn("local function selector_word", source)
        self.assertIn("selector_word, map_selector", source)
        self.assertIn("selector_word, descriptor_selector", source)
        self.assertNotIn("mem_read_word, map_selector", source)
        self.assertNotIn("mem_read_word, descriptor_selector", source)

    def test_player_collision_trace_distinguishes_near_leaf_returns(self):
        script = Path(__file__).resolve().parents[1] / "automation/quiky_player_trace.lua"
        source = script.read_text(encoding="utf-8")
        self.assertIn("local function collision_return_location", source)
        self.assertIn("hit.offset ~= 0x3a1f and hit.offset ~= 0x3df2", source)
        self.assertIn("collision_return_event.return_breakpoint", source)
        self.assertIn("collision.map_property = map_property_snapshot(hit)", source)
        host_source = (Path(__file__).resolve().parents[1] / "tools/quikytrace.py").read_text(
            encoding="utf-8"
        )
        self.assertIn("collision_patch_tile=args.player_collision_patch_tile", host_source)
        self.assertIn("collision-patch-tile requires --player-focus-callback", host_source)
        self.assertIn("collision_patch_side=args.player_collision_patch_side", host_source)

    def test_player_collision_trace_has_bounded_return_guard(self):
        script = Path(__file__).resolve().parents[1] / "automation/quiky_player_trace.lua"
        source = script.read_text(encoding="utf-8")
        self.assertIn("local function validate_return_address", source)
        self.assertIn("local function arm_validated_return", source)
        self.assertIn("collision_event_limit", source)
        self.assertIn("collision_repeat_limit", source)
        self.assertIn("sample.collision_trace_guard", source)
        self.assertIn("sample.branch_trace_guard", source)
        self.assertIn("sample.unresolved_returns", source)
        self.assertIn("while returned == nil and callback_return_valid", source)
        host_source = (Path(__file__).resolve().parents[1] / "tools/quikytrace.py").read_text(
            encoding="utf-8"
        )
        self.assertIn("--player-collision-event-limit", host_source)
        self.assertIn("--player-collision-repeat-limit", host_source)

    def test_player_record_capture_has_full_state_delta_path(self):
        script = Path(__file__).resolve().parents[1] / "automation/quiky_player_trace.lua"
        source = script.read_text(encoding="utf-8")
        self.assertIn("local player_record_size = 0x78", source)
        self.assertIn("velocity_x_fixed_signed", source)
        self.assertIn("vertical_step_signed", source)
        self.assertIn("capture_player_record", source)
        self.assertIn("sample.player_callback.writes", source)
        self.assertIn("sample.player_callback.global_writes", source)
        host_source = (Path(__file__).resolve().parents[1] / "tools/quikytrace.py").read_text(
            encoding="utf-8"
        )
        self.assertIn("--player-capture-record", host_source)
        self.assertIn("--player-input-key-2", host_source)

    def test_player_descriptor_census_config_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            descriptor_census=True,
            descriptor_count=256,
            map_width=55,
            map_height=60,
        )
        payload = player_trace_lua_config(config)
        self.assertTrue(payload["descriptor_census"])
        self.assertEqual(payload["descriptor_count"], 256)
        self.assertEqual(payload["map_width"], 55)
        self.assertEqual(payload["map_height"], 60)

    def test_normalize_player_descriptor_census_arrays(self):
        trace = normalize_player_trace({
            "descriptor_census": {
                "descriptor_table": {"entries": {"1": {"tile_id": 1}}},
                "map": {
                    "cells": {"1": {"x": 0, "y": 0}},
                    "flag_candidates": {"1": {"tile_id": 7}},
                },
            },
            "samples": [],
        })
        census = trace["descriptor_census"]
        self.assertEqual(census["descriptor_table"]["entries"][0]["tile_id"], 1)
        self.assertEqual(census["map"]["cells"][0]["x"], 0)
        self.assertEqual(census["map"]["flag_candidates"][0]["tile_id"], 7)

    def test_normalize_player_record_deltas(self):
        trace = normalize_player_trace({
            "samples": {
                "1": {
                    "player_callback": {
                        "writes": {"2": {"offset": 7}, "1": {"offset": 3}},
                        "global_writes": {"1": {"field": "camera_x"}},
                    }
                }
            }
        })
        callback = trace["samples"][0]["player_callback"]
        self.assertEqual([item["offset"] for item in callback["writes"]], [3, 7])
        self.assertEqual(callback["global_writes"][0]["field"], "camera_x")

    def test_player_spawn_probe_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, probe_spawn_emitter=True,
        )
        self.assertTrue(player_trace_lua_config(config)["probe_spawn_emitter"])

    def test_player_release_probe_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, probe_release_emitter=True,
        )
        self.assertTrue(player_trace_lua_config(config)["probe_release_emitter"])


if __name__ == "__main__":
    unittest.main()
