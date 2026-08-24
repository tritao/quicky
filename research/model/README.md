# Object behavior model

This is the first C++ model for the object-behavior research track. It is
intentionally independent of the main engine recreation until the contracts
are compared frame-by-frame against DOSBox.

Implemented evidence-backed behavior:

- first-free allocation and callback-only deactivation for the 64-record pool;
- scheduler-bank rebuild and stable phase ordering;
- descriptor load/advance behavior from `01F7:5D38` and `01F7:5D60`, including
  reload timers, byte cursors, signed relative jumps, and the `0x32` mode
  adjustment;
- type-0x34 strict proximity bounds, action word `4`, effect code `0x2A`, and
  observed player displacement output `-0x1B000`.

The model keeps executable offsets in the research notes rather than pretending
that the recovered labels are original source symbols. Unresolved MAP branch
semantics and the complete player-effect consumer remain outside this slice.

Build/test:

~~~sh
cmake -S research/model -B research/build/object-behavior-model
cmake --build research/build/object-behavior-model -j16
ctest --test-dir research/build/object-behavior-model --output-on-failure
~~~
