import io
import json
import tempfile
import unittest
from pathlib import Path
import sys

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from descriptor_census_report import (  # noqa: E402
    CensusReportError,
    census_data,
    render_report,
    write_json,
)


def census_trace():
    return {
        "events": [{
            "descriptor_census": {
                "descriptor_table": {
                    "entries": [
                        {"tile_id": 7, "offset": 0x100, "word": 0x60},
                    ],
                },
                "map": {
                    "cells": [
                        {"x": 2, "y": 3, "world_x": 32, "world_y": 48,
                         "cell": 0x0E07, "property": 7, "tile_id": 7,
                         "descriptor": 0x60},
                        {"x": 0, "y": 0, "world_x": 0, "world_y": 0,
                         "cell": 0, "property": 0, "tile_id": 0,
                         "descriptor": 0},
                    ],
                },
            },
        }],
    }


class DescriptorCensusReportTests(unittest.TestCase):
    def test_flattens_flagged_cells_and_masks(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "w1l1.json"
            path.write_text(json.dumps(census_trace()), encoding="utf-8")
            rows = census_data([path], ["W1L1"])
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]["tile_id"], 7)
        self.assertEqual(rows[0]["mask_0x20"], 0x20)
        self.assertEqual(rows[0]["mask_0x40"], 0x40)
        self.assertEqual(rows[0]["world_x"], 32)

    def test_report_and_json_schema(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "w1l1.json"
            path.write_text(json.dumps(census_trace()), encoding="utf-8")
            rows = census_data([path])
            output = io.StringIO()
            render_report(rows, output)
            self.assertIn("descriptor_candidates=1", output.getvalue())
            json_path = Path(directory) / "census.json"
            write_json(rows, json_path)
            payload = json.loads(json_path.read_text(encoding="utf-8"))
        self.assertEqual(payload["schema"], "quiky-player-descriptor-census-v1")

    def test_rejects_mismatched_labels(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "w1l1.json"
            path.write_text(json.dumps(census_trace()), encoding="utf-8")
            with self.assertRaises(CensusReportError):
                census_data([path], ["one", "two"])


if __name__ == "__main__":
    unittest.main()
