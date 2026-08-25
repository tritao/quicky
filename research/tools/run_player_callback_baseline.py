#!/usr/bin/env python3
"""Rebuild and export the raw-segment Ghidra player-callback baseline.

The executable is split into its six NE segment images, imported at address
zero with the protected-mode 16-bit x86 language, and annotated from the
authoritative closure ledger.  Two independent projects are produced and
their decompilation, exact body inventory, and call-graph exports are
byte-compared.  The final audit also checks near calls in Ghidra against the
ledger and NE far calls against the executable's relocation tables.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "research/ghidra/player-callback-closure.json"
SEGMENT_TOOL = ROOT / "research/tools/ghidra_ne_segments.py"
GENERATOR = ROOT / "research/tools/generate_player_closure_ghidra.py"
GRAPH_EXPORTER = ROOT / "research/tools/export_player_callback_graph.py"
VERIFIER = ROOT / "research/tools/verify_player_callback_closure.py"
GENERATED_REFERENCE = ROOT / "research/ghidra/ApplyPlayerCallbackClosure.java"
TOOLS = ROOT / "research/tools"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run(command: list[str], cwd: Path = ROOT) -> None:
    print("+", " ".join(str(part) for part in command))
    subprocess.run(command, cwd=cwd, check=True)


def ghidra_default() -> Path:
    explicit = os.environ.get("GHIDRA_ANALYZE_HEADLESS")
    if explicit:
        return Path(explicit)
    home = os.environ.get("GHIDRA_HOME")
    if home:
        return Path(home) / "support" / "analyzeHeadless"
    discovered = shutil.which("analyzeHeadless")
    if discovered:
        return Path(discovered)
    packaged = Path("/home/joao/dev/ghidra-12.1.3/support/analyzeHeadless")
    return packaged if packaged.is_file() else Path("analyzeHeadless")


def check_input_hash() -> Path:
    payload = json.loads(MANIFEST.read_text(encoding="utf-8"))
    executable = ROOT / payload["source"]["executable"]
    if sha256(executable) != payload["source"]["sha256"]:
        raise RuntimeError(f"executable SHA-256 mismatch: {executable}")
    return executable


def load_ranges() -> list[str]:
    payload = json.loads(MANIFEST.read_text(encoding="utf-8"))
    ranges: list[str] = []
    for item in payload["functions"]:
        selector, offset = item["address"].split(":", 1)
        if selector.upper() != "01F7" or item.get("range") is None:
            continue
        start, end = item["range"]
        ranges.append(f"{offset.upper()}:{start.upper()}:{end.upper()}")
    return ranges


def compare_files(left: Path, right: Path) -> None:
    if left.read_bytes() != right.read_bytes():
        raise RuntimeError(f"baseline export drift: {left} != {right}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ghidra", type=Path, default=ghidra_default())
    parser.add_argument(
        "--output", type=Path,
        default=ROOT / "research/build/player-callback-baseline",
    )
    args = parser.parse_args()

    output = args.output.resolve()
    if output.exists() and any(output.iterdir()):
        raise SystemExit(f"refusing non-empty output directory: {output}")
    output.mkdir(parents=True, exist_ok=True)

    executable = check_input_hash()
    segments = output / "segments"
    segments.mkdir()
    run(["python3", str(SEGMENT_TOOL), "--pad-to-memory",
         str(executable), str(segments)])

    generated = output / "ApplyPlayerCallbackClosure.java"
    run(["python3", str(GENERATOR), "--output", str(generated)])
    # This is a repository-level drift check, not a second invocation against
    # the just-created temporary file.
    run(["python3", str(GENERATOR), "--output", str(GENERATED_REFERENCE), "--check"])
    compare_files(generated, GENERATED_REFERENCE)

    segment_paths = [segments / f"QUIKY_SEG0{i}.bin" for i in range(1, 7)]
    ranges = load_ranges()
    offsets = [item.split(":", 1)[0] for item in ranges]
    exports: list[tuple[Path, Path, Path, Path]] = []
    for label in ("a", "b"):
        project_root = output / f"project-{label}"
        project_root.mkdir(parents=True, exist_ok=True)
        project_name = "QuikySegments"
        run([
            str(args.ghidra), str(project_root), project_name,
            "-import", *[str(path) for path in segment_paths],
            "-processor", "x86:LE:16:Protected Mode",
            "-loader", "BinaryLoader", "-loader-baseAddr", "0", "-noanalysis",
            "-scriptPath", str(TOOLS),
            "-commit", "Player callback closure baseline import",
        ])

        script_path = f"{TOOLS};{generated.parent}"
        decomp_dir = output / f"decomp-{label}"
        graph = output / f"callgraph-{label}.json"
        ghidra_graph = output / f"ghidra-callgraph-{label}.json"
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG03.bin", "-noanalysis",
            "-postScript", generated.name,
            "-postScript", "AnnotatePlayerCollision.java",
            "-postScript", "DumpPlayerCallbackClosure.java", str(decomp_dir), *ranges,
            "-postScript", "ExportPlayerCallbackCallGraph.java", str(ghidra_graph), *offsets,
            "-scriptPath", script_path,
            "-commit", "Player callback closure annotation export",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG02.bin", "-noanalysis",
            "-postScript", generated.name,
            "-scriptPath", script_path,
            "-commit", "Player callback external contract annotation",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG06.bin", "-noanalysis",
            "-postScript", generated.name,
            "-postScript", "AnnotatePlayerCollision.java",
            "-scriptPath", script_path,
            "-commit", "Player callback DS data annotation",
        ])
        run(["python3", str(GRAPH_EXPORTER), "--output", str(graph)])
        exports.append((
            decomp_dir / "QUIKY_SEG03.bin.c",
            decomp_dir / "QUIKY_SEG03.bin.functions.json",
            graph,
            ghidra_graph,
        ))

    for index in range(1, 4):
        compare_files(exports[0][index], exports[1][index])
    run([
        "python3", str(VERIFIER), "--manifest", str(MANIFEST),
        "--executable", str(executable),
        "--segment", str(segments / "QUIKY_SEG03.bin"),
        "--decomp", str(exports[0][0]),
        "--callgraph", str(exports[0][2]),
        "--ghidra-callgraph", str(exports[0][3]),
    ])
    print("OK: independent Ghidra imports matched decompilation, body inventory, and both call-graph exports")
    print(f"OK: executable sha256 {sha256(executable)}")
    print(f"OK: output {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
