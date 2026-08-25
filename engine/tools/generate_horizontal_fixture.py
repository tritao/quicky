#!/usr/bin/env python3
"""Generate the compact C++ fixture from the horizontal evidence ledgers.

The raw player traces are intentionally kept in research/evidence. This
generator copies only the complete pre-record and the horizontal post-state
values needed by the C++ updater tests, reducing the fixture to a small,
versioned artifact while retaining trace/sequence provenance.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


RELEVANT_FIELDS = (
    "action_word",
    "x_fixed",
    "velocity_x_fixed",
    "direction_byte",
    "motion_direction_byte",
)


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def build_fixture(evidence_dir: Path) -> dict:
    constants = read_json(evidence_dir / "constants.json")
    timelines = read_json(evidence_dir / "timelines.json")["scenarios"]
    scenarios = []
    checked_values = 0

    for trace_name in sorted(timelines):
        timeline = timelines[trace_name]
        rows = timeline["rows"]
        source = read_json(evidence_dir / trace_name)
        samples = source["events"][0]["samples"]
        if len(rows) != len(samples):
            raise ValueError(f"{trace_name}: timeline/raw sample count differs")

        formula_rows = []
        for row, sample in zip(rows, samples):
            expected = row.get("expected_motion")
            if expected is None:
                continue
            callback = sample["player_callback"]
            pre_raw = callback["pre_object"]["state_hex"]
            post = row["post"]
            formula_rows.append(
                {
                    "sequence": row["sequence"],
                    "frame": row["frame"],
                    "input_flags": row["pre"]["action_word"] & 0x3f,
                    "pre_record": pre_raw,
                    "expected_pre": {
                        "x_fixed": expected["pre_x_fixed"],
                        "velocity_x_fixed": expected["pre_velocity_x_fixed"],
                    },
                    "expected_post": {
                        "action_word": post["action_word"],
                        "x_fixed": expected["post_x_fixed"],
                        "velocity_x_fixed": expected["post_velocity_x_fixed"],
                        "direction_byte": post["animation_input_0"],
                        "motion_direction_byte": post["animation_input_1"],
                    },
                }
            )
            checked_values += 4

        if not formula_rows:
            continue
        scenarios.append(
            {
                "trace": trace_name,
                "provenance": {
                    "trace_schema": source.get("schema"),
                    "sequence_range": [
                        formula_rows[0]["sequence"],
                        formula_rows[-1]["sequence"],
                    ],
                },
                "initial_record": formula_rows[0]["pre_record"],
                "rows": formula_rows,
            }
        )

    return {
        "schema": "quiky-player-horizontal-fixture-v1",
        "source": {
            "constants_schema": constants["schema"],
            "executable_sha256": constants["source"]["executable_sha256"],
            "archive_sha256": constants["source"]["archive_sha256"],
            "callback": constants["source"]["callback"],
            "record_size": constants["source"]["record_size"],
        },
        "movement": constants["movement"],
        "checked_values": checked_values,
        "relevant_fields": list(RELEVANT_FIELDS),
        "scenarios": scenarios,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("evidence_dir", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    fixture = build_fixture(args.evidence_dir)
    if args.output.suffix == ".tsv":
        lines = [
            "# schema=quiky-player-horizontal-fixture-v1",
            f"# checked_values={fixture['checked_values']}",
            "# columns=record|trace|sequence|frame|update_input_flags|"
            "observed_action_flags|pre_record|expected_pre_x|expected_pre_v|expected_post_action|"
            "expected_post_x|expected_post_v|expected_direction|"
            "expected_motion_direction",
        ]
        for scenario in fixture["scenarios"]:
            provenance = scenario["provenance"]
            lines.append(
                "S\t{}\t{}\t{}\t{}\t{}".format(
                    scenario["trace"],
                    scenario["initial_record"],
                    provenance["trace_schema"],
                    provenance["sequence_range"][0],
                    provenance["sequence_range"][1],
                )
            )
            for row in scenario["rows"]:
                expected_pre = row["expected_pre"]
                expected_post = row["expected_post"]
                lines.append(
                    "\t".join(
                        str(value)
                        for value in (
                            "R",
                            scenario["trace"],
                            row["sequence"],
                            row["frame"],
                            0,
                            row["input_flags"],
                            row["pre_record"],
                            expected_pre["x_fixed"],
                            expected_pre["velocity_x_fixed"],
                            expected_post["action_word"],
                            expected_post["x_fixed"],
                            expected_post["velocity_x_fixed"],
                            expected_post["direction_byte"],
                            expected_post["motion_direction_byte"],
                        )
                    )
                )
        args.output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    else:
        args.output.write_text(
            json.dumps(fixture, separators=(",", ":"), sort_keys=True) + "\n",
            encoding="utf-8",
        )
    print(f"wrote {fixture['checked_values']} checked values to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
