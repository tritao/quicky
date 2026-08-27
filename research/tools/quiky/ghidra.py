"""Small dispatch helpers for the existing Ghidra command files."""

from __future__ import annotations

from pathlib import Path
from typing import Iterable

from .common import ToolError, run_compat_script, tools_root


def run(script: str | Path, args: Iterable[str] = ()) -> int:
    """Run a committed Ghidra-side Python helper through the old boundary."""

    path = Path(script)
    if path.name.endswith(".py"):
        candidate = path
    else:
        candidate = tools_root() / f"{path.name}.py"
    if not candidate.is_file():
        raise ToolError(f"unknown Ghidra helper: {script}")
    return run_compat_script(candidate, args)
