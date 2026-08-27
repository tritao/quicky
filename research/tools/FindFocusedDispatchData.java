// Search only the runtime dispatch-table words used by 01F7:0442/0598.
// This is intentionally narrower than FindQuikyReferences.java so the
// cross-segment writer census remains readable and auditable.

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.symbol.Reference;

public class FindFocusedDispatchData extends GhidraScript {
    private static final int[] TARGETS = {0x6d8a, 0x6d8e, 0x30d2};

    @Override
    public void run() throws Exception {
        println("Focused 5937/0598 dispatch-data census: " + currentProgram.getName());
        for (int target : TARGETS) {
            println(String.format("DATA 0x%04x", target));
            Address address = toAddr(target);
            for (Reference reference : getReferencesTo(address)) {
                println(String.format("  REFERENCE %s -> %s type=%s function=%s",
                    reference.getFromAddress(), reference.getToAddress(),
                    reference.getReferenceType(), functionName(reference.getFromAddress())));
            }
            InstructionIterator iterator = currentProgram.getListing().getInstructions(true);
            while (iterator.hasNext()) {
                Instruction instruction = iterator.next();
                if (!containsWord(instruction.getBytes(), target))
                    continue;
                println(String.format("  IMMEDIATE %s: %s function=%s",
                    instruction.getAddress(), instruction, functionName(instruction.getAddress())));
            }
        }
    }

    private String functionName(Address address) {
        Function function = currentProgram.getFunctionManager().getFunctionContaining(address);
        return function == null ? "<no function>" : function.getName();
    }

    private boolean containsWord(byte[] bytes, int value) {
        byte low = (byte)(value & 0xff);
        byte high = (byte)((value >>> 8) & 0xff);
        for (int i = 0; i + 1 < bytes.length; i++) {
            if (bytes[i] == low && bytes[i + 1] == high)
                return true;
        }
        return false;
    }
}
