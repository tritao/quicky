import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from phase_lifecycle_trace import (  # noqa: E402
    PhaseLifecycleConfig,
    lua_config,
)


class PhaseLifecycleTraceTests(unittest.TestCase):
    def test_config_serializes_lightweight_probe(self):
        config = PhaseLifecycleConfig(
            startup_recording=Path("startup.json"),
            sample_count=240,
            sample_interval=2,
            warmup_frames=480,
            scan_limit=20,
            force_phase=2,
            force_transition=1,
            force_x=300,
            force_y=500,
            force_player_with_owner=True,
            force_game_state=3,
            force_late_action_state=True,
            teardown_probe=True,
            teardown_timeout_ms=250,
            teardown_rearm_callbacks=True,
            teardown_max_hits=12,
            teardown_watch_callback_writes=True,
            teardown_watch_all_callbacks=True,
            teardown_watch_linked_records=True,
            teardown_watch_self_test=True,
            teardown_watch_b87b_gate=True,
            teardown_watch_main_transitions=True,
            teardown_watch_control_writes=True,
            teardown_hold_key="KBD_right",
            teardown_hold_jump=True,
        )
        payload = lua_config(config)
        self.assertEqual(payload["sample_count"], 240)
        self.assertEqual(payload["sample_interval"], 2)
        self.assertEqual(payload["warmup_frames"], 480)
        self.assertEqual(payload["scan_limit"], 20)
        self.assertEqual(payload["force_transition"], 1)
        self.assertTrue(payload["force_player_with_owner"])
        self.assertEqual(payload["force_game_state"], 3)
        self.assertTrue(payload["force_late_action_state"])
        self.assertTrue(payload["teardown_probe"])
        self.assertEqual(payload["teardown_timeout_ms"], 250)
        self.assertTrue(payload["teardown_rearm_callbacks"])
        self.assertEqual(payload["teardown_max_hits"], 12)
        self.assertTrue(payload["teardown_watch_callback_writes"])
        self.assertTrue(payload["teardown_watch_all_callbacks"])
        self.assertTrue(payload["teardown_watch_linked_records"])
        self.assertTrue(payload["teardown_watch_self_test"])
        self.assertTrue(payload["teardown_watch_b87b_gate"])
        self.assertTrue(payload["teardown_watch_main_transitions"])
        self.assertTrue(payload["teardown_watch_control_writes"])
        self.assertEqual(payload["teardown_hold_key"], "KBD_right")
        self.assertTrue(payload["teardown_hold_jump"])

    def test_config_can_leave_natural_state_unmodified(self):
        config = PhaseLifecycleConfig(startup_recording=Path("startup.json"))
        payload = lua_config(config)
        self.assertIsNone(payload["force_phase"])
        self.assertIsNone(payload["force_x"])
        self.assertIsNone(payload["force_y"])


if __name__ == "__main__":
    unittest.main()
