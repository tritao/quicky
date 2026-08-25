import json
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from player_parity_compare import compare  # noqa: E402
from player_replay_manifest import build_manifest, write_tsv  # noqa: E402


ROOT = Path(__file__).resolve().parents[2]


class PlayerParityWorkflowTests(unittest.TestCase):
    def test_manifest_restores_complete_pre_state_and_action(self):
        source = ROOT / "research/build/player-followup-standing-v1.json"
        manifest = build_manifest(source)
        self.assertEqual(manifest["schema"], "quiky.player-replay-v1")
        self.assertGreater(len(manifest["rows"]), 0)
        self.assertEqual(len(manifest["rows"][0][-1]), 0x78 * 2)
        self.assertEqual(manifest["rows"][0][1], "0")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "replay.tsv"
            write_tsv(manifest, path)
            fields = [line.split() for line in path.read_text().splitlines()
                      if line and not line.startswith("#")]
        self.assertEqual(len(fields[0]), 22)
        self.assertEqual(fields[0][-1], manifest["rows"][0][-1])

    def test_probe_coordinates_and_occupancy_are_compared(self):
        source = ROOT / "research/build/player-followup-side-rising-falling-v2.json"
        payload = json.loads(source.read_text(encoding="utf-8"))
        candidate = json.loads(source.read_text(encoding="utf-8"))
        candidate["events"][0]["samples"][0]["collisions"][0]["occupied"] = True
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "candidate.json"
            path.write_text(json.dumps(candidate), encoding="utf-8")
            mismatches = compare(source, path)
        self.assertTrue(any(item["field"] == "probes" for item in mismatches))

    def test_missing_probe_array_is_not_treated_as_empty(self):
        source = ROOT / "research/build/player-followup-side-rising-falling-v2.json"
        candidate = json.loads(source.read_text(encoding="utf-8"))
        sample = next(sample for sample in candidate["events"][0]["samples"]
                      if "collisions" in sample)
        del sample["collisions"]
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "candidate.json"
            path.write_text(json.dumps(candidate), encoding="utf-8")
            mismatches = compare(source, path)
        self.assertTrue(any(item["field"] == "probes" for item in mismatches))


if __name__ == "__main__":
    unittest.main()
