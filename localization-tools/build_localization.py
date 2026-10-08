#!/usr/bin/env python3
"""Create a safe handoff manifest for a reviewed Polish batch.

This intentionally does not write Coalesced or UE3 packages.  Package
construction remains disabled until locale resolution, font coverage and the
game's integrity checks have been proven on a disposable copy.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

from validate_translation import load_entries


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("batch", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    entries = load_entries(args.batch)
    manifest = {
        "format": "singularity-polish-handoff-v1",
        "package_generation": "DISABLED",
        "batch": str(args.batch),
        "batch_sha256": hashlib.sha256(args.batch.read_bytes()).hexdigest(),
        "entry_count": len(entries),
        "ids": [item.get("id", "") for item in entries],
        "next_gate": "font-and-locale-resolution",
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(manifest, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

