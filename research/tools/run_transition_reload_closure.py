#!/usr/bin/env python3
"""Rebuild and export the focused post-5010 reload/resource Ghidra closure."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
LEDGER = ROOT / "research/ghidra/transition-reload-closure.json"
SEGMENT_TOOL = ROOT / "research/tools/ghidra_ne_segments.py"
ANNOTATE = ROOT / "research/tools/AnnotateQuiky.java"
SCRIPT = ROOT / "research/tools/DumpReloadClosure.java"
TOOLS = ROOT / "research/tools"
SEGMENTS = [f"QUIKY_SEG0{i}.bin" for i in range(1, 7)]
TARGET_SEGMENTS = ["QUIKY_SEG01.bin", "QUIKY_SEG02.bin", "QUIKY_SEG03.bin",
                   "QUIKY_SEG04.bin", "QUIKY_SEG05.bin"]


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


def compare_exports(left: Path, right: Path) -> None:
    if left.read_bytes() != right.read_bytes():
        raise RuntimeError(f"reload closure export drift: {left} != {right}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ghidra", type=Path, default=ghidra_default())
    parser.add_argument("--output", type=Path,
                        default=ROOT / "research/build/transition-reload-closure")
    args = parser.parse_args()

    manifest = json.loads(LEDGER.read_text(encoding="utf-8"))
    executable = ROOT / manifest["source"]["executable"]
    if sha256(executable) != manifest["source"]["executable_sha256"]:
        raise RuntimeError(f"executable SHA-256 mismatch: {executable}")

    output = args.output.resolve()
    if output.exists() and any(output.iterdir()):
        raise SystemExit(f"refusing non-empty output directory: {output}")
    output.mkdir(parents=True, exist_ok=True)
    segments = output / "segments"
    run(["python3", str(SEGMENT_TOOL), "--pad-to-memory",
         str(executable), str(segments)])
    segment_paths = [segments / name for name in SEGMENTS]

    exports: dict[str, list[Path]] = {name: [] for name in TARGET_SEGMENTS}
    for label in ("a", "b"):
        project_root = output / f"project-{label}"
        project_root.mkdir(parents=True, exist_ok=True)
        project_name = "QuikyTransitionReload"
        run([
            str(args.ghidra), str(project_root), project_name,
            "-import", *[str(path) for path in segment_paths],
            "-processor", "x86:LE:16:Protected Mode",
            "-loader", "BinaryLoader", "-loader-baseAddr", "0", "-noanalysis",
            "-scriptPath", str(TOOLS),
            "-commit", "Focused transition reload closure import",
        ])
        script_path = f"{TOOLS};{output}"
        decomp_dir = output / f"decomp-{label}"
        for segment_name in TARGET_SEGMENTS:
            run([
                str(args.ghidra), str(project_root), project_name,
                "-process", segment_name, "-noanalysis",
                "-postScript", ANNOTATE.name,
                "-postScript", SCRIPT.name, str(decomp_dir),
                "-scriptPath", script_path,
                "-commit", "Focused transition reload closure decompilation",
            ])
            exports[segment_name].append(
                decomp_dir / f"{segment_name}.c")

    for segment_name, paths in exports.items():
        compare_exports(paths[0], paths[1])
        print(f"OK: {segment_name} independent Ghidra export "
              f"{sha256(paths[0])}")
    print(f"OK: executable sha256 {sha256(executable)}")
    print(f"OK: output {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
