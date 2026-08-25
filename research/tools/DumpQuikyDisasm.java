import java.io.File;
import java.io.PrintWriter;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Instruction;

public class DumpQuikyDisasm extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 1) {
            printerr("Usage: -postScript DumpQuikyDisasm.java <output-directory>");
            return;
        }
        File out = new File(args[0], currentProgram.getName().replaceAll("[^A-Za-z0-9_.-]", "_") + ".asm");
        try (PrintWriter writer = new PrintWriter(out, "UTF-8")) {
            for (int start : new int[] {0x487f, 0x489c, 0x4968, 0x4973, 0x497c, 0x4990, 0x4996, 0x49a0, 0x49f2,
                                        0x50b1, 0x5089, 0x1084, 0x0703}) {
                writer.println(String.format("; ---- %04x ----", start));
                Instruction instruction = currentProgram.getListing().getInstructionAt(toAddr(start));
                int count = 0;
                while (instruction != null && count++ < 24) {
                    writer.println(String.format("%s  %s", instruction.getAddress(), instruction));
                    instruction = instruction.getNext();
                }
                writer.println();
            }
        }
        println("Wrote " + out.getAbsolutePath());
    }
}
