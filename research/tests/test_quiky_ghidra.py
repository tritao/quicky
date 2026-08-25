import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "research" / "tools"))

from quiky_ghidra import AnalysisError, load_manifest


class QuikyGhidraManifestTests(unittest.TestCase):
    def test_canonical_manifest_matches_executable(self):
        manifest = load_manifest(
            ROOT / "research" / "ghidra" / "quiky-analysis.json", ROOT
        )
        symbols = {(item["segment"], item["offset"]): item for item in manifest["symbols"]}
        self.assertEqual(symbols[(3, "3FF8")]["name"], "update_player_record")
        self.assertEqual(symbols[(6, "881A")]["name"], "player_object_offset")

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
