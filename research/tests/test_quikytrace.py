import struct
import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIR))

from quikytrace import (  # noqa: E402
    TraceError,
    decode_lookup_call,
    decode_resource_state,
    trace_resources_lua,
)


class QuikyTraceTests(unittest.TestCase):
    def test_decode_lookup_call(self):
        stack = struct.pack("<HHHH", 0x36D0, 0x01D7, 0x1234, 0x0237)
        self.assertEqual(decode_lookup_call(stack), {
            "return_offset": 0x36D0, "return_segment": 0x01D7,
            "path_offset": 0x1234, "path_segment": 0x0237,
        })

    def test_decode_resource_state(self):
        raw = struct.pack("<III", 0x2F55A5, 0x2F3BD3, 0x19D2)
        self.assertEqual(decode_resource_state(raw), {
            "start": 0x2F3BD3, "end": 0x2F55A5, "size": 0x19D2,
        })

    def test_decoders_reject_truncated_data(self):
        with self.assertRaises(TraceError):
            decode_lookup_call(b"short")
        with self.assertRaises(TraceError):
            decode_resource_state(bytes(11))

    def test_lua_trace_loader_returns_events_in_sequence(self):
        class FakeApi:
            loaded_source = ""

            def request(self, method, path, text_body=None):
                self.loaded_source = text_body
                return {"status": "loaded"}

            def post(self, path):
                return {"status": "started"}

            def get(self, path):
                return {
                    "state": "completed",
                    "output": {"events": {"2": {"sequence": 2}, "1": {"sequence": 1}}},
                }

        api = FakeApi()
        script = Path(__file__).resolve().parents[1] / "automation/quiky_resource_trace.lua"
        events = trace_resources_lua(api, script, 2, 1, 0.01, True, False, 30)
        self.assertEqual([event["sequence"] for event in events], [1, 2])
        self.assertIn("TRACE_COUNT=2", api.loaded_source)
        self.assertIn("TRACE_PREPARE_W1L3=true", api.loaded_source)
        self.assertIn("TRACE_NAVIGATE_W1L3=false", api.loaded_source)
        self.assertIn("TRACE_NAVIGATE_LEVEL=", api.loaded_source)
        self.assertIn('TRACE_SELECT_LEVEL=""', api.loaded_source)
        self.assertIn("TRACE_TAIL_COUNT=0", api.loaded_source)

    def test_navigation_trace_quotes_level_and_replays_startup(self):
        class FakeApi:
            loaded_source = ""
            status_calls = 0
            replayed = False

            def request(self, method, path, text_body=None):
                self.loaded_source = text_body
                return {"status": "loaded"}

            def post(self, path, body=None):
                if path == "/api/v1/input/sequence":
                    self.replayed = True
                return {"status": "started"}

            def get(self, path):
                self.status_calls += 1
                if not self.replayed:
                    return {"state": "running", "output": {"awaiting_startup_replay": True}}
                return {"state": "completed", "output": {"events": {"1": {"sequence": 1}}}}

        api = FakeApi()
        script = Path(__file__).resolve().parents[1] / "automation/quiky_resource_trace.lua"
        recording = Path(__file__).resolve().parents[1] / "automation/startup-to-input.json"
        events = trace_resources_lua(
            api, script, 1, 1, 0.01, False, False, 30, recording,
            "W4L1", None, 0,
        )
        self.assertEqual([event["sequence"] for event in events], [1])
        self.assertIn('TRACE_NAVIGATE_LEVEL="W4L1"', api.loaded_source)
        self.assertTrue(api.replayed)


if __name__ == "__main__":
    unittest.main()
