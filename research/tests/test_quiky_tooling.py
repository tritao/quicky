import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1] / "tools"
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(TOOLS))

from quiky.runs import (RUN_SCHEMA, load_input_jsonl, save_input_jsonl,
                        stage_run_files, validate_run_directory,
                        verify_run_directory)
from quiky.state import (STATE_SCHEMA, compare_state, import_trace,
                         load_state_jsonl, save_state_jsonl)


def record(seed: str) -> str:
    return (seed * (0x78 * 2 // len(seed) + 1))[:0x78 * 2]


def trace_sample(sequence: int = 1) -> dict:
    return {"sequence": sequence,
            "globals": {"keyboard_action_flags": 0, "camera_x": 4, "camera_y": 8},
            "player_callback": {
                "pre_object": {"state_hex": record("00")},
                "post_object": {"state_hex": record("01")},
                "collisions": [], "global_writes": [], "effects": []},
            "scheduler_callbacks": [{"callback": {"offset": 0x3FF8}}],
            "entities": []}


class QuikyToolingTests(unittest.TestCase):
    def test_historical_shapes_exist_only_at_import(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            trace = root / "trace.json"
            trace.write_text(json.dumps({"events": [{"samples": [trace_sample()]}]}),
                             encoding="utf-8")
            state = root / "state.jsonl"
            save_state_jsonl(state, import_trace(trace, "exact"))
            rows = load_state_jsonl(state)
        self.assertEqual(rows[0]["schema"], STATE_SCHEMA)
        self.assertEqual(rows[0]["camera"], {"x": 4, "y": 8})
        self.assertNotIn("player_callback", rows[0])

    def test_state_reader_rejects_trace_envelopes(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "state.jsonl"
            path.write_text(json.dumps({"samples": [trace_sample()]}) + "\n",
                            encoding="utf-8")
            with self.assertRaisesRegex(Exception, "schema"):
                load_state_jsonl(path)

    def test_profiles_are_explicit(self):
        row = {"schema": STATE_SCHEMA, "sequence": 1,
               "checkpoint": "terminal_damage", "lifecycle": {"health": 0}}
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            expected, actual = root / "expected.jsonl", root / "actual.jsonl"
            save_state_jsonl(expected, [row])
            save_state_jsonl(actual, [dict(row, sequence=20)])
            self.assertFalse(compare_state(expected, actual, "lifecycle")[0])
            self.assertTrue(compare_state(expected, actual, "exact")[0])

    def test_input_stream_is_strict(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "input.jsonl"
            rows = [{"sequence": 1, "guest_frame": 100, "input_flags": 4,
                     "camera": {"x": 0, "y": 262}}]
            save_input_jsonl(path, rows)
            self.assertEqual(load_input_jsonl(path), rows)
            path.write_text(path.read_text() + "\n", encoding="utf-8")
            with self.assertRaisesRegex(Exception, "blank line"):
                load_input_jsonl(path)

    def test_named_run_verifies_canonical_state(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            state = root / "state.jsonl"
            save_state_jsonl(state, [{"schema": STATE_SCHEMA, "sequence": 1,
                                      "pre_record": record("00"),
                                      "post_record": record("01")}])
            run = root / "run"
            stage_run_files(run, name="smoke", profile="exact",
                            input_rows=[{"sequence": 1, "input_flags": 0}],
                            expected_state=state, actual_state=state)
            self.assertEqual(validate_run_directory(run)["schema"], RUN_SCHEMA)
            self.assertEqual(verify_run_directory(run), ([], []))
            validate_run_directory(run)

    def test_unified_cli_imports_and_verifies(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            trace = root / "trace.json"
            trace.write_text(json.dumps({"samples": [trace_sample()]}), encoding="utf-8")
            run = root / "run"
            imported = subprocess.run(
                [sys.executable, str(TOOLS / "quiky.py"), "run", "import", str(run),
                 "--name", "smoke", "--profile", "exact",
                 "--expected-trace", str(trace), "--actual-trace", str(trace)],
                cwd=ROOT, text=True, capture_output=True, check=False)
            self.assertEqual(imported.returncode, 0, imported.stderr)
            verified = subprocess.run(
                [sys.executable, str(TOOLS / "quiky.py"), "run", "verify", str(run)],
                cwd=ROOT, text=True, capture_output=True, check=False)
            self.assertEqual(verified.returncode, 0, verified.stderr)


if __name__ == "__main__":
    unittest.main()
