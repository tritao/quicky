# Focused player callback audit

The machine-readable audit is [`player-callback-focused-audit.json`](player-callback-focused-audit.json). It is a supplement to the closure ledger, not a second decompilation. The authoritative C-like control flow remains [`player-static-decomp.cpp`](../notes/player-static-decomp.cpp).

The raw 8086 decode closes the requested field writes and the non-obvious caller ordering:

| Path | Exact order | Result |
| --- | --- | --- |
| Positive mode `41E8` | side probes → `1C6E` alignment test → `3D02` → Y integration → `3A1F` | No `3DF2` call on the clear positive integration path. |
| Grounded/contact `427F` | `3D02` → `3DF2` → zero vertical velocity → mode `0` | `3DF2` can snap the integer Y word. |
| Ordinary correction `42C9` | `3DF2` → `3D02` → `3971` jump probe | This is the reverse order from `427F`. |
| Negative mode `4323` | `3986` → velocity/Y integration → `3986` | Reachable path does not call `3D02` or `3DF2`. |

The exact writes to `+0x37`, `+0x3A`, `+0x3B`, `+0x3E`, and `+0x40` are listed by instruction address, condition, value, and evidence in the JSON. The raw unreachable fragment at `436B–437F` is retained as a dead write site and is not promoted into the C-like reachable model.

## Dynamic status

The fresh main-repository callback traces answer natural jump initiation, the ascent-to-fall transition, and a subsequent landing/contact transition. Fresh side-probe traces answer call order and movement through a clear corridor. A dense player-scoped `5D60` watch additionally closes the animation boundary: the landing callback loads sequence `3156` (`+0x1E/+0x20=4`) before `5D60` decrements `+0x20` to `3`, and each following ordinary no-input callback reloads `3156` while `DS:4FEE < 0xD2`, preserving post-delay `3`. The C++ callback now implements that reload and the late-release replay has no post-record mismatch at this boundary. The low-Y ceiling trace is explicitly controlled and therefore does not assign floor/ceiling polarity. The moving-platform evidence is archival from the same executable but a different tracer worktree; a current-main capture stopped at the ARE declaration timeout. Those unresolved runtime points keep the implementation gate closed.

Run the audit check with:

```text
python3 research/tools/verify_player_callback_focused_audit.py
```

The optional ignored `research/build/` traces are hash-checked when present. Use `--require-traces` when reproducing the local capture set.
