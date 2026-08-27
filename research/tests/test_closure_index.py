import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "research" / "tools"))

from verify_closure_index import verify  # noqa: E402


INDEX = ROOT / "research/ghidra/closure-index.json"


class ClosureIndexTests(unittest.TestCase):
    def test_index_is_valid_json(self):
        payload = json.loads(INDEX.read_text(encoding="utf-8"))
        self.assertEqual(payload["schema"], "quiky.research-closure-index.v1")

    def test_index_contracts_pass(self):
        verify(INDEX, ROOT)


if __name__ == "__main__":
    unittest.main()
