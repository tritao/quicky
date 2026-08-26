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

    def test_player_return_followup_captures_full_record(self):
        contract_path = ROOT / "research" / "entity-platform-scheduler-order-evidence.json"
        contract = json.loads(contract_path.read_text())
        followup = contract["player_return_followup"]
        trace_path = ROOT / followup["trace"]
        trace = json.loads(trace_path.read_text())

        self.assertEqual(
            hashlib.sha256(trace_path.read_bytes()).hexdigest(),
            followup["trace_sha256"],
        )
        samples = trace["events"][0]["samples"]
        observed = [
            sample for sample in samples
            if sample["platform_player"].get("status") == "observed"
        ]
        self.assertGreaterEqual(len(observed), 3)
        for sample in observed:
            self.assertEqual(sample["callback"]["offset"], 0x9DC7)
            self.assertEqual(sample["platform_player"]["entry"]["offset"], 0x3FF8)
            self.assertEqual(sample["platform_player"]["return_hit"]["offset"], 0x0F26)
            self.assertEqual(len(sample["platform_player"]["before"]["raw_hex"]), 0x78 * 2)
            self.assertEqual(len(sample["platform_player"]["after"]["raw_hex"]), 0x78 * 2)

    def test_controlled_platform_jump_detachment_consumes_initial_carry(self):
        trace_path = ROOT / "research" / "evidence" / "entity-platform-player-jump-detach-v1.json"
        trace = json.loads(trace_path.read_text())
        self.assertEqual(
            hashlib.sha256(trace_path.read_bytes()).hexdigest(),
            "9b60c6a74d0abf47d203541587945254da15172e077e3bf98fbf7f35cac46387",
        )
        config = trace["config"]
        self.assertTrue(config["player_input_before_platform"])
        self.assertEqual(config["player_input_phases"][0]["keys"], ["KBD_space", "KBD_up"])
        samples = [
            sample for sample in trace["events"][0]["samples"]
            if sample["platform_player"].get("status") == "observed"
        ]
        self.assertGreaterEqual(len(samples), 3)
        first = samples[0]["platform_player"]
        self.assertEqual(first["globals_before"]["keyboard_flags"], 0x22)
        self.assertEqual(first["globals_before"]["platform_overlap_latch"], 0xFFFF)
        self.assertEqual(first["globals_before"]["player_carry_x_fixed"], 1)
        self.assertEqual(first["globals_before"]["player_carry_y_fixed"], 0xFFF80001)
        self.assertEqual(first["before"]["mode"], 0)
        self.assertEqual(first["after"]["mode"], 0xFF)
        self.assertEqual(first["globals_after"]["player_carry_x_fixed"], 0)
        self.assertEqual(first["globals_after"]["player_carry_y_fixed"], 0)
        self.assertEqual(len(first["before"]["raw_hex"]), 0x78 * 2)
        self.assertEqual(len(first["after"]["raw_hex"]), 0x78 * 2)
        later = samples[1]["platform_player"]
        self.assertEqual(later["globals_before"]["platform_overlap_latch"], 0)
        self.assertEqual(later["globals_before"]["player_carry_x_fixed"], 0)
        self.assertEqual(later["globals_before"]["player_carry_y_fixed"], 0)
        self.assertEqual(later["before"]["mode"], 0xFF)


if __name__ == "__main__":
    unittest.main()
