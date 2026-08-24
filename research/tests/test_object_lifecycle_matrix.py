import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from object_lifecycle_matrix import (  # noqa: E402
    _behavior_summary,
    _entity_summary,
    _movement_summary,
    parse_disassembly_writes,
)


class ObjectLifecycleMatrixTests(unittest.TestCase):
    def test_disassembly_parser_keeps_stores_and_ignores_loads(self):
        writes = parse_disassembly_writes("""
            1234: 8b 5d 18          mov    %es:0x18(%di),%bx
            1238: c7 45 18 27 3f    movw   $0x3f27,%es:0x18(%di)
            123d: c7 45 1a ff ff    movw   $0xffff,%es:0x1a(%di)
            1242: 89 4d 18          mov    %cx,%es:0x18(%di)
        """)
        self.assertEqual([write["address"] for write in writes],
                         ["1238", "123d", "1242"])
        self.assertEqual(writes[0]["action"], "install_callback")
        self.assertEqual(writes[0]["value"], 0x3F27)
        self.assertEqual(writes[1]["action"], "set_source")
        self.assertEqual(writes[1]["value"], 0xFFFF)
        self.assertEqual(writes[2]["action"], "modify_callback")

    def test_movement_summary_identifies_cull_and_reactivation(self):
        def frame(marker, callback, slot=1, active=True):
            raw = bytearray(0x40)
            raw[0x18:0x1A] = callback.to_bytes(2, "little")
            return {
                "object_state_hex": raw.hex(),
                "lifecycle": {
                    "source": {"marker_word": marker},
                    "pool": {"source_matches": [{
                        "index": slot,
                        "active": active,
                        "update_callback": callback,
                    }]},
                    "scheduler": {"banks": [
                        {"bank": 0, "entries": [{"object_offset": 120}]},
                        {"bank": 1, "entries": [{"object_offset": 120}]},
                    ]},
                },
            }

        summary = _movement_summary({
            "type": 0x2B,
            "record_offset": 0x1792,
            "source": {"selector": 0x37F, "offset": 0x1632},
            "frames": [
                frame(0x012B, 0x47E7),
                frame(0x002B, 0, active=False),
                frame(0x012B, 0x47E7),
            ],
        })
        self.assertTrue(summary["marker_cleared"])
        self.assertTrue(summary["marker_restored"])
        self.assertEqual(summary["category"],
                         "visibility_culled_then_reactivated")
        self.assertEqual(summary["first_pool_slot"], 1)
        self.assertEqual(summary["reactivated_pool_slot"], 1)
        self.assertEqual(summary["marker_clear_index"], 0)
        self.assertEqual(summary["marker_restore_index"], 1)

    def test_movement_summary_identifies_callback_end_without_cull(self):
        live = bytearray(0x40)
        live[0x18:0x1A] = (0x6DC4).to_bytes(2, "little")
        ended = bytearray(0x40)
        summary = _movement_summary({
            "type": 0x01,
            "frames": [{
                "object_state_hex": live.hex(),
                "lifecycle": {
                    "source": {"marker_word": 0x0101},
                    "pool": {"source_matches": [{
                        "index": 4, "active": True, "update_callback": 0x6DC4,
                    }]},
                    "scheduler": {"banks": []},
                },
            }, {
                "object_state_hex": ended.hex(),
                "lifecycle": {
                    "source": {"marker_word": 0x0101},
                    "pool": {"source_matches": [{
                        "index": 4, "active": False, "update_callback": 0,
                    }]},
                    "scheduler": {"banks": []},
                },
            }],
        })
        self.assertFalse(summary["marker_cleared"])
        self.assertTrue(summary["callback_cleared_while_source_processed"])
        self.assertEqual(summary["category"], "self_terminated_or_state_ended")

    def test_behavior_summary_distinguishes_persistence_and_callback_end(self):
        def sample(before, after, marker=0x012B):
            return {
                "object_before": {"update_callback": before},
                "object_after": {"update_callback": after},
                "source_before": {"marker_word": marker},
                "source_after": {"marker_word": marker},
                "callback": {"related_hits": []},
            }

        persistent = _behavior_summary({
            "type": 0x28,
            "samples": [sample(0x9256, 0x9256)],
        })
        ended = _behavior_summary({
            "type": 0x2B,
            "samples": [sample(0x47E7, 0)],
        })
        self.assertEqual(persistent["category"], "persistent_in_window")
        self.assertEqual(ended["category"], "self_terminated_or_state_ended")

    def test_entity_summary_keeps_factory_evidence_separate(self):
        raw = bytearray(0x40)
        raw[0x18:0x1A] = (0x8C4E).to_bytes(2, "little")
        summary = _entity_summary({
            "type": 0x2C,
            "record_offset": 0x1792,
            "object_class": 1,
            "object": {"offset": 120},
            "object_state_hex": raw.hex(),
            "update_callback": {"offset": 0x8C4E},
            "lifetime_samples": [{"object": {"offset": 120}}],
        })
        self.assertEqual(summary["category"],
                         "factory_snapshot_with_lifetime_samples")
        self.assertEqual(summary["object_callback_field"], 0x8C4E)
        self.assertEqual(summary["lifetime_object_offsets"], [120])


if __name__ == "__main__":
    unittest.main()
