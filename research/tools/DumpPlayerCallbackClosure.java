// Dump exact, range-checked function entries selected by the player ledger.
// Usage: -postScript DumpPlayerCallbackClosure.java <output-directory>
//        <offset:start:end> ...

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
            printerr("Usage: DumpPlayerCallbackClosure.java <output-directory> <offset:start:end> ...");
            return;
        }

        File outputDirectory = new File(args[0]);
        if (!outputDirectory.exists() && !outputDirectory.mkdirs())
            throw new Exception("Could not create output directory: " + outputDirectory);

        String program = currentProgram.getName();
        String filename = program.replaceAll("[^A-Za-z0-9_.-]", "_");
        File report = new File(outputDirectory, filename + ".c");
        File inventory = new File(outputDirectory, filename + ".functions.json");
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        decompiler.setSimplificationStyle("decompile");
        try (PrintWriter writer = new PrintWriter(report, "UTF-8");
             PrintWriter inventoryWriter = new PrintWriter(inventory, "UTF-8")) {
            writer.println("/* Decompiled from " + program + " */");
            writer.println("/* Entries are selected mechanically from player-callback-closure.json. */");
            writer.println();
            inventoryWriter.println("{");
            inventoryWriter.println("  \"program\": \"" + escape(program) + "\",");
            inventoryWriter.println("  \"functions\": [");
            boolean firstInventoryRow = true;

            for (int i = 1; i < args.length; i++) {
                String[] parts = args[i].toUpperCase().split(":");
                if (parts.length != 3)
                    throw new Exception("invalid function range: " + args[i]);
                String raw = parts[0];
                int offset = Integer.parseInt(raw, 16);
                int rangeStart = Integer.parseInt(parts[1], 16);
                int rangeEnd = Integer.parseInt(parts[2], 16);
                if (rangeStart >= rangeEnd || offset < rangeStart || offset >= rangeEnd)
                    throw new Exception("invalid function range: " + args[i]);

                Address address = toAddr(offset);
                Function function = currentProgram.getFunctionManager().getFunctionAt(address);
                if (function == null || !function.getEntryPoint().equals(address))
                    throw new Exception("missing exact function entry 0x" + raw);

                long bodyMin = function.getBody().getMinAddress().getOffset();
                long bodyMax = function.getBody().getMaxAddress().getOffset();
                long bodyAddresses = function.getBody().getNumAddresses();
                if (bodyMin != rangeStart || bodyMax != rangeEnd - 1 ||
                    bodyAddresses != rangeEnd - rangeStart) {
                    throw new Exception("function body mismatch for 0x" + raw +
                        ": expected " + parts[1] + ".." +
                        String.format("%04X", rangeEnd - 1) +
                        ", found " + String.format("%04X..%04X", bodyMin, bodyMax));
                }

                writer.println("/* " + function.getName() + " at 0x" + raw + " */");
                DecompileResults result = decompiler.decompileFunction(function, 120, monitor);
                if (!result.decompileCompleted() || result.getDecompiledFunction() == null)
                    throw new Exception("decompilation failed for " + function.getName() +
                        ": " + result.getErrorMessage());
                writer.println(result.getDecompiledFunction().getC());
                writer.println();

                if (!firstInventoryRow) inventoryWriter.println(",");
                firstInventoryRow = false;
                inventoryWriter.print("    {\"entry\":\"" + raw +
                    "\",\"name\":\"" + escape(function.getName()) +
                    "\",\"range\":[\"" + parts[1] + "\",\"" + parts[2] +
                    "\"],\"body_min\":\"" + String.format("%04X", bodyMin) +
                    "\",\"body_max\":\"" + String.format("%04X", bodyMax) +
                    "\",\"body_addresses\":" + bodyAddresses +
                    ",\"decompile_completed\":true}");
            }
            inventoryWriter.println();
            inventoryWriter.println("  ]");
            inventoryWriter.println("}");
        } finally {
            decompiler.dispose();
        }
        println("Wrote " + report.getAbsolutePath());
        println("Wrote " + inventory.getAbsolutePath());
    }

    private String escape(String value) {
        if (value == null) return "";
        return value.replace("\\", "\\\\").replace("\"", "\\\"");
    }
}
