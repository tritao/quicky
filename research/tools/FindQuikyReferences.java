// List raw-segment functions that reference selected Quiky data addresses.
// Usage through analyzeHeadless:
//   -postScript FindQuikyReferences.java

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;

public class FindQuikyReferences extends GhidraScript {

    private static final int[] TARGETS = {
        0x657a, 0x657c, 0x657e, 0x6580, 0x6986,
        0x36fc, 0x36fe, 0x3700, 0x3702,
    };

    @Override
    public void run() throws Exception {
        FunctionManager functions = currentProgram.getFunctionManager();
        Listing listing = currentProgram.getListing();
        println("Map/collision reference survey: " + currentProgram.getName());
        for (int target : TARGETS) {
            println(String.format("DATA 0x%04x", target));
            byte low = (byte)(target & 0xff);
            byte high = (byte)((target >> 8) & 0xff);
            for (Instruction instruction : listing.getInstructions(true)) {
                byte[] bytes = instruction.getBytes();
                boolean found = false;
                for (int i = 0; i + 1 < bytes.length; i++) {
                    if (bytes[i] == low && bytes[i + 1] == high) {
                        found = true;
                        break;
                    }
                }
                if (!found) continue;
                Function function = functions.getFunctionContaining(instruction.getAddress());
                String functionName = function == null ? "<no function>" : function.getName();
                String entry = function == null ? "----" : function.getEntryPoint().toString();
                println(String.format("  bytes at %s (%s, %s)",
                    instruction.getAddress(), entry, functionName));
            }
        }
    }
}
