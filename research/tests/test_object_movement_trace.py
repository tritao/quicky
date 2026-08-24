import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from object_movement_trace import build_parser  # noqa: E402


class ObjectMovementTraceTests(unittest.TestCase):
    def test_defaults_hold_right_after_initialization(self):
        args = build_parser().parse_args(["--output", "trace.json"])
        self.assertEqual(args.movement_key, "KBD_right")
        self.assertEqual(args.movement_frames, 240)
        self.assertEqual(args.return_key, "KBD_left")
        self.assertEqual(args.return_frames, 0)
        self.assertEqual(args.capture_frames, 32)
        self.assertEqual(args.frame_step, 5)

    def test_explicit_movement_parameters_are_preserved(self):
        args = build_parser().parse_args([
            "--output", "trace.json",
            "--entity-type", "0x01",
            "--record-offset", "0x177a",
            "--movement-key", "KBD_left",
            "--movement-frames", "90",
            "--return-key", "KBD_right",
            "--return-frames", "120",
            "--capture-frames", "12",
            "--frame-step", "3",
        ])
        self.assertEqual(args.entity_type, 1)
        self.assertEqual(args.record_offset, 0x177A)
        self.assertEqual(args.movement_key, "KBD_left")
        self.assertEqual(args.movement_frames, 90)
        self.assertEqual(args.return_key, "KBD_right")
        self.assertEqual(args.return_frames, 120)
        self.assertEqual(args.capture_frames, 12)
        self.assertEqual(args.frame_step, 3)


if __name__ == "__main__":
    unittest.main()
