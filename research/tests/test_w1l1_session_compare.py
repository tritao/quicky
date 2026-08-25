import copy
import json
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from w1l1_session_compare import compare  # noqa: E402


def record(seed: str) -> str:
    return (seed * (0x78 * 2 // len(seed) + 1))[:0x78 * 2]


def sample(sequence: int = 1) -> dict:
    return {
        "sequence": sequence,
        "input_flags": 0,
        "camera": {"x": 0, "y": 262},
        "player_callback": {
            "pre_record_hex": record("00"),
            "post_record_hex": record("01"),
            "collisions": [],
            "global_writes": [],
            "effects": [],
        },
        "scheduler_callbacks": [],
        "entities": [],
    }


class W1L1SessionCompareTests(unittest.TestCase):
    def write_pair(self, original, candidate):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        left = Path(directory.name) / "original.json"
        right = Path(directory.name) / "candidate.json"
        left.write_text(json.dumps(original), encoding="utf-8")
        right.write_text(json.dumps(candidate), encoding="utf-8")
        return left, right

    def test_identical_session_is_equal(self):
        payload = {"samples": [sample()]}
        left, right = self.write_pair(payload, copy.deepcopy(payload))
        mismatches, coverage = compare(left, right)
        self.assertEqual(mismatches, [])
        self.assertEqual(coverage, [])

    def test_first_record_drift_is_reported(self):
        original = {"samples": [sample()]}
        candidate = copy.deepcopy(original)
        candidate["samples"][0]["player_callback"]["post_record_hex"] = record("02")
        left, right = self.write_pair(original, candidate)
        mismatches, _ = compare(left, right)
        self.assertEqual(mismatches[0]["field"], "post_record")

    def test_dos_wrapper_and_missing_optional_capture_are_explicit(self):
        original = {"events": [{"samples": [
            {"sequence": 1,
             "globals": {"keyboard_action_flags": 0, "camera_x": 0,
                          "camera_y": 262},
             "player_callback": {
                 "pre_object": {"state_hex": record("00")},
                 "post_object": {"state_hex": record("01")},
             },
             "pool": {"objects": []}}
        ]}]}
        candidate = {"samples": [sample()]}
        left, right = self.write_pair(original, candidate)
        mismatches, coverage = compare(left, right)
        self.assertEqual(mismatches, [])
        self.assertTrue(any(item["field"] == "probes" for item in coverage))


if __name__ == "__main__":
    unittest.main()
