"""Thin unified frontend for the compatibility-preserving research tools."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from .common import ToolError, run_compat_script, tools_root


ALIASES = {
    "player": "player_parity_compare.py",
    "session": "w1l1_session_compare.py",
    "w1l1": "w1l1_session_compare.py",
    "w1l1-session": "w1l1_session_compare.py",
    "frame": "scene_frame_compare.py",
    "trace": "quikytrace.py",
    "ghidra-ne-segments": "ghidra_ne_segments.py",
}


def _top_parser() -> argparse.ArgumentParser:
    return argparse.ArgumentParser(
        prog="quiky",
        description=(
            "Thin dispatcher for Quiky research tooling. Legacy command files "
            "remain valid while shared APIs migrate behind them."
        ),
        epilog=(
            "Commands: parity player|session, frame, trace, verify SCRIPT, "
            "ghidra SCRIPT.\n"
            "Examples: quiky parity player --original DOS.json --candidate native.json; "
            "quiky frame native.bmp dos.png --strict; quiky trace --help"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )


def _parity_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="quiky parity",
        description="Dispatch a normalized player or W1L1 session comparison.",
    )
    parser.add_argument("kind", choices=("player", "session", "w1l1", "w1l1-session"))
    parser.add_argument("args", nargs=argparse.REMAINDER,
                        help="arguments accepted by the historical comparator")
    return parser


def _dispatch_named(group: str, args: list[str]) -> int:
    if not args or args[0] in ("-h", "--help"):
        if group == "verify":
            print("usage: quiky verify SCRIPT [SCRIPT_ARGS ...]")
            print("       SCRIPT is a legacy research/tools/*.py verifier")
        else:
            print("usage: quiky ghidra SCRIPT [SCRIPT_ARGS ...]")
            print("       SCRIPT is a legacy research/tools/*.py Ghidra helper")
        return 0 if args else 2
    name = args[0]
    if name in ALIASES:
        name = ALIASES[name]
    if group == "verify" and not name.startswith("verify_") and "/" not in name:
        name = f"verify_{name}"
    if not name.endswith(".py") and "/" not in name:
        name += ".py"
    path = Path(name)
    if not path.is_absolute():
        path = tools_root() / path
    return run_compat_script(path, args[1:])


def main(argv: list[str] | None = None) -> int:
    args = list(sys.argv[1:] if argv is None else argv)
    if not args or args[0] in ("-h", "--help"):
        _top_parser().print_help()
        return 0
    command, tail = args[0], args[1:]
    try:
        if command == "parity":
            parsed = _parity_parser().parse_args(tail)
            return run_compat_script(ALIASES[parsed.kind], parsed.args)
        if command == "frame":
            return run_compat_script(ALIASES["frame"], tail)
        if command == "trace":
            return run_compat_script(ALIASES["trace"], tail)
        if command == "verify":
            return _dispatch_named("verify", tail)
        if command == "ghidra":
            return _dispatch_named("ghidra", tail)
    except SystemExit as exc:
        return int(exc.code or 0)
    except ToolError as exc:
        print(f"quiky: {exc}", file=sys.stderr)
        return 2
    print(f"quiky: unknown command {command!r}; use --help", file=sys.stderr)
    return 2
