import struct
import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from quikytrace import (  # noqa: E402
    EntityTraceConfig,
    GlobalWordPatch,
    InputPhase,
    MapCellWordPatch,
    ObjectFocusConfig,
    ObjectWordPatch,
    PlayerTraceConfig,
    SelectorWordPatch,
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

    def test_player_input_phases_are_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            input_phases=(InputPhase("KBD_left", 30),
                          InputPhase("KBD_leftalt", 1, "KBD_space", "KBD_left")),
        )
        self.assertEqual(player_trace_lua_config(config)["input_phases"], [
            {"key": "KBD_left", "key_2": "", "key_3": "", "frames": 30},
            {"key": "KBD_leftalt", "key_2": "KBD_space",
             "key_3": "KBD_left", "frames": 1},
        ])

    def test_player_factory_focus_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            factory_focus=True,
            factory_start_sample=12,
        )
        self.assertTrue(player_trace_lua_config(config)["factory_focus"])
        self.assertEqual(player_trace_lua_config(config)["factory_start_sample"], 12)

    def test_player_pool_offsets_are_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            pool_offsets=(0, 0x168),
        )
        self.assertEqual(player_trace_lua_config(config)["pool_offsets"], [0, 0x168])

    def test_player_callback_return_following_can_be_disabled(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            focus_callback=True,
            focus_callback_offset=0xA101,
            callback_follow_return=False,
        )
        payload = player_trace_lua_config(config)
        self.assertEqual(payload["focus_callback_offset"], 0xA101)
        self.assertFalse(payload["callback_follow_return"])

    def test_player_callback_can_start_after_input_samples(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            focus_callback=True,
            focus_callback_offset=0x45AB,
            callback_start_sample=4,
        )
        payload = player_trace_lua_config(config)
        self.assertEqual(payload["callback_start_sample"], 4)

    def test_player_object_watch_offsets_are_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            focus_object=ObjectFocusConfig(
                sprite_slot_min=0, sprite_slot_max=0xffff,
                object_offset=0, callback_offset=0x3ff8,
            ),
            watch_offsets=(0x19A3, 0x1A3D),
            watch_segments=(0x01F7,),
            watch_start_sample=19,
        )
        self.assertEqual(player_trace_lua_config(config)["watch_offsets"],
                         [0x19A3, 0x1A3D])
        self.assertEqual(player_trace_lua_config(config)["watch_segments"],
                         [0x01F7])
        self.assertEqual(player_trace_lua_config(config)["watch_start_sample"], 19)

    def test_player_stop_on_watch_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            watch_offsets=(0x4EA0,),
            watch_segments=(0x01D7,),
            stop_on_watch=True,
        )
        self.assertTrue(player_trace_lua_config(config)["stop_on_watch"])

    def test_player_stop_watch_offsets_are_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            watch_offsets=(0x171F, 0x1724),
            watch_segments=(0x01D7,),
            stop_on_watch=True,
            stop_watch_offsets=(0x1724,),
        )
        self.assertEqual(
            player_trace_lua_config(config)["stop_watch_offsets"], [0x1724]
        )

    def test_player_exact_watch_targets_are_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            watch_targets=((0x01E7, 0x0CD3), (0x01D7, 0x1724)),
            stop_on_watch=True,
            stop_watch_offsets=(0x1724,),
        )
        self.assertEqual(
            player_trace_lua_config(config)["watch_targets"],
            [{"segment": 0x01E7, "offset": 0x0CD3},
             {"segment": 0x01D7, "offset": 0x1724}],
        )

    def test_player_exact_watch_targets_can_be_staged(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            watch_targets=((0x01D7, 0x4F0D, 4), (0x01E7, 0x0CAA, 6)),
        )
        self.assertEqual(
            player_trace_lua_config(config)["watch_targets"],
            [{"segment": 0x01D7, "offset": 0x4F0D, "start_sample": 4},
             {"segment": 0x01E7, "offset": 0x0CAA, "start_sample": 6}],
        )

    def test_player_selector_word_patches_are_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            selector_word_patches=(SelectorWordPatch(
                selector=0xFFFF, address=0x2FEB, value=0,
                start_sample=4, end_sample=8,
            ),),
        )
        self.assertEqual(
            player_trace_lua_config(config)["selector_word_patches"],
            [{"selector": 0xFFFF, "address": 0x2FEB, "value": 0,
              "start_sample": 4, "end_sample": 8}],
        )

    def test_player_watch_only_start_sample_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            watch_offsets=(0x4FAF,),
            watch_segments=(0x01D7,),
            watch_only_start_sample=8,
        )
        self.assertEqual(player_trace_lua_config(config)["watch_only_start_sample"], 8)

    def test_player_teleport_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            player_teleport_x=1600,
            player_teleport_y=500,
        )
        payload = player_trace_lua_config(config)
        self.assertEqual(payload["player_teleport_x"], 1600)
        self.assertEqual(payload["player_teleport_y"], 500)

    def test_player_teleport_start_sample_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            player_teleport_x=251,
            player_teleport_y=640,
            player_teleport_start_sample=6,
        )
        self.assertEqual(
            player_trace_lua_config(config)["player_teleport_start_sample"], 6
        )

    def test_player_teleport_persist_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            player_teleport_x=1000,
            player_teleport_y=650,
            player_teleport_persist=True,
        )
        self.assertTrue(player_trace_lua_config(config)["player_teleport_persist"])

    def test_player_patch_every_frame_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            patch_every_frame=True,
        )
        self.assertTrue(player_trace_lua_config(config)["patch_every_frame"])

    def test_player_death_bypass_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            player_death_bypass=True,
        )
        self.assertTrue(player_trace_lua_config(config)["player_death_bypass"])

    def test_player_death_bypass_start_sample_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            player_death_bypass=True,
            player_death_bypass_start_sample=41,
        )
        self.assertEqual(
            player_trace_lua_config(config)["player_death_bypass_start_sample"], 41
        )

    def test_player_global_word_patch_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            global_word_patches=(GlobalWordPatch(0x89EA, 0, 26),),
        )
        self.assertEqual(
            player_trace_lua_config(config)["global_word_patches"],
            [{"address": 0x89EA, "value": 0, "start_sample": 26}],
        )

    def test_player_map_cell_word_patch_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            map_cell_word_patches=(MapCellWordPatch(23, 36, 0x1001),),
        )
        self.assertEqual(
            player_trace_lua_config(config)["map_cell_word_patches"],
            [{"cell_x": 23, "cell_y": 36, "value": 0x1001,
              "start_sample": 1}],
        )

    def test_player_global_word_patch_range_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            global_word_patches=(GlobalWordPatch(0x88AE, 2, 2, 2),),
        )
        self.assertEqual(
            player_trace_lua_config(config)["global_word_patches"],
            [{"address": 0x88AE, "value": 2,
              "start_sample": 2, "end_sample": 2}],
        )

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
        self.assertEqual(player_trace_lua_config(config)["input_key_2"], "")
        self.assertEqual(player_trace_lua_config(config)["input_phases"], [])
        self.assertEqual(player_trace_lua_config(config)["focus_callback_offset"], 0x3FF8)
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

    def test_player_object_focus_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            focus_object=ObjectFocusConfig(
                sprite_slot_min=900, sprite_slot_max=999,
                object_offset=0x78, callback_offset=0xA234,
            ),
        )
        payload = player_trace_lua_config(config)
        self.assertEqual(payload["focus_object"], {
            "sprite_slot_min": 900,
            "sprite_slot_max": 999,
            "object_offset": 0x78,
            "callback_offset": 0xA234,
        })

    def test_player_multiple_object_focus_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            focus_objects=(
                ObjectFocusConfig(object_offset=0x78, callback_offset=0xB33B),
                ObjectFocusConfig(object_offset=0x1E0, callback_offset=0x45AB),
            ),
        )
        self.assertEqual(player_trace_lua_config(config)["focus_objects"], [
            {
                "sprite_slot_min": 900,
                "sprite_slot_max": 999,
                "object_offset": 0x78,
                "callback_offset": 0xB33B,
            },
            {
                "sprite_slot_min": 900,
                "sprite_slot_max": 999,
                "object_offset": 0x1E0,
                "callback_offset": 0x45AB,
            },
        ])

    def test_player_object_word_patch_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            object_word_patches=(ObjectWordPatch(0x168, 0x2C, 4, 4),),
        )
        self.assertEqual(player_trace_lua_config(config)["object_word_patches"], [{
            "object_offset": 0x168,
            "field_offset": 0x2C,
            "value": 4,
            "start_sample": 4,
        }])

    def test_player_object_word_patch_range_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            object_word_patches=(ObjectWordPatch(0x78, 0x38, 40, 2, 2),),
        )
        self.assertEqual(player_trace_lua_config(config)["object_word_patches"], [{
            "object_offset": 0x78, "field_offset": 0x38,
            "value": 40, "start_sample": 2, "end_sample": 2,
        }])

    def test_player_callback_word_patch_is_serialized(self):
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        config = PlayerTraceConfig(
            startup_recording=recording,
            object_word_patches=(
                ObjectWordPatch(0, 0x2A, 1, 4, 10, callback_offset=0x489C),
            ),
        )
        self.assertEqual(
            player_trace_lua_config(config)["object_word_patches"], [{
                "object_offset": 0, "field_offset": 0x2A, "value": 1,
                "start_sample": 4, "end_sample": 10,
                "callback_offset": 0x489C,
            }])

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
        )
        self.assertEqual(
            player_trace_lua_config(config)["collision_patch_tile"], 0x2A
        )

    def test_player_trace_uses_selector_safe_map_reads(self):
        script = Path(__file__).resolve().parents[1] / "automation/quiky_player_trace.lua"
        source = script.read_text(encoding="utf-8")
        self.assertIn("local function selector_word", source)
        self.assertIn("selector_word, map_selector", source)
        self.assertIn("selector_word, descriptor_selector", source)
        self.assertNotIn("mem_read_word, map_selector", source)
        self.assertNotIn("mem_read_word, descriptor_selector", source)

    def test_player_trace_supports_staged_watches_and_experiment_segments(self):
        script = Path(__file__).resolve().parents[1] / "automation/quiky_player_trace.lua"
        source = script.read_text(encoding="utf-8")
        self.assertIn("target.start_sample", source)
        self.assertIn("watch_target_consumed", source)
        self.assertIn("local function experiment_segment_read", source)
        self.assertIn('dosbox.mem_read(selector, address, size)', source)
        self.assertIn("skipped_for_transition_watch", source)
        self.assertIn("stopped_during_callback_watch", source)

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
