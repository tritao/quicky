import struct
import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from quikytrace import (  # noqa: E402
    EntityTraceConfig,
    PlayerTraceConfig,
    TraceError,
    StateMachineTraceConfig,
    decode_lookup_call,
    decode_resource_state,
    entity_trace_lua_config,
    lua_literal,
    normalize_entity_trace,
    normalize_player_trace,
    player_trace_lua_config,
    trace_player_lua,
    trace_entity_lua,
    trace_resources_lua,
)


class QuikyTraceTests(unittest.TestCase):
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
            ),
            select_level='W4L1',
        )
        payload = entity_trace_lua_config(config)
        self.assertEqual(payload["timeout_ms"], 2500)
        self.assertEqual(payload["state_machine"]["camera_x"], 0)
        source = "TRACE_CONFIG = " + lua_literal(payload)
        self.assertIn('["state_machine"]', source)
        self.assertIn('["patch_map_run"]=true', source)
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
        self.assertEqual(player_trace_lua_config(config)["input_secondary_key"], "")
        self.assertFalse(player_trace_lua_config(config)["input_hold_until_callback"])
        self.assertEqual(player_trace_lua_config(config)["focus_callback_offset"], 0x3FF8)
        self.assertFalse(player_trace_lua_config(config)["effect_table_focus"])
        self.assertFalse(player_trace_lua_config(config)["effect_table_factory_focus"])
        self.assertFalse(player_trace_lua_config(config)["effect_table_scheduler_focus"])
        self.assertFalse(player_trace_lua_config(config)["effect_table_force_gate"])
        self.assertIsNone(player_trace_lua_config(config)["force_player_action_word"])
        self.assertFalse(player_trace_lua_config(config)["collision_focus"])
        self.assertFalse(player_trace_lua_config(config)["map_focus"])
        self.assertFalse(player_trace_lua_config(config)["property_focus"])
        self.assertIsNone(player_trace_lua_config(config)["property_helper_offset"])
        self.assertFalse(player_trace_lua_config(config)["boss_stage_focus"])
        self.assertEqual(player_trace_lua_config(config)["boss_stage_events"], 64)
        self.assertFalse(player_trace_lua_config(config)["boss_damage_focus"])
        self.assertEqual(player_trace_lua_config(config)["boss_damage_hits"], 5)
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

    def test_player_boss_stage_focus_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, boss_stage_focus=True,
            boss_stage_events=12,
        )
        payload = player_trace_lua_config(config)
        self.assertTrue(payload["boss_stage_focus"])
        self.assertEqual(payload["boss_stage_events"], 12)

    def test_player_boss_damage_focus_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, boss_stage_focus=True,
            boss_damage_focus=True, boss_damage_hits=6,
        )
        payload = player_trace_lua_config(config)
        self.assertTrue(payload["boss_damage_focus"])
        self.assertEqual(payload["boss_damage_hits"], 6)
        self.assertEqual(payload["boss_damage_target_callback"], 0xA234)
        self.assertEqual(payload["boss_damage_callback_offset"], 0xB25D)

    def test_normalize_player_boss_stage_events(self):
        trace = normalize_player_trace({
            "boss_stage_trace": {
                "events": {"2": {"event_index": 2}, "1": {"event_index": 1}},
            },
            "samples": [],
        })
        self.assertEqual(trace["boss_stage_trace"]["events"], [
            {"event_index": 1}, {"event_index": 2},
        ])

    def test_player_branch_focus_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, branch_focus=True,
        )
        self.assertTrue(player_trace_lua_config(config)["branch_focus"])

    def test_player_effect_table_focus_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, effect_table_focus=True,
        )
        self.assertTrue(player_trace_lua_config(config)["effect_table_focus"])

    def test_player_effect_table_factory_focus_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, effect_table_focus=True,
            effect_table_factory_focus=True,
        )
        self.assertTrue(player_trace_lua_config(config)["effect_table_factory_focus"])

    def test_player_effect_table_scheduler_focus_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, effect_table_focus=True,
            effect_table_scheduler_focus=True,
        )
        self.assertTrue(player_trace_lua_config(config)["effect_table_scheduler_focus"])

    def test_player_effect_table_force_gate_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, effect_table_focus=True,
            effect_table_force_gate=True,
        )
        self.assertTrue(player_trace_lua_config(config)["effect_table_force_gate"])

    def test_player_force_action_word_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, effect_table_focus=True,
            force_player_action_word=0x10,
        )
        self.assertEqual(player_trace_lua_config(config)["force_player_action_word"], 0x10)

    def test_player_branch_patch_tile_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording, branch_focus=True,
            branch_patch_tile=0x160,
        )
        self.assertEqual(
            player_trace_lua_config(config)["branch_patch_tile"], 0x160
        )

    def test_player_trace_uses_selector_safe_map_reads(self):
        script = Path(__file__).resolve().parents[1] / "automation/quiky_player_trace.lua"
        source = script.read_text(encoding="utf-8")
        self.assertIn("local function selector_word", source)
        self.assertIn("selector_word, map_selector", source)
        self.assertIn("selector_word, descriptor_selector", source)
        self.assertNotIn("mem_read_word, map_selector", source)
        self.assertNotIn("mem_read_word, descriptor_selector", source)

    def test_player_trace_contains_boss_stage_probe(self):
        script = Path(__file__).resolve().parents[1] / "automation/quiky_player_trace.lua"
        source = script.read_text(encoding="utf-8")
        self.assertIn("boss_stage_focus", source)
        self.assertIn("boss_stage_trace", source)
        self.assertIn("offset = 0xa234", source)
        self.assertIn("prepare_boss_damage_fixture", source)

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


if __name__ == "__main__":
    unittest.main()
