# Selective player/input static slice

This is the first static-analysis slice aimed at a compatible C engine. It
stays at the level of facts that can be tied to executable bytes or an existing
runtime trace; names such as “player” and “collision” are not assigned where
the code does not yet prove them.

## Input path

The protected-mode selectors are the existing runtime mapping: segment 1 is
`01D7` and segment 3 is `01F7`.

| Address | Evidence-backed role | Important state |
| --- | --- | --- |
| `01F7:F17F` | IRQ1 handler; reads port `0x60` and writes the 32-byte scan-code ring | `FFFF:501E`, head `DS:503E` |
| `01F7:F1A8` | Consumes the ring, handles make/break bytes, maps arrows and Space to action bits | `DS:88BA`, `DS:88BC` |
| `01F7:F21B` | Returns the OR of normalized keyboard actions and the other action-word source | `DS:88BC \| DS:8196` |
| `01D7:01AC` | Menu/game helper tests directional/confirm flags | `DS:8196`, `DS:88BC` |
| `01D7:0203` | Waits until both action words are clear | `DS:8196`, `DS:88BC` |
| `01D7:4AC2` | Runtime-confirmed hidden level-selector loop | flags `1`, `2`, `0x20` |

The keyboard mapping is directly visible in the `F1A8` comparisons:

```text
scan 4B -> action 08   left
scan 4D -> action 04   right
scan 50 -> action 01   down
scan 48 -> action 02   up
scan 38 -> action 10   left shift/alternate action
scan 39 -> action 20   space/confirm
```

The high bit of a scan byte is treated as a break event. Make events OR the
mapped bit into `DS:88BC`; break events clear it. `DS:88BA` retains the most
recent seven-bit scan code. The action words are therefore a better C-engine
interface than exposing raw DOS keyboard bytes.

## Pooled-object scheduler

Segment 3 uses a 64-entry object pool. Each list entry is eight bytes and
contains an object callback/list reference. The object record fields already
used by the entity traces are:

```text
+0x02/+0x06   16.16 X/Y position
+0x12          logical sprite slot
+0x14          kind field (one helper searches for 0x64; meaning unresolved)
+0x17          update phase/state byte
+0x18          update callback offset
+0x2C          lifetime/counter field for short-lived objects
+0x2E          type-specific state/index
+0x32          update-state word
```

The static routines are:

| Address | Role |
| --- | --- |
| `01F7:0E06` | Find a free pool entry and initialize it; returns the object in `ES:DI`. |
| `01F7:0E66` | Count occupied pool entries into `DS:88C8`. |
| `01F7:0E96` | Run callbacks grouped by phase values `0`, `1`, and `2` in `+0x17`. |
| `01F7:0FA2` | Run callbacks for list entries with a non-null callback pointer; the list/object state meaning is unresolved. |
| `01F7:0F3C` | Search for `+0x14 == 0x64`; this is a useful breakpoint candidate, not yet a player identification. |

This explains why a compatible engine should model a stable pooled-object
identity and callback phase, rather than treating each ARE declaration as a
permanent object.

## Camera and MAP access

`01F7:1ED7` updates the 16.16 scroll accumulators and clamps them against the
target bounds in `DS:36FC/36FE/3700/3702`. It publishes the integer camera at
`DS:81C0/81C4` and derives `DS:81CE/81D0`. `01F7:1CDA` then uses those camera
values to stream 64-pixel ARE regions.

`01F7:3376` converts the supplied coordinates to a MAP cell using four-bit
shifts, the loaded MAP width (`DS:657E`), and the MAP base (`DS:657A`), then
masks the result to the low nine-bit tile ID. It therefore proves the
16-pixel addressing used by the state-machine traces, while also showing that
this helper intentionally discards the high MAP property bits; collision and
hazard semantics still need a separate path.

## Runtime follow-up

The existing `quikytrace --state-machine-samples` path now records the input
words, camera words, and camera target bounds at each `01F7:8E4B` update entry
and exit. That gives the static labels a repeatable runtime check without
guessing the player object. The next pass can set a breakpoint on the first
confirmed player callback once a live object has been correlated to a stable
kind/callback pair. The annotation and targeted decompiler scripts were also
rerun against disposable raw-segment Ghidra imports; generated projects and C
reports remain outside the repository.
