import json
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from contact_map_writer_report import (  # noqa: E402
    ContactMapWriterError,
    build_report,
)


def trace_payload():
    return {
        "inputs": {
            "select_level": "W1L1",
            "executable_sha256": "exe",
            "archive_sha256": "archive",
        },
        "events": [{
            "samples": [{
                "sequence": 1,
                "frame_index": 0,
                "execute_watches": [{
                    "offset": 0x16CE,
                    "map_writer": {
                        "coordinates": {"x": 128, "y": 400},
                        "effect_selector": 0x1F2,
                        "effect_word": 0x1F2,
                        "map_selector": 887,
                        "cell_offset": 13516,
                        "before_word": 5,
                        "after_word": 0x1F2,
                        "before_tile_id": 5,
                        "after_tile_id": 0x1F2,
                        "map_write_applied": True,
                    },
                }],
            }],
        }],
    }


class ContactMapWriterReportTests(unittest.TestCase):
    def test_report_preserves_ax_bx_dx_contract_and_mutation(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.json"
            path.write_text(json.dumps(trace_payload()), encoding="utf-8")
            report = build_report(path, "focused command")
        self.assertEqual(report["method"]["level"], "W1L1")
        self.assertEqual(report["observations"]["writer_events"], 1)
        self.assertEqual(report["observations"]["changed_events"], 1)
        row = report["observations"]["rows"][0]
        self.assertEqual(row["coordinates"], {"x": 128, "y": 400})
        self.assertEqual(row["effect_word"], 0x1F2)
        self.assertTrue(row["map_write_applied"])

    def test_report_fails_closed_without_writer_snapshot(self):
        payload = trace_payload()
        del payload["events"][0]["samples"][0]["execute_watches"][0][
            "map_writer"
        ]
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            with self.assertRaises(ContactMapWriterError):
                build_report(path)


if __name__ == "__main__":
    unittest.main()
