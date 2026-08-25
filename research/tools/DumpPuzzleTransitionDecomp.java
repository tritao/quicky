// Dump only the completion/transition continuation reached after 01D7:4EAA.
// Run with Ghidra's protected-mode raw-segment import:
//   -postScript DumpPuzzleTransitionDecomp.java <output-directory>

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

public class DumpPuzzleTransitionDecomp extends GhidraScript {
    private static final String[][] SEG01_TARGETS = {
        {"4EA0", "cloud_outer_state_consumer"},
        {"4EAA", "cloud_outer_state_positive_path"},
        {"4EDD", "transition_pending_wait_a"},
        {"4EE6", "transition_pending_wait_b"},
        {"4F0D", "transition_pending_intro"},
        {"14E1", "completion_effect_consumer"},
        {"1669", "completion_mask_check"},
        {"16C6", "completion_bonus_branch"},
        {"16DE", "completion_bonus_score"},
        {"16F0", "completion_bonus_text"},
        {"1704", "completion_flag_write"},
        {"1709", "completion_transition_sound"},
        {"4F10", "completion_handoff_gate"},
        {"4FAF", "completion_selector_handoff"},
        {"5010", "completion_reload_gate"},
        {"504F", "completion_dispatch_tail"},
        {"01F0", "completion_input_release_wait"},
        {"01D6", "completion_input_confirm_wait"}
    };

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 1) {
            printerr("Usage: -postScript DumpPuzzleTransitionDecomp.java <output-directory>");
            return;
        }
        if (!currentProgram.getName().contains("SEG01")) {
            println("Skipping " + currentProgram.getName() + "; targets are SEG01 only");
            return;
        }

        File outputDirectory = new File(args[0]);
        if (!outputDirectory.exists() && !outputDirectory.mkdirs()) {
            throw new Exception("Could not create output directory: " + outputDirectory);
        }
        File report = new File(outputDirectory,
            currentProgram.getName().replaceAll("[^A-Za-z0-9_.-]", "_") + ".c");
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        decompiler.setSimplificationStyle("decompile");
        try (PrintWriter writer = new PrintWriter(report, "UTF-8")) {
            writer.println("/* Focused Ghidra decompilation of the post-4EAA transition closure. */");
            writer.println("/* Addresses are segment-relative raw offsets. */");
            writer.println();
            for (String[] target : SEG01_TARGETS) {
                int offset = Integer.parseInt(target[0], 16);
                Address address = toAddr(offset);
                Function function = ensureFunction(address, target[1]);
                writer.println("/* TARGET " + target[1] + " at 01D7:" + target[0] +
                    "; resolved entry " + (function == null ? "MISSING" :
                    function.getEntryPoint()) + " */");
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

    private Function ensureFunction(Address address, String name) throws Exception {
        FunctionManager manager = currentProgram.getFunctionManager();
        Function function = manager.getFunctionAt(address);
        if (function == null) function = manager.getFunctionContaining(address);
        if (function != null) return function;
        disassemble(address);
        function = manager.getFunctionAt(address);
        if (function == null) function = createFunction(address, name);
        return function;
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
