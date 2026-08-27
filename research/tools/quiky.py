#!/usr/bin/env python3
"""Thin executable wrapper for the shared Quiky tooling frontend."""

from __future__ import annotations

from quiky.cli import main


if __name__ == "__main__":
    raise SystemExit(main())
