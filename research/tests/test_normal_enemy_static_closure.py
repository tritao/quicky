import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "research" / "tools"))

from verify_normal_enemy_static_closure import (  # noqa: E402
    check_contracts,
    check_runner,
    check_source,
    verify,
)


LEDGER = ROOT / "research/ghidra/normal-enemy-static-closure.json"


class NormalEnemyStaticClosureTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ledger = json.loads(LEDGER.read_text(encoding="utf-8"))

    def test_contracts_cover_all_remaining_families(self):
        check_source(self.ledger, ROOT)
        check_contracts(self.ledger, ROOT)

    def test_runner_is_bounded_ghidra_pipeline(self):
        check_runner(self.ledger, ROOT)

    def test_default_verification_passes(self):
        verify(LEDGER, ROOT)


if __name__ == "__main__":
    unittest.main()
