# Parity architecture

Parity is organized around named recorded runs. Raw DOS and native traces are
capture evidence, not comparison formats.

Each run contains:

- `manifest.json`: `quiky.recorded-run-v3`, including the explicit `exact` or
  `lifecycle` comparison profile and content fingerprints;
- `input.jsonl`: the only native replay input;
- `expected-state.jsonl`: canonical imported DOS state;
- `actual-state.jsonl`: canonical native replay state;
- `parity.json` and `coverage.json`: generated verification results; these are
  deliberately excluded from the manifest's immutable input fingerprints.

Both state streams use `quiky.parity-state-v1`. Verification reads this schema
strictly and does not detect trace envelopes or field aliases. Historical
formats are decoded only by `quiky run import`, allowing captures to be
recaptured or re-imported without compatibility code in replay or comparison.

## Workflow

```sh
python3 research/tools/quiky.py run import research/runs/NAME \
  --name NAME --profile exact --expected-trace DOS.json

python3 research/tools/quiky.py run replay research/runs/NAME \
  --binary build/engine/quiky-parity-replay \
  --archive game/NESTLE.DAT --map W1L1.MAP

python3 research/tools/quiky.py run verify research/runs/NAME
```

Use the `lifecycle` profile for sparse captures. Its named checkpoints are
materialized during import, so the verifier never guesses comparison mode from
sample counts.

Missing fields are coverage gaps. Fields published by both sides but carrying
different values are parity mismatches. Missing rows or lifecycle checkpoints
are mismatches.

## Invariants

- JSONL sequences are positive and strictly increasing.
- State fields have one spelling and one shape.
- Empty, absent, and `null` are not interchangeable.
- Recorded-run inputs and state are content-addressed by the manifest.
- Raw capture evidence remains immutable.
- TSV replay manifests and direct trace-to-trace comparators are unsupported.
