import sys
import unittest
from pathlib import Path


TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))

from score_dat import ScoreDatError, decode_score_dat  # noqa: E402


class ScoreDatTests(unittest.TestCase):
    def test_bundled_score_file_decodes(self):
        report = decode_score_dat(
            (Path(__file__).resolve().parents[2] / "game" / "SCORE.DAT").read_bytes()
        )
        self.assertEqual(report["record_count"], 8)
        self.assertEqual(report["stored_checksum"], 0x5884)
        self.assertEqual(report["computed_checksum"], 0x5884)
        self.assertEqual(report["records"][0]["name"], "SIMONERI")
        self.assertEqual(report["records"][3]["score"], 100000)
        self.assertEqual(report["records"][0]["score_aux_word"], 0)
        self.assertEqual(report["records"][0]["progression_byte"], 0x0F)

    def test_rejects_checksum_corruption(self):
        data = bytearray((Path(__file__).resolve().parents[2] / "game" / "SCORE.DAT").read_bytes())
        data[0] ^= 1
        with self.assertRaises(ScoreDatError):
            decode_score_dat(bytes(data))

    def test_rejects_invalid_record_name_length(self):
        data = bytearray((Path(__file__).resolve().parents[2] / "game" / "SCORE.DAT").read_bytes())
        # Re-encode a deliberately invalid first decoded byte and repair the
        # checksum so the record validation is the failing condition.
        data[0] = 0xC8 ^ 9
        data[128:130] = (0x2A + sum(data[:128])).to_bytes(2, "little")
        with self.assertRaises(ScoreDatError):
            decode_score_dat(bytes(data))


if __name__ == "__main__":
    unittest.main()
