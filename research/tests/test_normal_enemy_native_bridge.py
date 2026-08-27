import unittest

from research.tools.verify_normal_enemy_native_bridge import verify


class NormalEnemyNativeBridgeTests(unittest.TestCase):
    def test_bridge_matches_static_closure(self):
        verify()


if __name__ == "__main__":
    unittest.main()
