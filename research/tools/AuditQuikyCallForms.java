// Print call instructions and unresolved/indirect call forms in a raw segment.
// Usage through analyzeHeadless:
//   -postScript AuditQuikyCallForms.java [target-offset ...]

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.symbol.Reference;

public class AuditQuikyCallForms extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        int[] targets = new int[args.length];
        for (int i = 0; i < args.length; i++) {
            targets[i] = Integer.parseInt(args[i], 16);
        }
        Listing listing = currentProgram.getListing();
        println("Call-form survey: " + currentProgram.getName());
        for (Instruction instruction : listing.getInstructions(true)) {
            if (!instruction.getMnemonicString().toUpperCase().contains("CALL")) {
                continue;
            }
            String operand = instruction.getDefaultOperandRepresentation(0);
            boolean indirect = operand.contains("[") || operand.contains("ptr");
            boolean targetMatch = false;
            for (int target : targets) {
                if ((instruction.getAddress().getOffset() & 0xffff) == target) {
                    targetMatch = true;
                }
                for (Reference reference : instruction.getReferencesFrom()) {
                    if ((reference.getToAddress().getOffset() & 0xffff) == target) {
                        targetMatch = true;
                    }
                }
            }
            boolean selectedFunction = (instruction.getAddress().getOffset() & 0xffff) >= 0x1800
                && (instruction.getAddress().getOffset() & 0xffff) < 0x1a00;
            if (!indirect && !targetMatch && !selectedFunction) {
                continue;
            }
            StringBuilder refs = new StringBuilder();
            for (Reference reference : instruction.getReferencesFrom()) {
                if (refs.length() > 0) refs.append(",");
                refs.append(reference.getToAddress());
            }
            println(String.format("%s bytes=%s operand=%s indirect=%s refs=%s",
                instruction.getAddress(), bytesHex(instruction.getBytes()), operand,
                indirect, refs));
        }
    }

    private String bytesHex(byte[] bytes) {
        StringBuilder result = new StringBuilder();
        for (byte value : bytes) result.append(String.format("%02x", value & 0xff));
        return result.toString();
    }
}
