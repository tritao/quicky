#!/usr/bin/env python3
"""Generate the Ghidra preparation and annotation script from the player ledger.

The JSON ledger is authoritative.  Ranged entries are disassembled and given
their declared function bodies before names/comments are applied.  Entries
without ranges are external contracts and receive labels, never one-address
function stubs.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def java_string(value: str) -> str:
    return json.dumps(value, ensure_ascii=True)


def parse_address(value: str) -> tuple[int, int]:
    segment, offset = value.split(":", 1)
    selector_to_segment = {
        0x01D7: 1,
        0x01E7: 2,
        0x01F7: 3,
        0x0207: 4,
        0x0227: 5,
        0x0237: 6,
    }
    selector = int(segment, 16)
    try:
        return selector_to_segment[selector], int(offset, 16)
    except KeyError as exc:
        raise ValueError(f"unknown runtime selector {segment}") from exc


def generate(manifest: dict) -> str:
    functions = []
    contracts = []
    seen: set[tuple[int, int]] = set()
    for item in manifest["functions"]:
        segment, offset = parse_address(item["address"])
        identity = (segment, offset)
        if identity in seen:
            continue
        seen.add(identity)
        evidence = "; ".join(item.get("evidence", []))
        comment = f"closure={item['classification']}; evidence={evidence}"
        row = [java_string(str(segment)), java_string(f"{offset:04X}"),
               java_string(item["name"]), java_string(comment)]
        if item.get("range") is None:
            contracts.append("        {" + ", ".join(row) + "},")
        else:
            if len(item["range"]) != 2:
                raise ValueError(f"invalid range for {item['name']}")
            row.extend([java_string(item["range"][0].upper()),
                        java_string(item["range"][1].upper())])
            functions.append("        {" + ", ".join(row) + "},")

    labels = []
    for item in manifest.get("address_labels", []):
        segment, offset = parse_address(item["address"])
        comment = f"closure label; evidence={'; '.join(item.get('evidence', []))}"
        labels.append(
            "        {" + ", ".join(
                [java_string(str(segment)), java_string(f"{offset:04X}"),
                 java_string(item["name"]), java_string(comment)]
            ) + "},"
        )

    globals_by_offset: dict[tuple[int, int], str] = {}
    for item in manifest["typed_model"]["globals"]:
        address = item["address"]
        if not address.startswith("DS:"):
            continue
        offset = int(address[3:], 16)
        # DS is the data segment in the separate raw-segment model:
        # selector 0237 / SEG06, not the SEG03 code image.
        globals_by_offset[(6, offset)] = item["name"]
    globals_rows = []
    for (segment, offset), name in sorted(globals_by_offset.items()):
        globals_rows.append(
            "        {" + ", ".join(
                [java_string(str(segment)), java_string(f"{offset:04X}"),
                 java_string(name), java_string("typed player/global model")]
            ) + "},"
        )

    return f'''// GENERATED FILE. Do not edit; regenerate from
// research/ghidra/player-callback-closure.json with
// research/tools/generate_player_closure_ghidra.py.

import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.symbol.Symbol;
import ghidra.util.exception.DuplicateNameException;

public class ApplyPlayerCallbackClosure extends GhidraScript {{
    private static final String[][] FUNCTIONS = new String[][] {{
{chr(10).join(functions)}
    }};

    private static final String[][] CONTRACTS = new String[][] {{
{chr(10).join(contracts)}
    }};

    private static final String[][] LABELS = new String[][] {{
{chr(10).join(labels)}
    }};

    private static final String[][] GLOBALS = new String[][] {{
{chr(10).join(globals_rows)}
    }};

    @Override
    public void run() throws Exception {{
        int segment = segmentNumber(currentProgram.getName());
        for (String[] row : FUNCTIONS) {{
            if (Integer.parseInt(row[0]) == segment)
                applyFunction(Integer.parseInt(row[1], 16), row[2], row[3],
                    Integer.parseInt(row[4], 16), Integer.parseInt(row[5], 16));
        }}
        for (String[] row : CONTRACTS) {{
            if (Integer.parseInt(row[0]) == segment)
                applyContract(Integer.parseInt(row[1], 16), row[2], row[3]);
        }}
        for (String[] row : LABELS) {{
            if (Integer.parseInt(row[0]) == segment)
                applyLabel(Integer.parseInt(row[1], 16), row[2], row[3]);
        }}
        for (String[] row : GLOBALS) {{
            if (Integer.parseInt(row[0]) == segment)
                applyLabel(Integer.parseInt(row[1], 16), row[2], row[3]);
        }}
        println("Applied player callback ledger to " + currentProgram.getName());
    }}

    private int segmentNumber(String program) {{
        for (int i = 1; i <= 9; i++)
            if (program.contains(String.format("SEG%02d", i))) return i;
        return -1;
    }}

    private Address address(int offset) {{
        return currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(offset);
    }}

    private void applyFunction(int offset, String name, String comment,
                               int rangeStart, int rangeEnd) throws Exception {{
        Address entry = address(offset);
        AddressSet body = new AddressSet(address(rangeStart), address(rangeEnd - 1));
        DisassembleCommand command = new DisassembleCommand(address(rangeStart), body, true);
        command.enableCodeAnalysis(false);
        if (!command.applyTo(currentProgram, monitor))
            println("Disassembly reported a problem for " + name + " at " + entry);

        FunctionManager manager = currentProgram.getFunctionManager();
        Function function = manager.getFunctionAt(entry);
        if (function == null) {{
            try {{
                function = manager.createFunction(name, entry, body,
                    SourceType.USER_DEFINED);
            }} catch (Exception failure) {{
                println("Could not create " + name + " at " + entry + ": " +
                    failure.getClass().getSimpleName());
                return;
            }}
        }} else {{
            if (!function.getEntryPoint().equals(entry)) {{
                throw new Exception("requested entry " + entry +
                    " is contained by " + function.getEntryPoint());
            }}
            try {{ function.setName(name, SourceType.USER_DEFINED); }}
            catch (DuplicateNameException ignored) {{
                println("Could not rename " + entry + " to " + name);
            }}
        }}
        function.setBody(body);
        function.setComment(comment);
    }}

    private void applyContract(int offset, String name, String comment) throws Exception {{
        applyLabel(offset, name, comment);
    }}

    private void applyLabel(int offset, String name, String comment) throws Exception {{
        Address entry = address(offset);
        Symbol symbol = currentProgram.getSymbolTable().getPrimarySymbol(entry);
        if (symbol == null) {{
            currentProgram.getSymbolTable().createLabel(entry, name, SourceType.USER_DEFINED);
        }} else {{
            try {{ symbol.setName(name, SourceType.USER_DEFINED); }}
            catch (DuplicateNameException ignored) {{
                println("Could not rename " + entry + " to " + name);
            }}
        }}
        CodeUnit unit = currentProgram.getListing().getCodeUnitAt(entry);
        if (unit != null) unit.setComment(CodeUnit.PLATE_COMMENT, comment);
    }}
}}
'''


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path,
                        default=Path(__file__).resolve().parents[1] /
                        "ghidra/player-callback-closure.json")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--check", action="store_true",
                        help="fail if an existing output differs from the generated script")
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    expected = generate(manifest)
    if args.check:
        if not args.output.exists() or args.output.read_text(encoding="utf-8") != expected:
            print(f"FAIL: generated script drift at {args.output}")
            return 1
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(expected, encoding="utf-8")
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
