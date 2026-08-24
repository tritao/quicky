import struct
import json
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from quikyctl import (  # noqa: E402
    QuikyError,
    build_are_type_catalog,
    create_are_experiments,
    create_entity_variant,
    decode_bob_record,
    extract_archive,
    find_archive_bob_slots,
    index_archive,
    parse_are,
    parse_archive,
    parse_bob,
    parse_map,
    parse_ne,
    patch_are_entity_data,
    render_bob_sheet,
    render_level,
    iter_are_entity_placements,
    load_entity_type_names,
    load_dispatch_ledger,
    select_entity_representative,
)


class QuikyCtlTests(unittest.TestCase):
    def test_bob_parser_and_safe_blitter_decoder(self):
        code = (
            b"\xee\xd0\xc0\xc6\x84\x00\x00\x01"
            b"\xee\xd0\xc0\xc6\x84\x00\x00\x02\x58\x5e\xcb"
        )
        raw = (
            struct.pack("<6H", 7, 1, 2, 2, 1, 4)
            + struct.pack("<2H", 0, 8)
            + struct.pack("<H", len(code))
            + code
        )
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "TEST.BOB"
            path.write_bytes(raw)
            info = parse_bob(path)
            palette_path = Path(temp_dir) / "TEST.PCC"
            header = bytearray(128)
            header[0], header[3] = 0x0A, 0x08
            palette_path.write_bytes(header + b"\x0c" + bytes(768))
            sheet_path = Path(temp_dir) / "sheet.png"
            sheet = render_bob_sheet(path, palette_path, sheet_path)
            self.assertEqual(sheet.slot_rows, ((7,),))
            self.assertTrue(sheet_path.read_bytes().startswith(b"\x89PNG"))

        self.assertEqual(len(info.records), 1)
        record = info.records[0]
        self.assertEqual(
            (record.slot, record.origin_x, record.origin_y, record.width, record.height),
            (7, 1, 2, 2, 1),
        )
        self.assertEqual(decode_bob_record(record), (1, 2))

    def test_archive_parser_and_sizes(self):
        payloads = [("FIRST.BOB", b"abc"), ("SECOND.MAP", b"12345")]
        data = b"".join(payload for _, payload in payloads)
        directory_offset = len(data)
        directory = bytearray()
        offset = 0
        for name, payload in payloads:
            encoded = name.encode("ascii")
            directory += struct.pack("<H", len(encoded)) + encoded
            directory += struct.pack("<I", offset)
            offset += len(payload)
        archive = data + directory + struct.pack(
            "<II", directory_offset, len(payloads) - 1
        )

        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "NESTLE.DAT"
            path.write_bytes(archive)
            info = parse_archive(path)

        self.assertEqual(info.directory_offset, 8)
        expected_entries = [
            (name, offset, len(payload))
            for offset, (name, payload) in zip((0, 3), payloads)
        ]
        self.assertEqual(
            [(e.name, e.offset, e.size) for e in info.entries], expected_entries
        )
        self.assertEqual(info.entries[0].size, 3)
        self.assertEqual(info.entries[1].size, 5)

        with tempfile.TemporaryDirectory() as temp_dir:
            archive_path = Path(temp_dir) / "NESTLE.DAT"
            output_dir = Path(temp_dir) / "assets"
            archive_path.write_bytes(archive)
            extracted = extract_archive(archive_path, output_dir)
            self.assertEqual(
                [path.name for path in extracted], ["FIRST.BOB", "SECOND.MAP"]
            )
            self.assertEqual((output_dir / "FIRST.BOB").read_bytes(), b"abc")
            self.assertEqual((output_dir / "SECOND.MAP").read_bytes(), b"12345")
            with self.assertRaises(QuikyError):
                extract_archive(archive_path, output_dir)

            unsafe_name = b"../ESCAPE.BOB"
            unsafe_archive = b"x" + struct.pack("<H", len(unsafe_name))
            unsafe_archive += unsafe_name + struct.pack("<I", 0)
            unsafe_archive += struct.pack("<II", 1, 0)
            unsafe_path = Path(temp_dir) / "UNSAFE.DAT"
            unsafe_path.write_bytes(unsafe_archive)
            with self.assertRaises(QuikyError):
                extract_archive(unsafe_path, Path(temp_dir) / "unsafe-assets")

    def test_are_parser_decodes_references_and_entities(self):
        raw = bytearray(0x14E8)
        struct.pack_into(">HH", raw, 0x0E, 2, 1)
        struct.pack_into(">H", raw, 0x160, 0x1388)
        struct.pack_into(">H", raw, 0x162, 0xFFFF)
        raw.extend(
            struct.pack(
                ">HHHHHHH",
                0x002B,
                0x0010,
                0x0020,
                0x0065,
                0x0030,
                0x0040,
                0xFFFF,
            )
        )

        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "W1L1.ARE"
            path.write_bytes(raw)
            info = parse_are(path)

        self.assertEqual((info.layout_width, info.layout_height), (2, 1))
        self.assertEqual(info.layout_word_count, 2)
        self.assertEqual(info.zero_word_count, 0)
        self.assertEqual(info.blank_word_count, 1)
        self.assertEqual(info.unique_reference_count, 1)
        self.assertEqual(info.entity_count, 2)
        self.assertEqual(dict(info.entity_types), {0x2B: 1, 0x65: 1})
        self.assertEqual(info.references[0].target_offset, 0x14E8)
        self.assertEqual(
            [entity.record_offset for entity in info.references[0].entities],
            [0x14E8, 0x14EE],
        )
        self.assertEqual(
            [
                (entity.entity_type, entity.x, entity.y)
                for entity in info.references[0].entities
            ],
            [(0x2B, 0x10, 0x20), (0x65, 0x30, 0x40)],
        )

        patched = patch_are_entity_data(
            raw,
            reference=0x1388,
            entity_index=1,
            delta_x=0x10,
            entity_type=0x71,
        )
        self.assertEqual(
            struct.unpack_from(">HHH", patched, 0x14EE),
            (0x71, 0x40, 0x40),
        )

    def test_map_parser_is_big_endian_and_splits_flags(self):
        cells = (0x0123, 0x3812)
        raw = b"TLE1" + struct.pack(">HHH", 2, 1, 9) + struct.pack(">2H", *cells)

        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "W1L1.MAP"
            path.write_bytes(raw)
            info = parse_map(path)

        self.assertEqual((info.width, info.height, info.unknown), (2, 1, 9))
        self.assertEqual(info.expected_size, len(raw))
        self.assertEqual(info.max_tile, 0x123)
        self.assertEqual(dict(info.property_values), {0x00: 1, 0x1C: 1})

    def test_are_placements_include_each_reference_grid_cell(self):
        raw = bytearray(0x14E8)
        struct.pack_into(">HH", raw, 0x0E, 2, 1)
        struct.pack_into(">HH", raw, 0x160, 0x1388, 0x1388)
        raw.extend(struct.pack(">HHHH", 0x002B, 0x0010, 0x0020, 0xFFFF))
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "W1L1.ARE"
            path.write_bytes(raw)
            info = parse_are(path)
        placements = iter_are_entity_placements(info)
        self.assertEqual(
            [(item.region_x, item.region_y, item.world_x, item.world_y)
             for item in placements],
            [(0, 0, 16, 32), (64, 0, 80, 32)],
        )

    def test_entity_catalog_and_record_variant_cover_confirmed_2b(self):
        repo_root = Path(__file__).resolve().parents[2]
        archive = repo_root / "game" / "NESTLE.DAT"
        catalog = build_are_type_catalog(archive)
        candidate = next(item for item in catalog if item.entity_type == 0x2B)
        self.assertIn("W1L1.ARE", candidate.levels)
        self.assertEqual(candidate.dispatch_slot, "DS:81D2+0x0AC")
        dispatch = load_dispatch_ledger(
            repo_root / "research" / "build" / "entity-dispatch-table.json"
        )
        for entity_type, dispatch_slot, object_class in (
            (0x28, "DS:81D2+0x0A0", 0),
            (0x29, "DS:81D2+0x0A4", 1),
            (0x2A, "DS:81D2+0x0A8", 1),
        ):
            candidate = next(item for item in catalog if item.entity_type == entity_type)
            self.assertEqual(candidate.dispatch_slot, dispatch_slot)
            self.assertEqual(dispatch[entity_type].object_class, object_class)
        dedicated = next(item for item in catalog if item.entity_type == 0x65)
        self.assertEqual(dedicated.dispatch_entry, "01F7:178D")
        names = load_entity_type_names()
        self.assertEqual(names[0x6F].name, "ten_ammo_box")
        self.assertEqual(
            find_archive_bob_slots(archive, {607}),
            ({"slot": 607, "asset": "WERBE.BOB", "record_index": 0,
              "record_offset": 0, "origin_x": 0, "origin_y": 0,
              "width": 26, "height": 34},),
        )
        for entity_type, name, slot, dimensions in (
            (0x70, "extra_health_package", 608, (21, 22)),
            (0x71, "health_upgrade", 609, (22, 22)),
            (0x72, "temporary_invulnerability", 610, (15, 25)),
        ):
            self.assertEqual(names[entity_type].name, name)
            match = find_archive_bob_slots(archive, {slot})
            self.assertEqual(len(match), 1)
            self.assertEqual((match[0]["width"], match[0]["height"]), dimensions)

        self.assertEqual(names[0x73].confidence, "unknown")
        self.assertEqual(names[0x74].confidence, "unknown")
        for entity_type, name in (
            (0x65, "dedicated_event_variant_65"),
            (0x66, "dedicated_event_variant_66"),
            (0x67, "dedicated_event_variant_67"),
        ):
            self.assertEqual(names[entity_type].name, name)
            self.assertEqual(names[entity_type].confidence, "confirmed")
        for entity_type, name in (
            (0x01, "wurm2_variant_01"),
            (0x02, "wurm2_variant_02"),
            (0x03, "biene_variant_03"),
            (0x04, "biene_variant_04"),
            (0x05, "fisch_variant_05"),
            (0x06, "fisch_variant_06"),
            (0x07, "krabbe_variant_07"),
            (0x08, "krabbe_variant_08"),
            (0x09, "pengo_variant_09"),
            (0x0A, "pengo_variant_0a"),
            (0x0B, "prop_variant_0b"),
            (0x0C, "prop_variant_0c"),
            (0x15, "fliege_variant_15"),
            (0x16, "fliege_variant_16"),
            (0x17, "spinne_variant_17"),
            (0x18, "spinne_variant_18"),
            (0x19, "buggy_variant_19"),
            (0x1A, "buggy_variant_1a"),
            (0x1B, "ufo_variant_1b"),
            (0x1C, "ufo_variant_1c"),
        ):
            self.assertEqual(names[entity_type].name, name)
            self.assertEqual(names[entity_type].confidence, "probable")
        self.assertEqual(names[0x28].name, "cloud")
        self.assertEqual(names[0x1F].confidence, "unknown")
        self.assertEqual(names[0x20].confidence, "unknown")
        self.assertEqual(names[0x21].confidence, "unknown")
        for entity_type, name, slot, asset, dimensions in (
            (0x2C, "paper_effect", 710, "PAPIER.BOB", (18, 15)),
            (0x34, "bump_effect", 400, "BUMP_W1.BOB", (32, 23)),
        ):
            self.assertEqual(names[entity_type].name, name)
            match = [
                item for item in find_archive_bob_slots(archive, {slot})
                if item["asset"] == asset
            ]
            self.assertEqual(len(match), 1)
            self.assertEqual((match[0]["width"], match[0]["height"]), dimensions)
        for entity_type, name in (
            (0x33, "snow_effect_variant_51"),
            (0x35, "snow_effect_variant_53"),
            (0x36, "snow_effect_variant_54"),
            (0x3D, "moving_platform_variant_61"),
            (0x3E, "moving_platform_variant_62"),
            (0x3F, "moving_platform_variant_63"),
            (0x40, "moving_platform_variant_64"),
        ):
            self.assertEqual(names[entity_type].name, name)
        for entity_type, name, slot in (
            (0x29, "falling_leaves_variant_29", 700),
            (0x2A, "falling_leaves_variant_2a", 700),
            (0x79, "puzzle_letter_N", 600),
            (0x7A, "puzzle_letter_E", 601),
            (0x7B, "puzzle_letter_S", 602),
            (0x7C, "puzzle_letter_Q", 603),
            (0x7D, "puzzle_letter_U", 604),
            (0x7E, "puzzle_letter_I", 605),
            (0x7F, "puzzle_letter_K", 606),
        ):
            self.assertEqual(names[entity_type].name, name)
            match = find_archive_bob_slots(archive, {slot})
            if entity_type in (0x29, 0x2A):
                match = [item for item in match if item["asset"] == "BLATT.BOB"]
                self.assertEqual(len(match), 1)
                self.assertEqual((match[0]["width"], match[0]["height"]), (14, 12))
            else:
                self.assertEqual(len(match), 1)
                self.assertEqual(match[0]["asset"], "PUZZLE.BOB")
                self.assertEqual((match[0]["width"], match[0]["height"]), (16, 16))
        cloud = find_archive_bob_slots(archive, {413, 414, 415, 416})
        self.assertEqual({item["asset"] for item in cloud}, {"WOLKE.BOB"})
        self.assertEqual({(item["width"], item["height"]) for item in cloud}, {(32, 16)})

        with tempfile.TemporaryDirectory() as temp_dir:
            manifest = create_entity_variant(
                archive,
                Path(temp_dir),
                "W1L1.ARE",
                0x1792,
            )
            self.assertEqual(manifest["entity_type"], 0x2B)
            self.assertEqual(
                (manifest["placements"][0]["world_x"],
                 manifest["placements"][0]["world_y"]),
                (768, 224),
            )
            removed = Path(manifest["variants"][1]["archive"])
            removed_info = parse_archive(removed)
            entry = next(item for item in removed_info.entries
                         if item.name == "W1L1.ARE")
            payload = removed.read_bytes()[entry.offset:entry.offset + entry.size]
            self.assertEqual(struct.unpack_from(">H", payload, 0x1792)[0], 0)

            target_manifest = create_entity_variant(
                archive,
                Path(temp_dir) / "target",
                "W1L1.ARE",
                0x1792,
                target_type=0x29,
            )
            target_archive = Path(target_manifest["variants"][0]["archive"])
            target_info = parse_archive(target_archive)
            target_entry = next(
                item for item in target_info.entries if item.name == "W1L1.ARE"
            )
            target_bytes = target_archive.read_bytes()
            self.assertEqual(
                struct.unpack_from(">H", target_bytes, target_entry.offset + 0x1792)[0],
                0x29,
            )
            self.assertEqual(target_manifest["original_entity_type"], 0x2B)

    def test_entity_name_catalog_loads_confidence_ratings(self):
        names = load_entity_type_names()
        self.assertEqual(names[0x2B].name, "falling_leaves")
        self.assertEqual(names[0x2B].confidence, "confirmed")

    def test_representative_selection_uses_known_w1l1_anchor(self):
        archive = Path(__file__).resolve().parents[2] / "game" / "NESTLE.DAT"
        selected = select_entity_representative(archive, 0x2B)
        self.assertEqual(selected.record_offset, 0x1792)
        self.assertEqual((selected.world_x, selected.world_y), (768, 224))

        with self.assertRaises(QuikyError):
            select_entity_representative(archive, 0x02)

    def test_entity_variant_can_redirect_selected_declaration_to_stream_cell(self):
        archive = Path(__file__).resolve().parents[2] / "game" / "NESTLE.DAT"
        selected = select_entity_representative(archive, 0x28)
        with tempfile.TemporaryDirectory() as temp_dir:
            manifest = create_entity_variant(
                archive, Path(temp_dir), "W1L1.ARE", selected.record_offset,
                stream_cell=(12, 3),
            )
            redirect = manifest["stream_redirect"]
            self.assertEqual(redirect["selected_reference"], selected.reference)
            self.assertEqual(redirect["runtime_region_origin"], [768, 192])

    def test_dispatch_ledger_validates_and_decodes_entries(self):
        ledger = {
            "trace_kind": "dispatch",
            "events": [{
                "type": 0x2B, "slot": 0x81D2 + 0x2B * 4,
                "offset": 0x4727, "object_class": 1, "reserved": 0,
                "raw_bytes": [0x27, 0x47, 1, 0],
            }],
        }
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "dispatch.json"
            path.write_text(json.dumps(ledger), encoding="utf-8")
            entries = load_dispatch_ledger(path)
        self.assertEqual(entries[0x2B].group_key, (0x4727, 1, 0))

    def test_level_renderer_pairs_map_tiles_palette_and_are(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            map_path = root / "W1L1.MAP"
            map_path.write_bytes(
                b"TLE1" + struct.pack(">HHH", 2, 1, 9) + struct.pack(">2H", 0, 1)
            )

            palette = bytearray(256 * 3)
            palette[3:6] = bytes((255, 0, 0))
            palette[6:9] = bytes((0, 255, 0))
            pcc = bytearray(128)
            pcc[0] = 0x0A
            pcc[1] = 5
            pcc[2] = 1
            pcc[3] = 8
            pcc.extend(b"\x0C" + palette)
            (root / "W1.PCC").write_bytes(pcc)
            (root / "W1.ICO").write_bytes(bytes((1,)) * 256 + bytes((2,)) * 256)

            are = bytearray(0x14E8)
            struct.pack_into(">HH", are, 0x0E, 1, 1)
            struct.pack_into(">H", are, 0x160, 0x1388)
            are.extend(struct.pack(">HHHH", 0x0065, 0x0010, 0x0020, 0xFFFF))
            (root / "W1L1.ARE").write_bytes(are)

            output = root / "W1L1.png"
            summary = render_level(map_path, output)
            png = output.read_bytes()

        self.assertEqual((summary.pixel_width, summary.pixel_height), (32, 16))
        self.assertEqual(summary.tile_count, 2)
        self.assertEqual(summary.invalid_tile_cells, 0)
        self.assertEqual(summary.entity_count, 1)
        self.assertEqual(png[:8], b"\x89PNG\r\n\x1a\n")
        self.assertEqual(struct.unpack_from(">II", png, 16), (32, 16))

    def test_are_experiment_generates_valid_variant_archives(self):
        repo_root = Path(__file__).resolve().parents[2]
        with tempfile.TemporaryDirectory() as temp_dir:
            manifest = create_are_experiments(
                repo_root / "game" / "NESTLE.DAT",
                Path(temp_dir),
            )
            self.assertEqual(len(manifest["variants"]), 5)
            for variant in manifest["variants"]:
                index = index_archive(Path(variant["archive"]))
                self.assertEqual(index.entry_count, 142)
                self.assertEqual(index.assets[-1].name, "TITELD.SAM")

    def test_ne_parser_reads_segment_table(self):
        raw = bytearray(0x100)
        raw[:2] = b"MZ"
        struct.pack_into("<I", raw, 0x3C, 0x40)
        raw[0x40 : 0x42] = b"NE"
        raw[0x42 : 0x44] = bytes((6, 1))
        struct.pack_into("<HH", raw, 0x54, 0x20, 0x0001)  # IP, CS
        struct.pack_into("<HH", raw, 0x58, 0x10, 0x0002)  # SP, SS
        struct.pack_into("<HH", raw, 0x5C, 1, 0)  # segments, module refs
        struct.pack_into("<HH", raw, 0x62, 0x70, 8)  # segment table, resource
        raw[0x76] = 1  # target OS
        struct.pack_into("<H", raw, 0x72, 8)  # sector shift
        struct.pack_into("<HHHH", raw, 0xB0, 2, 0x30, 0x0010, 0x40)

        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "TEST.EXE"
            path.write_bytes(raw)
            info = parse_ne(path)

        self.assertEqual(info.ne_offset, 0x40)
        self.assertEqual(info.segment_count, 1)
        self.assertEqual(info.segments[0].file_offset, 0x200)
        self.assertEqual(info.segments[0].length, 0x30)

    def test_bundled_archive_and_executable(self):
        repo_root = Path(__file__).resolve().parents[2]
        archive = parse_archive(repo_root / "game" / "NESTLE.DAT")
        index = index_archive(repo_root / "game" / "NESTLE.DAT")
        executable = parse_ne(repo_root / "game" / "QUIKY.EXE")

        self.assertEqual(len(archive.entries), 142)
        self.assertEqual(archive.entries[0].name, "QUIKYW1.BOB")
        self.assertEqual(archive.entries[-1].name, "TITELD.SAM")
        self.assertEqual(
            {summary.extension: summary.count for summary in index.type_counts},
            {
                "ARE": 21,
                "BOB": 66,
                "ICO": 15,
                "MAP": 21,
                "PCC": 13,
                "SAM": 3,
                "TFX": 3,
            },
        )
        w1l1_map = next(asset for asset in index.assets if asset.name == "W1L1.MAP")
        self.assertEqual(
            (w1l1_map.map_width, w1l1_map.map_height, w1l1_map.map_max_tile),
            (270, 30, 468),
        )
        w1l1_are = next(asset for asset in index.assets if asset.name == "W1L1.ARE")
        self.assertEqual(
            (w1l1_are.are_unique_references, w1l1_are.are_entity_count),
            (109, 173),
        )
        self.assertEqual(executable.segment_count, 7)
        self.assertEqual(executable.sector_shift, 8)


if __name__ == "__main__":
    unittest.main()
