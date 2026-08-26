import hashlib
import json
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]


class EntityPlatformSchedulerOrderTests(unittest.TestCase):
    def test_combined_trace_preserves_static_order_contract(self):
        contract_path = ROOT / "research" / "entity-platform-scheduler-order-evidence.json"
        contract = json.loads(contract_path.read_text())
        trace_path = ROOT / contract["dynamic_trace"]["trace"]
        trace = json.loads(trace_path.read_text())
        sample = trace["events"][0]["samples"][0]

        self.assertEqual(
            hashlib.sha256(trace_path.read_bytes()).hexdigest(),
            contract["dynamic_trace"]["trace_sha256"],
        )
        self.assertEqual(sample["callback"]["offset"], 0x9DC7)
        helper_offsets = {
            item["offset"] for item in sample["collision_probe"]["hits"].values()
        }
        self.assertEqual(helper_offsets, {0xA075, 0xA0B2})
        player = sample["platform_player"]
        self.assertEqual(player["entry"]["offset"], 0x3FF8)
        self.assertEqual(
            sample["globals_after"]["platform_overlap_latch"], 0xFFFF
        )
        self.assertEqual(sample["globals_after"]["player_carry_x_fixed"], 1)
        self.assertEqual(
            sample["globals_after"]["player_carry_y_fixed"], 0xFFF80001
        )


if __name__ == "__main__":
    unittest.main()
