// Evidence-backed second-pass annotation for the player collision kernel.
//
// Run against the raw segment-3 Ghidra program with analyzeHeadless.  The
// player record is implicit ES:DI in the original ABI; the PlayerRecord data
// type below is an analysis aid for naming offsets, not a claim that the
// compiler passed a C pointer on the stack.

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.ByteDataType;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.IntegerDataType;
import ghidra.program.model.data.ShortDataType;
import ghidra.program.model.data.Structure;
import ghidra.program.model.data.StructureDataType;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.symbol.Symbol;
import ghidra.util.exception.DuplicateNameException;

public class AnnotatePlayerCollision extends GhidraScript {

    @Override
    public void run() throws Exception {
        if (currentProgram.getName().contains("SEG06")) {
            annotateGlobals();
            println("Annotated DS globals in " + currentProgram.getName());
            return;
        }
        if (!currentProgram.getName().contains("SEG03")) {
            println("Skipping " + currentProgram.getName() + "; player kernel is in SEG03");
            return;
        }

        annotateFunctions();
        createPlayerRecordType();
        println("Annotated player collision kernel in " + currentProgram.getName());
    }

    private void annotateFunctions() throws Exception {
        function(0x3f27, "initialize_player_record",
            "Initializes the persistent ES:DI player record and installs callback 3FF8.");
        function(0x3ff8, "update_player_record",
            "Player callback. Ordinary path calls hazard_right, hazard_plus5, transition_tile_probe, then state-dependent descriptor leaves.");
        function(0x3376, "map_tile_id_at_pixel",
            "Far leaf: AX=y pixels, BX=x pixels; reads MAP[(y>>4)*row_stride+(x>>4)*2] and returns raw_cell & 0x01ff.");
        function(0x5c27, "map_descriptor_quadrant_test",
            "Far leaf: AX=y, BX=x; reads descriptor record +2 and tests the low-nibble bit selected by AX/BX bit 3. Returns the test in flags.");
        function(0x5cc3, "map_descriptor_word_at_pixel",
            "Far leaf: AX=y, BX=x; returns descriptor record +2 in DX. Directly consumed by descriptor_response_resolver.");
        function(0x3a1f, "player_probe_side_clear",
            "Near leaf: if gate +0x38 is clear and mode +0x37 is not FF, probes x-5 then x+5 at current y through 5C27; writes +0x3B=FF only when both clear.");
        function(0x3df2, "player_snap_y_on_side_contact",
            "Near leaf: requires +0x3B!=0 and +0x3A==0, probes x-5/x+5 through 5C27, and writes object y high word &= FFF8 when either probe reports occupancy. It does not snap X.");
        function(0x3d02, "player_resolve_descriptor_response",
            "Near leaf: gates on +0x3B, reads descriptor at (x,y), retries at y-8 when DX&30 is clear, halves velocity Y, computes target Y, and returns at 3D44/3DE4/3DF1.");
        function(0x3a8a, "player_probe_transition_tiles",
            "Far leaf: when mode +0x37 is positive, reads the tile ID at (x,y) and dispatches IDs 0B/0C/0D to transition handlers.");
        function(0x648e, "player_probe_hazard_right",
            "Far hazard/effect probe at x+5. Uses tile IDs 5..A and spawns a transient object; this is separate from descriptor geometry.");
        function(0x6484, "player_probe_hazard_plus5",
            "Far wrapper: stores hazard probe offset +5 at DS:5003 and calls hazard_offset_probe.");
        function(0x6370, "player_probe_hazard_offset",
            "Hazard/effect probe using x+DS:5003 and y or y-+0x72; recognizes tile IDs 5..A and spawns a transient object.");
        function(0x1b07, "player_begin_tile_transition",
            "Transition-state helper: clears response bits, zeroes timer, sets mode FF, copies +0x64 into velocity Y, and clears +0x3B/+0x3A.");
        function(0x19e6, "player_apply_transition_reset",
            "Transition/reset helper reached from the 0B/0C/0D tile path; resets player motion and shared transition globals.");
        function(0x44dc, "player_update_transition_motion",
            "Transition motion helper reached from the callback's nonzero DS:89EA path.");
    }

    private void annotateGlobals() throws Exception {
        global(0x30d4, "tile_descriptor_record_stride", "Descriptor table record stride; initialized/runtime-confirmed as 4 bytes.");
        global(0x657a, "map_buffer_offset", "Offset of the currently loaded MAP buffer.");
        global(0x657c, "map_buffer_selector", "Selector of the currently loaded MAP buffer.");
        global(0x657e, "map_row_stride_bytes", "Byte stride between loaded MAP rows.");
        global(0x6582, "tile_descriptor_table_offset", "Offset of the active 512-entry tile descriptor table.");
        global(0x6584, "tile_descriptor_table_selector", "Selector of the active tile descriptor table.");
        global(0x4ffe, "hazard_probe_y_word", "Temporary integer Y used by the tile hazard/effect probes.");
        global(0x5000, "hazard_spawn_gate", "Temporary spawned-object gate byte: 0 for tile IDs 8..A, FF for IDs 5..7.");
        global(0x5001, "hazard_spawn_callback", "Temporary spawned-object callback selector/offset selected by tile ID.");
        global(0x5003, "hazard_probe_x_offset", "Temporary X offset consumed by hazard_offset_probe; hazard_plus5 writes 5.");
        global(0x612e, "pending_player_effect_code", "Shared pending action/effect word written as 7 by the hazard/effect path.");
        global(0x8810, "player_timer_shared", "Shared player timer cleared by the callback when +0x34 reaches zero.");
        global(0x8812, "platform_player_y_delta", "Fixed-point Y delta published by moving-platform carry and consumed at callback entry.");
        global(0x8816, "platform_player_x_delta", "Fixed-point X delta published by moving-platform carry and consumed by the player movement path.");
        global(0x881a, "player_record_offset", "Persistent ES:DI player record offset.");
        global(0x89ea, "player_transition_mode", "Shared zero/nonzero mode selecting ordinary callback versus transition block.");
    }

    private Structure createPlayerRecordType() throws Exception {
        Structure record = new StructureDataType("QuikyPlayerRecord", 0x78);
        field(record, 0x00, new ShortDataType(), "action_word", "Action/input word consumed by the callback.");
        field(record, 0x02, new IntegerDataType(), "x_fixed_16_16", "Fixed-point X position; integer high word is +0x04.");
        field(record, 0x06, new IntegerDataType(), "y_fixed_16_16", "Fixed-point Y position; integer high word is +0x08.");
        field(record, 0x0a, new IntegerDataType(), "velocity_x_fixed", "Fixed-point horizontal velocity/accumulator input.");
        field(record, 0x0e, new IntegerDataType(), "velocity_y_fixed", "Fixed-point vertical velocity.");
        field(record, 0x12, new ShortDataType(), "sprite_status_word", "Sprite/status word masked by the callback.");
        field(record, 0x18, new ShortDataType(), "callback_offset", "Current callback offset.");
        field(record, 0x28, new ByteDataType(), "direction_animation_byte", "Direction/animation-related byte.");
        field(record, 0x29, new ByteDataType(), "input_animation_byte", "Input/animation-related byte used in transition paths.");
        field(record, 0x2a, new ShortDataType(), "collision_effect_callback", "Callback/animation value selected by hazard effects.");
        field(record, 0x2e, new ShortDataType(), "vertical_step_state", "Vertical step/state word; initialized from +0x72.");
        field(record, 0x32, new ShortDataType(), "callback_state_word", "Callback state word cleared by hazard-spawn setup.");
        field(record, 0x36, new ByteDataType(), "animation_state_byte", "Animation/state byte.");
        field(record, 0x37, new ByteDataType(), "callback_mode_byte", "Signed mode byte selecting ordinary, negative, and positive paths.");
        field(record, 0x38, new ByteDataType(), "collision_gate_byte", "Gate byte checked by side probes and cleared in common callback tail.");
        field(record, 0x39, new ByteDataType(), "transition_latch_byte", "Transition latch consumed at callback +0x39 branch.");
        field(record, 0x3a, new ByteDataType(), "descriptor_response_latch", "3D02 writes 1/FF on response and clears on reject; 3DF2 only tests zero/nonzero.");
        field(record, 0x3b, new ByteDataType(), "side_probe_latch", "3A1F writes FF after two clear X-side probes; gates 3D02/3DF2.");
        field(record, 0x3e, new ShortDataType(), "reset_death_timer", "Reset/death timer incremented by callback state paths.");
        field(record, 0x40, new ShortDataType(), "callback_counter", "Per-callback counter incremented in the ordinary path.");
        field(record, 0x44, new IntegerDataType(), "saved_y_fixed", "Saved Y snapshot written before movement.");
        field(record, 0x48, new IntegerDataType(), "saved_x_fixed", "Saved X snapshot written before movement.");
        field(record, 0x4c, new IntegerDataType(), "horizontal_accel_constant", "Initializer writes 0x2800; exact phase-specific use remains qualified.");
        field(record, 0x50, new IntegerDataType(), "vertical_response_constant", "Initializer writes 0x2800; consumed by state paths.");
        field(record, 0x54, new IntegerDataType(), "accel_constant_2000", "Initializer writes 0x2000.");
        field(record, 0x58, new IntegerDataType(), "fall_accel_constant", "Initializer writes 0x2000; added on negative path.");
        field(record, 0x5c, new IntegerDataType(), "horizontal_speed_limit", "Initializer writes 0x18000.");
        field(record, 0x60, new IntegerDataType(), "fall_speed_limit", "Initializer writes 0x40000.");
        field(record, 0x64, new IntegerDataType(), "reset_velocity_y", "Initializer writes 0xFFFB6000; copied by transition/death path.");
        field(record, 0x72, new ShortDataType(), "vertical_step_40", "Initializer writes 0x28; used by hazard and motion probes.");
        return (Structure) currentProgram.getDataTypeManager().addDataType(record, null);
    }

    private void field(Structure structure, int offset, DataType type,
                       String name, String comment) throws Exception {
        structure.insertAtOffset(offset, type, type.getLength(), name, comment);
    }

    private void global(int offset, String name, String comment) throws Exception {
        Address address = toAddr(offset);
        Symbol symbol = currentProgram.getSymbolTable().getPrimarySymbol(address);
        if (symbol == null) {
            currentProgram.getSymbolTable().createLabel(address, name, SourceType.USER_DEFINED);
        } else {
            try {
                symbol.setName(name, SourceType.USER_DEFINED);
            } catch (DuplicateNameException ignored) {
                println("Could not rename global " + address + " to " + name);
            }
        }
        CodeUnit unit = currentProgram.getListing().getCodeUnitAt(address);
        if (unit != null) {
            unit.setComment(CodeUnit.PLATE_COMMENT, comment);
        }
    }

    private void function(int offset, String name, String comment) throws Exception {
        Address address = toAddr(offset);
        FunctionManager manager = currentProgram.getFunctionManager();
        Function function = manager.getFunctionAt(address);
        if (function == null) {
            try {
                // Recover a real body when this targeted script is used
                // outside the manifest-driven preparation pass.  Never
                // manufacture a one-address function merely to hold a name.
                disassemble(address);
                function = createFunction(address, name);
            } catch (Exception failure) {
                println("Could not create function at " + address + ": " +
                    failure.getClass().getSimpleName());
                return;
            }
        } else {
            try {
                function.setName(name, SourceType.USER_DEFINED);
            } catch (DuplicateNameException ignored) {
                println("Could not rename function " + address + " to " + name);
            }
        }
        function.setComment(comment);
    }
}
