import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "research" / "tools"))

from verify_player_factory_static import verify  # noqa: E402


class PlayerFactoryStaticTests(unittest.TestCase):
    def test_focused_factory_closure_is_reproducible(self):
        verify(ROOT / "research/ghidra/player-factory-static-closure.json", ROOT)


if __name__ == "__main__":
    unittest.main()
