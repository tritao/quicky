import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from high_effect_trace import (  # noqa: E402
    HighEffectConfig,
    lua_config,
    normalize_high_effect_trace,
)


class HighEffectTraceTests(unittest.TestCase):
    def test_config_serializes_probe_without_player_options(self):
        config = HighEffectConfig(
            frames=8,
            startup_recording=Path("startup.json"),
            frame_step=30,
            select_level="W5L3",
            input_key="KBD_right",
            input_frames=12,
            input_samples=4,
            target_y_delta=-10,
            target_cursor_offset=0x2A,
            force_object_x=250,
            force_object_y=500,
            stop_at_cursor=0x14,
            trace_render=True,
            render_trace_hits=12,
            capture_scheduled_pool=False,
            owner_probe_callback=0xb226,
            owner_probe_x=353,
            owner_probe_y=530,
            owner_probe_phase=4,
            owner_probe_timer=0xdc,
            owner_probe_transition=1,
            owner_probe_once=True,
        )
        payload = lua_config(config)
        self.assertEqual(payload["frames"], 8)
        self.assertEqual(payload["frame_step"], 30)
        self.assertEqual(payload["select_level"], "W5L3")
        self.assertEqual(payload["input_key"], "KBD_right")
        self.assertEqual(payload["target_cursor_offset"], 0x2A)
        self.assertEqual(payload["force_object_x"], 250)
        self.assertEqual(payload["force_object_y"], 500)
        self.assertEqual(payload["stop_at_cursor"], 0x14)
        self.assertTrue(payload["trace_render"])
        self.assertEqual(payload["render_trace_hits"], 12)
        self.assertFalse(payload["capture_scheduled_pool"])
        self.assertEqual(payload["owner_probe_callback"], 0xb226)
        self.assertEqual(payload["owner_probe_x"], 353)
        self.assertEqual(payload["owner_probe_y"], 530)
        self.assertEqual(payload["owner_probe_phase"], 4)
        self.assertEqual(payload["owner_probe_timer"], 0xdc)
        self.assertEqual(payload["owner_probe_transition"], 1)
        self.assertTrue(payload["owner_probe_once"])
        self.assertNotIn("focus_callback_offset", payload)
        self.assertNotIn("screenshot", payload)

    def test_normalizes_lua_arrays(self):
        trace = normalize_high_effect_trace({
            "frames": {"2": {"pool": {"objects": {"1": {"index": 3}}}},
                       "1": {"pool": {"objects": {}}}},
            "callback_events": {"2": {"family": "D53F/D55A"},
                                "1": {"family": "B20B/B25D"}},
        })
        self.assertEqual([item["family"] for item in trace["callback_events"]],
                         ["B20B/B25D", "D53F/D55A"])
        self.assertEqual(trace["frames"][1]["pool"]["objects"][0]["index"], 3)

    def test_normalizes_spawned_effect_events(self):
        trace = normalize_high_effect_trace({
            "spawned_effect_events": {
                "2": {"callback_entry": {"offset": 0x4C74}},
                "1": {"callback_entry": {"offset": 0x4B70}},
            },
        })
        self.assertEqual(
            [item["callback_entry"]["offset"]
             for item in trace["spawned_effect_events"]],
            [0x4B70, 0x4C74],
        )


if __name__ == "__main__":
    unittest.main()
