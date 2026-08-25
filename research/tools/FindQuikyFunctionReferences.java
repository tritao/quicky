// List callers of selected raw-segment function entries.
// Usage through analyzeHeadless:
//   -postScript FindQuikyFunctionReferences.java

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.symbol.Reference;

public class FindQuikyFunctionReferences extends GhidraScript {

    private static final int[] TARGETS = {
        0x44ff, 0x4519, 0x45ab, 0x470c,
    };

    @Override
    public void run() throws Exception {
        FunctionManager functions = currentProgram.getFunctionManager();
        println("Function-reference survey: " + currentProgram.getName());
        for (int target : TARGETS) {
            Address address = toAddr(target);
            Function destination = functions.getFunctionAt(address);
            println(String.format("TARGET 0x%04x %s", target,
                destination == null ? "<no function>" : destination.getName()));
            for (Reference reference : getReferencesTo(address)) {
                Function caller = functions.getFunctionContaining(reference.getFromAddress());
                String callerName = caller == null ? "<no function>" : caller.getName();
                String callerEntry = caller == null ? "----" : caller.getEntryPoint().toString();
                println(String.format("  %s from %s (%s, %s) %s",
                    reference.getReferenceType(), reference.getFromAddress(),
                    callerEntry, callerName, reference.getSource()));
            }
        }
        println("Raw little-endian target words:");
        for (int target : TARGETS) {
            scanRawWord(target);
        }
    }

    private void scanRawWord(int value) throws Exception {
        byte lo = (byte)(value & 0xff);
        byte hi = (byte)((value >>> 8) & 0xff);
        ghidra.program.model.mem.Memory memory = currentProgram.getMemory();
        for (ghidra.program.model.mem.MemoryBlock block : memory.getBlocks()) {
            for (long offset = 0; offset + 1 < block.getSize(); offset++) {
                Address address = block.getStart().add(offset);
                if (memory.getByte(address) == lo && memory.getByte(address.add(1)) == hi) {
                    println(String.format("  WORD 0x%04x at %s", value, address));
                }
            }
        }
    }
}
