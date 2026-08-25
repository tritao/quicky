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
        )
        payload = lua_config(config)
        self.assertEqual(payload["sample_count"], 240)
        self.assertEqual(payload["sample_interval"], 2)
        self.assertEqual(payload["warmup_frames"], 480)
        self.assertEqual(payload["scan_limit"], 20)
        self.assertEqual(payload["force_transition"], 1)

    def test_config_can_leave_natural_state_unmodified(self):
        config = PhaseLifecycleConfig(startup_recording=Path("startup.json"))
        payload = lua_config(config)
        self.assertIsNone(payload["force_phase"])
        self.assertIsNone(payload["force_x"])
        self.assertIsNone(payload["force_y"])


if __name__ == "__main__":
    unittest.main()
