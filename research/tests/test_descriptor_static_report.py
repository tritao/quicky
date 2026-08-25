import unittest
import json
from pathlib import Path
import sys

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from descriptor_static_report import build_report  # noqa: E402


class DescriptorStaticReportTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        executable = Path(__file__).resolve().parents[2] / "game" / "QUIKY.EXE"
        cls.report = build_report(executable)

    def test_table_schema_and_identity_records(self):
        construction = self.report["construction"]
        self.assertEqual(construction["static_decomp"]["entry"], "01E7:382B")
        self.assertIn("publishes the far pointer", construction["static_decomp"]["operations"][0])
        self.assertEqual(construction["table_allocation"]["allocation_bytes"], 0x800)
        self.assertEqual(construction["world_dispatch"]["selector_global"], "DS:85D8")
        self.assertIn("DS:6D86", construction["world_dispatch"]["post_initializer"])
        self.assertEqual(self.report["mapping"]["tile_id_mask"], "0x01ff")
        self.assertEqual(self.report["mapping"]["address_formula"],
                         "DS:6582 + (cell & 0x01ff) * DS:30D4")
        for world, initializer in self.report["initializers"].items():
            records = initializer["records"]
            self.assertEqual(len(records), 512, world)
            for tile_id, record in enumerate(records):
                self.assertEqual(record["tile_id"], tile_id)
                self.assertEqual(record["tile_index"], tile_id)
                self.assertEqual(record["record_offset"], tile_id * 4)

    def test_descriptor_field_consumers(self):
        fields = {field["offset"]: field
                  for field in self.report["initializers"]["W1"]["table"]["fields"]}
        self.assertEqual(fields[0]["consumers"], ["01F7:20C8", "01F7:2CB2"])
        self.assertIn("0x100-byte tile image block", fields[0]["meaning"])
        self.assertEqual(fields[2]["consumers"], ["01F7:5C27", "01F7:5CC3"])

    def test_world_specific_flag_ladders(self):
        w1 = self.report["initializers"]["W1"]["records"]
        self.assertEqual(w1[0x2A]["flags"], 0x70)
        self.assertEqual(w1[0x2B]["flags"], 0x30)
        self.assertEqual(w1[0x2D]["flags"], 0x0C)
        w3 = self.report["initializers"]["W3"]["records"]
        self.assertEqual(w3[0x00]["flags"], 0)
        self.assertEqual(w3[0x2A]["flags"], 0x70)
        self.assertEqual(w3[0x2B]["flags"], 0x30)
        self.assertEqual(w3[0xDA]["flags"], 0x0C)
        self.assertEqual(w3[0xDB]["flags"], 0x08)
        self.assertEqual(
            self.report["initializers"]["W3"]["duplicate_cases"],
            [
                {"tile_id": 0xDA, "first_flags": 0x0C, "first_store": 9803,
                 "duplicate_flags": 0x10, "duplicate_store": 10141},
                {"tile_id": 0xDB, "first_flags": 0x08, "first_store": 9829,
                 "duplicate_flags": 0x50, "duplicate_store": 10167},
            ],
        )

    def test_map_mutations_are_first_row_word_updates(self):
        loaders = self.report["map_mutations"][:2]
        self.assertEqual([loader["entry"] for loader in loaders],
                         ["01D7:365B", "01D7:3861"])
        self.assertEqual([loader["coverage"] for loader in loaders],
                         ["first row; DS:657E / 2 cells"] * 2)
        self.assertEqual([loader["word_delta"] for loader in loaders],
                         ["0x1000 when bit was clear"] * 2)
        self.assertTrue(all("OR byte" in loader["mutation"] for loader in loaders))

    def test_loader_callsite_roles(self):
        roles = self.report["loader_callsite_decomp"]
        self.assertEqual(roles["primary"]["callsite"], "01D7:4009")
        self.assertEqual(roles["secondary"]["scheduler"], "01D7:48B5")
        self.assertEqual(roles["secondary"]["selectors"],
                         ["0x0002", "0x0005", "0x0008", "0x000b", "0x000e"])
        self.assertEqual(len(roles["secondary"]["call_sites"]), 5)

    def test_scheduler_gate_static_roles(self):
        gate = self.report["scheduler_gate_decomp"]
        self.assertEqual(gate["global"], "DS:89EA")
        self.assertEqual(gate["clearer"]["entry"], "01F7:1AE6")
        self.assertEqual(gate["direct_callers"]["01F7:199D"], ["01F7:43D0"])
        self.assertEqual(gate["direct_callers"]["01F7:19E6"],
                         ["01F7:1BC4", "01F7:3AB3"])

    def test_flag_consumer_audit_is_narrow(self):
        consumers = self.report["flag_consumers"]
        self.assertEqual(consumers["low_nibble_query"]["direct_relocation_call_count"], 78)
        self.assertEqual(
            consumers["low_nibble_query"]["quadrant_bit_map"]["AX_bit3=1,BX_bit3=1"],
            "descriptor bit 0x0002",
        )
        self.assertIn("AX is the 16-pixel row coordinate",
                      consumers["low_nibble_query"]["coordinate_bits"])
        self.assertEqual(consumers["full_word_query"]["direct_callers"],
                         ["01F7:3D19", "01F7:3D31"])
        self.assertEqual(consumers["full_word_query"]["transition_dx_consumers"],
                         ["01F7:447B", "01F7:448C", "01F7:44A0", "01F7:44B1"])
        self.assertIn("no standalone gameplay name",
                      consumers["unresolved_bits"]["bit_10"])
        latch = consumers["collision_state_latch"]
        self.assertEqual(latch["field"], "player object +0x3a")
        self.assertIn("01F7:3D02", latch["producer"])
        self.assertIn("01F7:3DF2", latch["consumer"])
        self.assertIn("not a persistent surface type", latch["semantics"])

    def test_map_reader_audit_separates_upper_field(self):
        audit = self.report["map_read_audit"]
        self.assertEqual(
            [item["entry"] for item in audit["identified_readers"]],
            ["01F7:3376", "01F7:5C27", "01F7:5CC3", "01F7:20C8", "01F7:2CB2"],
        )
        self.assertIn("bits 9..15", audit["upper_bits"]["status"])
        self.assertIn("static audit boundary", audit["upper_bits"]["caveat"])

    def test_map_writer_call_form_audit_is_explicit(self):
        audit = self.report["map_writer_call_form_audit"]
        self.assertEqual(audit["segment"], "01F7")
        self.assertEqual(len(audit["indirect_call_forms"]), 2)
        self.assertEqual(audit["target_writer_indirect_hits"], [])
        self.assertIn("23 NE-relocated far calls", audit["interpretation"])

    def test_static_map_writer_inventory(self):
        writers = {item["entry"]: item for item in self.report["map_mutations"]}
        self.assertEqual(writers["01F7:16CE"]["direct_relocation_call_count"], 23)
        self.assertEqual(writers["01F7:16CE"]["direct_callers"][:3],
                         ["01F7:1892", "01F7:1944", "01F7:6359"])
        self.assertEqual(len(writers["01F7:16CE"]["direct_callers"]), 23)
        self.assertEqual(writers["01F7:33BF"]["direct_callers"],
                         ["01D7:37CB", "01D7:396D"])
        self.assertEqual(writers["01F7:339A"]["direct_relocation_call_count"], 0)
        self.assertEqual(writers["01F7:340A"]["direct_relocation_call_count"], 0)
        self.assertEqual(writers["01F7:5C9D"]["direct_relocation_call_count"], 0)
        for entry in ("01F7:339A", "01F7:340A", "01F7:5C9D"):
            self.assertEqual(writers[entry]["relocation_record_count"], 0)
            self.assertEqual(writers[entry]["embedded_target_offset_occurrences"], 0)
        self.assertIn("| CX (caller supplies low-ID bits)",
                      writers["01F7:339A"]["mutation"])
        self.assertIn("| CX (caller supplies upper-property bits)",
                      writers["01F7:340A"]["mutation"])

    def test_curated_collision_control_pair(self):
        evidence_path = Path(__file__).resolve().parents[1] / "notes" / "descriptor-collision-evidence.json"
        evidence = json.loads(evidence_path.read_text())
        cases = {item["case"]: item for item in evidence["runtime_cases"]}
        self.assertEqual(cases["current_trace_flags_0x10"]["branch_offsets"],
                         ["01F7:3D02", "01F7:3D1E", "01F7:3D45", "01F7:3DD0", "01F7:3DE4"])
        self.assertEqual(cases["current_trace_flags_0x50"]["descriptor"], "0x0050")
        self.assertEqual(cases["current_trace_flags_0x00"]["branch_offsets"][-1], "01F7:3D44")
        self.assertEqual(cases["current_trace_flags_0x00"]["patched_cell"]["readback"], "0x0160")
        self.assertEqual(cases["direct_descriptor_flags_0x20"]["return"], "01F7:3DE4")
        self.assertEqual(cases["direct_descriptor_flags_0x40"]["return"], "01F7:3D44")
        self.assertEqual(cases["direct_descriptor_flags_0x60"]["al"], 1)
        self.assertEqual(cases["natural_right_tile_0x2a"]["raw_cell"], "0x202a")
        self.assertEqual(cases["natural_right_tile_0x2a"]["descriptor"], "0x0070")
        self.assertEqual(cases["natural_right_tile_0x28"]["descriptor"], "0x0010")
        self.assertEqual(cases["natural_right_tile_0x28"]["al"], 1)
        falling = cases["vertical_motion_patch_flags_0x10"]
        self.assertEqual(falling["descriptor_original"], "0x0070")
        self.assertEqual(falling["descriptor_patched"], "0x0010")
        self.assertEqual(falling["branch_offsets"][-1], "01F7:3DE4")
        self.assertEqual(falling["al"], 0)
        self.assertEqual(falling["vertical_velocity_fixed"], "0x00009000")

    def test_curated_all_world_construction_census(self):
        evidence_path = Path(__file__).resolve().parents[1] / "notes" / "descriptor-construction-evidence.json"
        evidence = json.loads(evidence_path.read_text())
        self.assertEqual(evidence["field_consumers"]["record_plus_0"]["entries"],
                         ["01F7:20C8", "01F7:2CB2"])
        self.assertIn("0x100-byte", evidence["field_consumers"]["record_plus_0"]["meaning"])
        self.assertEqual(
            evidence["field_consumers"]["record_plus_2"]["quadrant_bit_map"]["AX_bit3=0,BX_bit3=0"],
            "0x08",
        )
        self.assertIn("not a persistent surface type",
                      evidence["field_consumers"]["collision_state_latch"]["meaning"])
        writer_audit = evidence["map_writer_call_audit"]
        self.assertEqual(writer_audit["01F7:16CE"]["direct_relocation_call_count"], 23)
        self.assertEqual(len(writer_audit["01F7:16CE"]["direct_callers"]), 23)
        effect_callsite = evidence["effect_writer_callsite_decomp"]
        self.assertIn("DS:6586", effect_callsite["callers"]["01F7:1892"])
        self.assertIn("DX & 0x01ff", effect_callsite["writer_operation"])
        constructor = evidence["descriptor_constructor_static_decomp"]
        self.assertEqual(constructor["entry"], "01E7:382B")
        self.assertIn("publishes the far pointer", constructor["operations"][0])
        world_dispatch = evidence["descriptor_world_dispatch_static_decomp"]
        self.assertEqual(world_dispatch["selector_global"], "DS:85D8")
        self.assertEqual(world_dispatch["world_initializers"]["5"], "01D7:2D9F")
        self.assertEqual([item["world"] for item in evidence["worlds"]],
                         ["W1", "W2", "W3", "W4", "W5"])
        self.assertTrue(all(item["descriptor_matches_static"] == 512
                            for item in evidence["worlds"]))
        w3 = next(item for item in evidence["worlds"] if item["world"] == "W3")
        self.assertEqual(w3["duplicate_tile_cases"][0]["first_flags"], "0x000c")
        self.assertEqual(w3["duplicate_tile_cases"][1]["duplicate_flags"], "0x0050")
        focus = evidence["writer_focus_probe"]
        self.assertEqual(focus["watched_entries"], ["01F7:339A", "01F7:340A", "01F7:5C9D"])
        self.assertEqual(focus["hits"], [])
        self.assertEqual(focus["state_at_timeout"]["object_count"], "0x0003")
        extended = evidence["writer_focus_extended_probe"]
        self.assertEqual(extended["watched_entries"],
                         ["01F7:16CE", "01F7:339A", "01F7:340A", "01F7:5C9D"])
        self.assertEqual(extended["transition_events"], 96)
        self.assertEqual(extended["hits"], [])
        effect_probes = evidence["normal_effect_writer_probes"]
        self.assertEqual([item["level"] for item in effect_probes],
                         ["W1L1", "W2L1", "W3L1", "W4L1", "W5L1"])
        self.assertTrue(all(item["caller_return"] == "01F7:1897"
                            for item in effect_probes))
        self.assertTrue(all(item["cell"]["after"] in ("0x01d7", "0x01d8", "0x01d9", "0x01e5")
                            for item in effect_probes))
        w1l3_focus = evidence["writer_focus_w1l3_probe"]
        self.assertEqual(w1l3_focus["hits"], [])
        self.assertEqual(w1l3_focus["state_at_timeout"]["level_index"], "0x0002")
        controlled = evidence["writer_focus_w1l3_controlled_probe"]
        self.assertTrue(controlled["controlled"])
        self.assertEqual(controlled["hits"], [])
        self.assertEqual(controlled["state_at_timeout"]["object_count"], "0x0003")
        event_probe = evidence["secondary_event_uninstrumented_probe"]
        self.assertEqual(event_probe["event_writer"], "01D7:493E")
        self.assertTrue(event_probe["event_writer_hit"])
        self.assertFalse(event_probe["secondary_loader_reached"])
        self.assertEqual(event_probe["final_cpu"]["eip"], "0x0023")
        timer_decomp = evidence["timer_wait_static_decomp"]
        self.assertEqual(timer_decomp["wait_entry"], "0207:0002")
        self.assertIn("DS:819E=0", timer_decomp["wait_operation"])
        timer_irq_decomp = evidence["timer_irq_static_decomp"]
        self.assertEqual(timer_irq_decomp["entry"], "01F7:F049")
        self.assertEqual(timer_irq_decomp["flag_write"], "DS:819E=1")
        self.assertIn("IRET", timer_irq_decomp["end_operation"])
        callback_pair = evidence["timer_callback_pair_static_decomp"]
        self.assertEqual(callback_pair["pair"], "DS:8952:DS:8954")
        self.assertIn(".\\Score.DAT", callback_pair["startup_resource"])
        self.assertIn("audio/resource", callback_pair["interpretation"])
        self.assertIn("not evidence", callback_pair["interpretation"])
        callback_runtime = evidence["timer_callback_pair_runtime_probe"]
        self.assertFalse(callback_runtime["controlled"])
        self.assertEqual([item["entry"] for item in callback_runtime["events"][-2:]],
                         ["01D7:48B5", "01D7:48BB"])
        self.assertEqual([item["callback"] for item in callback_runtime["events"][-2:]],
                         ["0xffff:0xffff", "0xffff:0xffff"])
        self.assertIn("not the missing 89EA/89E6", callback_runtime["interpretation"])
        timer_probe = evidence["secondary_timer_audit_probe"]
        self.assertEqual(timer_probe["timer_irq_entry"], "01F7:F049")
        self.assertEqual(timer_probe["timer_irq_hits"], 32)
        self.assertEqual(timer_probe["timer_irq_pre_event_hits"], 16)
        self.assertEqual(timer_probe["timer_irq_post_event_hits"], 16)
        self.assertEqual(timer_probe["event_generation"], 20)
        self.assertFalse(timer_probe["secondary_loader_reached"])
        post_wait = evidence["timer_post_wait_audit"]
        self.assertEqual(post_wait["barrier"], "01D7:48BB")
        self.assertEqual(post_wait["timer_irq_entry"], "01F7:F049")
        self.assertEqual(post_wait["post_wait_irq_hits"], 16)
        self.assertEqual(post_wait["all_samples"]["timer_callback_segment"], "0xffff")
        self.assertEqual(post_wait["all_samples"]["timer_callback_offset"], "0xffff")
        self.assertIn("audio/resource callback sentinel", post_wait["interpretation"])
        self.assertIn("does not explain or disprove", post_wait["interpretation"])
        self.assertFalse(post_wait["secondary_loader_reached"])
        state_probes = evidence["timer_state_trace_probes"]
        self.assertEqual([item["level"] for item in state_probes], ["W1L3", "W2L3"])
        self.assertTrue(all(item["controlled"] is False for item in state_probes))
        self.assertTrue(all(item["sequence"][-1] == "01D7:48BB" for item in state_probes))
        self.assertTrue(all(item["wait_gate_samples"]["wait_test_after_clear"] == "0x0000"
                            for item in state_probes))
        additional_state = evidence["timer_state_trace_additional_probes"]
        self.assertEqual([item["level"] for item in additional_state],
                         ["W3L3", "W4L3", "W5L3"])
        self.assertTrue(all(item["sequence_matches_w1l3"] for item in additional_state))
        self.assertTrue(all(not item["secondary_loader_reached"] for item in additional_state))
        other_worlds = evidence["writer_focus_other_world_l1_probes"]
        self.assertEqual([item["level"] for item in other_worlds],
                         ["W2L1", "W3L1", "W4L1", "W5L1"])
        self.assertTrue(all(item["hits"] == [] for item in other_worlds))

    def test_curated_state_machine_effect_probes(self):
        evidence_path = Path(__file__).resolve().parents[1] / "notes" / "descriptor-construction-evidence.json"
        evidence = json.loads(evidence_path.read_text())
        probes = evidence["state_machine_effect_probes"]
        self.assertTrue(probes["controlled"])
        self.assertEqual(probes["static_callback"], "01F7:8E4B")
        self.assertEqual(probes["tile_query"], "01F7:3376")
        self.assertEqual(probes["map_writer"], "01F7:16CE")
        self.assertEqual(probes["source_run"]["tile_ids"], [200, 201, 202, 203, 204])
        self.assertEqual(probes["source_run"]["effect_values"], [120, 121, 122, 123, 124])
        self.assertEqual([item["state"] for item in probes["probes"]], [4, 6, 8])
        for item in probes["probes"]:
            self.assertEqual(len(item["cells"]), 5)
            self.assertEqual([cell["after"] for cell in item["cells"]],
                             ["0x0078", "0x0079", "0x007a", "0x007b", "0x007c"])
            self.assertEqual([cell["delta"] for cell in item["cells"]], [-80] * 5)
        self.assertIn("controlled instrumentation", probes["interpretation"])

    def test_secondary_construction_probe_honors_requested_loader(self):
        source = (Path(__file__).resolve().parents[1] /
                  "automation" / "quiky_descriptor_construction_trace.lua").read_text()
        self.assertIn("local secondary_loader = trace_secondary", source)

    def test_secondary_writer_focus_is_bounded(self):
        source = (Path(__file__).resolve().parents[1] /
                  "automation" / "quiky_secondary_lifecycle_trace.lua").read_text()
        self.assertIn("local writer_focus = TRACE_SECONDARY_WRITER_FOCUS", source)
        self.assertIn('name = "map_low_id_writer"', source)
        self.assertIn('name = "map_property_writer"', source)
        self.assertIn('name = "map_cell_store_helper"', source)
        self.assertIn("TRACE_SECONDARY_EVENT_BEFORE_SECONDARY", source)
        self.assertIn("TRACE_SECONDARY_TIMER_AUDIT", source)
        self.assertIn("timer_callback_segment = ds_word(0x8952)", source)
        self.assertIn("timer_callback_offset = ds_word(0x8954)", source)

    def test_optional_follow_up_plan_has_gates_and_stop_rules(self):
        plan = (Path(__file__).resolve().parents[1] /
                "notes" / "runtime-descriptor-construction-plan.md").read_text()
        self.assertIn("## Optional follow-up plan", plan)
        for heading in (
            "### O1. Give flag `0x10` a historical gameplay name",
            "### O2. Resolve the natural W1L3-W5L3 secondary-loader trigger",
            "### O3. Exclude runtime-generated callers of `339A`, `340A`, and `5C9D`",
            "### O4. Optional live confirmation of state-10 termination",
        ):
            self.assertIn(heading, plan)
        self.assertGreaterEqual(plan.count("**Acceptance:**"), 4)
        self.assertGreaterEqual(plan.count("**Artifact:**"), 4)

    def test_optional_follow_up_results_are_curated(self):
        evidence_path = Path(__file__).resolve().parents[1] / "notes" / "descriptor-construction-evidence.json"
        evidence = json.loads(evidence_path.read_text())
        flag10 = evidence["optional_flag10_matrix"]
        self.assertEqual(flag10["historical_name_status"], "unresolved")
        self.assertEqual(flag10["static_status"], "mechanical_x_retry_suppression")
        self.assertEqual(flag10["flag10_map_occurrences"],
                         {"W1": 159, "W2": 0, "W3": 382, "W4": 156, "W5": 160})
        self.assertIn("vertical_motion_patch_flags_0x10", flag10["controlled_cases"])

        lifecycle = evidence["optional_secondary_lifecycle_matrix"]
        self.assertEqual(lifecycle["levels"], ["W1L3", "W2L3", "W3L3", "W4L3", "W5L3"])
        self.assertEqual(lifecycle["status"], "all_stopped_at_timer_transition_barrier")
        self.assertFalse(lifecycle["secondary_loader_reached"])

        writers = evidence["optional_map_writer_runtime_caller_audit"]
        self.assertEqual(writers["helper_runtime_hits"],
                         {"01F7:339A": 0, "01F7:340A": 0, "01F7:5C9D": 0})
        self.assertEqual(writers["status"], "implemented_but_unobserved_runtime_apis")

        state10 = evidence["optional_state10_probe"]
        self.assertEqual(state10["status"], "tooling_limited")
        self.assertIn("five state-10 MAP-derived writes", state10["static_expectation"])

    def test_optional_execution_results_and_tools_are_present(self):
        root = Path(__file__).resolve().parents[1]
        plan = (root / "notes" / "runtime-descriptor-construction-plan.md").read_text()
        self.assertIn("## Optional execution results", plan)
        for tool in ("optional_flag10_matrix.py", "optional_lifecycle_matrix.py",
                     "optional_map_writer_audit.py", "optional_state10_probe.py"):
            self.assertTrue((root / "tools" / tool).exists(), tool)
        self.assertIn("instruction limit before a state-machine sample", plan)


if __name__ == "__main__":
    unittest.main()
