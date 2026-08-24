import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.symbol.Reference;

/** Print targeted instructions/functions and their incoming references. */
public class InspectQuikyTargets extends GhidraScript {
    @Override
    public void run() throws Exception {
        String program = currentProgram.getName();
        if (program.contains("SEG01")) {
            inspect(new int[] {0x4e9d, 0x4ea0, 0x4eaa, 0x4f10, 0x504f});
        } else if (program.contains("SEG03")) {
            inspect(new int[] {0x0013, 0x0fdc, 0x1024, 0x106a, 0x3529, 0x9256, 0x9269});
        }
    }

    private void inspect(int[] offsets) {
        for (int offset : offsets) {
            Address address = toAddr(offset);
            Instruction instruction = currentProgram.getListing().getInstructionAt(address);
            Function at = currentProgram.getFunctionManager().getFunctionAt(address);
            Function containing = currentProgram.getFunctionManager().getFunctionContaining(address);
            println(String.format("TARGET %s instruction=%s functionAt=%s containing=%s", address,
                instruction == null ? "null" : instruction.toString(),
                at == null ? "null" : at.getEntryPoint().toString(),
                containing == null ? "null" : containing.getEntryPoint().toString()));
            Reference[] refs = getReferencesTo(address);
            for (Reference ref : refs) {
                println(String.format("  REF %s -> %s", ref.getFromAddress(), ref.getToAddress()));
            }
        }
    }
}
