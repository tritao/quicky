import json
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class PlayerLifecycleEvidenceTests(unittest.TestCase):
    def test_recovery_spawn_row_is_separate_from_callback_correction(self):
        path = ROOT / "research/evidence/player-dos-parity/player-death-recovery-spawn-v1.json"
        evidence = json.loads(path.read_text(encoding="utf-8"))

        self.assertEqual(evidence["schema"],
                         "quiky.player-death-recovery-spawn.v1")
        self.assertEqual(evidence["observations"]["route"],
                         ["01D7:4BD8", "01F7:1AAA", "01F7:3FF8"])
        self.assertEqual(evidence["observations"]["row_selector_85d2"], 0)
        self.assertEqual(evidence["observations"]["resource_handle_97e2"], 5)
        self.assertEqual(evidence["observations"]["row0_words"],
                         {"x": 1673, "y": 374})
        self.assertEqual(
            evidence["observations"]["first_recovered_callback"]["position_pixels"],
            [1673, 368],
        )
        self.assertNotEqual(
            evidence["observations"]["row0_words"]["y"],
            evidence["observations"]["first_recovered_callback"]["position_pixels"][1],
        )

    def test_recovery_scheduler_keeps_identity_across_bank_rebuild(self):
        path = ROOT / "research/evidence/player-dos-parity/player-death-recovery-scheduler-v1.json"
        evidence = json.loads(path.read_text(encoding="utf-8"))

        self.assertEqual(evidence["schema"],
                         "quiky.player-death-recovery-scheduler.v1")
        self.assertEqual(
            evidence["observations"]["route"],
            ["01D7:4BA4", "01D7:4BD8", "01F7:1AAA", "01F7:3FF8"],
        )
        self.assertEqual(
            evidence["observations"]["before_recovery"]["player_entries"],
            [
                {"bank": "0x7566", "index": 9, "object_offset": 0},
                {"bank": "0x7766", "index": 9, "object_offset": 0},
            ],
        )
        self.assertEqual(
            evidence["observations"]["first_recovered_callback"]["player_entries"],
            [{"bank": "0x7766", "index": 8, "object_offset": 0}],
        )

    def test_natural_gate_trace_records_recovery_order(self):
        path = ROOT / "research/evidence/player-dos-parity/player-death-recovery-gate-v1.json"
        evidence = json.loads(path.read_text(encoding="utf-8"))

        self.assertEqual(evidence["schema"],
                         "quiky.player-death-recovery-gate.v1")
        self.assertEqual(
            evidence["observations"]["countdown"]["last_callback_before_lifecycle_gate"],
            -347,
        )
        order = evidence["observations"]["recovery_order"]
        self.assertEqual(
            [item["address"] for item in order],
            ["01D7:4BA4", "01D7:4BD8", "01F7:1AE6", "01F7:3FF8"],
        )
        self.assertEqual(order[0]["transition_gate"], -350)
        self.assertEqual(order[1]["health"], 3)
        self.assertEqual(order[-1]["transition_gate"], 0)
        self.assertIn(
            "Whether -350 is universal across levels, transitions, or only this natural W1L1 route.",
            evidence["conclusion"]["still_open"],
        )


if __name__ == "__main__":
    unittest.main()
