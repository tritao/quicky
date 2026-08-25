import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;

/** Print functions and their bodies around a segment-relative address range. */
public class ListQuikyFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        int start = args.length > 0 ? Integer.parseInt(args[0], 16) : 0;
        int end = args.length > 1 ? Integer.parseInt(args[1], 16) : 0xffff;
        for (Function function : currentProgram.getFunctionManager().getFunctions(true)) {
            int entry = (int)(function.getEntryPoint().getOffset() & 0xffff);
            if (entry >= start && entry <= end) {
                println(String.format("%04x-%04x %s", entry,
                    (int)(function.getBody().getMaxAddress().getOffset() & 0xffff),
                    function.getName()));
            }
        }
    }
}
