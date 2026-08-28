import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

TOOLS = Path(__file__).resolve().parents[1] / "tools"
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(TOOLS))

from quiky.runs import (RUN_SCHEMA, load_input_jsonl, save_input_jsonl,
                        replay_run, stage_run_files, validate_run_directory,
                        verify_run_directory)
from quiky.common import file_fingerprint, write_json
from quiky.capture import capture_session
from quiky.capture_stream import (append as append_capture_record,
                                  read as read_capture_stream)
from quiky.state import (CHECKPOINTS, STATE_SCHEMA, compare_state, import_trace,
                         label_lifecycle, load_state_jsonl, save_state_jsonl)
from quiky.trace_import import global_writes


def record(seed: str) -> str:
    return (seed * (0x78 * 2 // len(seed) + 1))[:0x78 * 2]


def trace_sample(sequence: int = 1) -> dict:
    return {"sequence": sequence,
            "globals": {"keyboard_action_flags": 0, "input_action_flags": 0,
                        "camera_x": 4, "camera_y": 8},
            "player_callback": {
                "pre_object": {"state_hex": record("00")},
                "post_object": {"state_hex": record("01")},
                "collisions": [], "global_writes": [], "effects": []},
            "scheduler_callbacks": [{"callback": {"offset": 0x3FF8}}],
            "entities": []}


def capture(*samples: dict) -> dict:
    return {"schema": "quiky-player-dos-parity-v1",
            "source_trace": "test", "trace_kind": "player_callback",
            "events": [{"samples": list(samples)}]}


class QuikyToolingTests(unittest.TestCase):
    def test_capture_stream_round_trips_and_recovers_complete_frames(self):
        with tempfile.TemporaryDirectory() as directory:
            stream_path = Path(directory) / "capture.qcap"
            with stream_path.open("wb") as stream:
                append_capture_record(stream, trace_sample(1))
                append_capture_record(stream, trace_sample(2))
            self.assertEqual([row["sequence"] for row in read_capture_stream(
                stream_path)], [1, 2])
            stream_path.write_bytes(stream_path.read_bytes()[:-3])
            self.assertEqual([row["sequence"] for row in read_capture_stream(
                stream_path, tolerate_truncated_tail=True)], [1])

    def test_current_capture_imports_to_canonical_state(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            trace = root / "trace.json"
            trace.write_text(json.dumps(capture(trace_sample())),
                             encoding="utf-8")
            state = root / "state.jsonl"
            save_state_jsonl(state, import_trace(trace, "exact"))
            rows = load_state_jsonl(state)
        self.assertEqual(rows[0]["schema"], STATE_SCHEMA)
        self.assertEqual(rows[0]["camera"], {"x": 4, "y": 8})
        self.assertNotIn("player_callback", rows[0])

    def test_import_rejects_unversioned_trace_envelope(self):
        with tempfile.TemporaryDirectory() as directory:
            trace = Path(directory) / "trace.json"
            trace.write_text(json.dumps({"samples": [trace_sample()]}), encoding="utf-8")
            with self.assertRaisesRegex(Exception, "quiky-player-dos-parity-v1"):
                import_trace(trace, "exact")

    def test_active_capture_reproduces_committed_expected_state(self):
        capture_path = (ROOT / "research/evidence/player-dos-parity" /
                        "w1l1-jump-property-v3.json")
        imported = import_trace(capture_path, "exact")
        expected = load_state_jsonl(
            ROOT / "research/runs/w1l1-jump/expected-state.jsonl")
        self.assertEqual(imported, expected)

    def test_state_reader_rejects_trace_envelopes(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "state.jsonl"
            path.write_text(json.dumps({"samples": [trace_sample()]}) + "\n",
                            encoding="utf-8")
            with self.assertRaisesRegex(Exception, "schema"):
                load_state_jsonl(path)

    def test_profiles_are_explicit(self):
        row = {"schema": STATE_SCHEMA, "sequence": 1,
               "checkpoints": list(CHECKPOINTS), "lifecycle": {"health": 0}}
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            expected, actual = root / "expected.jsonl", root / "actual.jsonl"
            save_state_jsonl(expected, [row])
            save_state_jsonl(actual, [dict(row, sequence=20)])
            self.assertFalse(compare_state(expected, actual, "lifecycle")[0])
            self.assertTrue(compare_state(expected, actual, "exact")[0])

    def test_lifecycle_requires_every_checkpoint(self):
        row = {"schema": STATE_SCHEMA, "sequence": 1,
               "lifecycle": {"health": 3}}
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            expected, actual = root / "expected.jsonl", root / "actual.jsonl"
            save_state_jsonl(expected, [row])
            save_state_jsonl(actual, [row])
            mismatches, _ = compare_state(expected, actual, "lifecycle")
        self.assertEqual([item["checkpoint"] for item in mismatches],
                         list(CHECKPOINTS))

    def test_coincident_lifecycle_checkpoints_are_preserved(self):
        rows = [
            {"schema": STATE_SCHEMA, "sequence": 1,
             "lifecycle": {"health": 3, "gate": 0, "mode": 0}},
            {"schema": STATE_SCHEMA, "sequence": 2,
             "lifecycle": {"health": 0, "gate": -1, "mode": 255}},
        ]
        label_lifecycle(rows)
        self.assertEqual(rows[1]["checkpoints"],
                         ["terminal_damage", "death_hold"])

    def test_nested_state_shape_is_strict(self):
        row = {"schema": STATE_SCHEMA, "sequence": 1,
               "global_writes": [{"offset": 1, "width": 3,
                                    "before": 0, "after": 1}]}
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(Exception, "width is invalid"):
                save_state_jsonl(Path(directory) / "state.jsonl", [row])

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
            files_before = sorted(path.name for path in run.iterdir())
            self.assertEqual(verify_run_directory(run), ([], []))
            self.assertEqual(sorted(path.name for path in run.iterdir()), files_before)
            refreshed = validate_run_directory(run)
            self.assertNotIn("parity.json", refreshed["files"])
            self.assertNotIn("coverage.json", refreshed["files"])

    def test_manifest_fingerprint_size_is_enforced(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            state = root / "state.jsonl"
            save_state_jsonl(state, [{"schema": STATE_SCHEMA, "sequence": 1}])
            run = root / "run"
            stage_run_files(run, name="strict", profile="exact",
                            input_rows=[{"sequence": 1, "input_flags": 0}],
                            expected_state=state)
            manifest_path = run / "manifest.json"
            manifest = json.loads(manifest_path.read_text())
            manifest["files"]["input.jsonl"]["size"] += 1
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            with self.assertRaisesRegex(Exception, "digest mismatch"):
                validate_run_directory(run)

    def test_failed_replay_does_not_publish_recipe_or_state(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            state = root / "state.jsonl"
            archive = root / "archive.dat"
            archive.write_bytes(b"archive")
            save_state_jsonl(state, [{"schema": STATE_SCHEMA, "sequence": 1}])
            run = root / "run"
            stage_run_files(run, name="transaction", profile="exact",
                            input_rows=[{"sequence": 1, "input_flags": 0}],
                            expected_state=state)
            with self.assertRaisesRegex(Exception, "native replay failed"):
                replay_run(run, binary=Path("/bin/false"), archive=archive,
                           map_resource="W1L1.MAP")
            manifest = validate_run_directory(run)
            self.assertIsNone(manifest["replay"])
            self.assertFalse((run / "actual-state.jsonl").exists())

    def test_unified_cli_imports_a_run(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            trace = root / "trace.json"
            trace.write_text(json.dumps(capture(trace_sample())), encoding="utf-8")
            run = root / "run"
            imported = subprocess.run(
                [sys.executable, str(TOOLS / "quiky.py"), "run", "import", str(run),
                 "--name", "smoke", "--profile", "exact",
                 "--expected-trace", str(trace)],
                cwd=ROOT, text=True, capture_output=True, check=False)
            self.assertEqual(imported.returncode, 0, imported.stderr)
            validated = subprocess.run(
                [sys.executable, str(TOOLS / "quiky.py"), "run", "validate", str(run)],
                cwd=ROOT, text=True, capture_output=True, check=False)
            self.assertEqual(validated.returncode, 0, validated.stderr)

    def test_committed_run_verifies(self):
        mismatches, coverage = verify_run_directory(
            ROOT / "research/runs/w1l1-jump")
        self.assertEqual(mismatches, [])
        self.assertTrue(coverage)

    def test_completed_capture_processes_into_a_run(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            captured = root / "capture"
            captured.mkdir()
            source = captured / "capture.qcap"
            with source.open("wb") as stream:
                append_capture_record(stream, trace_sample())
            fingerprint = file_fingerprint(source)
            fingerprint["path"] = "capture.qcap"
            write_json(captured / "manifest.json", {
                "schema": "quiky.capture-session-v1", "format_version": 1,
                "name": "played", "status": "complete", "level": "W1L1",
                "files": {"capture.qcap": fingerprint},
            })
            run = root / "run"
            completed = subprocess.run([
                sys.executable, str(TOOLS / "quiky.py"), "capture", "process",
                str(captured), "--run", str(run),
            ], cwd=ROOT, text=True, capture_output=True, check=False)
            self.assertEqual(completed.returncode, 0, completed.stderr)
            self.assertEqual(validate_run_directory(run)["name"], "run")

    def test_incomplete_capture_recovers_complete_binary_frames(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            captured = root / "capture"
            captured.mkdir()
            source = captured / "capture.partial.qcap"
            with source.open("wb") as stream:
                append_capture_record(stream, trace_sample())
                append_capture_record(stream, trace_sample(2))
            source.write_bytes(source.read_bytes()[:-3])
            fingerprint = file_fingerprint(source)
            fingerprint["path"] = source.name
            write_json(captured / "manifest.json", {
                "schema": "quiky.capture-session-v1", "format_version": 1,
                "name": "played", "status": "incomplete", "level": "W1L1",
                "files": {source.name: fingerprint},
            })
            run = root / "run"
            completed = subprocess.run([
                sys.executable, str(TOOLS / "quiky.py"), "capture", "process",
                str(captured), "--run", str(run), "--recover-incomplete",
            ], cwd=ROOT, text=True, capture_output=True, check=False)
            self.assertEqual(completed.returncode, 0, completed.stderr)
            self.assertEqual(validate_run_directory(run)["name"], "run")

    def test_interactive_capture_has_one_command_happy_path(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)

            def complete(command, **_kwargs):
                output = Path(command[command.index("--output") + 1])
                with output.open("wb") as stream:
                    append_capture_record(stream, trace_sample())
                self.assertIn("--interactive-capture", command)
                self.assertIn("--player-minimal-callback-capture", command)
                self.assertIn("--player-record-input-stream", command)
                self.assertNotIn("--headless", command)
                return SimpleNamespace(returncode=0)

            with patch("quiky.capture.subprocess.run", side_effect=complete):
                captured, run = capture_session(
                    name="played", level="W1L1", runtime_dir=Path("game"),
                    profile="exact", capture_only=False,
                    captures_root=root / "captures", runs_root=root / "runs")
            self.assertEqual(captured.name, "played")
            self.assertEqual(run, root / "runs/played")
            self.assertEqual(validate_run_directory(run)["name"], "played")

    def test_diagnostic_capture_keeps_property_samples(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)

            def complete(command, **_kwargs):
                output = Path(command[command.index("--output") + 1])
                with output.open("wb") as stream:
                    append_capture_record(stream, trace_sample())
                self.assertIn("--player-property-focus", command)
                self.assertNotIn("--player-minimal-callback-capture", command)
                return SimpleNamespace(returncode=0)

            with patch("quiky.capture.subprocess.run", side_effect=complete):
                captured, run = capture_session(
                    name="diagnostic", level="W1L1", runtime_dir=Path("game"),
                    profile="exact", capture_only=True, diagnostic=True,
                    captures_root=root / "captures", runs_root=root / "runs")
            self.assertIsNone(run)
            manifest = json.loads((captured / "manifest.json").read_text())
            self.assertEqual(manifest["capture_mode"], "diagnostic")

    def test_diagnostic_input_latch_writes_are_not_replay_state(self):
        sample = trace_sample()
        sample["player_callback"]["global_writes"] = [
            {"field": "keyboard_action_flags", "before": 0, "after": 4},
            {"field": "timer_clear", "before": 9, "after": 0},
        ]
        self.assertEqual(global_writes(sample), [{
            "before": 9, "after": 0, "offset": 0x8810, "width": 2,
        }])


if __name__ == "__main__":
    unittest.main()
