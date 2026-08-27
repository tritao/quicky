import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from verify_wurm2_dos_trace import verify  # noqa: E402


def record(*, x: int, velocity: int, delay: int, map_latch: int) -> bytes:
    value = bytearray(0x78)
    value[0x02:0x06] = (x & 0xFFFFFFFF).to_bytes(4, "little")
    value[0x0A:0x0E] = (velocity & 0xFFFFFFFF).to_bytes(4, "little")
    value[0x18:0x1A] = (0x6DC4).to_bytes(2, "little")
    value[0x20:0x22] = (delay & 0xFFFF).to_bytes(2, "little")
    value[0x24:0x26] = (0x33F2).to_bytes(2, "little")
    value[0x29] = 1
    value[0x2C] = 0xFF
    value[0x2F] = map_latch & 0xFF
    return bytes(value)


class Wurm2DosTraceTests(unittest.TestCase):
    def test_initializer_latch_takes_ordinary_6f16_path(self):
        before = record(x=0x01A4C000, velocity=0x15000, delay=1,
                        map_latch=0xFF)
        after = record(x=0x01A61000, velocity=0x15000, delay=0,
                       map_latch=0xFF)
        result = verify({
            "events": [{"samples": [{
                "sequence": 1,
                "player_callback": {
                    "callback_offset": 0x6DC4,
                    "pre_object": {"state_hex": before.hex()},
                    "post_object": {"state_hex": after.hex()},
                },
                "globals": {"target_active_count": 0},
            }]}],
        })
        self.assertEqual(result["motion_steps_checked"], 1)
        self.assertEqual(result["callbacks_checked"], 1)


if __name__ == "__main__":
    unittest.main()
