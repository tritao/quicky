import copy
import hashlib
import json
import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from quiky.parity import compare_player as compare  # noqa: E402
from verify_player_contact_followup import verify  # noqa: E402


ROOT = Path(__file__).resolve().parents[2]


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
