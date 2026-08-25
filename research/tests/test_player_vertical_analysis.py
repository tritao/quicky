import json
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from player_vertical_analysis import (
    ASCENT_ACCELERATION,
    GRAVITY,
    JUMP_IMPULSE,
    RELEASE_FLOOR,
    TERMINAL_VELOCITY,
    build_validation,
    normalize_trace,
    predict_ascent_velocity,
    predict_descent_velocity,
    predict_position,
    validate_timeline,
)


def frame(sequence, y, vy, mode):
    return {
        "sequence": sequence,
        "frame_index": sequence - 1,
        "position": {"before": {"y_fixed_signed": y}, "after": {"y_fixed_signed": y}},
        "velocity": {"before": {"velocity_y_fixed_signed": vy}, "after": {"velocity_y_fixed_signed": vy}},
        "action_state": {
            "before": {"action_word": 0, "player_byte_0x37": mode},
            "after": {"action_word": 0, "player_byte_0x37": mode},
        },
    }


class PlayerVerticalAnalysisTests(unittest.TestCase):
    def test_fixed_point_constants_and_update_order(self):
        self.assertEqual(JUMP_IMPULSE, -0x4A000)
        self.assertEqual(predict_ascent_velocity(JUMP_IMPULSE), RELEASE_FLOOR)
        self.assertEqual(predict_ascent_velocity(-0x20000, 0x22), -0x1E000)
        self.assertEqual(predict_descent_velocity(0), GRAVITY)
        self.assertEqual(predict_descent_velocity(TERMINAL_VELOCITY), TERMINAL_VELOCITY)
        self.assertEqual(predict_position(0x10000, -0x2000), 0xE000)

    def test_normalize_current_raw_numeric_sample_object(self):
        payload = {
            "schema": "raw",
            "samples": {
                "1": {
                    "frame_index": 0,
                    "sequence": 1,
                    "player_callback": {
                        "pre_object": {"position": {"y_fixed": 0x10000}, "velocity_y_fixed": 0,
                                        "player_byte_0x37": 1},
                        "post_object": {"position": {"y_fixed": 0x12800}, "velocity_y_fixed": 0x2800,
                                         "player_byte_0x37": 1},
                    },
                }
            },
        }
        timeline = normalize_trace(payload)
        self.assertEqual(len(timeline["frames"]), 1)
        self.assertEqual(timeline["frames"][0]["position"]["after"]["y_fixed_signed"], 0x12800)
        self.assertEqual(timeline["frames"][0]["velocity"]["after"]["velocity_y_fixed_signed"], 0x2800)

    def test_normalize_archival_wip_sample_shape(self):
        payload = {
            "schema": "quiky-resource-trace-v1",
            "events": [{"samples": [{
                "frame_index": 7,
                "player_callback": {
                    "pre_object": {"position": {"y_fixed_signed": 0x20000},
                                    "velocity_y_fixed_signed": -0x2000,
                                    "player_byte_0x37": 255},
                    "post_object": {"position": {"y_fixed_signed": 0x1E000},
                                     "velocity_y_fixed_signed": -0x2000,
                                     "player_byte_0x37": 255},
                },
            }]}],
        }
        timeline = normalize_trace(payload)
        self.assertEqual(timeline["frames"][0]["frame_index"], 7)
        self.assertEqual(timeline["frames"][0]["position"]["after"]["y_fixed_signed"], 0x1E000)

    def test_validate_apex_and_free_space_values(self):
        first = frame(1, 0x100000, -0x2000, -1)
        second = frame(2, 0x100000, 0, 1)
        third = frame(3, 0x100000, 0, 1)
        third["position"]["after"]["y_fixed_signed"] = 0x102800
        third["velocity"]["after"]["velocity_y_fixed_signed"] = 0x2800
        timeline = {"schema": "test", "frames": [first, second, third]}
        result = validate_timeline(timeline)
        self.assertEqual(result["status"], "pass")
        self.assertEqual(result["mismatches"], 0)
        self.assertEqual(len(result["apex_transitions"]), 1)

    def test_validate_reports_first_mismatch(self):
        first = frame(1, 0x100000, 0, 1)
        second = frame(2, 0x100000, GRAVITY + 1, 1)
        second["position"]["after"]["y_fixed_signed"] = 0x100000 + GRAVITY + 1
        result = validate_timeline({"frames": [first, second]})
        self.assertEqual(result["status"], "fail")
        self.assertEqual(result["first_mismatch"]["field"], "vy")

    def test_build_validation_separates_roles(self):
        payload = {"schema": "normalized", "frames": [
            frame(1, 0x100000, 0, 1),
            {**frame(2, 0x100000 + GRAVITY, GRAVITY, 1),
             "position": {"before": {"y_fixed_signed": 0x100000},
                           "after": {"y_fixed_signed": 0x100000 + GRAVITY}}},
        ]}
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            result = build_validation([path], [path])
        self.assertEqual(result["derivation_trace_count"], 1)
        self.assertEqual(result["held_out_trace_count"], 1)
        self.assertEqual(result["status"], "pass")
        self.assertEqual(result["mismatches"], 0)


if __name__ == "__main__":
    unittest.main()
