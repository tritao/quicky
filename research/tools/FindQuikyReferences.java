// List raw-segment functions that reference selected Quiky data addresses.
// Usage through analyzeHeadless:
//   -postScript FindQuikyReferences.java

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.address.Address;
import ghidra.program.model.symbol.Reference;

public class FindQuikyReferences extends GhidraScript {

    private static final int[] TARGETS = {
        0x30d4, 0x657a, 0x657c, 0x657e, 0x6580, 0x6582, 0x6584, 0x6986,
        0x36fc, 0x36fe, 0x3700, 0x3702,
        0x755e, 0x7560, 0x7566, 0x7966,
        0x8196, 0x81c0, 0x81c4, 0x88ba, 0x88bc,
        0x796e, 0x87de, 0x87e0, 0x881a, 0x881c, 0x8828, 0x882a, 0x89ea,
    };

    private static final int[] CODE_TARGETS = {
        0x44ff, 0x4519, 0x45ab, 0x470c, 0x58a0,
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

        println("Contact-effect lifecycle references: " + currentProgram.getName());
        for (int target : CODE_TARGETS) {
            Address address = toAddr(target);
            println(String.format("CODE 0x%04x (%s)", target,
                currentProgram.getFunctionManager().getFunctionAt(address) == null
                    ? "<no function>"
                    : currentProgram.getFunctionManager().getFunctionAt(address).getName()));
            for (Reference reference : getReferencesTo(address)) {
                Function function = functions.getFunctionContaining(reference.getFromAddress());
                String functionName = function == null ? "<no function>" : function.getName();
                String entry = function == null ? "----" : function.getEntryPoint().toString();
                println(String.format("  %s (%s, %s, %s)", reference.getFromAddress(),
                    entry, functionName, reference.getReferenceType()));
            }
        }
    }
}
