import io
import json
import tempfile
import unittest
from pathlib import Path
import sys

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from player_branch_report import (  # noqa: E402
    BranchReportError,
    branch_rows,
    render_report,
    write_json,
)


def branch_trace():
    return {
        "events": [{"samples": [{
            "sequence": 1,
            "globals": {"player_object_offset": 0},
            "pool": {"objects": [{
                "offset": 0,
                "position": {"x": 128, "y": 400},
            }]},
            "branch_events": [
                {"offset": 0x3D02, "dx": 0x000C,
                 "object": {"player_byte_0x3a": 0}},
                {"offset": 0x3D1E, "dx": 0x0050,
                 "dx_mask_0x30": 0x10, "dx_mask_0x20": 0,
                 "dx_mask_0x40": 0x40,
                 "object": {"player_byte_0x3a": 1}},
                {"offset": 0x3DF1, "dx": 0x0050,
                 "object": {"player_byte_0x3a": 1}},
            ],
            "branch_return": {"offset": 0x3DF1,
                              "registers": {"eax": 0x101}},
        }]}],
    }


class PlayerBranchReportTests(unittest.TestCase):
    def test_flattens_masks_and_return_value(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "right.json"
            path.write_text(json.dumps(branch_trace()), encoding="utf-8")
            rows = branch_rows([path], ["right"])
        self.assertEqual([row["name"] for row in rows],
                         ["entry", "test_dx_30_first", "return_al"])
        self.assertEqual(rows[1]["dx_mask_0x30"], 0x10)
        self.assertEqual(rows[-1]["return_al"], 1)

    def test_render_and_json_artifact(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.json"
            path.write_text(json.dumps(branch_trace()), encoding="utf-8")
            rows = branch_rows([path])
            output = io.StringIO()
            render_report(rows, output)
            self.assertIn("test_dx_30_first", output.getvalue())
            artifact = Path(directory) / "branch.json"
            write_json(rows, artifact)
            self.assertEqual(
                json.loads(artifact.read_text(encoding="utf-8"))["schema"],
                "quiky-player-branch-v1",
            )

    def test_rejects_mismatched_labels(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.json"
            path.write_text(json.dumps({"samples": []}), encoding="utf-8")
            with self.assertRaises(BranchReportError):
                branch_rows([path], ["one", "two"])


if __name__ == "__main__":
    unittest.main()
