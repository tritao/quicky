import json
import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from entity_behavior_report import (
    DIMENSIONS, MatrixError, load_matrix, summarize, validate_matrix,
)
from quikyctl import build_are_type_catalog


class EntityBehaviorReportTests(unittest.TestCase):
    def test_repository_matrix_covers_all_used_archive_types(self):
        repo_root = Path(__file__).resolve().parents[2]
        matrix = load_matrix(repo_root / "research" / "entity-behavior-families.json")
        used_types = {
            f"0x{entry.entity_type:02X}"
            for entry in build_are_type_catalog(repo_root / "game" / "NESTLE.DAT")
        }
        matrix_types = {entity_type for family in matrix["families"]
                        for entity_type in family["types"]}
        self.assertEqual(matrix_types, used_types)
        self.assertEqual(len(matrix["families"]), 21)
        self.assertEqual(len(matrix_types), 50)

    def test_every_dimension_has_explicit_status_and_evidence(self):
        repo_root = Path(__file__).resolve().parents[2]
        matrix = load_matrix(repo_root / "research" / "entity-behavior-families.json")
        for family in matrix["families"]:
            self.assertEqual(set(DIMENSIONS), {
                key for key in family if key in DIMENSIONS
            })
            for dimension in DIMENSIONS:
                self.assertIn(family[dimension]["status"], {
                    "confirmed", "partial", "unresolved", "not_applicable"
                })
                self.assertTrue(family[dimension]["known"].strip())

    def test_family_worlds_match_archive_catalog(self):
        repo_root = Path(__file__).resolve().parents[2]
        matrix = load_matrix(repo_root / "research" / "entity-behavior-families.json")
        catalog = build_are_type_catalog(repo_root / "game" / "NESTLE.DAT")
        worlds_by_type = {
            f"0x{entry.entity_type:02X}": {
                level[:2].upper() for level in entry.levels
            }
            for entry in catalog
        }
        for family in matrix["families"]:
            expected = set().union(*(worlds_by_type[entity_type]
                                     for entity_type in family["types"]))
            self.assertEqual(set(family["worlds"]), expected, family["id"])

    def test_summary_counts_are_machine_readable(self):
        repo_root = Path(__file__).resolve().parents[2]
        summary = summarize(load_matrix(
            repo_root / "research" / "entity-behavior-families.json"
        ))
        self.assertEqual(summary["family_count"], 21)
        self.assertEqual(summary["type_count"], 50)
        self.assertEqual(
            summary["dimension_status_counts"]["update_callback_state_machine"]["confirmed"],
            11,
        )
        self.assertGreater(
            summary["dimension_status_counts"]["movement_ai"]["partial"], 0
        )

    def test_validator_rejects_missing_dimension(self):
        repo_root = Path(__file__).resolve().parents[2]
        payload = json.loads(
            (repo_root / "research" / "entity-behavior-families.json").read_text()
        )
        del payload["families"][0]["animation_table"]
        with self.assertRaises(MatrixError):
            validate_matrix(payload)

    def test_validator_rejects_duplicate_type(self):
        repo_root = Path(__file__).resolve().parents[2]
        payload = json.loads(
            (repo_root / "research" / "entity-behavior-families.json").read_text()
        )
        payload["families"][1]["types"].append("0x01")
        with self.assertRaises(MatrixError):
            validate_matrix(payload)


if __name__ == "__main__":
    unittest.main()
