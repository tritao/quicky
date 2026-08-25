import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "research" / "tools"))

from quiky_ghidra import AnalysisError, load_manifest
from generate_quiky_analysis import generate


class QuikyGhidraManifestTests(unittest.TestCase):
    def test_canonical_manifest_matches_executable(self):
        manifest = load_manifest(
            ROOT / "research" / "ghidra" / "quiky-analysis.json", ROOT
        )
        symbols = {(item["segment"], item["offset"]): item for item in manifest["symbols"]}
        self.assertEqual(symbols[(3, "3FF8")]["name"], "update_player")
        self.assertEqual(symbols[(6, "881A")]["name"], "player_offset")
        self.assertEqual(
            manifest["source_ledger"],
            "research/ghidra/player-callback-closure.json",
        )

    def test_audit_manifest_is_generated_from_callback_closure(self):
        closure = json.loads(
            (ROOT / "research/ghidra/player-callback-closure.json").read_text(
                encoding="utf-8"
            )
        )
        actual = json.loads(
            (ROOT / "research/ghidra/quiky-analysis.json").read_text(
                encoding="utf-8"
            )
        )
        self.assertEqual(generate(closure), actual)

    def test_duplicate_symbol_is_rejected(self):
        source = ROOT / "research" / "ghidra" / "quiky-analysis.json"
        payload = json.loads(source.read_text(encoding="utf-8"))
        payload["symbols"].append(dict(payload["symbols"][0]))
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "manifest.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            with self.assertRaises(AnalysisError):
                load_manifest(path, ROOT)


if __name__ == "__main__":
    unittest.main()
