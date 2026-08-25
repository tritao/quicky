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
                    // Some hand-entered transition labels are not reached by
                    // Ghidra's initial analysis.  Decode and create a local
                    // function at the requested entry in the disposable
                    // analysis project, then decompile that recovered body.
                    try {
                        disassemble(address);
                        function = createFunction(address, expectedName);
                    } catch (Exception ignored) {
                        // Keep the explicit MISSING marker below if decoding
                        // cannot establish a valid function boundary.
                    }
                }
                if (function == null) {
                    writer.println("/* MISSING 0x" + target[0] + " " + expectedName + " */");
                    writer.println();
                    continue;
                }

                writer.println("/* " + expectedName + " at 0x" + target[0] +
                    "; decompiled function entry 0x" + function.getEntryPoint() + " */");
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
                {"01ac", "menu_input_action_pending"},
                {"0203", "wait_for_input_release"},
                {"34c8", "load_are_resource"},
                {"365b", "load_map_resource_primary"},
                {"3861", "load_map_resource_secondary"},
                {"3808", "descriptor_world_dispatch"},
                {"4009", "primary_loader_callsite"},
                {"399e", "load_bob_resource"},
                {"3bbd", "load_ico_resource"},
                {"313d", "seg1_target_313d"},
                {"47f0", "reset_game_input_flags"},
                {"4ac2", "level_selector_input_loop"},
                {"4859", "transition_main_loop_entry"},
                {"48b5", "transition_wait_gate_clear"},
                {"48bb", "transition_wait_gate_test"},
                {"48c2", "transition_post_wait_entry"},
                {"48cc", "transition_post_wait_state_check"},
                {"48dc", "transition_post_wait_branch"},
                {"48e6", "transition_post_wait_event_gate"},
                {"493e", "transition_write_event_flag"},
                {"4968", "transition_dispatch"},
                {"4ba4", "transition_scheduler_gate"},
                {"4bae", "transition_scheduler_dispatch"},
                {"4b8d", "transition_dispatch_tail"},
                {"4bd8", "transition_secondary_loader_gate"},
                {"4ea0", "transition_pending_gate"},
                {"4eaa", "transition_pending_setup"},
                {"4edd", "transition_pending_wait_a"},
                {"4ee6", "transition_pending_wait_b"},
                {"4f0d", "transition_pending_intro"},
                {"5010", "transition_main_loop_event"},
                {"504f", "transition_main_loop_dispatch"},
            };
        }
        if ("SEG02".equals(segment)) {
            return new String[][] {
                {"36ed", "startup_audio_or_timer_setup"},
                {"382b", "descriptor_table_constructor"},
                {"3874", "descriptor_table_allocate"},
                {"387c", "descriptor_table_publish_base"},
                {"387f", "descriptor_table_publish_selector"},
                {"3883", "descriptor_table_zero_fill"},
                {"085e", "load_sam_tfx_resource"},
                {"0caa", "seg2_target_0caa"},
                {"0fcf", "dispatch_pending_sound_effect"},
                {"168d", "dispatch_audio_driver_command"},
                {"1761", "set_sound_blaster_rate"},
                {"17b5", "start_sound_blaster_output"},
                {"17f1", "sound_blaster_irq_handler"},
                {"190e", "program_sound_blaster_dma"},
                {"19ad", "stop_sound_blaster_output"},
                {"1a26", "render_audio_buffer"},
                {"1a89", "mix_voice_into_output_buffer"},
                {"1f7f", "convert_mixed_word_to_output_byte"},
                {"2809", "control_audio_driver_tick"},
                {"285e", "render_audio_driver_tick"},
                {"2984", "update_audio_driver_voices"},
                {"2a0f", "commit_voice_mixer_state"},
                {"2b42", "run_voice_macro"},
                {"2f3e", "advance_music_voice"},
                {"31f5", "advance_music_sequence"},
                {"3237", "initialize_effect_voice"},
                {"3360", "select_effect_voice"},
                {"33f8", "finish_music_sequence"},
            };
        }
        if ("SEG03".equals(segment)) {
            return new String[][] {
                {"08c9", "release_map_buffer"},
                {"0a43", "initialize_game_state"},
                {"0a15", "seg3_target_0a15"},
                {"0e06", "are_object_factory"},
                {"0e66", "object_pool_count_active"},
                {"0e96", "object_update_pass_by_phase"},
                {"1036", "register_object_scheduler_entry"},
                {"0f3c", "find_object_kind_0x64"},
                {"0fa2", "object_update_pass_nonzero_state"},
                {"05a0", "seg3_target_05a0"},
                {"106a", "seg3_target_106a"},
                {"1cda", "stream_are_regions"},
                {"1ec4", "seg3_target_1ec4"},
                {"1892", "dedicated_event_tile_rewrite_callsite"},
                {"1944", "dedicated_event_tile_rewrite_callsite_b"},
                {"6359", "short_tile_animation_rewrite_callsite"},
                {"1ed7", "update_camera_scroll"},
                {"1dca", "object_camera_visibility_gate"},
                {"1dee", "deactivate_object_outside_camera"},
                {"1b5d", "apply_player_displacement"},
                {"1b77", "save_collision_probe_context"},
                {"1c4d", "check_object_map_contact"},
                {"1c6e", "map_word_probe_16px"},
                {"0e06", "are_object_factory"},
                {"1036", "register_object_scheduler_entry"},
                {"b1f0", "create_b226_linked_object"},
                {"b20b", "create_b25d_linked_object"},
                {"b142", "create_b33b_owner"},
                {"b226", "update_b226_animation"},
                {"b25d", "update_b25d_animation"},
                {"b33b", "update_b33b_owner"},
                {"b84c", "prepare_b87b_transition"},
                {"b84d", "initialize_b87b_transition"},
                {"b87b", "update_b87b_transition"},
                {"487f", "initialize_late_owner"},
                {"489c", "update_late_owner"},
                {"20c8", "render_map_column"},
                {"2cb2", "render_map_strip"},
                {"332c", "seg3_target_332c"},
                {"335e", "seg3_target_335e"},
                {"33bf", "map_low_id_normalizer"},
                {"16ce", "map_effect_tile_rewrite"},
                {"339a", "map_low_id_writer"},
                {"340a", "map_property_writer"},
                {"3376", "map_tile_id_at_pixel"},
                {"5c27", "map_descriptor_quadrant_test"},
                {"5cc3", "map_descriptor_word_at_pixel"},
                {"5c9d", "map_cell_word_store"},
                {"5d00", "map_cell_descriptor_5d00"},
                {"5d38", "load_animation_descriptor"},
                {"5d60", "advance_animation_descriptor"},
                {"6370", "player_probe_hazard_offset"},
                {"342f", "seg3_target_342f"},
                {"393c", "compute_state_machine_bounds"},
                {"3909", "contact_effect_callback_assignment"},
                {"39fe", "query_player_collision_state"},
                {"3f27", "initialize_player_record"},
                {"3ff8", "update_player_record"},
                {"44ff", "reset_contact_effect_table"},
                {"4519", "spawn_contact_effect_entry"},
                {"45ab", "update_contact_effect_entry"},
                {"470c", "remove_contact_effect_entry"},
                {"58a0", "clear_ufo_contact_effect"},
                {"3a1f", "player_probe_side_clear"},
                {"3a62", "player_collision_probe_3a62"},
                {"3a8a", "player_probe_transition_tiles"},
                {"3ab9", "player_collision_probe_3ab9"},
                {"3d02", "player_resolve_descriptor_response"},
                {"3df2", "player_snap_y_on_side_contact"},
                {"3e41", "player_collision_probe_3e41"},
                {"6484", "player_probe_hazard_plus5"},
                {"648e", "player_probe_hazard_right"},
                {"69ff", "player_bounds_or_collision_69ff"},
                {"44dc", "player_control_transition_44dc"},
                {"8d20", "update_collectible_effect"},
                {"8d31", "update_collectible_state"},
                {"19a3", "write_scheduler_gate_start"},
                {"19e6", "scheduler_gate_overlap_path"},
                {"1a3d", "write_scheduler_gate_state"},
                {"1ae6", "clear_scheduler_gate"},
                {"199d", "write_scheduler_gate_callback"},
                {"1bc4", "scheduler_gate_overlap_callsite"},
                {"3ab3", "scheduler_gate_motion_callsite"},
                {"43d0", "player_boundary_gate_callsite"},
                {"43d1", "player_boundary_gate_entry"},
                {"4727", "update_falling_leaves_types_29_2b"},
                {"8c4e", "init_are_type_2c"},
                {"87d1", "init_are_type_33"},
                {"882f", "update_are_type_33"},
                {"9bee", "init_are_type_34"},
                {"9c0c", "update_are_type_34"},
                {"9c29", "test_type34_proximity"},
                {"8e4b", "update_tile_effect_state_machine"},
                {"f17f", "keyboard_irq1_handler"},
                {"f1a8", "poll_keyboard_ring_to_input_flags"},
                {"f21b", "read_normalized_input_flags"},
                {"f049", "timer_irq_entry"},
            };
        }
        if ("SEG04".equals(segment)) {
            return new String[][] {
                {"0002", "wait_frames_on_timer_flag"},
                {"0014", "clear_timer_wait_flag"},
                {"001e", "timer_wait_yield_callsite"},
                {"101f", "timer_routine_clear_wait_flag"},
                {"10a3", "timer_routine_recursive_callsite"},
                {"10a9", "pit_timer_wait_helper"},
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
