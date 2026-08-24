// Dump decompiler output for the evidence-backed Quiky loader functions.
// Usage through analyzeHeadless:
//   -postScript DumpQuikyDecomp.java <output-directory>

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

public class DumpQuikyDecomp extends GhidraScript {

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 1) {
            printerr("Usage: -postScript DumpQuikyDecomp.java <output-directory>");
            return;
        }

        File outputDirectory = new File(args[0]);
        if (!outputDirectory.exists() && !outputDirectory.mkdirs()) {
            throw new Exception("Could not create output directory: " + outputDirectory);
        }

        String program = currentProgram.getName();
        String segment = segmentName(program);
        if (segment == null) {
            return;
        }

        String[][] targets = targetsFor(segment);
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        decompiler.setSimplificationStyle("decompile");

        File report = new File(outputDirectory, program.replaceAll("[^A-Za-z0-9_.-]", "_") + ".c");
        try (PrintWriter writer = new PrintWriter(report, "UTF-8")) {
            writer.println("/* Decompiled from " + program + " */");
            writer.println("/* Segment-relative addresses match the raw Ghidra import. */");
            writer.println();

            for (String[] target : targets) {
                int offset = Integer.parseInt(target[0], 16);
                String expectedName = target[1];
                Address address = toAddr(offset);
                Function function = currentProgram.getFunctionManager().getFunctionAt(address);
                if (function == null) {
                    function = currentProgram.getFunctionManager().getFunctionContaining(address);
                }
                if (function == null) {
                    writer.println("/* MISSING 0x" + target[0] + " " + expectedName + " */");
                    writer.println();
                    continue;
                }

                writer.println("/* " + expectedName + " at 0x" + target[0] + " */");
                DecompileResults result = decompiler.decompileFunction(function, 120, monitor);
                if (result.decompileCompleted() && result.getDecompiledFunction() != null) {
                    writer.println(result.getDecompiledFunction().getC());
                } else {
                    writer.println("/* DECOMPILATION FAILED: " + result.getErrorMessage() + " */");
                }
                writer.println();
            }
        } finally {
            decompiler.dispose();
        }
        println("Wrote " + report.getAbsolutePath());
    }

    private String segmentName(String program) {
        for (int i = 1; i <= 6; i++) {
            if (program.contains(String.format("SEG%02d", i))) {
                return String.format("SEG%02d", i);
            }
        }
        return null;
    }

    private String[][] targetsFor(String segment) {
        if ("SEG01".equals(segment)) {
            return new String[][] {
                {"1670", "puzzle_completion_mask_check"},
                {"16c6", "puzzle_completion_presentation"},
                {"1704", "puzzle_completion_presentation_return"},
                {"1084", "gameplay_state_update_candidate"},
                {"14e1", "completion_effect_consumer_candidate"},
                {"01ac", "menu_input_action_pending"},
                {"0203", "wait_for_input_release"},
                {"34c8", "load_are_resource"},
                {"365b", "load_map_resource_primary"},
                {"3861", "load_map_resource_secondary"},
                {"399e", "load_bob_resource"},
                {"3bbd", "load_ico_resource"},
                {"313d", "seg1_target_313d"},
                {"47f0", "reset_game_input_flags"},
                {"4ac2", "level_selector_input_loop"},
                {"4ea0", "cloud_outer_state_consumer"},
                {"4eaa", "cloud_outer_state_positive_path"},
                {"4f10", "puzzle_completion_handoff"},
                {"4faf", "puzzle_completion_reload_setup"},
                {"5017", "puzzle_completion_reload_transition"},
                {"5038", "puzzle_completion_transition_flags"},
                {"5047", "puzzle_completion_transition_dispatch"},
            };
        }
        if ("SEG02".equals(segment)) {
            return new String[][] {
                {"085e", "load_sam_tfx_resource"},
                {"0caa", "seg2_target_0caa"},
                {"0d18", "cloud_branch_audio_or_effect_candidate"},
            };
        }
        if ("SEG03".equals(segment)) {
            return new String[][] {
                {"0013", "special_render_entry_candidate"},
                {"08c9", "release_map_buffer"},
                {"0a43", "initialize_game_state"},
                {"0a15", "seg3_target_0a15"},
                {"0e06", "are_object_factory"},
                {"0e66", "object_pool_count_active"},
                {"0e96", "object_update_pass_by_phase"},
                {"0f3c", "find_object_kind_0x64"},
                {"0fa2", "object_update_pass_nonzero_state"},
                {"0fdc", "cloud_render_queue_candidate"},
                {"1024", "cloud_render_dispatch_candidate"},
                {"05a0", "seg3_target_05a0"},
                {"106a", "seg3_target_106a"},
                {"1cda", "stream_are_regions"},
                {"1e04", "instantiate_are_declaration"},
                {"1ec4", "seg3_target_1ec4"},
                {"1ed7", "update_camera_scroll"},
                {"1dca", "object_camera_visibility_gate"},
                {"1dee", "deactivate_object_outside_camera"},
                {"20c8", "render_map_column"},
                {"2cb2", "render_map_strip"},
                {"332c", "seg3_target_332c"},
                {"335e", "seg3_target_335e"},
                {"33bf", "seg3_target_33bf"},
                {"3529", "normal_object_renderer"},
                {"34bc", "render_queue_append"},
                {"34e3", "render_queue_finalize_entry"},
                {"3587", "render_queue_draw_pass"},
                {"5936", "paper_counter_hud_consumer_candidate"},
                {"5bee", "paper_counter_hud_reset_candidate"},
                {"53f7", "ufo_effect_common_setup"},
                {"546d", "ufo_effect_update_state_machine"},
                {"58a0", "ufo_effect_exit_clear"},
                {"5637", "ufo_effect_player_gate"},
                {"87d1", "wind_effect_initializer"},
                {"882f", "wind_effect_update_state_machine"},
                {"8c4e", "paper_effect_initializer"},
                {"8d08", "paper_effect_apply_global_effect"},
                {"8d20", "paper_effect_shared_callback"},
                {"8d31", "paper_effect_overlap_gate"},
                {"8e4b", "animated_tile_effect_state_machine"},
                {"10b5", "dedicated_event_child_callback"},
                {"1186", "dedicated_event_renderer"},
                {"11b4", "dedicated_event_renderer_tail"},
                {"16ce", "dedicated_event_loop"},
                {"1693", "dedicated_event_visibility_gate"},
                {"1749", "dedicated_event_creator"},
                {"178d", "dedicated_event_type_65"},
                {"1798", "dedicated_event_type_66"},
                {"17a3", "dedicated_event_type_67"},
                {"4087", "player_cloud_state_reader_4087"},
                {"4406", "player_cloud_state_reader_4406"},
                {"3376", "map_tile_id_lookup_16px"},
                {"5c27", "map_tile_descriptor_query_5c27"},
                {"5cc3", "map_tile_descriptor_query_5cc3"},
                {"5d00", "map_cell_descriptor_5d00"},
                {"5d38", "map_cell_descriptor_5d38"},
                {"5d60", "map_cell_state_decay_5d60"},
                {"6370", "player_collision_helper_6370"},
                {"342f", "seg3_target_342f"},
                {"393c", "compute_state_machine_bounds"},
                {"3f27", "initialize_player_object"},
                {"3ff8", "update_player_object"},
                {"3a1f", "player_collision_probe_3a1f"},
                {"3a62", "player_collision_probe_3a62"},
                {"3a8a", "player_collision_helper_3a8a"},
                {"3ab9", "player_collision_probe_3ab9"},
                {"3d02", "player_collision_helper_3d02"},
                {"3df2", "player_collision_helper_3df2"},
                {"3e41", "player_collision_probe_3e41"},
                {"6484", "player_collision_helper_6484"},
                {"648e", "player_collision_helper_648e"},
                {"69ff", "player_bounds_or_collision_69ff"},
                {"44dc", "player_control_transition_44dc"},
                {"44ff", "effect_table_reset"},
                {"4519", "effect_table_spawn_entry"},
                {"45ab", "effect_table_entry_update"},
                {"470c", "effect_table_remove_entry"},
                {"f07b", "cloud_branch_segment3_helper_candidate"},
                {"9256", "update_are_type_28"},
                {"9269", "update_are_type_28_steady"},
                {"f17f", "keyboard_irq1_handler"},
                {"f1a8", "poll_keyboard_ring_to_input_flags"},
                {"f21b", "read_normalized_input_flags"},
            };
        }
        if ("SEG04".equals(segment)) {
            return new String[][] {
                {"0002", "cloud_branch_seg4_entry_candidate"},
                {"022a", "seg4_target_022a"},
                {"125b", "seg4_target_125b"},
                {"170a", "seg4_target_170a"},
                {"1737", "seg4_target_1737"},
                {"17a0", "seg4_target_17a0"},
                {"1859", "seg4_target_1859"},
                {"18c7", "seg4_target_18c7"},
                {"19ff", "seg4_target_19ff"},
                {"1a37", "seg4_target_1a37"},
                {"1a73", "seg4_target_1a73"},
                {"1b45", "seg4_target_1b45"},
                {"1bde", "seg4_target_1bde"},
                {"2a07", "seg4_target_2a07"},
                {"2a5c", "seg4_target_2a5c"},
                {"2aaf", "seg4_target_2aaf"},
                {"2af3", "seg4_target_2af3"},
                {"2b0d", "seg4_target_2b0d"},
                {"2b38", "seg4_target_2b38"},
                {"08d8", "cloud_branch_seg4_effect_candidate"},
            };
        }
        if ("SEG05".equals(segment)) {
            return new String[][] {
                {"033e", "seg5_target_033e"},
                {"0358", "seg5_target_0358"},
                {"05cd", "seg5_target_05cd"},
                {"0591", "seg5_target_0591"},
                {"08e9", "seg5_target_08e9"},
                {"0a0d", "seg5_target_0a0d"},
                {"0d72", "seg5_target_0d72"},
                {"0e87", "seg5_target_0e87"},
                {"0f06", "seg5_target_0f06"},
                {"1b5a", "seg5_target_1b5a"},
                {"1b7e", "seg5_target_1b7e"},
            };
        }
        return new String[0][0];
    }
}
