import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "research" / "tools"))

from verify_biene_runtime_table_static import (  # noqa: E402
    check_contracts,
    check_runner,
    verify,
)


LEDGER = ROOT / "research/ghidra/biene-runtime-table-static-closure.json"


class BieneRuntimeTableStaticTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ledger = json.loads(LEDGER.read_text(encoding="utf-8"))

    def test_contracts_cover_builder_and_float_boundary(self):
        check_contracts(self.ledger)

    def test_runner_is_ghidra_only_and_exports_coefficients(self):
        check_runner(self.ledger, ROOT)

    def test_default_verification_passes(self):
        verify(LEDGER, ROOT)


if __name__ == "__main__":
    unittest.main()
