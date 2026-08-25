import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.symbol.Reference;

/** Print targeted instructions/functions and their incoming references. */
public class InspectQuikyTargets extends GhidraScript {
    @Override
    public void run() throws Exception {
        String program = currentProgram.getName();
        scanImmediateReferences(new int[] {0x612e, 0x8806, 0x8808, 0x880a, 0x880c, 0x881a, 0x881c, 0x8822, 0x8824, 0x8828, 0x882a, 0x89e6});
        if (program.contains("SEG01")) {
            inspect(new int[] {0x1084, 0x114f, 0x14e1, 0x1467, 0x4e9d, 0x4ea0, 0x4eaa, 0x4f10, 0x504f});
        } else if (program.contains("SEG03")) {
            inspect(new int[] {0x0013, 0x0fdc, 0x1024, 0x106a, 0x3529, 0x5936, 0x5a03, 0x5bee, 0x5b3a, 0x9256, 0x9269});
            inspectData(new int[] {0x612e, 0x6986, 0x6d86, 0x8806, 0x8808, 0x880a, 0x880c, 0x881a, 0x881c, 0x8822, 0x8824, 0x8828, 0x882a, 0x89e6});
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

    private void inspectData(int[] offsets) {
        for (int offset : offsets) {
            Address address = toAddr(offset);
            println(String.format("DATA %s", address));
            Reference[] refs = getReferencesTo(address);
            for (Reference ref : refs) {
                Function containing = currentProgram.getFunctionManager().getFunctionContaining(ref.getFromAddress());
                println(String.format("  DATA_REF %s -> %s type=%s function=%s",
                    ref.getFromAddress(), ref.getToAddress(), ref.getReferenceType(),
                    containing == null ? "null" : containing.getEntryPoint().toString()));
            }
        }
    }

    private void scanImmediateReferences(int[] offsets) throws Exception {
        InstructionIterator iterator = currentProgram.getListing().getInstructions(true);
        while (iterator.hasNext()) {
            Instruction instruction = iterator.next();
            byte[] bytes = instruction.getBytes();
            for (int offset : offsets) {
                if (containsWord(bytes, offset)) {
                    Function containing = currentProgram.getFunctionManager().getFunctionContaining(instruction.getAddress());
                    println(String.format("IMMEDIATE 0x%04x at %s: %s function=%s operands=%s",
                        offset, instruction.getAddress(), instruction,
                        containing == null ? "null" : containing.getEntryPoint().toString(),
                        instruction.getDefaultOperandRepresentation(0)));
                }
            }
        }
    }

    private boolean containsWord(byte[] bytes, int value) {
        byte lo = (byte)(value & 0xff);
        byte hi = (byte)((value >>> 8) & 0xff);
        for (int i = 0; i + 1 < bytes.length; i++) {
            if (bytes[i] == lo && bytes[i + 1] == hi) {
                return true;
            }
        }
        return false;
    }
}
