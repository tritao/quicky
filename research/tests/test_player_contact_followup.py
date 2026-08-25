import copy
import json
import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from player_parity_compare import compare  # noqa: E402
from verify_player_contact_followup import verify  # noqa: E402


ROOT = Path(__file__).resolve().parents[2]


class PlayerContactFollowupTests(unittest.TestCase):
    def test_ledger_and_ignored_trace_hashes_verify(self):
        warnings = verify(
            ROOT / "research/ghidra/player-contact-followup.json",
            ROOT,
            require_traces=True,
        )
        self.assertEqual(warnings, [])

    def test_parity_accepts_identical_complete_callback_trace(self):
        trace = ROOT / "research/build/player-followup-side-rising-falling-v2.json"
        self.assertEqual(compare(trace, trace), [])

    def test_parity_rejects_a_single_record_byte_drift(self):
        source = ROOT / "research/build/player-followup-side-rising-falling-v2.json"
        payload = json.loads(source.read_text(encoding="utf-8"))
        candidate = copy.deepcopy(payload)
        sample = candidate["events"][0]["samples"][1]
        state = sample["player_callback"]["post_object"]["state_hex"]
        replacement = ("00" if state[:2] != "00" else "01") + state[2:]
        sample["player_callback"]["post_object"]["state_hex"] = replacement
        import tempfile

        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "candidate.json"
            path.write_text(json.dumps(candidate), encoding="utf-8")
            mismatches = compare(source, path)
        self.assertTrue(any(item["field"] == "post_record" for item in mismatches))


if __name__ == "__main__":
    unittest.main()
