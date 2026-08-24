import tempfile
import unittest
from pathlib import Path
import sys

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from quikyctl import _write_rgb_png  # noqa: E402
from quikyentity import analyze_frame_differences  # noqa: E402


class QuikyEntityImageTests(unittest.TestCase):
    def test_union_intersection_and_bbox(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            baseline = []
            inert = []
            # Frame 0 changes one pixel; frame 1 changes that pixel and one
            # additional pixel. This exercises both persistence metrics.
            for index, changed in enumerate(((0,), (0, 5))):
                left = bytes(3 * 8)
                right = bytearray(left)
                for pixel in changed:
                    right[pixel * 3 : pixel * 3 + 3] = b"\xff\xff\xff"
                left_path = root / f"baseline-{index}.png"
                right_path = root / f"inert-{index}.png"
                _write_rgb_png(left_path, 4, 2, left)
                _write_rgb_png(right_path, 4, 2, bytes(right))
                baseline.append(left_path)
                inert.append(right_path)
            result = analyze_frame_differences(baseline, inert, root / "out")
        self.assertEqual(result["changed_pixels_by_frame"], [1, 2])
        self.assertEqual(result["union_pixels"], 2)
        self.assertEqual(result["intersection_pixels"], 1)
        self.assertEqual(result["union_bbox"], [0, 0, 2, 2])

    def test_zero_difference_is_preserved(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            left = root / "left.png"
            right = root / "right.png"
            _write_rgb_png(left, 2, 1, b"\x01\x02\x03" * 2)
            _write_rgb_png(right, 2, 1, b"\x01\x02\x03" * 2)
            result = analyze_frame_differences([left], [right], root / "out")
        self.assertEqual(result["changed_pixels_by_frame"], [0])
        self.assertEqual(result["union_pixels"], 0)
        self.assertEqual(result["intersection_pixels"], 0)
        self.assertIsNone(result["union_bbox"])


if __name__ == "__main__":
    unittest.main()
