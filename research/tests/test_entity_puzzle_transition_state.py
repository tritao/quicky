import hashlib
import json
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]


class EntityPuzzleTransitionStateTests(unittest.TestCase):
    def test_trace_and_contract_are_self_consistent(self):
        contract_path = ROOT / "research" / "entity-puzzle-transition-state-evidence.json"
        contract = json.loads(contract_path.read_text())
        trace_path = ROOT / contract["dynamic_trace"]["trace"]
        trace = json.loads(trace_path.read_text())

        self.assertEqual(contract["inputs"]["ghidra"], "12.1.3")
        self.assertEqual(trace["schema"], "quiky-goal-transition-trace-v1")
        self.assertEqual(
            hashlib.sha256(trace_path.read_bytes()).hexdigest(),
            contract["dynamic_trace"]["trace_sha256"],
        )

        checkpoints = {item["name"]: item for item in trace["checkpoints"]}
        state = checkpoints["native-post-input-01d7:5010"]["gameplay_state"]
        self.assertEqual(state["score_low"], 2950)
        self.assertEqual(state["ammo"], 0)
        self.assertEqual(state["lives"], 4)
        self.assertEqual(state["pending_action"], 12)

        lookup = checkpoints["native-post-input-0207:18c7"]["resource_lookup"]
        self.assertEqual(lookup["path"]["text"], "GAMEDATA\\W1L4.map")
        self.assertEqual(
            checkpoints["native-post-input-01f7:35c7"]["gameplay_state"]["puzzle_mask"],
            0,
        )


if __name__ == "__main__":
    unittest.main()
