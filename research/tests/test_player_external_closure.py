import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "research" / "tools"))

from verify_player_external_closure import (  # noqa: E402
    REQUIRED_ADDRESSES,
    check_5937_dispatch_static,
    check_camera_map_refresh_static,
    check_contracts,
    check_death_recovery_static,
    check_natural_flagged_contact_evidence,
    check_natural_tile41_contact_evidence,
    check_negative_mode_second_probe_evidence,
    check_runtime_scheduler_membership,
    check_scheduler_lifecycle_static,
    check_scheduler_order,
    check_transition_effect_static,
    check_transition_writer_callback_evidence,
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

    def test_runtime_scheduler_membership_is_audited(self):
        check_runtime_scheduler_membership(self.ledger, ROOT)

    def test_scheduler_lifecycle_static_contract_is_audited(self):
        check_scheduler_lifecycle_static(self.ledger, ROOT)

    def test_death_recovery_static_closure_is_audited(self):
        check_death_recovery_static(self.ledger, ROOT)

    def test_5937_shared_dispatch_body_is_audited(self):
        check_5937_dispatch_static(self.ledger, ROOT)

    def test_transition_effect_relocated_closure_is_audited(self):
        check_transition_effect_static(self.ledger, ROOT)

    def test_transition_writer_callback_is_audited(self):
        check_transition_writer_callback_evidence(self.ledger, ROOT)

    def test_natural_flagged_contact_is_audited(self):
        check_natural_flagged_contact_evidence(self.ledger, ROOT)

    def test_natural_tile41_contact_is_audited(self):
        check_natural_tile41_contact_evidence(self.ledger, ROOT)

    def test_negative_mode_second_probe_is_audited(self):
        check_negative_mode_second_probe_evidence(self.ledger, ROOT)

    def test_camera_map_refresh_static_closure_is_audited(self):
        check_camera_map_refresh_static(self.ledger, ROOT)


if __name__ == "__main__":
    unittest.main()
