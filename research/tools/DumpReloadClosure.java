// Dump the focused level-reload/resource closure reached after 01D7:5010.
// Usage through analyzeHeadless:
//   -postScript DumpReloadClosure.java <output-directory>
//
// The raw NE segments are imported independently at offset zero. Segment
// relative addresses therefore remain the original selector-relative offsets.

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;

public class DumpReloadClosure extends GhidraScript {
    private static final String[][] SEG01_TARGETS = {
        {"5010", "completion_reload_gate"},
        {"5017", "reload_delay_callsite"},
        {"5038", "reload_transition_buffer_copy_callsite"},
        {"503D", "reload_player_reposition_callsite"},
        {"5042", "reload_camera_rebuild_callsite"},
        {"5047", "reload_world_rebuild_callsite"},
        {"504F", "completion_dispatch_tail"},
        {"313D", "reload_world_selector_dispatch"},
    };

    private static final String[][] SEG02_TARGETS = {
        {"0CAA", "transition_effect_finalize"},
        {"0D18", "transition_effect_setup"},
    };

    private static final String[][] SEG03_TARGETS = {
        {"0908", "reload_transition_delay_loop"},
        {"0931", "reload_transition_delay_tick"},
        {"1AAA", "reload_player_reposition"},
        {"1AE6", "reload_scheduler_gate_clear"},
        {"20AF", "reload_publish_camera_fixed"},
        {"3062", "reload_refresh_vga_map"},
        {"31D1", "reload_clamp_camera"},
        {"321F", "reload_rebuild_camera_map"},
        {"5D38", "reload_load_animation_descriptor"},
    };

    private static final String[][] SEG04_TARGETS = {
        {"022A", "transition_fade_helper"},
        {"18C7", "reload_resource_lookup"},
    };

    private static final String[][] SEG05_TARGETS = {
        {"05CD", "reload_transition_setup_tick"},
        {"0D5A", "reload_transition_buffer_copy"},
    };

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 1) {
            printerr("Usage: -postScript DumpReloadClosure.java <output-directory>");
            return;
        }

        String segment = segmentName(currentProgram.getName());
        String[][] targets = targetsFor(segment);
        if (targets == null) {
            println("Skipping " + currentProgram.getName());
            return;
        }

        File outputDirectory = new File(args[0]);
        if (!outputDirectory.exists() && !outputDirectory.mkdirs()) {
            throw new Exception("Could not create output directory: " + outputDirectory);
        }

        FunctionManager manager = currentProgram.getFunctionManager();
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        decompiler.setSimplificationStyle("decompile");
        File report = new File(outputDirectory,
            currentProgram.getName().replaceAll("[^A-Za-z0-9_.-]", "_") + ".c");
        try (PrintWriter writer = new PrintWriter(report, "UTF-8")) {
            writer.println("/* Focused Ghidra decompilation of the post-5010 reload closure. */");
            writer.println("/* Addresses are segment-relative raw offsets. */");
            writer.println();
            for (String[] target : targets) {
                int offset = Integer.parseInt(target[0], 16);
                Address address = toAddr(offset);
                Function function = manager.getFunctionAt(address);
                if (function == null) function = manager.getFunctionContaining(address);
                if (function == null) {
                    try {
                        disassemble(address);
                        function = createFunction(address, target[1]);
                    } catch (Exception ignored) {
                        // Preserve an explicit missing marker below.
                    }
                }

                writer.println("/* TARGET " + target[1] + " at " + segment + ":" +
                    target[0] + "; resolved entry " +
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

    private String segmentName(String program) {
        for (int i = 1; i <= 6; i++) {
            String name = String.format("SEG%02d", i);
            if (program.contains(name)) return name;
        }
        return null;
    }

    private String[][] targetsFor(String segment) {
        if ("SEG01".equals(segment)) return SEG01_TARGETS;
        if ("SEG02".equals(segment)) return SEG02_TARGETS;
        if ("SEG03".equals(segment)) return SEG03_TARGETS;
        if ("SEG04".equals(segment)) return SEG04_TARGETS;
        if ("SEG05".equals(segment)) return SEG05_TARGETS;
        return null;
    }

    private void writeCallers(PrintWriter writer, Function function) {
        writer.println("/* CALLERS of " + function.getEntryPoint() + ": */");
        ReferenceIterator references = currentProgram.getReferenceManager()
            .getReferencesTo(function.getEntryPoint());
        boolean found = false;
        while (references.hasNext()) {
            Reference reference = references.next();
            Function caller = currentProgram.getFunctionManager()
                .getFunctionContaining(reference.getFromAddress());
            writer.println("/*   " + reference.getFromAddress() + " " +
                reference.getReferenceType() + " in " +
                (caller == null ? "<unknown>" : caller.getEntryPoint()) + " */");
            found = true;
        }
        if (!found) writer.println("/*   <none resolved; interior label or relocated call> */");
    }
}
