// Enumerate code/data references to selected MAP-writer entries in a raw segment.
// Usage through analyzeHeadless:
//   -postScript FindQuikyCallReferences.java

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceManager;

public class FindQuikyCallReferences extends GhidraScript {
    private static final int[] TARGETS = {0x16ce, 0x339a, 0x340a, 0x5c9d};

    @Override
    public void run() throws Exception {
        ReferenceManager references = currentProgram.getReferenceManager();
        FunctionManager functions = currentProgram.getFunctionManager();
        println("Reference survey: " + currentProgram.getName());
        for (int target : TARGETS) {
            Address address = toAddr(target);
            println(String.format("TARGET 0x%04x", target));
            for (Reference reference : references.getReferencesTo(address)) {
                Function function = functions.getFunctionContaining(reference.getFromAddress());
                String functionName = function == null ? "<no function>" : function.getName();
                String functionEntry = function == null ? "----" : function.getEntryPoint().toString();
                println(String.format("  %s -> %s type=%s primary=%s function=%s (%s)",
                    reference.getFromAddress(), reference.getToAddress(),
                    reference.getReferenceType(), reference.isPrimary(),
                    functionEntry, functionName));
            }
        }
    }
}
