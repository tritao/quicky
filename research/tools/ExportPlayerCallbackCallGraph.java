// Export the call edges that can be recovered from the raw Ghidra program.
//
// BinaryLoader imports each NE segment independently, so far-call relocation
// slots are not resolved by Ghidra. Direct near CALL flow is nevertheless
// recoverable from the disassembled bytes and is exported here as independent
// evidence. Far and indirect CALL instructions are recorded explicitly as
// unresolved; their targets are checked by the Python NE-relocation audit.

import java.io.File;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;

public class ExportPlayerCallbackCallGraph extends GhidraScript {
    private static final Pattern SEGMENT_NAME = Pattern.compile(".*_SEG(\\d+)(?:\\..*)?");

    private static class Edge {
        String source;
        String sourceName;
        String callSite;
        String target;
        String targetName;
    }

    private static class Unresolved {
        String source;
        String sourceName;
        String callSite;
        String classification;
        int opcode;
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) {
            printerr("Usage: ExportPlayerCallbackCallGraph.java <output.json> <offset> ...");
            return;
        }
        int segment = segmentNumber();
        List<Long> selected = new ArrayList<>();
        for (int i = 1; i < args.length; i++)
            selected.add(Long.parseLong(args[i], 16));

        List<Edge> edges = new ArrayList<>();
        List<Unresolved> unresolved = new ArrayList<>();
        Listing listing = currentProgram.getListing();
        for (Long offset : selected) {
            Address entry = toAddr(offset);
            Function function = currentProgram.getFunctionManager().getFunctionAt(entry);
            if (function == null) {
                printerr(String.format("No function at %04X", offset));
                continue;
            }
            for (Instruction instruction : listing.getInstructions(function.getBody(), true)) {
                if (!instruction.getMnemonicString().equalsIgnoreCase("CALL"))
                    continue;
                byte[] bytes = instruction.getBytes();
                int opcode = bytes.length == 0 ? -1 : bytes[0] & 0xff;
                if (opcode == 0xe8) {
                    Address[] flows = instruction.getFlows();
                    if (flows.length == 1 && flows[0] != null) {
                        edges.add(edge(function, entry, instruction, flows[0], segment));
                    } else {
                        unresolved.add(unresolved(function, entry, instruction, opcode,
                            "near", segment));
                    }
                } else {
                    unresolved.add(unresolved(function, entry, instruction, opcode,
                        "far-or-indirect", segment));
                }
            }
        }

        File output = new File(args[0]);
        File parent = output.getParentFile();
        if (parent != null) parent.mkdirs();
        try (PrintWriter writer = new PrintWriter(output, "UTF-8")) {
            writer.println("{");
            writer.println("  \"schema\": \"quiky.player-callback-ghidra-callgraph.v2\",");
            writer.println("  \"program\": \"" + escape(currentProgram.getName()) + "\",");
            writer.println("  \"segment\": " + segment + ",");
            writer.println("  \"graph_kind\": \"ghidra-near-calls\",");
            writer.println("  \"edges\": [");
            for (int i = 0; i < edges.size(); i++) {
                Edge edge = edges.get(i);
                writer.print("    {\"source\":\"" + edge.source +
                    "\",\"source_name\":\"" + escape(edge.sourceName) +
                    "\",\"call_site\":\"" + edge.callSite +
                    "\",\"target\":\"" + edge.target +
                    "\",\"call_kind\":\"near\"");
                if (edge.targetName != null)
                    writer.print(",\"target_name\":\"" + escape(edge.targetName) + "\"");
                writer.print("}");
                if (i + 1 < edges.size()) writer.print(",");
                writer.println();
            }
            writer.println("  ],");
            writer.println("  \"unresolved_calls\": [");
            for (int i = 0; i < unresolved.size(); i++) {
                Unresolved call = unresolved.get(i);
                writer.print("    {\"source\":\"" + call.source +
                    "\",\"source_name\":\"" + escape(call.sourceName) +
                    "\",\"call_site\":\"" + call.callSite +
                    "\",\"classification\":\"" + call.classification +
                    "\",\"opcode\":\"" + String.format("%02X", call.opcode) + "\"}");
                if (i + 1 < unresolved.size()) writer.print(",");
                writer.println();
            }
            writer.println("  ]");
            writer.println("}");
        }
        println("Wrote " + output.getAbsolutePath());
    }

    private Edge edge(Function sourceFunction, Address sourceEntry,
                      Instruction instruction, Address targetAddress, int segment) {
        Function target = currentProgram.getFunctionManager().getFunctionAt(targetAddress);
        if (target == null)
            target = currentProgram.getFunctionManager().getFunctionContaining(targetAddress);
        Edge edge = new Edge();
        edge.source = qualified(segment, sourceEntry);
        edge.sourceName = sourceFunction.getName();
        edge.callSite = qualified(segment, instruction.getAddress());
        edge.target = qualified(segment, targetAddress);
        edge.targetName = target == null ? null : target.getName();
        return edge;
    }

    private Unresolved unresolved(Function sourceFunction, Address sourceEntry,
                                  Instruction instruction, int opcode,
                                  String classification, int segment) {
        Unresolved call = new Unresolved();
        call.source = qualified(segment, sourceEntry);
        call.sourceName = sourceFunction.getName();
        call.callSite = qualified(segment, instruction.getAddress());
        call.classification = classification;
        call.opcode = opcode;
        return call;
    }

    private String qualified(int segment, Address address) {
        return String.format("%d:%04X", segment, address.getOffset());
    }

    private int segmentNumber() {
        Matcher matcher = SEGMENT_NAME.matcher(currentProgram.getName());
        if (!matcher.matches())
            throw new IllegalArgumentException("program name has no _SEGNN suffix: " + currentProgram.getName());
        return Integer.parseInt(matcher.group(1));
    }

    private String escape(String value) {
        if (value == null) return "";
        return value.replace("\\", "\\\\").replace("\"", "\\\"");
    }
}
