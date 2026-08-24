import io
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from player_trace_report import (  # noqa: E402
    TraceReportError,
    correlate_samples,
    render_report,
    write_calibration_csv,
)


def sample(sequence, x, y, callback, *, offset=0):
    return {
        "sequence": sequence,
        "globals": {
            "player_object_offset": offset,
            "camera_x": 0,
            "camera_y": 262,
            "input_action_flags": 0,
        },
        "pool": {
            "selector": 0x027F,
            "objects": [
                {
                    "offset": offset,
                    "position": {
                        "x": x,
                        "y": y,
                        "x_fixed": x << 16,
                        "y_fixed": y << 16,
                    },
                    "callback": callback,
                    "sprite_slot": 0,
                    "kind": 0,
                },
                {
                    "offset": 0x78,
                    "position": {
                        "x": 768,
                        "y": 224,
                        "x_fixed": 768 << 16,
                        "y_fixed": 224 << 16,
                    },
                    "callback": 0x4727,
                    "sprite_slot": 0x2BF,
                    "kind": 0,
                },
            ],
        },
    }


class PlayerTraceReportTests(unittest.TestCase):
    def test_correlates_ds_player_offset_to_pool_object(self):
        trace = {"samples": [sample(1, 128, 400, 0x3F27), sample(2, 129, 400, 0x3FF8)]}
        result = correlate_samples(trace)
        self.assertEqual([(item.selector, item.offset) for item in result],
                         [(0x027F, 0), (0x027F, 0)])
        self.assertEqual([item.object["callback"] for item in result], [0x3F27, 0x3FF8])

    def test_rejects_missing_or_ambiguous_identity(self):
        with self.assertRaises(TraceReportError):
            correlate_samples({"samples": [sample(1, 128, 400, 0x3F27)]}, offset=0x99)

    def test_report_and_csv_keep_provisional_fields_explicit(self):
        trace = {"samples": [sample(1, 128, 400, 0x3F27), sample(2, 129, 400, 0x3FF8)]}
        result = correlate_samples(trace)
        report = io.StringIO()
        render_report(result, report)
        self.assertIn("player_identity=0x027f:0x0000", report.getvalue())
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "player.csv"
            write_calibration_csv(result, output, 2, 1, "right")
            lines = output.read_text(encoding="utf-8").splitlines()
            self.assertIn("# grounded/facing are explicit provisional values", lines[2])
            self.assertEqual(lines[5], "dosbox,2,129,400,32768,0,1,right")


if __name__ == "__main__":
    unittest.main()
