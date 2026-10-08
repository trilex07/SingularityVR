#!/usr/bin/env python3
"""Inventory Coalesced strings for localization research.

Full source strings are written only to the explicitly supplied output path.
The repository-facing manifest contains IDs, counts and hashes, so it is safe
to commit without shipping the game's text or a generated game package.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from collections import Counter
from pathlib import Path

from coalesced_format import encode, iter_entries, parse

TOKEN_RE = re.compile(r"\[[A-Za-z0-9_]+\]|%[-+0-9.]*[A-Za-z]|\{\d+\}|\\n|\\t")
TAG_RE = re.compile(r"</?[A-Za-z][^>]*>")


def category(record: str, section: str, key: str, value: str) -> str:
    haystack = " ".join((record, section, key, value)).lower()
    if "subtitle" in haystack or section.lower() == "subtitles":
        return "subtitle"
    if any(word in haystack for word in ("objective", "checkpoint", "mission")):
        return "objective"
    if any(word in haystack for word in ("tutorial", "hint", "combat", "impulse", "chronolight")):
        return "tutorial"
    if any(word in haystack for word in ("rvui", "menu", "option", "dialog", "tcr")):
        return "ui"
    if any(word in haystack for word in ("weapon", "ammo", "grenade", "rifle", "shotgun")):
        return "item"
    if any(word in haystack for word in ("loading", "tip", "hud", "score")):
        return "hud"
    return "other"


def entry_id(record: str, section: str, key: str) -> str:
    return f"{record}|{section}|{key}"


def digest_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def make_entry(record: str, section: str, key: str, value: str) -> dict:
    return {
        "id": entry_id(record, section, key),
        "record": record,
        "section": section,
        "key": key,
        "category": category(record, section, key, value),
        "text": value,
        "tokens": TOKEN_RE.findall(value),
        "markup": TAG_RE.findall(value),
        "newline_count": value.count("\n"),
        "utf8_sha256": digest_text(value),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path, help="local JSONL output")
    parser.add_argument("--manifest", type=Path, help="repository-safe summary JSON")
    parser.add_argument("--label", default=None, help="locale label, e.g. INT")
    args = parser.parse_args()

    data = args.source.read_bytes()
    records = parse(data)
    if encode(records) != data:
        raise SystemExit("FAIL: untouched Coalesced round-trip is not byte-identical")
    entries = [make_entry(*entry) for entry in iter_entries(records)]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="utf-8", newline="\n") as handle:
        for item in entries:
            handle.write(json.dumps(item, ensure_ascii=False, separators=(",", ":")) + "\n")

    counts = Counter(item["category"] for item in entries)
    summary = {
        "source_name": args.source.name,
        "source_sha1": hashlib.sha1(data).hexdigest(),
        "source_size": len(data),
        "locale": args.label,
        "records": len(records),
        "entries": len(entries),
        "categories": dict(sorted(counts.items())),
        "roundtrip": "PASS",
        "bulk_inventory": str(args.output),
    }
    if args.manifest:
        args.manifest.parent.mkdir(parents=True, exist_ok=True)
        args.manifest.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    sys.exit(main())

