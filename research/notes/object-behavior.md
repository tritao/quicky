# Object behavior research

The stable semantic names for this track are maintained in the
[object-behavior glossary](object-behavior-glossary.md). They are deliberately
behavioral labels with confidence levels; segment-relative addresses remain
the source-of-truth identifiers until an original symbol is recovered.

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
boundaries for the seven families. The first pass for `0x29`-`0x2B` is their
initialization transition, so a second callback is needed before making any
claim about persistence or self-termination.

### Frame-synchronized steady callback pass

The tracer now follows the accepted callback into the next scheduler frame. It
stops the first sample at the class post-callback site, then watches the next
`01F7:0x0E96` frame boundary and directly hooks the selected object's live
callback. Shared callback entries are stepped over until the breakpoint has
the target `ES:DI`; the callback's actual near return address is read from the
stack rather than assumed from the scheduler class. The camera override is
reasserted once at `01F7:0x1DCA` for each steady callback, preserving the
accepted-camera condition without repeatedly stopping inside the gate.

Eight-sample runs (`steady-accepted-*-s8.json` in the ignored build directory)
cover the same seven W1L1 fixtures:

| Types | First callback transition | Steady callback after sample 1 | Source marker | Exit sites |
| --- | --- | --- | --- | --- |
| `0x28` | `0x9256 → 0x9269` | `0x9269` | remains `0x0128` | none |
| `0x29`-`0x2B` | `0x4727 → 0x47E7` | `0x47E7` | remains `0x0129`-`0x012B` | none |
| `0x2C` | `0x8C4E → 0x8D20` | `0x8D20` | remains `0x012C` | none |
| `0x33` | `0x87D1 → 0x882F` | `0x882F` | remains `0x0133` | none |
| `0x34` | `0x9BEE → 0x9C0C` | `0x9C0C` | remains `0x0134` | none |

Every sample classifies as `callback_survived`; the only related site is the
one-time camera check at `0x1DCA`. No sample reaches `0x1DEE` or any of the
known state-machine exits, and no source marker loses its processed high byte.
For type `0x2B`, the live callback advances the object downward by roughly one
pixel per sample while retaining callback `0x47E7`. This rules out an immediate
one-shot callback for these accepted families over the captured eight-frame
window. It does not replace the separate out-of-window traversal needed to
study eventual camera culling, object death, or reactivation.

### Long-window boundary result

The tracer supports `--no-pool-snapshots` for long runs. This keeps the object,
source, position, callback, and termination evidence while omitting the costly
64-entry pool tables. With the normal accepted camera `(500,100)`, type `0x2B`
survives 192 samples and reaches `y=360` with callback `0x47E7` and source
marker `0x012B`; type `0x28` survives 128 samples at a fixed position with
callback `0x9269` and marker `0x0128`.

The remaining accepted families complete the same 128-sample lightweight
window: `0x29` and `0x2A` retain `0x47E7`, `0x2C` retains `0x8D20`, `0x33`
retains `0x882F`, and `0x34` retains `0x9C0C`. Their source markers remain
`0x0129`, `0x012A`, `0x012C`, `0x0133`, and `0x0134`, respectively. The full
accepted-camera matrix therefore classifies all seven tested families as
`persistent_in_window`; the only related site in those runs is the camera
check at `0x1DCA`.

A controlled boundary run moves the camera window upward without moving the
object horizontally: type `0x2B` starts accepted at camera `(500,0)`, then
reaches `y=305` on sample 74. That callback hits `01F7:0x1DCA` followed by
`01F7:0x1DEE`, clears `object+0x18` (`0x47E7 → 0`), and changes the source
marker `0x012B → 0x002B`. The tracer stops after this terminal sample and marks
the trace `terminated=true`. This is the complete runtime signature of camera
culling, distinct from a callback-specific end: the same callback survives
192 accepted-camera samples when the object remains inside the window.

## Static callback slices and real-input result

The raw segment-3 Ghidra pass now includes the custom callback families and
their initializer-to-steady transitions. NE relocations resolve the important
internal calls even though the raw-segment decompiler cannot connect all far
call targets automatically:

| Type | Initializer writes | Steady callback structure |
| ---: | --- | --- |
| `0x2C` | `+0x18=0x8D20`, sprite slot `0x02C6`, position `+3,+7`, `+0x2C=5` | `0x1DCA` visibility check, optional `0x1DEE`, then local `0x8D31`; that helper calls `0x393C` for bounds and contains a terminal callback-clear path at `0x8E42` |
| `0x33` | `+0x18=0x882F`, Y `+0x20`, velocity `-0x5000`, direction/phase bytes, timer `+0x2D=0x14` | `0x1DCA`/`0x1DEE`, then `0x1B77`, `0x1C4D`, and two `0x5C27` MAP-descriptor probes; runtime `+0x2E` settles at `0x0100` |
| `0x34` | `+0x18=0x9C0C`, X `+0x10`, Y `+0x20`, then `0x5D38` | tests `DS:0x85DA < 0x32`, uses `0x1DCA`/`0x1DEE`, `0x5D60`, and local `0x9C29`; accepted path writes action word `DS:0x612E=4` through the helper chain |

The static slices show that these are autonomous object behaviors, not player
callback branches. Their steady routines do not directly load the normalized
keyboard words `DS:0x8196`/`DS:0x88BC`; type `0x33` and `0x34` do consult MAP or
state helpers indirectly, so this is a bounded negative result rather than a
claim that every downstream helper is input-independent.

The bounded negative result is also confirmed dynamically. The callback tracer
held guest key `KBD_right` while collecting 48 real callbacks for each of the
three fixtures at accepted camera `(500,100)`:

| Type | Input trace | Callback sequence | Position/state versus no-input trace | Lifecycle result |
| ---: | --- | --- | --- | --- |
| `0x2C` | `input-callback-2c-right48.json` | `0x8C4E → 0x8D20`, then `0x8D20` | identical; state `0/0` | 48 callbacks survived |
| `0x33` | `input-callback-33-right48.json` | `0x87D1 → 0x882F`, then `0x882F` | identical autonomous motion; state settles `0xFF00 → 0x0100` | 48 callbacks survived |
| `0x34` | `input-callback-34-right48.json` | `0x9BEE → 0x9C0C`, then `0x9C0C` | identical; state `0/0` | 48 callbacks survived |

Every input trace retained its processed source marker and hit only the
accepted `0x1DCA` check; none reached `0x1DEE`, a known state-machine exit, or
a callback-specific clear. This motivates the helper-level pass below: the
remaining work is to correlate its register and MAP results with object state,
rather than repeat right-input runs against these same autonomous objects.

## Helper-level callback pass

The object tracer now accepts `--helper-trace`. For the selected callback it
arms breakpoints on the far helpers `0x0FCF` (segment `0x01E7`), `0x1B5D`,
`0x1B77`, `0x1C4D`, `0x1C6E`, `0x393C`, `0x39FE`, `0x5C27`, `0x5D38`, and
`0x5D60`, reads the far return address from `SS:SP`, and records the registers
at helper entry and return. The return breakpoint is matched by far segment
and offset, so nested helper calls do not get attributed to the wrong callback.
This also required avoiding a re-arm at the currently stopped helper IP;
otherwise the debugger repeatedly stopped on the same instruction and never
reached the callback return.

The corrected 48-sample accepted-camera traces are
`helper-2c-s48-mapprobe.json`, `helper-33-s48-mapprobe.json`, and
`helper-34-s48-mapprobe.json` in the ignored object-behavior build directory.
The first callback is the initializer; later samples are the steady callback:

| Type | Initializer helper | Steady helper order | Representative low-16-bit results |
| ---: | --- | --- | --- |
| `0x2C` | none observed (`0x8C4E`) | `0x393C` | entry `AX=0x0103, BX=0x0200, CX=0x0BC8, DX=0x0E3F`; return `AX=0x0076, BX=0x01B8, CX=0x008A, DX=0x0190` |
| `0x33` | `0x5D38` | `0x1B77 → 0x393C → 0x1C4D → 0x1C6E → 0x5C27 → 0x5D60` | `0x5D38` returns `AX=0x00D6`; `0x1C4D/0x1C6E` expose MAP tile words; `0x5C27` reads descriptor flags; `0x5D60` preserves `AX=0x0400` while its `BX` input changes with object state |
| `0x34` | `0x5D38` | `0x5D60 → 0x39FE` (then conditional `0x5D38 → 0x1B5D → 0x5D38 → 0x0FCF`) | `0x39FE` returns player X/Y and `CL`; the conditional chain writes action `DS:0x612E=4` |

These values are register evidence, not final helper signatures: the 16-bit
code often leaves upper register halves and transient descriptor state intact.
The stable call order is stronger than any single return value. It confirms
that type `0x33` combines a bounds/helper chain with a MAP-state helper, while
type `0x34` consumes a MAP-state result through its local proximity test.

The corrected type `0x33` run reaches `0x5C27` once on every steady callback
(47 calls). The helper receives `AX=0x0100` and an X-like `BX` that moves from
`0x0319` down to `0x02DA` as the object moves left; it preserves those
registers on return. The MAP selector is `0x0377` with row stride `0x021C`.
The descriptor selector is a separate `0x0277`; reading the descriptor word
through `DS` produced the misleading old value `0x6544`. The executable uses
the selector stored at `DS:0x6584`, and the corrected probe reads the live
descriptor through that selector. The directional `5C27` call returns through
`0x888A` or `0x8876` and its zero flag, not carry, controls the transition
branch.

## Controlled MAP and proximity probes

The tracer now supports debugger-only `--probe-position-x/--probe-position-y`
overrides, records the bounds/player object at `DS:0x881A`, and can override
`DS:0x85DA` plus the bounds byte `+0x37`. These writes are applied immediately
before each selected callback and are not game or executable changes.

The type `0x33` X sweep at `Y=256` confirms that the callback consumes raw MAP
words rather than an opaque boolean. The final steady samples were:

| Target X | `0x5C27` tile / flags | `0x1C4D → 0x1C6E` tile word |
| ---: | --- | ---: |
| `700` | `0x32 / 0` | `0x1E` |
| `730` | `0x33 / 0` | `0x1E` |
| `760` | `0x33 / 0` | `0x33` |
| `790` | `0x33 / 0` | `0x33` |
| `820` | `0x33 / 0` | `0x1F` |
| `850` | `0x33 / 0` | `0x92` |

The `0x1C6E` disassembly is now resolved: it computes a 16-pixel MAP address
using `DS:657A/657C/657E`, returns the raw MAP word in `AX`, and tests bit
`0x4000`. The `0x1C4D` wrapper forms the probe coordinate from the object
position and signed direction byte `+0x29`, then calls `0x1C6E`; its carry
return is the tested `0x4000` result. The separate directional `0x5C27` calls
use the low-9-bit tile ID and the descriptor selector at `DS:6584`, then test
the low-nibble flag selected by the coordinate bit-3 pattern. The two helper
results are therefore independent MAP inputs, not one combined boolean.

At forced position `(738,400)` with direction `+1`, the boundary probe is
`(763,400)`, MAP tile `0x2E`, descriptor word `0x000C`, and descriptor flags
`0xC`; `5C27` returns flags `0x3202` (ZF clear), so `+0x2F` remains zero. At
the original `(738,256)` probe, the boundary tile is `0x33` with descriptor
word zero and `5C27` returns `0x3246` (ZF set), so `+0x2F` becomes one. This
pair isolates the state machine from the MAP pre-state.

The type `0x34` proximity matrix used player X/Y `(128,400)`, `CL=1` via
bounds byte `+0x37`, and `DS:0x85DA=49`. It confirms strict inequalities:
`104 <= object.x <= 152` and `401 <= object.y <= 407` enter the action branch;
`x=103/153` and `y=400/408` do not. A hit changes `DS:0x612E` from `0` to
`4` and reaches `0x5D38 → 0x1B5D → 0x5D38 → 01E7:0x0FCF`. `0x39FE` is now
resolved statically: when `DS:0x89EA==0`, it loads bounds-object X from `+4`,
Y from `+8`, and byte `+0x37` into `CL`; otherwise it returns zero X/Y.
The `0x0FCF` helper sets `DS:0x504C=0x2A` and conditionally forwards the
current action word to the shared effect routine.

The type `0x2C` bounds sweep at X `600`, `700`, `800`, and `900` with Y `200`
returned the same `0x393C` result each time:
`AX=0x0076`, `BX=0x01B8`, `CX=0x008A`, `DX=0x0190`. Static decoding explains
this: `0x393C` ignores the probed type-`0x2C` position and returns the four
bounds derived from the object at `DS:0x881A` (`x/+0x2C`, `y/+0x2E`,
`x/+0x30`, `y/+0x32`), or zeroes when `DS:0x89EA` is nonzero.

These probes close the helper-input and branch conditions needed for the
first C++ behavior models. The next comparison target is callback state and
action output frame-by-frame against DOSBox.

## Targeted static pass: type-0x33 state machine and type-0x34 gate

The remaining broad decompilation pass was narrowed to the two callback
families whose high-level branch meanings were still open. Raw segment
disassembly is more reliable here than the raw-segment C output because the
far-call relocations are represented as placeholder calls by Ghidra.

The type-`0x33` callback at `01F7:882F` has four explicit motion substates in
`object+0x32`, plus a separate transition flag at `object+0x2F`:

| State / field | Static behavior |
| --- | --- |
| entry | Runs the camera gate; an out-of-window object goes through `0x1DEE`. The accepted path performs the `0x1B77 → 0x393C → 0x1C4D → 0x1C6E` pre-check, then performs a separate directional `0x5C27` descriptor probe. |
| `+0x32 == 0` | Applies signed acceleration to `+0x0A` (`+/-0x400`), integrates it into `+0x02`, clamps velocity to `+/-0x6000`, and decrements `+0x2D`. The sign flip at expiry reverses direction/phase and reloads `+0x2D=0x14`. |
| `+0x32 == 1` | Applies a smaller signed velocity step (`+/-0x100`) toward a zero-crossing. When the direction-specific crossing occurs, velocity is zeroed, state becomes `2`, descriptor `DS:3510` is loaded, and the callback continues through the common tail. |
| `+0x32 == 2` | Holds for `0x2E` ticks using `+0x33`, then changes to state `3`, resets `+0x33`, and reloads descriptor `DS:3504`. |
| `+0x32 == 3` | Applies signed `+0x200` acceleration, clamps to `+/-0x5000`, and returns to state `0` at the direction-specific limit. |
| `+0x2A/+0x35` | The state-0 travel counter advances animation timing: `+0x35` resets at `0x51`, while `+0x2A` advances until `0x32`, then consumes the byte ring at `DS:646C` and enters state `1`. |

The two previously observed return sites are now placed exactly in this
control flow. The left/right boundary probes call `0x5C27` at `0x8871` and
`0x8885`; their conditional return sites are `0x8876` and `0x888A`, and both
use `JZ` to feed the common `+0x2F=1` transition path at `0x888E`. Thus the
sites are directional MAP/descriptor decisions, not separate callbacks or
scheduler exits. `01F7:1C4D` itself preserves the incoming Y probe in `CX`,
selects the X offset sign from signed `+0x29`, adds it to object X, adds the Y
offset, and forwards the result to `0x1C6E`; `1C6E` tests raw MAP bit `0x4000`
and `1C4D` converts that result to carry.

The debugger-only state overrides and the forced `(738,400)` run validate the
motion branches independently of that MAP result: state `0` with transition
zero integrates the current velocity and advances `+0x35/+0x2A`; transition
one applies signed `+/-0x400` acceleration and expires `+0x2D` at the signed
zero crossing; state `1` decelerates by `+/-0x100` and loads `DS:3510` on the
zero crossing; state `2` counts through `0x2D`, then falls through state `3`
with `+/-0x200` acceleration and reloads `DS:3504`; state `3` returns to
state `0` at `+/-0x5000`. These observations are now implemented in
`step_type33_motion` and checked by the C++ model tests and helper-enabled
Python comparator.

### Common target-list tail

The type-`0x33` tail at `01F7:8AE5` is now resolved. When `DS:8806` (the
active producer count) is nonzero it uses object `+0x30` as a cursor, wraps it
to zero when it reaches `DS:8808` (the list capacity), and reads a signed
`(x,y)` pair from `DS:87DE + cursor*4`. A target is consumed by clearing only
its X word when it lies strictly inside
`object.x-10 < target.x < object.x+10` and
`object.y-0x23 < target.y < object.y+5`; the Y word survives. The cursor is
incremented on every active pass, including a pass that does not match.

Debugger-only target probes confirm both sides: `(object=(738,400),
target=(740,380))` changes the target from `(740,380)` to `(0,380)` and
advances the cursor, while `(760,380)` remains unchanged. This behavior is
implemented as `step_type33_target_tail` and checked by the frame comparator.

The static producer side is also identified. `01F7:44FF` resets the shared
list with capacity `4`, clears the active-producer count at `DS:8806`, and
zeroes ten `(x,y)` slots. `01F7:4519` admits a new emitter only while
`DS:8806 < DS:8808`, increments the active count, reserves the first slot
whose X+Y pair is zero, and installs callback `01F7:45AB`. The `45AB` callback
publishes its current object position into that slot every update; if the
slot has been cleared or the object leaves its camera window it switches to
`01F7:470C`. `470C` decrements the active count, clears both slot words, and
clears the emitter callback. This explains why the `8AE5` consumer clears only
X: the producer notices the cleared slot on its next update and performs the
full release path.

A debugger-only player probe confirms the producer lifecycle in DOSBox. At
the post-update call site `01F7:38EC`, forcing the player action bit `0x10`,
clearing byte `+0x3C`, and setting `DS:88AE=1` leads to `01F7:45AB`; the first
sample has slot offset `+0x2A=0`, active count `1`, capacity `4`, and target
`(1,0)`. After the callback returns the target is `(132,383)`, and subsequent
`45AB` samples continue publishing `(144,381)`, `(160,381)`, and so on. This
verifies the admission, slot selection, and publication edges. A second
debugger-only probe then clears slot zero immediately after `45AB` publishes
`(132,383)`. The producer returns with callback `01F7:470C`; after that release
callback, the object callback is zero, `DS:8806` is zero, and both words of
slot zero are zero. This confirms the complete publish -> consumer-clear ->
release lifecycle dynamically, including the active-count teardown.

The list is shared across more than this one callback. Static scanning finds
17 inline consumers. They all gate on `DS:8806`, advance a per-object cursor,
and clear only target X, but their windows and post-hit actions differ. The
known variants are:

| Tail | Cursor | Strict target window | Post-hit action |
| --- | --- | --- | --- |
| `5399` | `+0x30` | X `-20..+20`, Y `-25..+5` | callback `4AB3` |
| `58A7`, `62AE`, `67E0`, `6D01`, `83AF` | `+0x30` | X `-15..+15`, Y `-25..+5` | `6D01/83AF`: callback `4AB3`; `58A7/62AE`: advance `+0x42` path; `67E0`: clear only |
| `707B` | `+0x30` | X `-25..+25`, Y `-15..+5` | callback `4AB3` |
| `76BF` | `+0x30` | X `-15..+15`, Y `-35..+5` | callback `4AB3`; uses `<=` rather than `<` for the cursor bound |
| `7A85` | `+0x30` | X `-10..+10`, Y `-35..+5` | callback `4BA0` |
| `7E1A`, `8773` | `+0x30` | X `-17..+18`, Y `-20..+5` | callback `4BA0`/`4AB3` |
| `8AE5` | `+0x30` | X `-10..+10`, Y `-35..+5` | clear only |
| `B266`, `BB17`, `C331`, `CDAC`, `D563` | `+0x2A` | X `-15..+15`, Y `-25..+5` | advance `+0x2C`, call `4B70`, set phase byte `+0x17=2` |

Static dispatch tracing now maps the normal entity families to these tails.
The association is made from each family initializer's installed callback and
the following callback body, ending at the next inline target scan; it does
not rely on the type name alone:

| ARE types | Installed update callback | Shared target tail | Family label |
| --- | --- | --- | --- |
| `0x01/0x02` | `6DC4` | `707B` | WURM2 |
| `0x03/0x04` | `68C0` | `6D01` | BIENE |
| `0x05/0x06` | `7B71` | `7E1A` | FISCH |
| `0x07/0x08` | `778C` | `7A85` | KRABBE |
| `0x09/0x0A` | `715E` | `76BF` | PENGO |
| `0x0B/0x0C` | `66E1` | `67E0` | SCHNEE |
| `0x15/0x16` | `7EF8` | `83AF` | FLIEGE |
| `0x17/0x18` | `8472` | `8773` | SPINNE |
| `0x19/0x1A` | `5071` | `5399` | BUGGY |
| `0x1B/0x1C` | `5F28` | `62AE` | UFO |
| `0x33` | `882F` | `8AE5` | snow-family object |
| `0x35/0x36` | `546D` | `58A7` | later normal family |

The five high-address tails (`B266`, `BB17`, `C331`, `CDAC`, and `D563`) are
not promoted to entity names yet. They have the same target window but a
distinct `+0x2A` cursor and `4B70` phase-transition action, so they should be
traced as a separate family of stateful effect handlers rather than forced
into the normal dispatch table.

### Dynamic family pass

The object tracer now has a generic target-list probe. It writes one signed
target relative to the selected object's current pixel position, sets
`DS:8806=1`/`DS:8808=1`, and resets a caller-selected cursor field. The default
cursor is `object+0x30`; `--probe-target-cursor-offset 0x2A` is available for
the high-address handlers. This is deliberately independent of the existing
type-`0x33` absolute-coordinate probe.

The first controlled target was placed at `object.x, object.y-10`. Because the
normal initializers move their objects before entering the steady callback,
the target is consumed on the following callback. Camera-centered runs then
gave these direct checks:

| Representative | Steady callback | Static tail | Target result | Post-hit result |
| --- | --- | --- | --- | --- |
| type `0x01` | `6DC4` | `707B` | X cleared, Y retained, cursor advanced | callback `4AB3` |
| type `0x03` | `68C0` | `6D01` | X cleared, Y retained, cursor advanced | callback `4AB3` |
| type `0x09` | `715E` | `76BF` | X cleared, Y retained, cursor advanced | callback `4AB3` |
| type `0x0B` | `66E1` | `67E0` | X cleared, Y retained, cursor advanced | callback survives; clear-only tail |
| type `0x19` | `5071` | `5399` | X cleared, Y retained, cursor advanced | callback `4AB3` |
| type `0x1B` | `5F28` | `62AE` | X cleared, Y retained, cursor advanced | `+0x42` action chain; callback remains until its next state step |
| types `0x35/0x36` | `546D` | `58A7` | X cleared, Y retained, cursor advanced | `+0x42` action chain, then callback `4AB3` |

The type-`0x35` and type-`0x36` traces are especially useful because they
reach the shared `4B70`/effect helper path: the target hit records helper
`01E7:0x0FCF`, changes the callback's state/counter fields, and the next
steady step installs `4AB3`. This confirms the static `58A7` classification
and shows that the post-hit action is not merely a sprite-slot change.

The type-`0x03`, `0x0B`, `0x35`, and `0x36` fixtures use redirected stream
records; camera-centered coordinates must be taken from the live object after
streaming, not from the catalog placement. Runs centered on the redirected
runtime object avoid falsely classifying a tail as a visibility deactivation.

The five high tails remain a separate, statically characterized family. Each
is reached by a dedicated steady callback and has the same contract:

| Tail | Pre-tail steady callback | Initializer | Distinct state |
| --- | --- | --- | --- |
| `B266` | `B25D` | `B20B` | `+0x2A` target cursor, `+0x2C` hit counter |
| `BB17` | `BB0E` | `BABC` | `+0x2A` target cursor, `+0x2C` hit counter |
| `C331` | `C328` | `C30D` | `+0x2A` target cursor, `+0x2C` hit counter |
| `CDAC` | `CDA3` | `CD88` | `+0x2A` target cursor, `+0x2C` hit counter |
| `D563` | `D55A` | `D53F` | `+0x2A` target cursor, `+0x2C` hit counter |

On a match, each clears target X, increments `+0x2C`, calls `4B70`, sets
`object+0x17=2`, and continues through the family-specific effect update.
No gameplay names are assigned until one of these handlers is reached in a
camera-centered live spawn; the static contract is sufficient to model the
shared target handoff without conflating it with normal ARE sprites.

The player-side routine at `01F7:69FF` reaches the `6D01` tail after its
player-position gate; the target scan itself begins at `6D01`. This distinction
matters for breakpoints and callback reconstruction. The shared list is a
cross-family collision/effect handoff, not a type-`0x33` private queue. The
remaining work is to dynamically verify the family-specific post-hit actions,
especially the `+0x42` paths and the five high-address effect handlers.

The type-`0x34` gate is now exact. `01F7:9C0C` begins with
`CMP byte DS:85DA,0x32` followed by `JGE return`; only values `0x00..0x31`
can reach the visibility check, descriptor advance, and local `0x9C29`
proximity test. A controlled accepted-camera pair confirms the boundary:
with the object forced to `(128,404)`, player bounds `(128,400)`, and
collision class `1`, `DS:85DA=0x31` changes `DS:612E` from `0` to `4` and
reaches the `0x5D38 -> 0x1B5D -> 0x5D38 -> 01E7:0x0FCF` chain; `0x32` reaches
none of those helpers and leaves `DS:612E=0`. The C++ model and test now use
this strict-less-than gate.

## Descriptor state-machine pass

The tracer now records the descriptor fields and the first eight words at the
sequence base in the live `DS` segment. This turns the previous provisional
names into an exact state contract:

| Object field | Semantic name | Exact behavior |
| --- | --- | --- |
| `+0x1E` | `descriptor_reload_delay` | Reload value copied from `[DS:SI]` by `5D38`. |
| `+0x20` | `descriptor_timer` | Decremented once per `5D60` call; reloaded from `+0x1E` after an entry resolves. |
| `+0x22` | `descriptor_sequence_base` | Set to `DS:SI+2` by `5D38`; remains fixed during the sequence. |
| `+0x24` | `descriptor_sequence_cursor` | Advances by two bytes at expiry and follows signed relative jump words. |
| `+0x28` | `descriptor_mode` | When byte `0xFF`, adds `0x32` to the resolved action value. |

`5D38` therefore performs:

```text
reload_delay = word[DS:SI]
timer        = reload_delay
sequence_base = DS:SI + 2
sequence_cursor = sequence_base
action = sequence_word(sequence_cursor)
         + (mode == 0xff ? 0x32 : 0)
```

`5D60` returns after only decrementing `timer` when it is nonzero. At zero it
increments `sequence_cursor` by one word, reads a signed sequence value, and
repeats `cursor += signed_value * 2` while that value is negative. It then
publishes the nonnegative value as the action and reloads `timer`.

The live W1L1 sequence windows are:

| Family | `DS:SI` | Reload | Sequence words at `+0x22` | Jump result |
| --- | ---: | ---: | --- | --- |
| Type `0x33` | `0x3504` | `10` | `0x00D6, 0x00D7, 0x00D8, 0x00D9, 0xFFFC` | From `0x350E`, `-4` returns the cursor to `0x3506`. |
| Type `0x34` | `0x3568` | `6` | `0x0190, 0x0192, 0x0193, 0x0191, 0xFFFF` | From `0x3572`, `-1` returns the cursor to `0x3570`. |

The debugger-only timer/mode probe forces expiry on every callback. With
`mode=0xFF`, type `0x33` publishes `0x0108..0x010B` before looping, and type
`0x34` publishes `0x01C2, 0x01C4, 0x01C5, 0x01C3` before holding on the final
entry. These values match the executable's `+0x32` adjustment exactly.

This closes the descriptor mutation needed by the first model. MAP behavior
remains a separate input to type `0x33`: the earlier X sweep established that
`5C27`/`1C6E` choose the higher-level branch from raw MAP and descriptor data;
the descriptor timer itself does not read MAP.

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
