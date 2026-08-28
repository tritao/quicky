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
from quiky.parity import (  # noqa: E402
    compare_player,
    compare_session,
    compare_session_checkpoints,
)
from quiky.runs import (  # noqa: E402
    RUN_SCHEMA,
    load_input_jsonl,
    new_run_manifest,
    save_input_jsonl,
    save_manifest,
    load_manifest,
    stage_run_files,
    validate_run_directory,
    verify_run_directory,
)
from quiky.trace import load_trace  # noqa: E402
from recorded_run_from_trace import input_rows  # noqa: E402


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


def lifecycle_sample(sequence: int, health: int, gate: int, mode: int,
                     x: int, y: int, *, native: bool) -> dict:
    state = bytearray.fromhex(record("00"))
    state[0x37] = mode & 0xff
    state_hex = state.hex()
    common = {
        "sequence": sequence,
        "scheduler_callbacks": [{"callback": {"offset": 0x3FF8}}],
        "entities": [],
    }
    if native:
        common.update({
            "player_record_hex": state_hex,
            "gameplay_state": {
                "current_health_8822": health,
                "lives_880a": 3,
                "transition_gate_89ea": gate,
                "terminal_x_8828": x,
                "terminal_y_882a": y,
            },
        })
    else:
        common.update({
            "globals": {
                "dispatch_health_8822": health,
                "dispatch_lives_880a": 3,
                "transition_mode": gate,
            },
            "player_callback": {
                "post_object": {
                    "state_hex": state_hex,
                    "player_byte_0x37": mode,
                    "position": {"x": x, "y": y},
                },
            },
        })
    return common


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

    def test_checkpoint_parity_matches_sparse_lifecycle_barriers(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            left = root / "dos.json"
            right = root / "native.json"
            states = [
                (3, 0, 0, 100, 200),
                (0, 0, 0, 101, 200),
                (0, -100, 255, 101, 200),
                (0, -350, 255, 101, 200),
                (3, 0, 0, 1673, 374),
            ]
            left.write_text(json.dumps({
                "events": [{"samples": [
                    lifecycle_sample(i, *state, native=False)
                    for i, state in enumerate(states, 1)
                ]}]
            }), encoding="utf-8")
            right.write_text(json.dumps({
                "samples": [
                    lifecycle_sample(i, *state, native=True)
                    for i, state in enumerate(states, 1)
                ]
            }), encoding="utf-8")
            mismatches, coverage = compare_session_checkpoints(left, right)
        self.assertEqual(mismatches, [])
        self.assertEqual(coverage, [])

    def test_checkpoint_parity_rejects_missing_recovery_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            left = root / "dos.json"
            right = root / "native.json"
            states = [(3, 0, 0, 100, 200), (0, 0, 255, 100, 200),
                      (3, 0, 0, 1673, 374)]
            left.write_text(json.dumps({"samples": [
                lifecycle_sample(i, *state, native=False)
                for i, state in enumerate(states, 1)
            ]}), encoding="utf-8")
            right.write_text(json.dumps({"samples": [
                lifecycle_sample(i, *state, native=True)
                for i, state in enumerate(states, 1)
            ]}), encoding="utf-8")
            mismatches, _ = compare_session_checkpoints(left, right)
        self.assertTrue(any(item.get("checkpoint") == "recovery_gate"
                            for item in mismatches))

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

    def test_canonical_input_stream_is_strict_and_round_trips(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "input.jsonl"
            rows = [
                {"sequence": 1, "guest_frame": 100,
                 "input_flags": 4, "camera": {"x": 0, "y": 262}},
                {"sequence": 2, "guest_frame": 101, "input_flags": 0},
            ]
            save_input_jsonl(path, rows)
            self.assertEqual(load_input_jsonl(path), rows)
            path.write_text(path.read_text(encoding="utf-8") + "\n",
                            encoding="utf-8")
            with self.assertRaisesRegex(Exception, "blank line"):
                load_input_jsonl(path)

    def test_named_run_validates_canonical_file_digests(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            trace = root / "trace.json"
            trace.write_text(json.dumps({"samples": [sample()]}),
                             encoding="utf-8")
            run = root / "w1l1-death-recovery"
            stage_run_files(
                run,
                input_rows=[{"sequence": 1, "input_flags": 0}],
                dos_state=trace,
                native_state=trace,
            )
            manifest = validate_run_directory(run)
            self.assertEqual(manifest["format_version"], 2)
            self.assertIn("input.jsonl", manifest["files"])
            (run / "native-state.json").write_text("tampered",
                                                    encoding="utf-8")
            with self.assertRaisesRegex(Exception, "digest mismatch"):
                validate_run_directory(run)

    def test_unified_frontend_validates_named_run(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            trace = root / "trace.json"
            trace.write_text(json.dumps({"samples": [sample()]}),
                             encoding="utf-8")
            run = root / "run"
            stage_run_files(run, input_rows=[{"sequence": 1,
                                              "input_flags": 0}],
                            native_state=trace)
            completed = subprocess.run(
                [sys.executable, str(TOOLS / "quiky.py"), "run", "validate",
                 str(run)], cwd=ROOT, text=True, capture_output=True,
                check=False)
            self.assertEqual(completed.returncode, 0, completed.stderr)

    def test_named_run_verify_writes_parity_and_coverage(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            trace = root / "trace.json"
            trace.write_text(json.dumps({"samples": [sample()]}),
                             encoding="utf-8")
            run = root / "run"
            stage_run_files(run, input_rows=[{"sequence": 1,
                                              "input_flags": 0}],
                            dos_state=trace, native_state=trace)
            mismatches, coverage = verify_run_directory(run)
            self.assertEqual(mismatches, [])
            self.assertEqual(coverage, [])
            self.assertEqual(json.loads((run / "parity.json").read_text())[
                "status"], "pass")
            validate_run_directory(run)

    def test_raw_trace_import_materializes_explicit_input_rows(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            trace = root / "trace.json"
            trace.write_text(json.dumps({"samples": [sample()]}),
                             encoding="utf-8")
            rows = input_rows(trace)
            self.assertEqual(rows, [{
                "sequence": 1, "guest_frame": 1, "input_flags": 0,
                "camera": {"x": 4, "y": 8},
            }])


if __name__ == "__main__":
    unittest.main()
