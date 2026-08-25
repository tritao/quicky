// Dump the focused external-state closure without changing the authoritative
// callback range manifest.
// Usage: -postScript DumpExternalClosure.java <output-directory> <offset> ...

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

public class DumpExternalClosure extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) {
            printerr("Usage: DumpExternalClosure.java <output-directory> <offset> ...");
            return;
        }
        File output = new File(args[0]);
        if (!output.exists() && !output.mkdirs())
            throw new Exception("Could not create output directory: " + output);
        String program = currentProgram.getName();
        File report = new File(output, program.replaceAll("[^A-Za-z0-9_.-]", "_") + ".c");
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        decompiler.setSimplificationStyle("decompile");
        try (PrintWriter writer = new PrintWriter(report, "UTF-8")) {
            writer.println("/* Decompiled focused external-state closure from " + program + " */");
            writer.println("/* Entries are address-qualified; containing functions are reported explicitly. */");
            writer.println();
            for (int i = 1; i < args.length; i++) {
                String raw = args[i].toUpperCase();
                int offset = Integer.parseInt(raw, 16);
                Address address = toAddr(offset);
                Function function = currentProgram.getFunctionManager().getFunctionAt(address);
                if (function == null)
                    function = currentProgram.getFunctionManager().getFunctionContaining(address);
                if (function == null) {
                    try {
                        disassemble(address);
                        function = createFunction(address, "player_external_" + raw);
                    } catch (Exception ignored) {
                        // Keep the explicit unresolved marker below.
                    }
                }
                if (function == null) {
                    writer.println("/* MISSING 0x" + raw + " */");
                    writer.println();
                    continue;
                }
                writer.println("/* requested 0x" + raw + "; function " + function.getName() +
                    " at 0x" + function.getEntryPoint().getOffset() + " */");
                DecompileResults result = decompiler.decompileFunction(function, 120, monitor);
                if (result.decompileCompleted() && result.getDecompiledFunction() != null)
                    writer.println(result.getDecompiledFunction().getC());
                else
                    writer.println("/* DECOMPILATION FAILED: " + result.getErrorMessage() + " */");
                writer.println();
            }
        } finally {
            decompiler.dispose();
        }
        println("Wrote " + report.getAbsolutePath());
    }
}
