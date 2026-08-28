import json
import hashlib
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from player_parity_compare import compare  # noqa: E402
from w1l1_session_compare import compare as compare_session_gate  # noqa: E402
from player_replay_manifest import (  # noqa: E402
    ReplayManifestError,
    build_manifest,
    write_tsv,
)
from w1l1_session_parity import build_command  # noqa: E402
from quiky.parity import (  # noqa: E402
    canonical_global_write,
    canonical_probes,
    compare_player,
    player_parity_coverage,
)


ROOT = Path(__file__).resolve().parents[2]


class PlayerParityWorkflowTests(unittest.TestCase):
    def test_w1l1_effect_boundary_parity_is_pinned(self):
        evidence = ROOT / "research/evidence/player-dos-parity/player-w1l1-effect-parity-v1.json"
        payload = json.loads(evidence.read_text(encoding="utf-8"))
        source = payload["source"]
        for relative, key in (("game/QUIKY.EXE", "executable_sha256"),
                              ("game/NESTLE.DAT", "archive_sha256"),
                              ("research/automation/quiky_player_trace.lua", "trace_script_sha256")):
            digest = hashlib.sha256((ROOT / relative).read_bytes()).hexdigest()
            self.assertEqual(digest, source[key], relative)

        result = payload["result"]
        self.assertEqual(result["diagnostic_parity"], "pass")
        self.assertEqual(result["callbacks_checked"], 20)
        self.assertTrue(result["complete_player_records"])
        for field in ("normalized_input_mismatches", "post_record_mismatches",
                      "ordered_property_probe_mismatches",
                      "callback_global_write_mismatches", "known_effect_mismatches"):
            self.assertEqual(result[field], 0, field)
        self.assertEqual(result["effect_events"], [
            {"sequence": 2, "address": "01E7:0FCF", "code": "0x0000"}
        ])

    def test_w1l2_input_parity_evidence_records_closed_boundaries(self):
        evidence = ROOT / "research/evidence/player-dos-parity/player-w1l2-input-parity-v1.json"
        payload = json.loads(evidence.read_text(encoding="utf-8"))
        result = payload["result"]
        self.assertEqual(result["diagnostic_parity"], "pass")
        self.assertEqual(result["callbacks_checked"], 8)
        self.assertTrue(result["complete_player_records"])
        self.assertEqual(result["post_record_mismatches"], 0)
        self.assertEqual(result["ordered_property_probe_mismatches"], 0)
        self.assertEqual(result["callback_global_write_mismatches"], 0)

    def test_w1l2_landing_parity_evidence_records_arc_and_contact(self):
        evidence = ROOT / "research/evidence/player-dos-parity/player-w1l2-landing-parity-v1.json"
        payload = json.loads(evidence.read_text(encoding="utf-8"))
        result = payload["result"]
        self.assertEqual(result["diagnostic_parity"], "pass")
        self.assertEqual(result["callbacks_checked"], 20)
        self.assertEqual(result["post_record_mismatches"], 0)
        self.assertEqual(result["ordered_property_probe_mismatches"], 0)
        self.assertEqual(payload["capture"]["landing"]["post_mode"], "0x00")
        self.assertEqual(payload["capture"]["landing"]["post_vertical_velocity_fixed"], 0)

    def test_current_w1l1_landing_fixture_records_closed_arc(self):
        evidence = ROOT / "research/evidence/player-dos-parity/player-w1l1-jump-landing-parity-current-v1.json"
        payload = json.loads(evidence.read_text(encoding="utf-8"))
        result = payload["result"]
        self.assertEqual(result["diagnostic_parity"], "pass")
        self.assertEqual(result["callbacks_checked"], 20)
        self.assertTrue(result["complete_player_records"])
        self.assertEqual(result["ordered_property_probe_mismatches"], 0)
        self.assertEqual(result["callback_global_write_mismatches"], 0)
        self.assertEqual(payload["capture"]["landing"]["post_mode"], "0x00")
        self.assertEqual(payload["capture"]["landing"]["post_vertical_velocity_fixed"], 0)
        self.assertFalse(payload["capture"]["collision_trace_guards"])

    def test_committed_w1l1_dos_fixture_has_a_closed_candidate(self):
        source = ROOT / "research/evidence/player-dos-parity/w1l1-jump-property-v3.json"
        candidate = ROOT / "research/evidence/player-dos-parity/w1l1-jump-property-v3-candidate.json"
        manifest = build_manifest(source)
        self.assertEqual(len(manifest["rows"]), 10)
        self.assertEqual(manifest["rows"][1][1], "32")
        self.assertFalse(compare(source, candidate))

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

    def test_manifest_replays_the_static_normalized_input_word(self):
        source = ROOT / "research/build/player-followup-standing-v1.json"
        payload = json.loads(source.read_text(encoding="utf-8"))
        sample = payload["events"][0]["samples"][0]
        sample["globals"]["input_action_flags"] = 0x04
        sample["globals"]["keyboard_action_flags"] = 0x20
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "input.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            manifest = build_manifest(path)
        self.assertEqual(manifest["rows"][0][1], "36")

    def test_dispatch_bearing_fixture_uses_complete_v2_schema(self):
        source = ROOT / "research/build/traces/player-w1l1-callback-aligned-properties-v4.json"
        manifest = build_manifest(source)
        self.assertEqual(manifest["schema"], "quiky.player-replay-v2")
        self.assertEqual(len(manifest["fields"]), 34)
        self.assertEqual(len(manifest["rows"][0]), 34)
        self.assertEqual(manifest["rows"][0][31], "1")
        self.assertFalse(manifest["unmapped_globals"])

    def test_dispatch_bearing_fixture_fails_closed_when_one_field_is_missing(self):
        source = ROOT / "research/build/traces/player-w1l1-callback-aligned-properties-v4.json"
        payload = json.loads(source.read_text(encoding="utf-8"))
        del payload["events"][0]["samples"][0]["globals"]["dispatch_aux_4ff8"]
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "missing-dispatch.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            with self.assertRaises(ReplayManifestError):
                build_manifest(path)

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

    def test_input_comparison_uses_the_normalized_word(self):
        source = ROOT / "research/build/player-followup-standing-v1.json"
        payload = json.loads(source.read_text(encoding="utf-8"))
        sample = payload["events"][0]["samples"][0]
        sample["globals"]["input_action_flags"] = 0x04
        sample["globals"]["keyboard_action_flags"] = 0x20
        candidate = json.loads(json.dumps(payload))
        candidate["events"][0]["samples"][0]["player_callback"][
            "input_flags"] = 0x24
        with tempfile.TemporaryDirectory() as directory:
            original_path = Path(directory) / "original.json"
            candidate_path = Path(directory) / "candidate.json"
            original_path.write_text(json.dumps(payload), encoding="utf-8")
            candidate_path.write_text(json.dumps(candidate), encoding="utf-8")
            mismatches = compare(original_path, candidate_path)
        self.assertFalse(any(item["field"] == "input_flags" for item in mismatches))

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

    def test_probe_comparison_excludes_pre_callback_property_focus_event(self):
        sample = {
            "map_properties": [
                {"scope": "outside_player_callback", "coordinates": {"x": 1, "y": 2},
                 "raw_cell_word": 9, "descriptor_word": 0},
                {"scope": "player_callback", "coordinates": {"x": 3, "y": 4},
                 "raw_cell_word": 10, "descriptor_word": 0},
            ]
        }
        probes = canonical_probes(sample)
        self.assertEqual(len(probes), 1)
        self.assertEqual(probes[0]["x"], 3)
        self.assertEqual(probes[0]["y"], 4)

    def test_pending_event_global_write_has_address_contract(self):
        self.assertEqual(
            canonical_global_write({"field": "pending_event", "before": 1,
                                    "after": 0}),
            {"offset": 0x612E, "width": 2, "before": 1, "after": 0},
        )

    def test_complete_mode_rejects_missing_effect_and_global_arrays(self):
        source = ROOT / "research/build/player-followup-standing-v1.json"
        payload = json.loads(source.read_text(encoding="utf-8"))
        sample = payload["events"][0]["samples"][0]
        sample["player_callback"].pop("global_writes", None)
        candidate = json.loads(json.dumps(payload))
        with tempfile.TemporaryDirectory() as directory:
            original_path = Path(directory) / "original.json"
            candidate_path = Path(directory) / "candidate.json"
            original_path.write_text(json.dumps(payload), encoding="utf-8")
            candidate_path.write_text(json.dumps(candidate), encoding="utf-8")
            mismatches = compare(original_path, candidate_path,
                                 require_complete=True)
        self.assertTrue(any(item["field"] == "global_writes"
                            and "missing" in item.get("error", "")
                            for item in mismatches))

    def test_field_selective_mode_keeps_uninstrumented_fields_explicit(self):
        source = ROOT / "research/build/player-followup-standing-v1.json"
        payload = json.loads(source.read_text(encoding="utf-8"))
        candidate = json.loads(json.dumps(payload))
        candidate["events"][0]["samples"][0]["player_callback"]["effects"] = []
        with tempfile.TemporaryDirectory() as directory:
            original_path = Path(directory) / "original.json"
            candidate_path = Path(directory) / "candidate.json"
            original_path.write_text(json.dumps(payload), encoding="utf-8")
            candidate_path.write_text(json.dumps(candidate), encoding="utf-8")
            mismatches = compare_player(
                original_path, candidate_path, required_fields=["effects"]
            )
        self.assertFalse(any(item["field"] == "global_writes"
                             for item in mismatches))
        self.assertTrue(any(item["field"] == "effects"
                            and "missing" in item.get("error", "")
                            for item in mismatches))

    def test_player_parity_coverage_distinguishes_missing_from_empty(self):
        source = ROOT / "research/build/player-followup-standing-v1.json"
        payload = json.loads(source.read_text(encoding="utf-8"))
        sample = payload["events"][0]["samples"][0]
        sample["player_callback"]["effects"] = []
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "coverage.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            coverage = player_parity_coverage(path)
        self.assertEqual(coverage["schema"], "quiky.player-parity-coverage.v1")
        self.assertEqual(coverage["fields"]["effects"]["present"], 1)
        self.assertEqual(coverage["fields"]["factory_objects"]["missing"],
                         list(range(1, 9)))

    def test_w1l1_session_gate_injects_derived_ring_and_replay_inputs(self):
        command = build_command(
            Path("build/engine/quiky-w1l1-trace"),
            Path("game/NESTLE.DAT"),
            "W1L1.MAP",
            Path("/tmp/candidate.json"),
            frames=4,
            action_flags="0",
            input_tsv=None,
            leaf_prng_index=0,
            leaf_prng_ring_hex="00" * 0x100,
        )
        self.assertEqual(command[:4], [
            "build/engine/quiky-w1l1-trace",
            "game/NESTLE.DAT",
            "W1L1.MAP",
            "/tmp/candidate.json",
        ])
        self.assertIn("--frames", command)
        self.assertIn("--action-flags", command)
        self.assertEqual(command[-1], "00" * 0x100)

    def test_w1l1_session_gate_accepts_canonical_input_jsonl(self):
        command = build_command(
            Path("build/engine/quiky-w1l1-trace"),
            Path("game/NESTLE.DAT"), "W1L1.MAP", Path("/tmp/candidate.json"),
            frames=0, action_flags="0", input_tsv=None,
            leaf_prng_index=0, leaf_prng_ring_hex="00" * 0x100,
            input_jsonl=Path("run/input.jsonl"),
        )
        self.assertIn("--input-jsonl", command)
        self.assertNotIn("--input-tsv", command)

    def test_session_compat_gate_switches_to_checkpoints_for_sparse_trace(self):
        source = ROOT / "research/build/player-followup-standing-v1.json"
        payload = json.loads(source.read_text(encoding="utf-8"))
        payload["events"][0]["samples"] = payload["events"][0]["samples"][:1]
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / "sparse.json"
            candidate.write_text(json.dumps(payload), encoding="utf-8")
            mismatches, _ = compare_session_gate(source, candidate)
        self.assertTrue(mismatches)
        self.assertTrue(all("checkpoint" in item for item in mismatches))


if __name__ == "__main__":
    unittest.main()
