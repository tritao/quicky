// Dump Ghidra's decoded instruction listing for focused address/count pairs.
// Usage: -postScript DumpFocusedDisasm.java <output-directory> <offset> <count> ...

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;

public class DumpFocusedDisasm extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 3 || ((args.length - 1) % 2) != 0) {
            printerr("Usage: DumpFocusedDisasm.java <output-directory> <offset> <count> ...");
            return;
        }
        File output = new File(args[0]);
        if (!output.exists() && !output.mkdirs())
            throw new Exception("Could not create output directory: " + output);
        File report = new File(output,
            currentProgram.getName().replaceAll("[^A-Za-z0-9_.-]", "_") + ".asm");
        try (PrintWriter writer = new PrintWriter(report, "UTF-8")) {
            for (int i = 1; i < args.length; i += 2) {
                int offset = Integer.parseInt(args[i], 16);
                String countText = args[i + 1];
                int count = Integer.parseInt(countText,
                    countText.startsWith("0x") || countText.startsWith("0X") ? 16 : 10);
                writer.printf("; ---- %04X count=%d ----%n", offset, count);
                Instruction instruction = currentProgram.getListing().getInstructionAt(toAddr(offset));
                for (int n = 0; instruction != null && n < count; n++) {
                    writer.printf("%s  %s%n", instruction.getAddress(), instruction);
                    instruction = instruction.getNext();
                }
                writer.println();
            }
        }
        println("Wrote " + report.getAbsolutePath());
    }
}
