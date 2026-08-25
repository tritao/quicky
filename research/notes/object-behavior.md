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
initializer can populate `object+0x18` with a different callback.

The first controlled W1L1 samples use prepared type mutations:

| Type | Fixture position | Scheduler callback | First observed change |
| ---: | ---: | ---: | --- |
| `0x01` | `(816,272)` | `0x6DC4` | clears `object+0x18` (`0x6DC4 -> 0`) |
| `0x2B` | `(768,224)` | `0x47E7` | clears `object+0x18` (`0x47E7 -> 0`) |

These are first-callback observations, not yet complete behavior names. The
objects stop producing additional samples after this transition, so the next
pass must capture the callback's internal calls and determine whether the
clear is a visibility/lifetime gate, a handoff to another pool, or normal
one-shot behavior. Camera overrides are supported for that controlled pass.
