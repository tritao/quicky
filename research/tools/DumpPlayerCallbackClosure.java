// Dump the exact function entries selected by the player-closure ledger.
// Usage: -postScript DumpPlayerCallbackClosure.java <output-directory> <offset> ...

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

public class DumpPlayerCallbackClosure extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) {
            printerr("Usage: DumpPlayerCallbackClosure.java <output-directory> <offset> ...");
            return;
        }

        File outputDirectory = new File(args[0]);
        if (!outputDirectory.exists() && !outputDirectory.mkdirs())
            throw new Exception("Could not create output directory: " + outputDirectory);

        String program = currentProgram.getName();
        File report = new File(outputDirectory,
            program.replaceAll("[^A-Za-z0-9_.-]", "_") + ".c");
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        decompiler.setSimplificationStyle("decompile");
        try (PrintWriter writer = new PrintWriter(report, "UTF-8")) {
            writer.println("/* Decompiled from " + program + " */");
            writer.println("/* Entries are selected mechanically from player-callback-closure.json. */");
            writer.println();
            for (int i = 1; i < args.length; i++) {
                String raw = args[i].toUpperCase();
                int offset = Integer.parseInt(raw, 16);
                Address address = toAddr(offset);
                Function function = currentProgram.getFunctionManager().getFunctionAt(address);
                if (function == null)
                    function = currentProgram.getFunctionManager().getFunctionContaining(address);
                if (function == null) {
                    writer.println("/* MISSING 0x" + raw + " */");
                    writer.println();
                    continue;
                }

                writer.println("/* " + function.getName() + " at 0x" + raw + " */");
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
