// Evidence-backed labels for the raw-segment Quiky Ghidra project.
//
// Run with analyzeHeadless using this directory as a script path and
// -postScript AnnotateQuiky.java.  Segment-relative addresses are intentional:
// each raw segment was imported as its own Ghidra program at address zero.

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.symbol.Symbol;
import ghidra.util.exception.DuplicateNameException;

public class AnnotateQuiky extends GhidraScript {

    @Override
    public void run() throws Exception {
        String program = currentProgram.getName();
        if (program.contains("SEG01")) {
            annotateSegment1();
        } else if (program.contains("SEG02")) {
            annotateSegment2();
        } else if (program.contains("SEG03")) {
            annotateSegment3();
        } else if (program.contains("SEG04")) {
            annotateSegment4();
        } else if (program.contains("SEG05")) {
            annotateSegment5();
        }
        println("Annotated " + program);
    }

    private void annotateSegment1() throws Exception {
        label(0x101a, "cheat_string_QUIKYISTHEBEST", "Pascal string: QUIKYISTHEBEST");
        label(0x1029, "cheat_string_GOQUIKYGOQUIKY", "Pascal string: GOQUIKYGOQUIKY");
        label(0x1038, "cheat_string_NESQUIKISGREAT", "Pascal string: NESQUIKISGREAT");
        label(0x1047, "cheat_string_RUNQUIKYRUNRUN", "Pascal string: RUNQUIKYRUNRUN");
        label(0x1056, "cheat_string_THROWQUIKYDOIT", "Pascal string: THROWQUIKYDOIT");
        label(0x1065, "cheat_string_QUIKYSUPERHERO", "Pascal string: QUIKYSUPERHERO");

        label(0x3214, "path_smfont_pcc", "Pascal path: GAMEDATA\\SMFONT.PCC");
        label(0x3228, "path_infont_pcc", "Pascal path: GAMEDATA\\INFONT.PCC");
        label(0x323c, "path_bigfont_pcc", "Pascal path: GAMEDATA\\BIGFONT.PCC");
        label(0x3251, "path_menu_pcc", "Pascal path: GAMEDATA\\MENU.PCC");
        label(0x3263, "path_gamebar_pcc", "Pascal path: GAMEDATA\\GAMEBAR.PCC");
        label(0x3278, "path_introbar_pcc", "Pascal path: GAMEDATA\\INTROBAR.PCC");
        label(0x328e, "path_nes_pcc", "Pascal path: GAMEDATA\\NES.PCC");
        label(0x329f, "path_nes2_pcc", "Pascal path: GAMEDATA\\NES2.PCC");

        label(0x34b8, "path_template_are", "Pascal fragments: GAMEDATA\\ + .ARE");
        function(0x34c8, "load_are_resource", "Builds/loads an ARE resource after the .ARE path fragments.");
        label(0x364c, "path_template_map_primary", "Pascal fragments: GAMEDATA\\ + .MAP");
        function(0x365b, "load_map_resource_primary", "Builds/loads a MAP resource after the first .MAP path fragments.");
        label(0x3852, "path_template_map_secondary", "Pascal fragments: GAMEDATA\\ + .MAP");
        function(0x3861, "load_map_resource_secondary", "Second MAP-loading routine following the .MAP path fragments.");
        label(0x398e, "path_template_bob", "Pascal fragments: GAMEDATA\\ + .BOB");
        function(0x399e, "load_bob_resource", "Builds/loads a BOB resource after the .BOB path fragments.");
        label(0x3bae, "path_template_ico", "Pascal fragments: GAMEDATA\\ + .ICO");
        function(0x3bbd, "load_ico_resource", "Builds/loads an ICO resource after the .ICO path fragments.");
        function(0x4009, "load_initial_map_and_assets",
            "Primary level setup: calls 01D7:365B, then loads ICO/BOB assets selected by DS:85D4.");
        function(0x48b5, "transition_scheduler_loop",
            "Transition scheduler: waits on DS:819E, dispatches DS:89E6 events, and calls 01D7:3861 for third-level selectors 2/5/8/11/14 when DS:89EA and DS:880A permit.");

        label(0x491d, "level_selector_cheat_branch", "Runtime trace: checks the cheat-enabled level selector state.");
        function(0x01ac, "menu_input_action_pending",
            "Reads DS:8196 and DS:88BC; returns nonzero when an input action flag is pending.");
        function(0x0203, "wait_for_input_release",
            "Polls the normalized input flags until DS:8196 and DS:88BC are both clear.");
        function(0x47f0, "reset_game_input_flags",
            "Clears DS:8196 and DS:88BC before the state-machine/gameplay dispatch loop.");
        function(0x4ac2, "level_selector_input_loop",
            "Runtime-confirmed selector loop: consumes normalized flags 1/2 for level movement and 0x20 for launch.");
        function(0x0703, "high_score_insert",
            "Inserts an eligible score into the eight-record table and writes SCORE.DAT.");
        function(0x1084, "high_score_dispatch",
            "Dispatches high-score insertion and renders the post-session menu.");
        function(0x0470, "info_sound_quicky_exit_menu",
            "Post-death INFO/SOUND/QUICKY/EXIT menu renderer and result loop.");
        function(0x04ba, "info_sound_quicky_exit_menu_input",
            "Input/result helper for the post-death INFO/SOUND/QUICKY/EXIT menu.");
        function(0x3fad, "finalization_menu_update",
            "Menu/update helper called by 50B1 before the high-score dispatcher returns a result byte.");
        function(0x50b1, "preserved_score_finalization",
            "One-time preserved-score/finalization entry; selects the 1084 high-score dispatcher and loops on menu result.");
        function(0x347a, "session_save_cleanup",
            "Final save/cleanup helper reached after the result-3 finalization branch.");
    }

    private void annotateSegment2() throws Exception {
        label(0x084b, "path_template_sam_tfx", "Pascal fragments: GAMEDATA\\ + .SAM + .TFX");
        function(0x085e, "load_sam_tfx_resource", "Builds/loads SAM and TFX audio resources.");
        function(0x0fcf, "dispatch_pending_sound_effect",
            "Dispatches the pending gameplay effect when the FX gate and audio system are ready.");
        relocationTarget(0x0caa, "seg2_target_0caa", "NE relocation target referenced by segment 1.");
        relocationTarget(0x0fcf, "dispatch_pending_sound_effect",
            "Dispatches a pending gameplay effect; type-0x34 action paths also reach this sink.");
        function(0x168d, "dispatch_audio_driver_command",
            "Dispatches an audio-driver command through the driver jump table.");
        function(0x1761, "set_sound_blaster_rate",
            "Writes the Sound Blaster playback rate and command sequence.");
        function(0x17b5, "start_sound_blaster_output",
            "Starts the Sound Blaster output path after the driver buffer is ready.");
        function(0x17f1, "sound_blaster_irq_handler",
            "Acknowledges the Sound Blaster DSP and PIC interrupt for the output buffer.");
        function(0x190e, "program_sound_blaster_dma",
            "Programs the Sound Blaster DMA transfer from the prepared output buffer.");
        function(0x19ad, "stop_sound_blaster_output",
            "Stops the Sound Blaster output path and releases its active state.");
        function(0x1a26, "render_audio_buffer",
            "Renders one prepared driver output buffer from the software mixer state.");
        function(0x1a89, "mix_voice_into_output_buffer",
            "Mixes one software voice into the driver's 16-bit output buffer.");
        function(0x1f7f, "convert_mixed_word_to_output_byte",
            "Converts a mixed output word through the driver's byte table.");
        function(0x2984, "update_audio_driver_voices",
            "Advances music and effect voice state once per driver update.");
        function(0x2809, "control_audio_driver_tick",
            "Starts or stops the audio driver and derives its update cadence.");
        function(0x285e, "render_audio_driver_tick",
            "Renders one audio-driver buffer from the current software voice state.");
        function(0x2a0f, "commit_voice_mixer_state",
            "Commits pending voice control bits into the mixer channel state.");
        function(0x2b42, "run_voice_macro",
            "Runs one voice macro command or decrements its per-update wait counter.");
        function(0x2f3e, "advance_music_voice",
            "Advances the music-side voice state after the eight driver channels update.");
        function(0x31f5, "advance_music_sequence",
            "Advances the active music sequence when its voice reaches a boundary.");
        function(0x3237, "initialize_effect_voice",
            "Initializes a selected voice from the effect table and macro pointer.");
        function(0x3360, "select_effect_voice",
            "Selects and accepts or rejects an effect voice by priority and status.");
        function(0x33f8, "finish_music_sequence",
            "Finishes the active music sequence when its terminal voice is exhausted.");
    }

    private void annotateSegment4() throws Exception {
        label(0x0cb1, "path_nes_pcc_world_loader", "Pascal path: GAMEDATA\\NES.PCC");
        label(0x0cc1, "path_nes2_pcc_world_loader", "Pascal path: GAMEDATA\\NES2.PCC");
        label(0x0cd3, "path_nes3_pcc_world_loader", "Pascal path: GAMEDATA\\NES3.PCC");
        label(0x0ce5, "path_menu_pcc_world_loader", "Pascal path: GAMEDATA\\MENU.PCC");
        label(0x0cf7, "path_w1_pcc", "Pascal path: GAMEDATA\\W1.PCC");
        label(0x0d07, "path_w2_pcc", "Pascal path: GAMEDATA\\W2.PCC");
        label(0x0d17, "path_w3_pcc", "Pascal path: GAMEDATA\\W3.PCC");
        label(0x0d27, "path_w4_pcc", "Pascal path: GAMEDATA\\W4.PCC");
        label(0x0d37, "path_w5_pcc", "Pascal path: GAMEDATA\\W5.PCC");
        label(0x0d48, "path_ants_pcc", "Pascal path: GAMEDATA\\ANTS.PCC");
        label(0x0d5a, "path_gamebar_pcc_world_loader", "Pascal path: GAMEDATA\\GAMEBAR.PCC");
        label(0x0d6f, "path_introbar_pcc_world_loader", "Pascal path: GAMEDATA\\INTROBAR.PCC");
        label(0x8a92, "archive_last_entry_index", "Runtime-confirmed u32 trailer value; 141 for the 142-entry NESTLE.DAT index.");
        label(0x8a96, "archive_pascal_names", "Runtime-confirmed table of 13-byte Pascal-name slots populated from the variable-length archive directory.");
        label(0x94be, "archive_payload_offsets", "Runtime-confirmed parallel table of little-endian u32 absolute payload offsets.");
        label(0x97de, "archive_directory_offset", "Runtime-confirmed u32 read from the first half of the final 8-byte NESTLE.DAT trailer.");
        label(0x97e2, "archive_file_handle", "Open NESTLE.DAT file handle used by resource lookup and stream helpers.");
        int[] targets = {
            0x022a, 0x125b, 0x170a, 0x1737, 0x17a0, 0x1859,
            0x18c7, 0x19ff, 0x1a37, 0x1a73, 0x1b45, 0x1bde,
            0x2a07, 0x2a5c, 0x2aaf, 0x2af3, 0x2b0d, 0x2b38,
        };
        for (int target : targets) {
            relocationTarget(target, String.format("seg4_target_%04x", target),
                "NE relocation target in segment 4; semantics not assigned yet.");
        }
        relocationTarget(0x18c7, "resource_entry_lookup",
            "Runtime-confirmed with W1L3.MAP and W1L3.ARE: accepts a far Pascal path and writes end/start/size at DS:97E4/97E8/97EC.");
        relocationTarget(0x19ff, "resource_end_check_candidate",
            "Inferred from decompilation: compares the current resource position with the shared end state.");
        relocationTarget(0x125b, "resource_seek_relative",
            "Runtime-confirmed with W1L3.MAP: seeks to resource start plus the supplied unsigned 32-bit offset; offset 4 reached the first MAP field.");
        relocationTarget(0x1a37, "resource_tell_relative",
            "Runtime-confirmed with W1L3.MAP: returns the current stream position relative to DS:97E8; returned 10 after three big-endian words.");
        relocationTarget(0x1bde, "resource_read_be_u16",
            "Runtime-confirmed with W1L3.MAP: consumed bytes 00 37 at resource offset 4 and returned 0037.");
        relocationTarget(0x170a, "resource_buffer_fill",
            "Runtime-confirmed with W1L1.MAP: reads the supplied byte count from the supplied handle into the far buffer at DS:8A8C and resets DS:8A90.");
        relocationTarget(0x1737, "resource_buffer_read_u8",
            "Runtime-confirmed with W1L1.MAP: returns the byte at DS:8A8C plus DS:8A90, then increments DS:8A90.");
        function(0x1c1d, "file_seek_absolute",
            "DOS int 21h/AH=42h, origin 0: seeks the supplied handle to the supplied unsigned 32-bit offset.");
        function(0x1c43, "file_seek_end_and_tell",
            "DOS int 21h/AH=42h, origin 2, offset 0: seeks to EOF and returns the absolute file size.");
    }

    private void annotateSegment3() throws Exception {
        int[] targets = {0x05a0, 0x1036, 0x106a, 0x1ec4, 0x332c, 0x335e, 0x33bf, 0x342f,
            0x1b5d, 0x1b77, 0x1c4d, 0x1c6e, 0x39fe,
            0x5c27, 0x5cc3, 0x5d00, 0x5d38, 0x5d60, 0x6370};
        for (int target : targets) {
            relocationTarget(target, String.format("seg3_target_%04x", target),
                "NE relocation target in segment 3; semantics not assigned yet.");
        }
        function(0x33bf, "map_low_id_normalizer",
            "Whole-MAP pass: for low IDs 2, 3, and 4, preserves upper property bits and reinserts the ID.");
        function(0x1cda, "stream_are_regions",
            "Streams ARE declarations for newly visible 64-pixel regions using camera coordinates and the reference grid.");
        function(0x0e66, "object_pool_count_active",
            "Counts non-free entries in the 64-entry pooled-object array and publishes DS:88C8.");
        function(0x0e96, "object_update_pass_by_phase",
            "Runs pooled-object callbacks in phase order using object byte +0x17 values 0, 1, and 2.");
        function(0x1036, "register_object_scheduler_entry",
            "Appends the live callback and object offset to the active scheduler bank and writes its terminator.");
        function(0x0f3c, "find_object_kind_0x64",
            "Scans the object list for an object whose +0x14 kind field equals 0x64; ownership semantics remain unresolved.");
        function(0x0fa2, "object_update_pass_nonzero_state",
            "Runs callbacks for list entries with a non-null callback pointer; list/object state semantics remain unresolved.");
        function(0x0b142, "create_b33b_owner",
            "Creates the B33B phased owner and its linked B226/B25D records; lifecycle contract under dynamic validation.");
        function(0x0b1f0, "create_b226_linked_object",
            "Initializes the B226 linked animation record through the shared sequence loader.");
        function(0x0b20b, "create_b25d_linked_object",
            "Initializes the B25D linked animation record through the shared sequence loader.");
        function(0x0b226, "update_b226_animation",
            "B226 linked animation callback with phase-dependent camera visibility gate.");
        function(0x0b25d, "update_b25d_animation",
            "B25D linked animation callback and target/effect tail dispatch.");
        function(0x0b266, "b25d_tail_dispatch",
            "B25D tail branch reached after the target/effect update; teardown behavior under validation.");
        function(0x0b2b0, "b25d_animation_step",
            "B25D animation-step helper reached from the linked callback.");
        function(0x0b2b8, "b25d_animation_step_alt",
            "B25D animation-step branch.");
        function(0x0b2ba, "b25d_animation_step_alt2",
            "B25D animation-step branch.");
        function(0x0b2bf, "b25d_animation_step_alt3",
            "B25D animation-step branch.");
        function(0x0b2c4, "b25d_effect_action_call",
            "B25D action/effect call site.");
        function(0x0b303, "b25d_callback_tail",
            "B25D callback tail and return path.");
        function(0x0b33b, "update_b33b_owner",
            "B33B phased owner callback; advances linked records and creates the B84D/B87B transition.");
        function(0x0b84c, "prepare_b87b_transition",
            "B84C transition helper associated with the B84D/B87B linked lifecycle.");
        function(0x0b84d, "initialize_b87b_transition",
            "B84D replaces the current callback with B87B and initializes its transition state.");
        function(0x0b87b, "update_b87b_transition",
            "B87B moving transition callback with strict camera gate and MAP descriptor probes.");
        function(0x0487f, "initialize_late_owner",
            "Late-phase owner initializer reached after the B33B phase-5 conversion.");
        function(0x0489c, "update_late_owner",
            "Steady late-phase owner callback reached after the 487F initializer.");
        function(0x08c9, "release_map_buffer",
            "Releases the current MAP buffer through the runtime helper and clears DS:657A/657C when it is present.");
        function(0x0a43, "initialize_game_state",
            "Initializes MAP pointers, input words, event storage, PRNG bytes, and sprite-slot tables before gameplay.");
        function(0x1ed7, "update_camera_scroll",
            "Updates 16.16 camera scroll from DS:36FC/36FE/3700/3702 target bounds, clamps it, and derives DS:81CE/81D0.");
        function(0x1e04, "instantiate_are_declaration",
            "Runtime-confirmed six-byte ARE record walker: type, local X, local Y; marks records processed and creates objects at region origin plus local coordinates.");
        function(0x1dca, "object_camera_visibility_gate",
            "Returns carry set when ES:DI+04/+08 falls outside the camera window derived from DS:81C0/81C4; uses a 0x80 margin and 0x240/0x1B0 extents.");
        function(0x1dee, "deactivate_object_outside_camera",
            "Clears ES:DI+18 and the byte at FS:[ES:DI+1A+1] after the camera gate rejects an object.");
        function(0x393c, "compute_player_collision_bounds",
            "Returns four bounds from the object pointed to by DS:881A: position fields plus +2C/+30/+2E/+32, or four zeroes when DS:89EA is nonzero.");
        function(0x39fe, "query_player_collision_state",
            "Returns the persistent-player X/Y and collision-class byte used by the type-0x34 proximity test.");
        function(0x3f27, "initialize_player_record",
            "Initializes the persistent player object: stores ES:DI into DS:881A, clears player globals, and installs callback 01F7:3FF8; runtime W1L1 pool offset 0 confirms this record.");
        function(0x3ff8, "update_player_record",
            "Persistent player callback installed by 01F7:3F27; checks DS:89EA, runs MAP collision probes, and advances the player record at ES:DI.");
        function(0x44ff, "reset_contact_effect_table",
            "Resets the contact-effect table and its runtime ring state.");
        function(0x4519, "spawn_contact_effect_entry",
            "Adds a contact-effect entry and publishes its ring coordinates.");
        function(0x45ab, "update_contact_effect_entry",
            "Updates an active contact-effect entry and its ring coordinates.");
        function(0x470c, "remove_contact_effect_entry",
            "Removes a contact-effect entry and advances the ring state.");
        function(0x58a0, "clear_ufo_contact_effect",
            "Clears the UFO contact-effect state and its ring coordinates.");
        function(0x3a1f, "player_probe_side_clear",
            "Player callback helper reached during grounded/collision resolution; exact return flags remain to be correlated with controlled input.");
        function(0x3a62, "player_collision_probe_3a62",
            "Player callback helper reached during finalization of a movement step; exact field semantics remain provisional.");
        function(0x3a8a, "player_probe_transition_tiles",
            "Static target in the zero-DS:89EA player path; correlate its MAP reads and return flags with controlled input.");
        function(0x3ab9, "player_collision_probe_3ab9",
            "Player callback helper reached near the final object update path; semantics remain provisional.");
        function(0x3d02, "player_resolve_descriptor_response",
            "Calls the descriptor query, retries at y-8 when DX&30 is clear, then uses DX&20 for vertical response polarity/state and DX&40 for the integer-Y target alignment before returning the correction result.");
        function(0x3df2, "player_snap_y_on_side_contact",
            "Player movement helper: when the response state permits, probes descriptor quadrants at X-5/X+5 and snaps the integer Y word to an eight-pixel boundary when either probe reports occupancy.");
        function(0x3e41, "player_collision_probe_3e41",
            "Player callback helper reached while committing the post-collision state.");
        function(0x6484, "player_probe_hazard_plus5",
            "Collision helper identified by the player callback call graph; runtime input correlation is pending.");
        function(0x648e, "player_probe_hazard_right",
            "Collision helper reached by the player callback; a controlled W1L1 right-input run hits this entry at the persistent ES:0000 record; return semantics remain under test.");
        function(0x487f, "alternate_completion_initializer",
            "Initializes the late ending/cutscene object; runtime callers are the world-specific blocks at B82B/C0E2/C933/D2A8/DBE9.");
        function(0x489c, "alternate_completion_callback",
            "Updates the late ending/cutscene object and enters the +5000 tally on terminal overlap.");
        function(0x4968, "alternate_score_tally",
            "Adds the late +5000 completion score and selects the route-specific continuation.");
        function(0xb82b, "ending_caller_world_1",
            "World-specific ending/cutscene caller; writes DS:88AE phase 5 and creates the follow-on object.");
        function(0xc0e2, "ending_caller_world_2",
            "World-specific ending/cutscene caller; writes DS:88AE phase 5 and creates the follow-on object.");
        function(0xc933, "ending_caller_world_3",
            "World-specific ending/cutscene caller; writes DS:88AE phase 6 and creates the follow-on object.");
        function(0xd2a8, "ending_caller_world_4",
            "World-specific ending/cutscene caller; writes DS:88AE phase 5 and creates the follow-on object.");
        function(0xdbe9, "ending_caller_world_5",
            "World-specific ending/cutscene caller; writes DS:88AE phase 6 and creates the follow-on object.");
        function(0x69ff, "player_bounds_or_collision_69ff",
            "Reads the persistent offset-zero player record while comparing another object position; higher-level role remains provisional.");
        function(0x44dc, "player_control_transition_44dc",
            "Decrements DS:89EA and handles the transitional vertical-motion control path.");
        function(0x0e06, "are_object_factory",
            "Scans the 64-entry pooled-object array and initializes a free object; the normal ARE path returns ES:DI and type 0x2B is initialized by the caller.");
        function(0x1749, "create_dedicated_are_effect",
            "Shared creator used by types 0x65/0x66/0x67 after selecting subtype 0x00/0x08/0x10.");
        function(0x178d, "create_are_type_65", "Dedicated ARE type 0x65 wrapper.");
        function(0x1798, "create_are_type_66", "Dedicated ARE type 0x66 wrapper.");
        function(0x17a3, "create_are_type_67", "Dedicated ARE type 0x67 wrapper.");
        function(0x4727, "update_falling_leaves_types_29_2b",
            "Dispatch-table callback shared by ARE types 0x29, 0x2A, and confirmed falling-leaves type 0x2B.");
        function(0x8c4e, "init_are_type_2c",
            "Observed initializer callback for ARE type 0x2C; runtime changes its slot to 0x8D20 after the first update.");
        function(0x8d20, "update_are_type_2c",
            "Observed steady callback for ARE type 0x2C; input-trace samples show stable state fields and a persistent object callback.");
        function(0x8d31, "update_are_type_2c_action_helper",
            "Type 0x2C callback helper: consumes bounds from 0x393C, branches on object+0x2C, and has a terminal callback-clear path at 0x8E42.");
        function(0x87d1, "init_are_type_33",
            "Observed initializer callback for ARE type 0x33; runtime changes its slot to 0x882F after the first update.");
        function(0x882f, "update_are_type_33",
            "Observed steady callback for ARE type 0x33; runtime state transitions during initialization settle to state 0x0100.");
        function(0x9bee, "init_are_type_34",
            "Observed initializer callback for ARE type 0x34; runtime changes its slot to 0x9C0C after the first update.");
        function(0x9c0c, "update_are_type_34",
            "Observed steady callback for ARE type 0x34; input-trace samples show stable state fields and a persistent object callback.");
        function(0x9c29, "test_type34_proximity",
            "Type-0x34 helper: applies strict X/Y proximity bounds gated by the persistent player's collision-class byte.");
        function(0x9256, "update_are_type_28",
            "Dispatch-table callback for normal ARE type 0x28, whose object class is zero.");
        function(0x3376, "map_tile_id_lookup_16px",
            "Converts 16-pixel coordinates in AX/BX to a MAP cell address using DS:657A/657E and returns only the low 9-bit tile ID.");
        function(0x16ce, "map_effect_tile_rewrite",
            "Rewrites one loaded MAP cell as (word & 0xfe00) | (DX & 0x01ff), unless DX bit 0x8000 requests the non-MAP path; called by tile-effect state updates.");
        function(0x339a, "map_low_id_writer",
            "Coordinate-selected loaded-MAP writer: preserves upper property bits, then ORs unmasked CX; caller supplies the low-ID bits.");
        function(0x340a, "map_property_writer",
            "Coordinate-selected loaded-MAP writer: preserves the low nine-bit tile ID, then ORs unmasked CX; caller supplies the upper-property bits.");
        function(0x1b5d, "apply_player_displacement",
            "Type-0x34 action helper: updates player state and applies the observed fixed-point displacement before the effect sink.");
        function(0x1b77, "save_collision_probe_context",
            "Saves the four incoming collision registers before the type-0x33 MAP contact chain.");
        function(0x1c4d, "check_object_map_contact",
            "Forms the directional type-0x33 MAP probe and forwards it to the 16-pixel raw MAP-word helper.");
        function(0x1c6e, "map_word_probe_16px",
            "Computes a 16-pixel MAP address, returns the raw word, and tests bit 0x4000.");
        function(0x5c27, "map_descriptor_quadrant_test",
            "Masks a raw MAP cell to its low 9-bit tile ID, indexes DS:6582 by DS:30D4, and tests descriptor flags against coordinate bit 3.");
        function(0x5cc3, "map_descriptor_word_at_pixel",
            "Masks a raw MAP cell to its low 9-bit tile ID, indexes DS:6582 by DS:30D4, and returns the descriptor word in DX.");
        function(0x5c9d, "map_cell_word_store",
            "Stores a complete CX word into one loaded MAP cell using the row stride and coordinate-derived byte offset.");
        function(0x5d00, "map_cell_descriptor_5d00",
            "Builds a nearby MAP-cell descriptor used by player movement; exact field meanings remain under analysis.");
        function(0x5d38, "load_animation_descriptor",
            "Loads a descriptor/table entry into object animation state and selects the current slot/action.");
        function(0x5d60, "advance_animation_descriptor",
            "Decrements the active descriptor timer or advances the descriptor cursor when it expires.");
        function(0x6370, "player_probe_hazard_offset",
            "MAP tile-ID collision helper parallel to 648E; calls 3376 and applies tile IDs 5-10 to player state.");
        function(0x8d20, "update_collectible_effect",
            "Updates collectible-effect objects: runs the visibility gate, then delegates to the player-bounds state routine.");
        function(0x8d31, "update_collectible_state",
            "Collectible state routine: tests strict player-bounds overlap, applies object state, and writes pending effect IDs.");
        function(0x8e4b, "update_tile_effect_state_machine",
            "Dispatches the shared tile-effect object state machine through object +0x32; state branches call the MAP lookup and create transient effects.");
        function(0x20c8, "render_map_column",
            "Reads MAP cells, masks each to the low 9-bit tile ID, and writes column-interleaved VGA tile pixels.");
        function(0x2cb2, "render_map_strip",
            "Reads MAP cells, masks each to the low 9-bit tile ID, and writes a second camera-oriented VGA strip.");
        function(0xf17f, "keyboard_irq1_handler",
            "IRQ1 handler: stores port-60 scan bytes in the 32-byte ring at selector FFFF:501E and advances DS:503E.");
        function(0xf1a8, "poll_keyboard_ring_to_input_flags",
            "Consumes the keyboard ring, handles make/break bytes, maps arrows/Space to DS:88BC, and stores the last scan code in DS:88BA.");
        function(0xf21b, "read_normalized_input_flags",
            "Returns DS:88BC OR DS:8196, the normalized action flags consumed by the game/menu loops.");
        label(0x3714, "are_region_origin_x", "Runtime-confirmed 64-pixel-aligned X origin used while instantiating an ARE declaration.");
        label(0x3716, "are_region_origin_y", "Runtime-confirmed 64-pixel-aligned Y origin used while instantiating an ARE declaration.");
        label(0x81d2, "are_entity_dispatch_table", "Four-byte entries indexed by ARE entity type; normal types feed these values to the object factory.");
        label(0x8196, "input_action_flags", "Normalized action flags ORed with DS:88BC by the input helper.");
        label(0x81c0, "camera_x", "Current integer camera X used by visibility, streaming, and renderer clipping.");
        label(0x81c4, "camera_y", "Current integer camera Y used by visibility, streaming, and renderer clipping.");
        label(0x81ce, "camera_subtile_x", "Derived camera sub-tile X value written by update_camera_scroll.");
        label(0x81d0, "camera_subtile_phase", "Derived camera phase byte written by update_camera_scroll.");
        label(0x755e, "object_pool_base", "Base of the 64-entry pooled-object array.");
        label(0x7566, "scheduler_bank_a", "First eight-byte object scheduler bank.");
        label(0x7766, "scheduler_bank_b", "Second eight-byte object scheduler bank.");
        label(0x7966, "scheduler_insert_cursor", "Scheduler bank selector and insertion cursor.");
        label(0x7968, "stream_reference_grid_stride", "Reference-grid stride used by directional ARE streaming.");
        label(0x796e, "are_source_segment_selector", "Segment value used to access the source ARE marker through object+0x1A.");
        label(0x30ce, "object_stride_bytes", "Pooled-object record stride, 0x78 bytes.");
        label(0x30d4, "tile_descriptor_stride", "Stride used to index a tile descriptor record.");
        label(0x657a, "map_buffer_offset", "Offset of the currently loaded MAP buffer.");
        label(0x657c, "map_buffer_segment", "Segment of the currently loaded MAP buffer.");
        label(0x657e, "map_row_stride_bytes", "Byte stride between MAP rows.");
        label(0x6582, "tile_descriptor_table_offset", "Offset of the tile descriptor table.");
        label(0x6584, "tile_descriptor_table_segment", "Segment of the tile descriptor table.");
        label(0x85da, "type34_activation_state", "State word checked by the type-0x34 callback against 0x32; broader role is provisional.");
        label(0x881a, "player_object_offset", "Persistent player object offset; W1L1 uses pool record offset zero.");
        label(0x89ea, "player_transition_mode", "Shared zero/nonzero mode selecting ordinary callback versus transition block.");
        label(0x612e, "pending_action_word", "Action word changed from zero to four on the controlled type-0x34 proximity hit.");
        label(0x504c, "pending_effect_code", "Effect code published by the type-0x34 action chain; observed value is 0x2A.");
        label(0x88bc, "keyboard_action_flags", "Normalized make/break action flags populated by poll_keyboard_ring_to_input_flags.");
        label(0x88ba, "last_keyboard_scan_code", "Most recently consumed keyboard scan code.");
    }

    private void annotateSegment5() throws Exception {
        int[] targets = {
            0x033e, 0x0358, 0x05cd, 0x0591, 0x08e9, 0x0a0d,
            0x0d72, 0x0e87, 0x0f06, 0x1b5a, 0x1b7e,
        };
        for (int target : targets) {
            relocationTarget(target, String.format("seg5_target_%04x", target),
                "NE relocation target in segment 5; semantics not assigned yet.");
        }
        relocationTarget(0x05cd, "runtime_stack_probe",
            "Confirmed from decompilation: checks available stack space before a large local frame.");
        relocationTarget(0x0e87, "pascal_string_copy",
            "Confirmed from decompilation: copies a length-prefixed Pascal string.");
        relocationTarget(0x0f06, "pascal_string_append",
            "Confirmed from decompilation: appends a length-prefixed string to another Pascal string.");
    }

    private void label(int offset, String name, String comment) throws Exception {
        Address address = toAddr(offset);
        Symbol symbol = currentProgram.getSymbolTable().getPrimarySymbol(address);
        if (symbol == null) {
            currentProgram.getSymbolTable().createLabel(address, name, SourceType.USER_DEFINED);
        } else {
            try {
                symbol.setName(name, SourceType.USER_DEFINED);
            } catch (DuplicateNameException ignored) {
                println("Could not rename existing symbol at " + address + " to " + name);
            }
        }
        CodeUnit unit = currentProgram.getListing().getCodeUnitAt(address);
        if (unit != null && comment != null) {
            unit.setComment(CodeUnit.PLATE_COMMENT, comment);
        }
    }

    private void function(int offset, String name, String comment) throws Exception {
        Address address = toAddr(offset);
        FunctionManager manager = currentProgram.getFunctionManager();
        Function function = manager.getFunctionAt(address);
        if (function == null) {
            try {
                function = manager.createFunction(name, address, new AddressSet(address), SourceType.USER_DEFINED);
            } catch (Exception overlap) {
                // Auto-analysis may already have claimed a containing function.
                // Keep the annotation pass useful for the remaining targets.
                println("Could not create function at " + address + ": " + overlap.getClass().getSimpleName());
                return;
            }
        } else {
            try {
                function.setName(name, SourceType.USER_DEFINED);
            } catch (DuplicateNameException ignored) {
                println("Could not rename existing function at " + address + " to " + name);
            }
        }
        function.setComment(comment);
    }

    private void relocationTarget(int offset, String name, String comment) throws Exception {
        function(offset, name, comment);
    }

}
