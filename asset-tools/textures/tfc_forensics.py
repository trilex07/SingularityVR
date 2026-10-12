#!/usr/bin/env python3
"""TFC forensics (offline, read-only).

Samples N spread windows of files given on the command line and reports
SHA-256 per window. Used to answer one question with measurements instead of
assumptions: do the local Textures_P*.tfc files actually hold distinct cache
contents, or are some of them copies of one blob?

Nothing is written, nothing is uploaded. Paths come from argv so no
machine-specific layout is baked in.

Usage:
    python asset-tools/textures/tfc_forensics.py <tfc1> [tfc2 ...] [--windows N]
"""

from __future__ import annotations

import hashlib
import sys
from pathlib import Path

WINDOW = 1 << 20


def sample(path: Path, offsets: list[int]) -> list[str]:
    out = []
    with path.open("rb") as f:
        for off in offsets:
            f.seek(off)
            out.append(hashlib.sha256(f.read(WINDOW)).hexdigest()[:12])
    return out


def main() -> None:
    args = [a for a in sys.argv[1:] if not a.startswith("--windows")]
    nwin = 12
    for a in sys.argv[1:]:
        if a.startswith("--windows"):
            nwin = int(a.split("=", 1)[1])
    if not args:
        print(__doc__)
        sys.exit(2)
    paths = [Path(a) for a in args]
    size = paths[0].stat().st_size
    offsets = [0x1000 + i * (size // max(nwin, 1)) for i in range(nwin)]
    rows = {str(p): sample(p, offsets) for p in paths}
    for j, off in enumerate(offsets):
        vals = [rows[str(p)][j] for p in paths]
        mark = "IDENTICAL" if len(set(vals)) == 1 else "differ"
        print(f"+0x{off:08X}: {vals}  {mark}")


if __name__ == "__main__":
    main()
