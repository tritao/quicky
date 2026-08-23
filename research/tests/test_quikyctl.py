import struct
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from quikyctl import parse_archive, parse_map, parse_ne  # noqa: E402


class QuikyCtlTests(unittest.TestCase):
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
        executable = parse_ne(repo_root / "game" / "QUIKY.EXE")

        self.assertEqual(len(archive.entries), 142)
        self.assertEqual(archive.entries[0].name, "QUIKYW1.BOB")
        self.assertEqual(archive.entries[-1].name, "TITELD.SAM")
        self.assertEqual(executable.segment_count, 7)
        self.assertEqual(executable.sector_shift, 8)


if __name__ == "__main__":
    unittest.main()
