# Object behavior research

This track is isolated from the player tracer. It observes normal ARE object
scheduler entries and the selected object's update callback, recording the
object bytes before and after each callback plus the exact changed offsets.
It does not modify or break on the player callbacks or MAP collision helpers.

The implementation is:

- [`quiky_object_behavior_trace.lua`](../automation/quiky_object_behavior_trace.lua),
  which synchronizes with the three normal object scheduler class loops at
  `01F7:0EC7`, `01F7:0EEE`, and `01F7:0F14`;
- [`object_behavior_trace.py`](../tools/object_behavior_trace.py), which owns
  the separate `quiky-object-behavior-v1` envelope and normalizes Lua arrays.

The scheduler uses class-specific near-return sites `0x0ED3`, `0x0EFD`, and
`0x0F26`. The dispatch table at `DS:81D2 + type*4` supplies the type-specific
initializer; the actual callback used by the scheduler is read from the pool
entry at the scheduler breakpoint. This distinction is important because the
initializer can populate `object+0x18` with a different callback. The tracer
now also captures the scheduler post-callback sites (`0x0EDB`, `0x0F05`, and
`0x0F2E`), both scheduler banks, the source ARE record, and the callback's
internal visibility-gate calls.

## Recovered scheduler and pool lifecycle

The normal object pool is a 64-entry array at `DS:[0x755E]`, with stride
`DS:0x30CE = 0x78`. The allocator at `01F7:0E06` scans those 64 records and
treats an object as free when its word at `+0x18` is zero. It does not clear
the rest of the record, so a logically freed object can still retain its
position, lifetime, class, source pointer, and state bytes in memory.

The scheduler is a two-bank list at `DS:0x7566` and `DS:0x7766`. Each entry is
eight bytes: callback offset, callback selector, object offset, and an unused
object-selector word. `DS:0x7966` is both the insertion cursor and bank bit.
The registration path at `01F7:0x1036` appends an object only when
`object+0x18 != 0`, copies `object+0x18`/`+0x1C` into the callback far pointer,
copies the object offset, advances `DS:0x7966` by eight, and writes a `0xFFFF`
callback sentinel after the new entry.

At `01F7:0x0E96`, the update pass flips the bank, walks the previously built
bank to its `0xFFFF` sentinel, and dispatches in three phases selected by
object byte `+0x17 == 0, 1, 2`. The near callback returns to `0x0ED3`,
`0x0EFD`, or `0x0F26`; the following far helper and post sites then finish the
entry and increment `DS:0x88C8`. This explains why a table entry can remain
visible in a snapshot after its object callback has been cleared: the table is
the current pass's immutable work list, while the next bank is rebuilt for a
later pass.

## Visibility, deactivation, and reactivation marker

The camera gate at `01F7:0x1DCA` accepts approximately
`camera_x - 0x80 <= object.x <= camera_x + 0x1C0` and
`camera_y - 0x80 <= object.y <= camera_y + 0x130`, using unsigned arithmetic.
On rejection, `01F7:0x1DEE` clears `object+0x18` and clears byte `FS:[BX+1]`,
where `BX = object+0x1A` and `FS = DS:0x796E`.

`01F7:0x1E96` stores the source ARE declaration pointer in `object+0x1A`.
Therefore the second write is the source declaration's processed marker: a
normal declaration word changes from `0x0101`/`0x012B`/`0x0128` to
`0x0001`/`0x002B`/`0x0028` when the object is rejected by the camera gate.
The record is not zeroed, but it becomes logically free (`+0x18 == 0`) and is
eligible for allocation and for later declaration processing. The streaming
routine at `01F7:0x1CDA` only revisits declarations in newly visible 64-pixel
regions, so clearing this marker prepares reactivation; it does not
immediately recreate the object in the same pass.

Runtime evidence:

| Controlled case | Gate path | Source marker | Next scheduler bank |
| --- | --- | --- | --- |
| Type `0x01`, actual camera `(0,150)` | `1DCA -> 1DEE` | `0x0101 -> 0x0001` | target absent; pool bytes retained |
| Type `0x2B`, actual camera `(0,262)` | `1DCA -> 1DEE` | `0x012B -> 0x002B` | target absent; pool bytes retained |
| Type `0x01`, forced gate camera `(700,150)` | `1DCA`, no `1DEE` | remains `0x0101` | target present with callback `0x6DC4` |
| Type `0x2B`, forced gate camera `(500,100)` | `1DCA`, no `1DEE` | remains `0x012B` | target present with callback `0x47E7` |
| Type `0x28`, forced gate camera `(700,100)` | class-0 `1DCA`, no `1DEE` | remains `0x0128` | target present with callback `0x9269` |

The requested camera override is written after factory creation, but the game
can overwrite `DS:0x81C0` before the callback. The tracer therefore reasserts
the override at the `1DCA` breakpoint and records both values. This is a
measurement aid, not a claim that the game normally writes the camera there.

The first controlled W1L1 samples use prepared type mutations:

| Type | Fixture position | Scheduler callback | First observed change |
| ---: | ---: | ---: | --- |
| `0x01` | `(816,272)` | `0x6DC4` | clears `object+0x18` (`0x6DC4 -> 0`) |
| `0x2B` | `(768,224)` | `0x47E7` | clears `object+0x18` (`0x47E7 -> 0`) |

The original one-sample result was caused by the camera gate, not by a hidden
second pool: after the rejected callback, the object remains in the same pool
slot but is absent from the newly built scheduler bank. With an accepted gate,
the same slot is registered in the next bank and continues updating. The
shared-callback tracer also handles callbacks that are used by several objects
and verifies the selected `ES:DI` before accepting a callback entry.

## Region reactivation confirmed

The stream pass at `01F7:1CDA` is directional and circular. When the camera
advances in X it scans a newly exposed column, starting from the previous Y
stream tracker; its reference-grid stride is `DS:0x7968 = 136` bytes. The
reactivation probe sets that tracker to the target declaration's row and moves
the camera across the corresponding column, then stops at the exact
`01F7:1E04` call. This writes only debugger state; the declaration walker,
allocator, initializer, and scheduler table are the original game paths.

Two controlled W1L1 runs establish the complete return path:

| Type / source | Before re-entry | Re-entry result | Pool/scheduler result |
| --- | --- | --- | --- |
| `0x01` / `0x161A` | source `0x0001`, object `027F:0078` callback clear | `1E04` revisited; source `0x0001 -> 0x0101`; factory returns `027F:0078` | object callback restored; target appears in both banks at offset `0x0078` |
| `0x2B` / `0x1632` | source `0x002B`, object `027F:0078` callback clear | `1E04` revisited; source `0x002B -> 0x012B`; factory returns `027F:0078` | object callback `0x4727` restored; target appears in both banks at offset `0x0078` |

The source marker is therefore the durable reactivation latch: rejection
clears its high byte, and a later visit to the declaration sets it again before
allocation. The allocator then scans from the start and reuses the first free
record in this fixture, which is the same record that was just deactivated.
The immediate post-initialization pool snapshot contains the recreated object
in the current and next scheduler banks, so reactivation is not merely source
visibility—it restores active scheduling. The machine-readable probes are
`entity-01-w1l1-reactivate-camera10.json` and
`entity-2b-w1l1-reactivate-camera1.json` under the ignored object-behavior
build directory.

This closes the object lifecycle slice: declaration processing, pool
allocation/reuse, callback initialization, visibility deactivation, source
marker clearing, scheduler bank handoff, and region reactivation are now
static-plus-runtime established. The only variable not generalized by these
two samples is which other free slot the allocator chooses when the original
slot has already been reused by another object; the static first-free scan
defines that rule.

## Real-input lifetime pass: callback end versus visibility cull

The separate `object_movement_trace.py` driver reuses the proven entity
factory trace, injects a real `KBD_right` sequence only after initialization,
and captures synchronized object bytes every five guest frames. The older
`entity-01-real-right40.json` and `entity-2b-real-right40.json` probes did not
capture the source declaration marker, so their callback clears are useful
observations but cannot distinguish type-specific end logic from the camera
gate by themselves.

The W1L1 type `0x2B` run (`entity-2b-real-right40.json`) kept object
`027F:0078` at `(768,224)` with callback `0x47E7`, source `0x1632`, and slot
700 through capture index 7. At index 8 (about 40 guest frames), the callback
became zero while the position and source pointer were still unchanged. The
record was then reused: by index 11 its source pointer was `0xFFFF`, slot was
`0xFFFF`, and the transient callback `0x10B5` appeared intermittently in the
same pool slot. This is a callback/pool transition, not yet a proof of
self-termination.

The type `0x01` run (`entity-01-real-right40.json`) first settled from its
factory callback `0x6DA3` to its live callback `0x6DC4`, slot 281, and source
`0x161A`. Around capture index 19 the live callback cleared; the source pointer
and old slot bytes remained for several captures before the same slot was
reused by callback `0x10B5`. As with type `0x2B`, the missing source-marker
timeline leaves the exact cause open.

The source-aware movement driver now scans all 64 pool records and both
scheduler banks at every barrier, keyed by `object+0x1A == source_offset`.
Those traces provide the missing discriminator: when the marker high byte
changes from nonzero to zero at the same transition, the callback clear is the
camera visibility rejection. A type-specific self-termination claim requires
an accepted-camera trace with the marker remaining processed.

## Accepted-camera callback pass

The object-behavior tracer now arms only the scheduler phase selected by the
dispatch class and explicitly steps non-target entries to their class loop
heads (`0x0EBA`, `0x0EE4`, or `0x0F0D`). This avoids the debugger's current-IP
breakpoint suppression and makes the class-1 target at `01F7:0x0EEE`
repeatable. The callback sample also records whether `01F7:1DEE` or the
`01F7:8E4B` state-machine exits were encountered before the callback return.

Accepted-camera W1L1 fixture runs are recorded as
`accepted-28.json`, `accepted-29.json`, `accepted-2a.json`, `accepted-2b.json`,
`accepted-2c.json`, `accepted-33.json`, and `accepted-34.json` in the ignored
object-behavior build directory. All seven leave the source marker processed,
keep a nonzero callback, and therefore classify as `callback_survived` in the
one-sample window. The observed first callback transitions are:

| Type | Class / scheduler | Callback before → after | Source marker |
| ---: | --- | --- | --- |
| `0x28` | class 0 / `0x0EC7` | `0x9256 → 0x9269` | `0x0128 → 0x0128` |
| `0x29` | class 1 / `0x0EEE` | `0x4727 → 0x47E7` | `0x0129 → 0x0129` |
| `0x2A` | class 1 / `0x0EEE` | `0x4727 → 0x47E7` | `0x012A → 0x012A` |
| `0x2B` | class 1 / `0x0EEE` | `0x4727 → 0x47E7` | `0x012B → 0x012B` |
| `0x2C` | class 1 / `0x0EEE` | `0x8C4E → 0x8D20` | `0x012C → 0x012C` |
| `0x33` | class 1 / `0x0EEE` | `0x87D1 → 0x882F` | `0x0133 → 0x0133` |
| `0x34` | class 1 / `0x0EEE` | `0x9BEE → 0x9C0C` | `0x0134 → 0x0134` |

This pass establishes accepted-camera persistence and the callback installation
boundaries for the seven families. It does not yet prove a later self-ending
path: the first pass for `0x29`-`0x2B` is their initialization transition, and
the next steady callback belongs in a separate frame-synchronized probe.

## Real-input reactivation and allocator choice

The guest-timed movement probe holds the key through `dosbox.key` while the
capture coroutine advances, then returns across the same regions. This avoids
the host input-replay timing boundary and records the camera/stream state along
with the pool ledger.

The long W1L1 type `0x2B` traversal (`entity-2b-source-long-out-back-direct.json`)
shows the source marker `0x012B -> 0x002B` at the initial camera, followed by
`0x002B -> 0x012B` when the camera reaches the declaration region at about
`x=491`. The recreated object is in the original pool slot index 1 at offset
`0x0078`; both scheduler banks contain the new entry.

The type `0x01` traversal (`entity-01-source-out-back-direct.json`) reaches a
different allocator state. After the target marker changes from `0x0101` to
`0x0001`, pool slot index 1 is occupied by another source when the declaration
is revisited. The allocator chooses the first free slot after it, index 4 at
offset `0x01E0`; the source marker returns to `0x0101` and both scheduler banks
register that new slot. The source-aware timeline therefore shows a
visibility-cull, reactivation, and first-free allocation—not self-termination.

Together with the static allocator scan, these runs establish the normal
rule: reactivation is source-marker driven, and allocation is a first-free
pool scan. Reuse of the original slot is common but not required; if another
object occupies it first, the source moves to the next available slot. The
scheduler entries follow the selected pool offset, so they are rebuilt for the
new slot rather than retaining object identity by declaration alone.

## Lifecycle matrix and current coverage

`tools/object_lifecycle_matrix.py` turns the static and runtime observations
into one ignored JSON report. It parses only stores whose destination is
`%es:0x18(%di)` or `%es:0x1a(%di)` (loads are excluded), adds the anchored
factory/gate/scheduler sites above, and classifies movement traces as
visibility cull, reactivation, or callback end. Entity/resource traces are
kept as factory snapshots rather than being over-promoted to lifecycle proof.

For a local report using the current evidence set:

```sh
PYTHONPATH=research/tools python3 research/tools/object_lifecycle_matrix.py \
  --catalog research/entity-types.json \
  --disassembly research/build/quiky-exe-i8086.asm \
  --trace research/build/object-behavior/entity-01-source-out-back-direct.json \
  --trace research/build/object-behavior/entity-2b-source-long-out-back-direct.json \
  --trace research/build/object-behavior/entity-28-w1l1-v31.json \
  --output research/build/object-behavior/lifecycle-matrix.json
```

The machine-readable categories are deliberately conservative: an accepted
camera with a processed source marker and a callback that survives is
`persistent_in_window`; a callback that clears while that marker remains set
is `self_terminated_or_state_ended`; a marker high-byte clear is
`visibility_culled`, with a later high-byte restore classified as
`visibility_culled_then_reactivated`.
