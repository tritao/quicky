import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))

from scene_frame_compare import Image, compare_pixels  # noqa: E402


class SceneFrameCompareTests(unittest.TestCase):
    def test_exact_match(self):
        image = Image(2, 1, ((1, 2, 3), (4, 5, 6)))
        report = compare_pixels(image, image)
        self.assertEqual(report["mismatched_pixels"], 0)
        self.assertEqual(report["match_ratio"], 1.0)

    def test_region_and_tolerance(self):
        left = Image(3, 2, ((0, 0, 0), (10, 10, 10), (0, 0, 0),
                            (0, 0, 0), (20, 20, 20), (0, 0, 0)))
        right = Image(3, 2, ((0, 0, 0), (11, 10, 10), (0, 0, 0),
                             (0, 0, 0), (20, 22, 20), (0, 0, 0)))
        report = compare_pixels(left, right, (1, 0, 1, 2), tolerance=1)
        self.assertEqual(report["pixels_compared"], 2)
        self.assertEqual(report["mismatched_pixels"], 1)
        self.assertEqual(report["mismatch_bbox"], [1, 1, 1, 1])


if __name__ == "__main__":
    unittest.main()
