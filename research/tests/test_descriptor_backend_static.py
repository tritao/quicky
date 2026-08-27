import unittest
from pathlib import Path

from research.tools.verify_descriptor_backend_static import verify


ROOT = Path(__file__).resolve().parents[2]


class DescriptorBackendStaticTest(unittest.TestCase):
    def test_descriptor_backend_ledger(self) -> None:
        verify(ROOT / "research/ghidra/descriptor-backend-static-closure.json", ROOT)


if __name__ == "__main__":
    unittest.main()
