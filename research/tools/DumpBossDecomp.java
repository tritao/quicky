// Dump only the boss-transition and completion-related functions.
// Run after importing the raw QUIKY segments with Ghidra's binary loader.

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.symbol.SourceType;

public class DumpBossDecomp extends GhidraScript {

    private static final String[][] SEG01_TARGETS = {
        {"4601", "main_game_loop"},
        {"48DC", "completion_signal_gate"},
        {"4859", "completion_input_helper"},
        {"493E", "transition_word_write_site"},
        {"4968", "completion_transition_branch"},
        {"4B8D", "completion_reentry_helper"},
        {"4EA0", "boss_transition_consumer"},
        {"503D", "post_transition_reentry_call_site"},
        {"504A", "clear_boss_transition_word"},
        {"504F", "post_transition_dispatch"},
        {"5165", "completion_word_check"},
        {"51F5", "completion_fallback_check"},
        {"313D", "reset_or_rebuild_game_state"},
        {"3FAD", "level_rebuild_and_resource_loader"},
        {"44D0", "completion_transition_helper"},
        {"44F0", "scripted_boss_sequence_helper"},
        {"45B1", "main_game_loop_entry"},
        {"347A", "final_progression_helper"},
        {"1084", "progression_input_helper"},
        {"1140", "initial_completion_gate_init"},
        {"114A", "initial_completion_signal_init"},
        {"114F", "initial_transition_word_init"},
        {"4B4F", "reentry_completion_signal_write"},
        {"4B82", "reentry_completion_gate_write"},
        {"4B87", "reentry_transition_word_write"},
        {"4C3A", "reentry_completion_signal_write_b"},
        {"4CA8", "reentry_completion_signal_write_c"},
        {"4CF3", "reentry_completion_signal_write_d"},
        {"4E10", "transition_gate_write_a"},
        {"4E8E", "transition_gate_write_b"},
        {"4ECC", "transition_gate_write_c"},
        {"5023", "post_dispatch_completion_write"},
        {"50E7", "post_dispatch_gate_write"},
        {"50EC", "post_dispatch_signal_write"},
        {"50F1", "post_dispatch_transition_write"},
        {"517C", "completion_loop_gate_clear"},
        {"5181", "completion_loop_signal_clear"},
        {"5203", "completion_fallback_gate_check"},
        {"520C", "completion_fallback_signal_clear"},
    };

    private static final String[][] SEG03_TARGETS = {
        {"0A43", "initialize_game_state"},
        {"0E06", "are_object_factory"},
        {"1036", "object_scheduler_insert"},
        {"1B77", "boss_helper_runtime_1b77"},
        {"1C6E", "boss_helper_runtime_1c6e"},
        {"393C", "compute_state_machine_bounds"},
        {"4B70", "shared_phase_child_constructor"},
        {"6616", "w3_phase_helper_constructor"},
        {"C955", "w3_phase_constructor_c955"},
        {"C9F8", "w3_phase_constructor_c9f8"},
        {"CA9B", "w3_phase_constructor_ca9b"},
        {"DC09", "w5_phase_constructor_dc09"},
        {"DCAC", "w5_phase_constructor_dcac"},
        {"DFB3", "w5_scrap_constructor_dfb3"},
        {"E01D", "w5_scrap_constructor_e01d"},
        {"E39E", "w5_effect_constructor_e39e"},
        {"E087", "w5_effect_constructor_e087"},
        {"E0BE", "w5_effect_constructor_e0be"},
        {"5D38", "map_cell_descriptor_5d38"},
        {"5D60", "map_cell_state_decay_5d60"},
        {"5C11", "boss_random_byte_5c11"},
        {"5C27", "map_tile_descriptor_query_5c27"},
        {"1BD1", "boss_map_helper_1bd1"},
        {"4C74", "w1_post_boss_child_callback_4c74"},
        {"CB11", "w3_phase_actor_callback_cb11"},
        {"DD22", "w5_phase_actor_callback_dd22"},
        {"E44B", "w5_effect_callback_e44b"},
        {"E1E0", "w5_effect_callback_e1e0"},
        {"DEF2", "w5_random_helper_def2"},
        {"0E66", "object_pool_count_active"},
        {"0E96", "object_update_pass_by_phase"},
        {"0FA2", "object_update_pass_nonzero_state"},
        {"1749", "create_dedicated_are_effect"},
        {"178D", "create_are_type_65"},
        {"1798", "create_are_type_66"},
        {"17A3", "create_are_type_67"},
        {"1E04", "instantiate_are_declaration"},
        {"199D", "player_transition_entry_a"},
        {"19E6", "player_transition_entry_b"},
        {"1A97", "player_transition_state_setup"},
        {"1AAA", "player_reentry_setup"},
        {"1AE6", "clear_player_transition_word"},
        {"3FF8", "update_player_object"},
        {"3A8A", "player_collision_transition_caller"},
        {"3AB3", "collision_transition_call_site"},
        {"1BC4", "object_overlap_transition_call_site"},
        {"43D0", "camera_boundary_transition_call_site"},
        {"4416", "player_transition_branch"},
        {"44DC", "player_transition_countdown"},
        {"4A5E", "w1_post_boss_child_callback"},
        {"49F2", "w1_post_boss_animation_advance"},
        {"45AB", "shared_projectile_callback"},
        {"487F", "shared_post_boss_contact_constructor"},
        {"489C", "shared_post_boss_contact_callback"},
        {"49FF", "w1_transition_aux_constructor"},
        {"92B3", "transition_aux_constructor_common_a"},
        {"92F2", "transition_aux_constructor_a"},
        {"95C7", "transition_aux_constructor_common_b"},
        {"9614", "transition_aux_constructor_b"},
        {"991A", "transition_aux_constructor_c"},
        {"98DB", "transition_aux_constructor_common_c"},
        {"1C4D", "transition_collision_helper_1c4d"},
        {"9627", "w1_post_boss_contact_followup"},
        {"9313", "w1_transition_child_callback_a"},
        {"993B", "w1_transition_child_callback_b"},
        {"B84D", "w1_short_lived_child_callback_a"},
        {"B87B", "w1_short_lived_child_callback_b"},
        {"B142", "w1_main_boss_constructor"},
        {"B1F0", "w1_helper_constructor"},
        {"B20B", "w1_doktor_constructor"},
        {"44FF", "reset_projectile_table"},
        {"44F8", "w1_cutscene_completion_write"},
        {"4996", "w1_transition_completion_write"},
        {"4A93", "w1_child_completion_write"},
        {"4AAC", "w1_child_transition_write"},
        {"92A9", "w1_effect_transition_write"},
        {"A2A5", "w1in_completion_write_a"},
        {"A2FE", "w1in_completion_write_b"},
        {"A82E", "w1in_completion_write_c"},
        {"A874", "w1in_completion_write_d"},
        {"F111", "program_timer_helper"},
        {"0B81", "initialize_are_dispatch_table"},
        {"106A", "completion_segment3_helper"},
        {"F07B", "completion_segment3_dispatch_helper"},
        {"0908", "completion_segment3_render_helper"},
        {"0A15", "completion_segment3_map_helper"},
        {"08C9", "completion_segment3_release_helper"},
        {"321F", "completion_segment3_transition_helper"},
        {"B25D", "w1_doktor_damage_callback"},
        {"B33B", "w1_main_boss_callback"},
        {"B11B", "w1_main_constructor_call_site"},
        {"B188", "w1_helper_constructor_dispatch"},
        {"B1BF", "w1_doktor_constructor_dispatch"},
        {"BB0E", "w2_doktor_damage_callback"},
        {"BBEC", "w2_main_boss_callback"},
        {"B9CC", "w2_main_constructor_call_site"},
        {"BA39", "w2_prop_constructor_dispatch"},
        {"BA70", "w2_doktor_constructor_dispatch"},
        {"B9F3", "w2_main_boss_constructor"},
        {"BAA1", "w2_prop_constructor"},
        {"BABC", "w2_doktor_constructor"},
        {"C104", "w2_helper_constructor_a"},
        {"C147", "w2_helper_constructor_b"},
        {"C18A", "w2_helper_constructor_c"},
        {"E572", "w2_effect_constructor_a"},
        {"E5D8", "w2_effect_constructor_b"},
        {"E63E", "w2_effect_constructor_c"},
        {"E671", "w2_effect_constructor_d"},
        {"C1A0", "w2_boss_helper_callback"},
        {"BAD7", "w2_boss_prop_callback"},
        {"E6A4", "w2_boss_effect_callback_a"},
        {"E836", "w2_boss_effect_callback_b"},
        {"C328", "w3_doktor_damage_callback"},
        {"C40B", "w3_main_boss_callback"},
        {"C264", "w3_main_constructor_call_site"},
        {"C2D6", "w3_doktor_constructor_dispatch"},
        {"C28A", "w3_main_boss_constructor"},
        {"C30D", "w3_doktor_constructor"},
        {"E9ED", "w3_effect_constructor_a"},
        {"EA53", "w3_effect_constructor_b"},
        {"EAB9", "w3_effect_constructor_c"},
        {"EAEC", "w3_effect_constructor_d"},
        {"EB1F", "w3_boss_effect_callback_a"},
        {"ECB1", "w3_boss_effect_callback_b"},
        {"EBE8", "w3_boss_effect_callback_c"},
        {"CDA3", "w4_doktor_damage_callback"},
        {"CE81", "w4_main_boss_callback"},
        {"CC41", "w4_main_constructor_call_site"},
        {"CCB7", "w4_helper_constructor_dispatch"},
        {"CCEE", "w4_doktor_constructor_dispatch"},
        {"CC68", "w4_main_boss_constructor"},
        {"CD25", "w4_helper_constructor"},
        {"CD88", "w4_doktor_constructor"},
        {"CD40", "w4_boss_helper_callback"},
        {"D55A", "w5_doktor_damage_callback"},
        {"D63D", "w5_main_boss_callback"},
        {"D2D0", "w5_main_constructor_call_site"},
        {"D379", "w5_doktor_constructor_dispatch"},
        {"D2F6", "w5_main_boss_constructor"},
        {"D3E1", "w5_effect_constructor_a"},
        {"D420", "w5_effect_constructor_b"},
        {"D498", "w5_effect_constructor_c"},
        {"D53F", "w5_doktor_constructor"},
        {"DFB6", "w5_scrap_constructor_a"},
        {"E020", "w5_scrap_constructor_b"},
        {"D3EE", "w5_boss_effect_callback_a"},
        {"D4D9", "w5_boss_effect_callback_b"},
        {"D438", "w5_boss_effect_callback_c"},
        {"E0F5", "w5_scrap_effect_callback_a"},
        {"E2BF", "w5_scrap_effect_callback_b"},
    };

    private static final String[][] SEG02_TARGETS = {
        {"0CAA", "state_probe_segment2_helper"},
    };

    private static final String[][] SEG05_TARGETS = {
        {"05CD", "runtime_stack_probe"},
        {"1B7E", "memory_fill_helper"},
        {"0271", "runtime_error_helper"},
    };

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 1) {
            printerr("Usage: -postScript DumpBossDecomp.java <output-directory>");
            return;
        }

        File outputDirectory = new File(args[0]);
        if (!outputDirectory.exists() && !outputDirectory.mkdirs()) {
            throw new Exception("Could not create output directory: " + outputDirectory);
        }

        String programName = currentProgram.getName();
        String[][] targets;
        if (programName.contains("SEG01")) {
            targets = SEG01_TARGETS;
        } else if (programName.contains("SEG02")) {
            targets = SEG02_TARGETS;
        } else if (programName.contains("SEG05")) {
            targets = SEG05_TARGETS;
        } else {
            targets = SEG03_TARGETS;
        }
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        decompiler.setSimplificationStyle("decompile");

        File report = new File(outputDirectory,
                currentProgram.getName().replaceAll("[^A-Za-z0-9_.-]", "_") + "-boss.c");
        try (PrintWriter writer = new PrintWriter(report, "UTF-8")) {
            writer.println("/* Targeted boss-transition decompilation from " +
                    currentProgram.getName() + " */");
            writer.println("/* Addresses are segment-relative raw offsets. */");
            writer.println();
            writeObjectLayout(writer);

            for (String[] target : targets) {
                int offset = Integer.parseInt(target[0], 16);
                Address address = toAddr(offset);
                Function function = ensureFunction(address, target[1]);
                writer.println("/* TARGET " + target[1] + " at 0x" + target[0] +
                        "; resolved function entry " +
                        (function == null ? "MISSING" : function.getEntryPoint()) + " */");
                if (function == null) {
                    writer.println();
                    continue;
                }
                writeCallers(writer, function);
                DecompileResults result = decompiler.decompileFunction(function, 180, monitor);
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

    private void writeCallers(PrintWriter writer, Function function) {
        writer.println("/* CALLERS of " + function.getEntryPoint() + ": */");
        ReferenceIterator references =
                currentProgram.getReferenceManager().getReferencesTo(function.getEntryPoint());
        boolean found = false;
        while (references.hasNext()) {
            Reference reference = references.next();
            Address from = reference.getFromAddress();
            Function caller = currentProgram.getFunctionManager().getFunctionContaining(from);
            writer.println("/*   " + from + " " + reference.getReferenceType() +
                    " in " + (caller == null ? "<unknown>" : caller.getEntryPoint()) +
                    " */");
            found = true;
        }
        if (!found) writer.println("/*   <none resolved> */");
    }

    private void writeObjectLayout(PrintWriter writer) {
        writer.println("/* Known pooled-object layout (0x78-byte stride; roles are provisional where noted): */");
        writer.println("/*   +0x02/+0x06  signed 16.16 world X/Y positions; +0x04/+0x08 are their integer words. */");
        writer.println("/*   +0x12        logical sprite slot (0xffff means no standard BOB slot). */");
        writer.println("/*   +0x14        ARE/object kind byte/word used by the generic kind scan. */");
        writer.println("/*   +0x17        scheduler phase byte; generic update passes visit phases 0, 1, 2. */");
        writer.println("/*   +0x18        far update callback (offset, segment). */");
        writer.println("/*   +0x1c        callback segment word initialized by the generic allocator. */");
        writer.println("/*   +0x1a        callback-side auxiliary word; allocator initializes it to 0xffff. */");
        writer.println("/*   +0x2a        role-dependent cursor/link; Doktor uses it as projectile scan cursor. */");
        writer.println("/*   +0x2c        role-dependent counter; Doktor uses it as damage/phase hit count. */");
        writer.println("/*   +0x2e/+0x2f  role-dependent state/timer bytes; Doktor uses the pair for hit rearm. */");
        writer.println("/*   +0x32        state-machine counter in shared transient/effect callbacks. */");
        writer.println("/*   +0x34        role-dependent mode/state byte in player and boss records. */");
        writer.println("/*   +0x36        role-dependent child/link word used by boss constructors. */");
        writer.println("/*   +0x38/+0x3c/+0x3e/+0x40/+0x42/+0x44  boss movement, animation, and timer words. */");
        writer.println("/*   +0x46/+0x48  additional world-specific phase/child-link words (W3/W5 evidence). */");
        writer.println("/*   The generic allocator scans a 64-entry pool; level-specific boss constructors */");
        writer.println("/*   then install custom callbacks, so bosses are not ARE dispatch entries. */");
        writer.println("/*   Factory -> scheduler insertion is visible at 01F7:0E60 -> 01F7:1036. */");
        writer.println();
    }

    private Function ensureFunction(Address address, String name) throws Exception {
        FunctionManager manager = currentProgram.getFunctionManager();
        Function exact = manager.getFunctionAt(address);
        if (exact != null) {
            return exact;
        }
        Function function = manager.getFunctionContaining(address);
        if (function != null) {
            // The raw 16-bit segment import can incorrectly absorb a later
            // world-specific entry into an earlier function (notably the W5
            // constructor immediately following the W4 ending caller). Split
            // that stale boundary in this disposable analysis project so the
            // report is keyed to the requested raw entry, not the container.
            manager.removeFunction(function.getEntryPoint());
        }
        if (currentProgram.getListing().getInstructionAt(address) == null) {
            disassemble(address);
        }
        function = manager.getFunctionAt(address);
        if (function == null) {
            try {
                function = manager.createFunction(name, address,
                        new AddressSet(address), SourceType.USER_DEFINED);
            } catch (Exception ignored) {
                function = manager.getFunctionContaining(address);
            }
        }
        return function;
    }
}
