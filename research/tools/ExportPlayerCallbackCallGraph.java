// Export the call edges for the function entries supplied after the output path.
// The target list is generated from player-callback-closure.json by the
// reproducible baseline runner.

import java.io.File;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.symbol.Reference;

public class ExportPlayerCallbackCallGraph extends GhidraScript {
    private static class Edge {
        String source;
        String sourceName;
        String callSite;
        String target;
        String targetName;
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) {
            printerr("Usage: ExportPlayerCallbackCallGraph.java <output.json> <offset> ...");
            return;
        }
        List<Long> selected = new ArrayList<>();
        for (int i = 1; i < args.length; i++)
            selected.add(Long.parseLong(args[i], 16));

        List<Edge> edges = new ArrayList<>();
        Listing listing = currentProgram.getListing();
        for (Long offset : selected) {
            Address entry = toAddr(offset);
            Function function = currentProgram.getFunctionManager().getFunctionAt(entry);
            if (function == null) continue;
            for (Instruction instruction : listing.getInstructions(function.getBody(), true)) {
                boolean emitted = false;
                for (Reference reference : instruction.getReferencesFrom()) {
                    if (!reference.getReferenceType().isCall()) continue;
                    edges.add(edge(function, entry, instruction, reference.getToAddress()));
                    emitted = true;
                }
                // Raw Binary imports with -noanalysis do not create CALL
                // references.  Recover direct near-call flow from the
                // instruction prototype so the export remains useful even
                // when NE far relocations are intentionally unresolved.
                if (!emitted && instruction.getMnemonicString().startsWith("CALL")) {
                    Address[] flows = instruction.getFlows();
                    if (flows.length == 1 && flows[0] != null)
                        edges.add(edge(function, entry, instruction, flows[0]));
                }
            }
        }

        File output = new File(args[0]);
        File parent = output.getParentFile();
        if (parent != null) parent.mkdirs();
        try (PrintWriter writer = new PrintWriter(output, "UTF-8")) {
            writer.println("{");
            writer.println("  \"program\": \"" + escape(currentProgram.getName()) + "\",");
            writer.println("  \"edges\": [");
            for (int i = 0; i < edges.size(); i++) {
                Edge edge = edges.get(i);
                writer.print("    {\"source\":\"" + edge.source +
                    "\",\"source_name\":\"" + escape(edge.sourceName) +
                    "\",\"call_site\":\"" + edge.callSite +
                    "\",\"target\":\"" + edge.target + "\"");
                if (edge.targetName != null)
                    writer.print(",\"target_name\":\"" + escape(edge.targetName) + "\"");
                writer.print("}");
                if (i + 1 < edges.size()) writer.print(",");
                writer.println();
            }
            writer.println("  ]");
            writer.println("}");
        }
        println("Wrote " + output.getAbsolutePath());
    }

    private Edge edge(Function sourceFunction, Address sourceEntry,
                      Instruction instruction, Address targetAddress) {
        Function target = currentProgram.getFunctionManager().getFunctionContaining(targetAddress);
        Edge edge = new Edge();
        edge.source = String.format("%04X", sourceEntry.getOffset());
        edge.sourceName = sourceFunction.getName();
        edge.callSite = String.format("%04X", instruction.getAddress().getOffset());
        edge.target = String.format("%04X", targetAddress.getOffset());
        edge.targetName = target == null ? null : target.getName();
        return edge;
    }

    private String escape(String value) {
        if (value == null) return "";
        return value.replace("\\", "\\\\").replace("\"", "\\\"");
    }
}
