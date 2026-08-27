import copy
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1] / "tools"
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(TOOLS))

from quiky.coverage import coverage_for_trace  # noqa: E402
from quiky.parity import compare_player, compare_session  # noqa: E402
from quiky.runs import RUN_SCHEMA, new_run_manifest, save_manifest, load_manifest  # noqa: E402
from quiky.trace import load_trace  # noqa: E402


def record(seed: str) -> str:
    return (seed * (0x78 * 2 // len(seed) + 1))[:0x78 * 2]


def sample(sequence: int = 1) -> dict:
    return {
        "sequence": sequence,
        "globals": {"keyboard_action_flags": 0, "camera_x": 4, "camera_y": 8},
        "player_callback": {
            "pre_object": {"state_hex": record("00")},
            "post_object": {"state_hex": record("01")},
            "collisions": [],
            "global_writes": [],
            "effects": [],
        },
        "scheduler_callbacks": [{"callback": {"offset": 0x3FF8}}],
        "entities": [],
    }


class QuikyToolingTests(unittest.TestCase):
    def test_trace_adapter_accepts_both_historical_envelopes(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            top = root / "top.json"
            dos = root / "dos.json"
            payload = {"samples": [sample()]}
            top.write_text(json.dumps(payload), encoding="utf-8")
            dos.write_text(json.dumps({"events": [{"samples": [sample()]}]}),
                           encoding="utf-8")
            top_trace = load_trace(top)
            dos_trace = load_trace(dos)
        self.assertEqual(top_trace.shape, "samples")
        self.assertEqual(dos_trace.shape, "events[0].samples")
        self.assertEqual(top_trace.by_sequence[1].camera, (4, 8))
        self.assertEqual(top_trace.by_sequence[1].pre_record_hex, record("00"))

    def test_shared_parity_preserves_player_and_session_contracts(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            left = root / "left.json"
            right = root / "right.json"
            payload = {"samples": [sample()]}
            left.write_text(json.dumps(payload), encoding="utf-8")
            right.write_text(json.dumps(copy.deepcopy(payload)), encoding="utf-8")
            self.assertEqual(compare_player(left, right), [])
            mismatches, coverage = compare_session(left, right)
        self.assertEqual(mismatches, [])
        self.assertEqual(coverage, [])

    def test_coverage_and_manifest_are_dependency_free(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            trace = root / "trace.json"
            manifest_path = root / "run.json"
            trace.write_text(json.dumps({"samples": [sample()]}), encoding="utf-8")
            coverage = coverage_for_trace(trace)
            manifest = new_run_manifest("smoke", native_trace=trace)
            save_manifest(manifest_path, manifest)
            loaded = load_manifest(manifest_path)
        self.assertEqual(coverage["callbacks"], [{"offset": 0x3FF8, "count": 1}])
        self.assertEqual(loaded["schema"], RUN_SCHEMA)
        self.assertEqual(loaded["traces"]["native"]["sha256"], manifest["traces"]["native"]["sha256"])

    def test_unified_frontend_dispatches_legacy_parity(self):
        source = ROOT / "research/build/player-followup-standing-v1.json"
        command = [
            sys.executable,
            str(TOOLS / "quiky.py"),
            "parity", "player",
            "--original", str(source),
            "--candidate", str(source),
        ]
        completed = subprocess.run(command, cwd=ROOT, text=True,
                                   capture_output=True, check=False)
        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn("OK: player callback parity", completed.stdout)


if __name__ == "__main__":
    unittest.main()
