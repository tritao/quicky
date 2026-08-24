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

        label(0x491d, "level_selector_cheat_branch", "Runtime trace: checks the cheat-enabled level selector state.");
        function(0x4727, "init_falling_leaves_entity_type_2b",
            "Runtime-confirmed dispatch entry for ARE type 0x2B; controlled replacement with inert type 0 removes the falling leaves at W1L1 record 0x1792.");

    }

    private void annotateSegment2() throws Exception {
        label(0x084b, "path_template_sam_tfx", "Pascal fragments: GAMEDATA\\ + .SAM + .TFX");
        function(0x085e, "load_sam_tfx_resource", "Builds/loads SAM and TFX audio resources.");
        relocationTarget(0x0caa, "seg2_target_0caa", "NE relocation target referenced by segment 1.");
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
        int[] targets = {0x05a0, 0x106a, 0x1ec4, 0x332c, 0x335e, 0x33bf, 0x342f};
        for (int target : targets) {
            relocationTarget(target, String.format("seg3_target_%04x", target),
                "NE relocation target in segment 3; semantics not assigned yet.");
        }
        function(0x1cda, "stream_are_regions",
            "Streams ARE declarations for newly visible 64-pixel regions using camera coordinates and the reference grid.");
        function(0x1e04, "instantiate_are_declaration",
            "Runtime-confirmed six-byte ARE record walker: type, local X, local Y; marks records processed and creates objects at region origin plus local coordinates.");
        function(0x0e06, "are_object_factory",
            "Runtime-confirmed factory called by the normal ARE dispatch path; type 0x2B returns ES:DI and is initialized by the caller.");
        function(0x1749, "create_dedicated_are_effect",
            "Shared creator used by types 0x65/0x66/0x67 after selecting subtype 0x00/0x08/0x10.");
        function(0x178d, "create_are_type_65", "Dedicated ARE type 0x65 wrapper.");
        function(0x1798, "create_are_type_66", "Dedicated ARE type 0x66 wrapper.");
        function(0x17a3, "create_are_type_67", "Dedicated ARE type 0x67 wrapper.");
        function(0x4727, "update_falling_leaves_types_29_2b",
            "Dispatch-table callback shared by ARE types 0x29, 0x2A, and confirmed falling-leaves type 0x2B.");
        function(0x9256, "update_are_type_28",
            "Dispatch-table callback for normal ARE type 0x28, whose object class is zero.");
        label(0x3714, "are_region_origin_x", "Runtime-confirmed 64-pixel-aligned X origin used while instantiating an ARE declaration.");
        label(0x3716, "are_region_origin_y", "Runtime-confirmed 64-pixel-aligned Y origin used while instantiating an ARE declaration.");
        label(0x81d2, "are_entity_dispatch_table", "Four-byte entries indexed by ARE entity type; normal types feed these values to the object factory.");
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
            function = manager.createFunction(name, address, new AddressSet(address), SourceType.USER_DEFINED);
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
