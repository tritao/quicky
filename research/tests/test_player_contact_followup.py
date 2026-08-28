import copy
import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from quiky.state import compare_state, load_state_jsonl, save_state_jsonl  # noqa: E402
from verify_player_contact_followup import verify  # noqa: E402


ROOT = Path(__file__).resolve().parents[2]


def compare(left: Path, right: Path):
    return compare_state(left, right, "exact")[0]


class PlayerContactFollowupTests(unittest.TestCase):
    def test_held_jump_ordinary_apex_negative_control_is_audited(self):
        path = ROOT / "research/evidence/player-dos-parity/player-natural-apex-held-negative-control-v1.json"
        evidence = json.loads(path.read_text(encoding="utf-8"))
        self.assertEqual(evidence["schema"], "quiky.player-natural-apex-held-negative-control.v1")

        source = evidence["source"]
        for relative, key in (("game/QUIKY.EXE", "executable_sha256"),
                              ("game/NESTLE.DAT", "archive_sha256"),
                              ("research/automation/quiky_player_trace.lua", "trace_script_sha256")):
            digest = hashlib.sha256((ROOT / relative).read_bytes()).hexdigest()
            self.assertEqual(digest, source[key], relative)

        observations = evidence["observations"]
        self.assertEqual(observations["samples"], 46)
        self.assertEqual(observations["jump_entry"]["action_word"], 0x26)
        self.assertEqual(observations["ordinary_apex_join"]["post_mode"], 1)
        self.assertEqual(observations["landing"]["post_mode"], 0)
        self.assertEqual(observations["landing"]["post_velocity_y_fixed_signed"], 0)
        watches = observations["unique_execute_watch_hits"]
        self.assertEqual(watches["01F7:4323"], 9)
        self.assertEqual(watches["01F7:3986"], 9)
        for address in ("01F7:4363", "01F7:4366", "01F7:4368", "01F7:41C1", "01F7:41CF"):
            self.assertEqual(watches[address], 0, address)

    def test_ledger_and_ignored_trace_hashes_verify(self):
        warnings = verify(
            ROOT / "research/ghidra/player-contact-followup.json",
            ROOT,
            require_traces=True,
        )
        self.assertEqual(warnings, [])

    def test_parity_accepts_identical_canonical_state(self):
        state = ROOT / "research/runs/w1l1-jump/expected-state.jsonl"
        self.assertEqual(compare(state, state), [])

    def test_parity_rejects_a_single_record_byte_drift(self):
        source = ROOT / "research/runs/w1l1-jump/expected-state.jsonl"
        candidate = copy.deepcopy(load_state_jsonl(source))
        state = candidate[1]["post_record"]
        candidate[1]["post_record"] = ("00" if state[:2] != "00" else "01") + state[2:]
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "candidate.jsonl"
            save_state_jsonl(path, candidate)
            mismatches = compare(source, path)
        self.assertTrue(any(item["field"] == "post_record" for item in mismatches))


if __name__ == "__main__":
    unittest.main()
