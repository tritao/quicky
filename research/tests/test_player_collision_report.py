import io
import json
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from player_collision_report import (  # noqa: E402
    CollisionReportError,
    collision_rows,
    render_report,
    write_csv,
)


def trace(*helpers):
    events = []
    for index, helper in enumerate(helpers):
        events.append({
            "event_index": index + 1,
            "helper_offset": helper,
            "registers": {"eax": 0x10000 + index, "ebx": 0x20,
                           "ecx": 0x30, "edx": 0x40},
            "object": {"player_byte_0x36": 1, "player_byte_0x37": 2,
                        "player_byte_0x38": 3, "player_byte_0x39": 4,
                        "player_byte_0x3a": 5, "player_byte_0x3b": 6},
            "return_breakpoint": {
                "segment": 0x1F7, "offset": 0x4000 + index,
                "registers": {"eax": 0x90 + index, "edx": 0xA0 + index,
                               "flags": 0x200},
            },
            "map_property": {
                "map_lookup": {"x": 123, "y": 400, "cell_word": 0x2D,
                                "tile_id": 0x2D},
                "descriptor_word": 0x0C,
            },
            "patches": [{"side": "right", "tile_id": 0x2A,
                          "descriptor_word": 0x70, "readback": 0x2A}],
        })
    return {"events": [{"samples": [{
        "sequence": 1,
        "frame_index": 12,
        "globals": {"player_object_offset": 0},
        "pool": {"selector": 0x27F, "objects": [{
            "offset": 0, "position": {"x": 128, "y": 400},
        }]},
        "collisions": events,
    }]}]}


class PlayerCollisionReportTests(unittest.TestCase):
    def test_preserves_order_and_masks_registers(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.json"
            path.write_text(json.dumps(trace(0x648E, 0x6484, 0x3DF2)), encoding="utf-8")
            rows = collision_rows([path], ["right"])
        self.assertEqual([row["helper_offset"] for row in rows],
                         [0x648E, 0x6484, 0x3DF2])
        self.assertEqual(rows[0]["helper_name"], "player_collision_648e")
        self.assertEqual(rows[0]["eax"], 0)
        self.assertEqual(rows[-1]["player_x"], 128)
        self.assertEqual(rows[-1]["object_0x3a"], 5)
        self.assertEqual(rows[-1]["return_ax"], 0x92)
        self.assertEqual(rows[-1]["return_offset"], 0x4002)
        self.assertEqual(rows[-1]["map_tile_id"], 0x2D)
        self.assertEqual(rows[-1]["descriptor_word"], 0x0C)
        self.assertEqual(rows[-1]["patch_sides"], "right")
        self.assertEqual(rows[-1]["patch_descriptors"], "0x0070")

    def test_report_groups_path_by_sample(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.json"
            path.write_text(json.dumps(trace(0x648E, 0x3DF2)), encoding="utf-8")
            rows = collision_rows([path], ["neutral"])
        report = io.StringIO()
        render_report(rows, report)
        self.assertIn("collision_rows=2", report.getvalue())
        self.assertIn("player_collision_648e>descriptor_probe_3df2", report.getvalue())

    def test_rejects_label_mismatch(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.json"
            path.write_text(json.dumps(trace(0x648E)), encoding="utf-8")
            with self.assertRaises(CollisionReportError):
                collision_rows([path], [])

    def test_csv_has_register_and_tail_columns(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.json"
            output = Path(directory) / "collision.csv"
            path.write_text(json.dumps(trace(0x648E)), encoding="utf-8")
            write_csv(collision_rows([path]), output)
            header = output.read_text(encoding="utf-8").splitlines()[0]
        self.assertIn("helper_name", header)
        self.assertIn("eax", header)
        self.assertIn("object_0x3a", header)
        self.assertIn("return_ax", header)
        self.assertIn("descriptor_word", header)
        self.assertIn("patch_sides", header)

    def test_accepts_lua_numeric_object_for_patch_array(self):
        payload = trace(0x3DF2)
        payload["events"][0]["samples"][0]["collisions"][0]["patches"] = {
            "1": {"side": "left", "tile_id": 0x2A,
                   "descriptor_word": 0x70, "readback": 0x2A},
        }
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            rows = collision_rows([path])
        self.assertEqual(rows[0]["patch_sides"], "left")


if __name__ == "__main__":
    unittest.main()
