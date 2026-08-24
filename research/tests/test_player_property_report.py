import io
import json
import tempfile
import unittest
from pathlib import Path
import sys

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from player_property_report import (  # noqa: E402
    PropertyReportError,
    property_rows,
    render_report,
    write_csv,
    write_json,
)


def trace_sample(sequence, helper, x=128, y=400):
    return {
        "sequence": sequence,
        "globals": {"player_object_offset": 0},
        "pool": {"objects": [{
            "offset": 0,
            "position": {"x": x, "y": y},
        }]},
        "map_property": {
            "helper_offset": helper,
            "coordinates": {"x": x, "y": y, "ax_bit_3": (y >> 3) & 1,
                            "bx_bit_3": (x >> 3) & 1},
            "map_lookup": {"cell_word": 0xEC8B, "tile_id": 0x08B},
            "raw_cell_word": 0xEC8B,
            "map_property_field": 0x76,
            "tile_id": 0x08B,
            "descriptor_offset": 0x166,
            "descriptor_word": 0x32,
            "descriptor_low_nibble": 2,
            "quadrant_flag_mask": 2,
            "quadrant_bits": 2,
            "descriptor_flag_set": True,
        },
    }


class PlayerPropertyReportTests(unittest.TestCase):
    def test_normalizes_property_rows_and_joins_player_position(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "left.json"
            path.write_text(json.dumps({"events": [{"samples": [
                trace_sample(1, 0x5C27), trace_sample(2, 0x5CC3),
            ]}]}), encoding="utf-8")
            rows = property_rows([path], ["left"])
        self.assertEqual(len(rows), 2)
        self.assertEqual(rows[0]["player_x"], 128)
        self.assertEqual(rows[0]["scenario"], "left")
        self.assertEqual(rows[0]["quadrant_flag_mask"], 2)
        self.assertIsNone(rows[1]["quadrant_flag_mask"])

    def test_report_and_artifacts_are_machine_readable(self):
        rows = [{**trace_sample(1, 0x5C27)["map_property"],
                 "trace": "x", "scenario": "left", "sequence": 1,
                 "helper_offset": 0x5C27, "x": 128, "y": 400,
                 "player_x": 128, "player_y": 400, "ax_bit_3": 0,
                 "bx_bit_3": 0}]
        output = io.StringIO()
        render_report(rows, output)
        self.assertIn("property_rows=1", output.getvalue())
        self.assertIn("0x5c27", output.getvalue())
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            csv_path = root / "property.csv"
            json_path = root / "property.json"
            write_csv(rows, csv_path)
            write_json(rows, json_path)
            self.assertIn("descriptor_word", csv_path.read_text(encoding="utf-8"))
            self.assertEqual(
                json.loads(json_path.read_text(encoding="utf-8"))["schema"],
                "quiky-player-property-v1",
            )

    def test_rejects_mismatched_labels(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.json"
            path.write_text(json.dumps({"samples": []}), encoding="utf-8")
            with self.assertRaises(PropertyReportError):
                property_rows([path], ["one", "two"])


if __name__ == "__main__":
    unittest.main()
