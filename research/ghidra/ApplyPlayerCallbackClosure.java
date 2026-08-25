// GENERATED FILE. Do not edit; regenerate from
// research/ghidra/player-callback-closure.json with
// research/tools/generate_player_closure_ghidra.py.

import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.symbol.Symbol;
import ghidra.util.exception.DuplicateNameException;

public class ApplyPlayerCallbackClosure extends GhidraScript {
    private static final String[][] FUNCTIONS = new String[][] {
        {"3", "3FF8", "update_player", "closure=inline; evidence=raw:3FF8-44DB instruction decode; reloc:segment-3 callback callsites; trace:callback-barrier; trace:branch-focus", "3FF8", "44DC"},
        {"3", "44DC", "player_update_transition_motion", "closure=inline; evidence=raw:44DC-44FE; focused-audit:transition-gate-decrement", "44DC", "44FF"},
        {"3", "3F27", "initialize_player", "closure=inline; evidence=raw:3F27-3FF7; static:callback-pointer-write; trace:player-pool-offset-zero", "3F27", "3FF8"},
        {"3", "3AB9", "integrate_horizontal_motion", "closure=inline; evidence=raw:3AB9-3D01; reloc:3AB9 animation calls; trace:horizontal-fixtures", "3AB9", "3D02"},
        {"3", "3D02", "apply_descriptor_vertical_correction", "closure=inline; evidence=raw:3D02-3DF1; static:DX-20-40-branch-decode; trace:descriptor-branch-matrix", "3D02", "3DF2"},
        {"3", "3DF2", "snap_player_y_on_side_contact", "closure=inline; evidence=raw:3DF2-3E2F; trace:ordered-side-probe", "3DF2", "3E30"},
        {"3", "3A1F", "probe_player_side_clear", "closure=inline; evidence=raw:3A1F-3A61; trace:ordered-side-probe", "3A1F", "3A62"},
        {"3", "3A62", "apply_action_contact_side_effect", "closure=inline; evidence=raw:3A62-3A89; near-call:4392", "3A62", "3A8A"},
        {"3", "3998", "probe_forward_surface", "closure=inline; evidence=raw:3998-39FD; static:probe-order-and-step-gate", "3998", "39FE"},
        {"3", "3971", "probe_vertical_10px", "closure=inline; evidence=raw:3971-3985; static:coordinate-formula", "3971", "3986"},
        {"3", "3986", "probe_vertical_step", "closure=inline; evidence=raw:3986-3997; static:coordinate-formula", "3986", "3998"},
        {"3", "3376", "map_tile_id_at_pixel", "closure=inline; evidence=raw:3376-3399; trace:tile-id-probes", "3376", "339A"},
        {"3", "5C27", "probe_descriptor_quadrant", "closure=inline; evidence=raw:5C27-5C9C; trace:selector-safe-property-matrix", "5C27", "5C9D"},
        {"3", "5CC3", "read_descriptor_word", "closure=inline; evidence=raw:5CC3-5CFF; trace:descriptor-census", "5CC3", "5D00"},
        {"3", "1C6E", "probe_map_word_bit_4000", "closure=inline; evidence=raw:1C6E-1C91; callsite:4218", "1C6E", "1C92"},
        {"3", "1C92", "probe_map_word_bit_1000", "closure=inline; evidence=raw:1C92-1CB5; callsite:3971/3986", "1C92", "1CB6"},
        {"3", "6370", "probe_contact_tile_offset", "closure=inline; evidence=raw:6370-6483; trace:contact-tile-matrix", "6370", "6484"},
        {"3", "6484", "probe_contact_plus5", "closure=inline; evidence=raw:6484-648D; callsite:400F", "6484", "648E"},
        {"3", "648E", "probe_contact_right", "closure=inline; evidence=raw:648E-659B; trace:contact-tile-matrix", "648E", "659C"},
        {"3", "3A8A", "dispatch_special_tile_contact", "closure=inline; evidence=raw:3A8A-3AB8; reloc:tile-transition-targets", "3A8A", "3AB9"},
        {"3", "38CA", "apply_special_speed_cap_38CA", "closure=inline; evidence=raw:38CA-38EB", "38CA", "38EC"},
        {"3", "38EC", "player_action_effect_38EC", "closure=inline; evidence=raw:38EC-393B; static:action-bit-10-child-copy", "38EC", "393C"},
        {"3", "4519", "spawn_contact_effect_entry", "closure=contract; evidence=raw:4519-45AA; callsite:38EC", "4519", "45AB"},
        {"3", "5D38", "load_animation_descriptor", "closure=inline; evidence=raw:5D38-5D5F; trace:animation-frame-table", "5D38", "5D60"},
        {"3", "5D60", "advance_animation_descriptor", "closure=inline; evidence=raw:5D60-5D9F; trace:animation-frame-table", "5D60", "5DA0"},
        {"3", "3E41", "compute_view_delta", "closure=inline; evidence=raw:3E41-3F26; trace:camera-delta-fixtures", "3E41", "3F27"},
        {"3", "20AF", "publish_view_delta", "closure=inline; evidence=raw:20AF-20C7; callsite:4398/4445", "20AF", "20C8"},
        {"3", "199D", "write_transition_gate_199D", "closure=inline; evidence=raw:199D-19E5; callsite:43D0", "199D", "19E6"},
        {"3", "1BD1", "probe_transition_descriptor", "closure=contract; evidence=raw:1BD1-1C4C; callsite:44C5", "1BD1", "1C4D"},
        {"3", "1B07", "apply_tile_transition_1B07", "closure=contract; evidence=raw:1B07-1B42; callsite:3AAE", "1B07", "1B43"},
        {"3", "19E6", "apply_transition_reset_19E6", "closure=contract; evidence=raw:19E6-1A94; callsite:3AB3", "19E6", "1A95"},
        {"3", "F17F", "keyboard_irq1_ring_producer", "closure=contract; evidence=raw:F17F-F1A7; input:scan-code-ring", "F17F", "F1A8"},
        {"3", "F1A8", "poll_keyboard_ring_to_action_bits", "closure=inline; evidence=raw:F1A8-F21A; input:scan-code-mapping", "F1A8", "F21B"},
        {"3", "F21B", "read_normalized_action_bits", "closure=inline; evidence=raw:F21B-F231; reloc:callback direct target F21C", "F21B", "F232"},
    };

    private static final String[][] CONTRACTS = new String[][] {
        {"3", "0E06", "object_pool_factory_0E06", "closure=contract; evidence=reloc:6370/648E factory calls; trace:contact-object-creation"},
        {"2", "0FCF", "dispatch_pending_sound_effect", "closure=contract; evidence=reloc:segment-2 target; trace:sfx-dispatch"},
        {"2", "0CE3", "dispatch_transition_effect_0CE3", "closure=contract; evidence=reloc:199D call target"},
        {"3", "5937", "player_helper_5937", "closure=unresolved; evidence=reloc:3FF8 call target; dynamic:callback-entry barrier only"},
        {"3", "F21C", "input_dispatch_f21C", "closure=inline; evidence=reloc:callback direct target F21C; raw:F21B-F231"},
    };

    private static final String[][] LABELS = new String[][] {
        {"3", "3142", "animation_sequence_3142", "closure label; evidence=SI immediate before 5D38 calls"},
        {"3", "3156", "animation_sequence_3156", "closure label; evidence=SI immediate before 5D38 calls"},
        {"3", "3160", "animation_sequence_3160", "closure label; evidence=SI immediate before 5D38 calls"},
        {"3", "316A", "animation_sequence_316A", "closure label; evidence=SI immediate before 5D38 calls"},
        {"3", "3186", "animation_sequence_3186", "closure label; evidence=SI immediate before 5D38 calls"},
        {"3", "3190", "animation_sequence_3190", "closure label; evidence=SI immediate before 5D38 calls"},
        {"3", "31A4", "animation_sequence_31A4", "closure label; evidence=SI immediate before 5D38/5D60 calls"},
        {"3", "31BA", "animation_sequence_31BA", "closure label; evidence=SI immediate before 5D38 call"},
    };

    private static final String[][] GLOBALS = new String[][] {
        {"6", "30D4", "descriptor_record_stride", "typed player/global model"},
        {"6", "4FE2", "horizontal_accumulator", "typed player/global model"},
        {"6", "4FE4", "view_state_a", "typed player/global model"},
        {"6", "4FE6", "view_state_b", "typed player/global model"},
        {"6", "4FE8", "horizontal_accel", "typed player/global model"},
        {"6", "4FEC", "input_run_counter", "typed player/global model"},
        {"6", "4FEE", "idle_counter", "typed player/global model"},
        {"6", "4FF0", "action_low_copy", "typed player/global model"},
        {"6", "4FFE", "contact_y_scratch", "typed player/global model"},
        {"6", "5000", "contact_subtype", "typed player/global model"},
        {"6", "5001", "contact_code", "typed player/global model"},
        {"6", "5003", "contact_x_offset", "typed player/global model"},
        {"6", "60DC", "published_view_x", "typed player/global model"},
        {"6", "60E0", "published_view_y", "typed player/global model"},
        {"6", "612E", "pending_event", "typed player/global model"},
        {"6", "657A", "map_buffer_offset", "typed player/global model"},
        {"6", "657C", "map_buffer_selector", "typed player/global model"},
        {"6", "657E", "map_row_stride_bytes", "typed player/global model"},
        {"6", "6582", "descriptor_table_offset", "typed player/global model"},
        {"6", "6584", "descriptor_table_selector", "typed player/global model"},
        {"6", "8196", "secondary_actions", "typed player/global model"},
        {"6", "81C0", "camera_x", "typed player/global model"},
        {"6", "81C4", "camera_y", "typed player/global model"},
        {"6", "81CC", "camera_y_limit", "typed player/global model"},
        {"6", "85DA", "activation_state", "typed player/global model"},
        {"6", "880A", "transition_object_counter", "typed player/global model"},
        {"6", "8810", "timer_clear", "typed player/global model"},
        {"6", "8812", "deferred_y", "typed player/global model"},
        {"6", "8816", "external_x_delta", "typed player/global model"},
        {"6", "881A", "player_offset", "typed player/global model"},
        {"6", "8822", "transition_scratch", "typed player/global model"},
        {"6", "88B6", "special_speed_cap_mode", "typed player/global model"},
        {"6", "88BA", "last_scan_code", "typed player/global model"},
        {"6", "88BC", "keyboard_actions", "typed player/global model"},
        {"6", "8950", "transition_effect_bits", "typed player/global model"},
        {"6", "89E6", "action_suppressor", "typed player/global model"},
        {"6", "89EA", "collision_transition_mode", "typed player/global model"},
        {"6", "89EC", "transition_state", "typed player/global model"},
    };

    @Override
    public void run() throws Exception {
        int segment = segmentNumber(currentProgram.getName());
        for (String[] row : FUNCTIONS) {
            if (Integer.parseInt(row[0]) == segment)
                applyFunction(Integer.parseInt(row[1], 16), row[2], row[3],
                    Integer.parseInt(row[4], 16), Integer.parseInt(row[5], 16));
        }
        for (String[] row : CONTRACTS) {
            if (Integer.parseInt(row[0]) == segment)
                applyContract(Integer.parseInt(row[1], 16), row[2], row[3]);
        }
        for (String[] row : LABELS) {
            if (Integer.parseInt(row[0]) == segment)
                applyLabel(Integer.parseInt(row[1], 16), row[2], row[3]);
        }
        for (String[] row : GLOBALS) {
            if (Integer.parseInt(row[0]) == segment)
                applyLabel(Integer.parseInt(row[1], 16), row[2], row[3]);
        }
        println("Applied player callback ledger to " + currentProgram.getName());
    }

    private int segmentNumber(String program) {
        for (int i = 1; i <= 9; i++)
            if (program.contains(String.format("SEG%02d", i))) return i;
        return -1;
    }

    private Address address(int offset) {
        return currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(offset);
    }

    private void applyFunction(int offset, String name, String comment,
                               int rangeStart, int rangeEnd) throws Exception {
        Address entry = address(offset);
        AddressSet body = new AddressSet(address(rangeStart), address(rangeEnd - 1));
        disassembleRange(body, name);

        FunctionManager manager = currentProgram.getFunctionManager();
        Function function = manager.getFunctionAt(entry);
        if (function == null) {
            try {
                function = manager.createFunction(name, entry, body,
                    SourceType.USER_DEFINED);
            } catch (Exception failure) {
                println("Could not create " + name + " at " + entry + ": " +
                    failure.getClass().getSimpleName());
                return;
            }
        } else {
            if (!function.getEntryPoint().equals(entry)) {
                throw new Exception("requested entry " + entry +
                    " is contained by " + function.getEntryPoint());
            }
            try { function.setName(name, SourceType.USER_DEFINED); }
            catch (DuplicateNameException ignored) {
                println("Could not rename " + entry + " to " + name);
            }
        }
        function.setBody(body);
        function.setComment(comment);
    }

    private void disassembleRange(AddressSet body, String name) throws Exception {
        Address cursor = body.getMinAddress();
        while (cursor != null && body.contains(cursor)) {
            Instruction existing = currentProgram.getListing().getInstructionContaining(cursor);
            if (existing != null) {
                cursor = existing.getMaxAddress().add(1);
                continue;
            }
            DisassembleCommand command = new DisassembleCommand(cursor, body, false);
            command.enableCodeAnalysis(false);
            if (!command.applyTo(currentProgram, monitor))
                println("Disassembly reported a problem for " + name + " at " + cursor);
            Instruction decoded = currentProgram.getListing().getInstructionContaining(cursor);
            cursor = decoded == null ? cursor.add(1) : decoded.getMaxAddress().add(1);
        }
    }

    private void applyContract(int offset, String name, String comment) throws Exception {
        applyLabel(offset, name, comment);
    }

    private void applyLabel(int offset, String name, String comment) throws Exception {
        Address entry = address(offset);
        Symbol symbol = currentProgram.getSymbolTable().getPrimarySymbol(entry);
        if (symbol == null) {
            currentProgram.getSymbolTable().createLabel(entry, name, SourceType.USER_DEFINED);
        } else {
            try { symbol.setName(name, SourceType.USER_DEFINED); }
            catch (DuplicateNameException ignored) {
                println("Could not rename " + entry + " to " + name);
            }
        }
        CodeUnit unit = currentProgram.getListing().getCodeUnitAt(entry);
        if (unit != null) unit.setComment(CodeUnit.PLATE_COMMENT, comment);
    }
}
