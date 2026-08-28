# Parity architecture

Parity is organized around named recorded runs. Raw DOS and native traces are
capture evidence, not comparison formats.

Each run contains:

- `manifest.json`: `quiky.recorded-run-v5`, including the explicit `exact` or
  `lifecycle` comparison profile and content fingerprints;
- `input.jsonl`: the only native replay input;
- `expected-state.jsonl`: canonical imported DOS state;
- `actual-state.jsonl`: canonical native replay state.

Both state streams use `quiky.parity-state-v2`. Import accepts only the current
`quiky-player-dos-parity-v1` DOS capture schema. Verification reads canonical
state strictly and does not detect trace envelopes or field aliases. Older
evidence is archival and must be recaptured before it can become a run.

## Workflow

For an interactive capture, launch a visible game session and play normally:

```sh
python3 research/tools/quiky.py capture NAME --level W1L1
```

Close the game window to finalize `research/captures/NAME` and automatically
create `research/runs/NAME`. The raw `quiky.capture-session-v1` capture is
content-addressed and retained separately from the processed run. Callback
snapshots are appended to a length-framed CBOR stream while the window is
open; a crash leaves an `incomplete` session with a recoverable
`capture.partial.qcap` stream when at least one callback was observed. Use
`--capture-only` to skip automatic processing, then run:

```sh
python3 research/tools/quiky.py capture process research/captures/NAME \
  --run research/runs/NAME
```

The lower-level import workflow remains available for an existing strict DOS
capture:

```sh
python3 research/tools/quiky.py run import research/runs/NAME \
  --name NAME --profile exact --expected-trace DOS.json

python3 research/tools/quiky.py run replay research/runs/NAME \
  --archive game/NESTLE.DAT --map W1L1.MAP

python3 research/tools/quiky.py run verify research/runs/NAME
```

Use the `lifecycle` profile for sparse captures. Its named checkpoints are
materialized during import, so the verifier never guesses comparison mode from
sample counts.

Replay derives `QUIKYWn.BOB` from a `Wn*.MAP` name. Use `--player-bob` when a
world intentionally shares a differently named player resource.
The first replay stores this binary-independent recipe and archive digest in
the manifest; subsequent runs need only `quiky run replay RUN`.

Missing fields are coverage gaps. Fields published by both sides but carrying
different values are parity mismatches. Missing rows or lifecycle checkpoints
are mismatches. Verification is read-only and reports its summary to stdout.

## Invariants

- JSONL sequences are positive and strictly increasing.
- State fields have one spelling and one shape.
- Empty, absent, and `null` are not interchangeable.
- Recorded-run inputs and state are content-addressed by the manifest.
- Raw capture evidence remains immutable.
- TSV replay manifests and direct trace-to-trace comparators are unsupported.
