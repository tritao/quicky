import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from player_trace_matrix import (  # noqa: E402
    AssertionFailure,
    Experiment,
    ExperimentCatalog,
    FocusSpec,
    RunnerConfig,
    assert_expected_outputs,
    build_player_config,
    compare_traces,
    compact_trace,
    extract_trace_fields,
    load_catalog,
    normalize_frame_timeline,
    output_values,
    render_markdown_summary,
    ExperimentRunner,
)


FIXTURE = Path(__file__).resolve().parent / "fixtures/player_trace_minimal.json"


class PlayerTraceMatrixTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw = json.loads(FIXTURE.read_text(encoding="utf-8"))
        cls.normalized = compact_trace(cls.raw)

    def test_normalizes_lua_arrays_and_orders_by_frame_then_sequence(self):
        samples = normalize_frame_timeline(self.raw)
        self.assertEqual([sample["sequence"] for sample in samples], [1, 2])
        self.assertEqual(samples[0]["frame_index"], 0)
        self.assertEqual(samples[1]["frame_index"], 5)
        self.assertEqual(samples[0]["breakpoint_owners"], ["callback"])

    def test_extracts_player_callback_fields_and_events(self):
        frames = extract_trace_fields(normalize_frame_timeline(self.raw))
        first, second = frames
        self.assertEqual(first["position"]["before"]["x"], 128)
        self.assertEqual(first["position"]["after"]["x"], 129)
        self.assertEqual(second["velocity"]["after"]["velocity_x_fixed"], 131072)
        self.assertEqual(second["action_state"]["after"]["phase"], 3)
        self.assertEqual(second["callback_writes"]["record"][0]["offset"], 3)
        self.assertEqual(second["globals"]["after"]["player_control_word"], 2)
        self.assertEqual(second["collision_events"][0]["tile_id"], 256)
        self.assertEqual(second["map_cells"][0]["property"], 1)
        self.assertEqual(second["descriptor_properties"][0]["descriptor_word"], 48)

    def test_exact_and_transition_assertions(self):
        result = assert_expected_outputs(self.normalized, {
            "exact": {
                "frames[0].position.before.x": 128,
                "frames[1].callback_writes.record[0].offset": 3,
            },
            "transitions": [
                {"path": "position.after.x", "from": 129, "to": 130, "at": 1},
            ],
        })
        self.assertTrue(result["passed"])
        self.assertEqual(output_values(self.normalized, "frames[*].position.after.x"), [129, 130])
        with self.assertRaises(AssertionFailure):
            assert_expected_outputs(self.normalized, {"exact": {"frames[0].position.before.x": 999}})

    def test_comparison_reports_field_level_difference(self):
        changed = json.loads(json.dumps(self.normalized))
        changed["frames"][1]["position"]["after"]["x"] = 131
        comparison = compare_traces(self.normalized, changed)
        self.assertFalse(comparison["equal"])
        self.assertTrue(any("position.after.x" in item["path"] for item in comparison["changes"]))

    def test_experiment_schema_maps_to_stable_player_config(self):
        experiment = Experiment.from_dict({
            "name": "right-step",
            "category": "horizontal",
            "level": "W1L1",
            "sample_count": 2,
            "input_phases": ["KBD_right:5", "WAIT:2"],
            "execute_watches": ["0x1f7:0x3df2"],
            "focus": {"kind": "object", "callback_offset": "0x3ff8", "object_offset": "0x78"},
            "reversible_patches": ["player:0x3e:u16=0"],
        })
        config = build_player_config(experiment, Path("startup.json"))
        self.assertEqual(config.samples, 2)
        self.assertEqual(config.select_level, "W1L1")
        self.assertEqual(config.input_phases[0].keys, ("KBD_right",))
        self.assertEqual(config.execute_watches[0].offset, 0x3DF2)
        self.assertEqual(config.object_focus.object_offset, 0x78)
        self.assertEqual(config.patches[0].value, 0)

    def test_catalog_supports_named_suites(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "experiments.json"
            path.write_text(json.dumps({
                "experiments": [
                    {"name": "a", "category": "movement", "sample_count": 1},
                    {"name": "b", "category": "collision", "sample_count": 1},
                ],
                "suites": {"smoke": ["a", "b"]},
            }), encoding="utf-8")
            catalog = load_catalog(path)
        self.assertIsInstance(catalog, ExperimentCatalog)
        self.assertEqual([item.name for item in catalog.select(suite="smoke")], ["a", "b"])

    def test_summary_is_useful_for_failed_or_completed_artifacts(self):
        experiment = Experiment("fixture", "unit", "W1L1", 2)
        summary = render_markdown_summary(experiment, self.normalized, status="passed")
        self.assertIn("# fixture", summary)
        self.assertIn("| Sequence | Frame |", summary)
        self.assertIn("| 1 | 0 |", summary)

    def test_suite_keeps_failed_experiments_rerunnable_and_writes_manifests(self):
        class FakeApi:
            def get(self, path):
                if path != "/api/v1/dosbox/info":
                    raise AssertionError(path)
                return {"features": {"debugger": True}}

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            runner = ExperimentRunner(RunnerConfig(
                repo_root=Path(__file__).resolve().parents[2],
                output_root=root / "runs",
                runtime_dir=root,
                startup_recording=FIXTURE,
            ), api=FakeApi())
            good = Experiment.from_dict({
                "name": "good", "category": "unit", "sample_count": 2,
                "expected_outputs": {"exact": {"frames[0].position.before.x": 128}},
            })
            bad = Experiment.from_dict({
                "name": "bad", "category": "unit", "sample_count": 2,
                "expected_outputs": {"exact": {"frames[0].position.before.x": 999}},
            })
            with patch("player_trace_matrix.trace_player_lua", return_value=(self.raw, [])):
                results = runner.run_suite([good, bad])
            self.assertEqual([result.status for result in results], ["passed", "failed"])
            self.assertTrue(results[0].normalized_path.is_file())
            failed_manifest = json.loads(results[1].manifest_path.read_text(encoding="utf-8"))
            self.assertEqual(failed_manifest["status"], "failed")
            self.assertEqual(failed_manifest["error"]["type"], "AssertionFailure")
            self.assertEqual(failed_manifest["experiment"]["name"], "bad")


if __name__ == "__main__":
    unittest.main()
