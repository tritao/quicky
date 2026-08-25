import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "research" / "tools"))

from generate_player_closure_ghidra import generate  # noqa: E402
from ghidra_ne_segments import read_segments  # noqa: E402
from ne_relocs import read_relocations  # noqa: E402
from quiky_ne import parse_address, read_ne  # noqa: E402
from verify_player_callback_closure import (  # noqa: E402
    classify_expected_call,
    check_hashes,
    ClosureError,
    expected_call_edges,
    check_independent_callgraph,
)


class GhidraToolTests(unittest.TestCase):
    def test_shared_address_model_accepts_runtime_and_raw_forms(self):
        runtime = parse_address("01F7:3ff8")
        raw = parse_address("3:3FF8")
        self.assertEqual(runtime, raw)
        self.assertEqual(runtime.segment, 3)
        self.assertEqual(runtime.offset, 0x3FF8)
        self.assertEqual(runtime.as_runtime(), "01F7:3FF8")

    def test_shared_ne_image_matches_segment_and_relocation_adapters(self):
        executable = ROOT / "game/QUIKY.EXE"
        image = read_ne(executable)
        segment = image.segment(3)
        self.assertEqual(segment.runtime_selector, 0x01F7)
        blob = executable.read_bytes()
        self.assertEqual(
            image.raw_bytes(3),
            blob[segment.file_offset:segment.file_offset + segment.file_length],
        )
        self.assertEqual(read_segments(executable), image.segment_dicts())
        self.assertEqual(
            image.memory_bytes(6)[:image.segment(6).file_length],
            image.raw_bytes(6),
        )
        self.assertTrue(image.memory_bytes(6)[-1:] == b"\0")
        self.assertEqual(
            image.relocation_dicts(3),
            [record for record in read_relocations(executable)
             if record["segment"] == 3],
        )

    def test_ne_segment_table_uses_header_relative_offset(self):
        blob = bytearray(0x120)
        blob[0:2] = b"MZ"
        struct.pack_into("<I", blob, 0x3C, 0x80)
        blob[0x80:0x82] = b"NE"
        struct.pack_into("<H", blob, 0x80 + 0x1C, 1)
        struct.pack_into("<H", blob, 0x80 + 0x22, 0x50)
        struct.pack_into("<H", blob, 0x80 + 0x32, 4)
        struct.pack_into("<HHHH", blob, 0x80 + 0x50, 0x10, 4, 0x1234, 4)
        blob[0x100:0x104] = b"CODE"
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sample.exe"
            path.write_bytes(blob)
            segment = read_segments(path)[0]
        self.assertEqual(segment["file_offset"], 0x100)
        self.assertEqual(segment["file_length"], 4)
        self.assertEqual(segment["runtime_selector"], "01D7")

    def test_generated_script_prepares_ranges_and_maps_ds_to_seg06(self):
        manifest = json.loads(
            (ROOT / "research/ghidra/player-callback-closure.json").read_text()
        )
        script = generate(manifest)
        self.assertIn('new DisassembleCommand(cursor, body, false)', script)
        self.assertIn('private void disassembleRange(AddressSet body, String name)', script)
        self.assertIn('{"3", "3FF8", "update_player"', script)
        self.assertIn('"3FF8", "44DC"', script)
        self.assertIn('"44DC", "44FF"', script)
        self.assertIn('{"6", "881A", "player_offset"', script)
        self.assertNotIn('{"3", "881A", "player_offset"', script)
        self.assertIn('private static final String[][] CONTRACTS', script)
        self.assertNotIn('new AddressSet(entry)', script)

    def test_independent_callgraph_accepts_near_ghidra_edges_and_ne_relocations(self):
        manifest = json.loads(
            (ROOT / "research/ghidra/player-callback-closure.json").read_text()
        )
        names = {item["address"]: item["name"] for item in manifest["functions"]}
        edges = []
        unresolved = []
        for source, site, target, _name in expected_call_edges(manifest):
            kind = classify_expected_call(ROOT / "game/QUIKY.EXE", source, site, target)
            if kind == "far":
                unresolved.append({
                    "source": f"{source[0]}:{source[1]:04X}",
                    "source_name": names.get(f"{source[0]}:{source[1]:04X}", ""),
                    "call_site": f"{source[0]}:{site:04X}",
                    "classification": "far-or-indirect",
                    "opcode": "9A",
                })
                continue
            if kind != "near":
                continue
            source_text = f"{source[0]}:{source[1]:04X}"
            target_text = f"{target[0]}:{target[1]:04X}"
            edges.append({
                "source": source_text,
                "source_name": names.get(source_text, ""),
                "call_site": f"{source[0]}:{site:04X}",
                "target": target_text,
                "call_kind": "near",
            })
        graph = {
            "schema": "quiky.player-callback-ghidra-callgraph.v2",
            "program": "QUIKY_SEG03.bin",
            "segment": 3,
            "edges": edges,
            "unresolved_calls": unresolved,
        }
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "ghidra-callgraph.json"
            path.write_text(json.dumps(graph))
            check_independent_callgraph(
                manifest, ROOT / "game/QUIKY.EXE", path
            )

    def test_verifier_hashes_the_supplied_segment(self):
        manifest = json.loads(
            (ROOT / "research/ghidra/player-callback-closure.json").read_text()
        )
        executable = ROOT / "game/QUIKY.EXE"
        segment = read_segments(executable)[2]
        blob = executable.read_bytes()
        start = int(segment["file_offset"])
        length = int(segment["file_length"])
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "fresh-segment.bin"
            path.write_bytes(blob[start:start + length])
            check_hashes(manifest, ROOT, path)
            path.write_bytes(b"stale")
            with self.assertRaises(ClosureError):
                check_hashes(manifest, ROOT, path)


if __name__ == "__main__":
    unittest.main()
