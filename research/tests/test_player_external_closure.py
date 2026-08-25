import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "research" / "tools"))

from verify_player_external_closure import (  # noqa: E402
    REQUIRED_ADDRESSES,
    check_contracts,
    check_scheduler_order,
    verify,
)


LEDGER = ROOT / "research/ghidra/player-external-state-closure.json"


class PlayerExternalClosureTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ledger = json.loads(LEDGER.read_text(encoding="utf-8"))

    def test_required_external_contracts_are_present(self):
        addresses = {item["address"] for item in self.ledger["functions"]}
        self.assertTrue(REQUIRED_ADDRESSES.issubset(addresses))
        check_contracts(self.ledger)

    def test_scheduler_pairs_are_ordered(self):
        check_scheduler_order(self.ledger)
        self.assertEqual(
            [item["order"] for item in self.ledger["static_order"]["main_loop_pairs"]],
            ["0E96 before 0FA2"] * 3,
        )

    def test_default_ledger_is_reproducible(self):
        verify(LEDGER, ROOT)


if __name__ == "__main__":
    unittest.main()
