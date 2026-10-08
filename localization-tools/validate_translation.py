#!/usr/bin/env python3
"""Validate a small translation batch before it is handed to a packager."""
from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter
from pathlib import Path

TOKEN_RE = re.compile(r"\[[A-Za-z0-9_]+\]|%[-+0-9.]*[A-Za-z]|\{\d+\}|\\n|\\t")
TAG_RE = re.compile(r"</?([A-Za-z][\w-]*)(?:\s[^>]*)?>")
UNCHANGED_ALLOWLIST = {"E99", "TMD", "Deadlock", "Chronolight", "Katorga 12"}


def token_multiset(text: str) -> Counter:
    return Counter(TOKEN_RE.findall(text))


def tag_signature(text: str) -> list[str]:
    return [match.group(1).lower() for match in TAG_RE.finditer(text)]


def load_entries(path: Path) -> list[dict]:
    raw = path.read_text(encoding="utf-8")
    if path.suffix.lower() == ".jsonl":
        return [json.loads(line) for line in raw.splitlines() if line.strip()]
    loaded = json.loads(raw)
    if isinstance(loaded, dict):
        loaded = loaded.get("entries", [])
    if not isinstance(loaded, list):
        raise ValueError("translation input must be a JSON array or JSONL")
    return loaded


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("batch", type=Path)
    args = parser.parse_args()
    try:
        entries = load_entries(args.batch)
    except (OSError, UnicodeError, json.JSONDecodeError, ValueError) as exc:
        print(f"ERROR input: {exc}")
        return 2

    errors: list[str] = []
    warnings: list[str] = []
    seen: set[str] = set()
    for index, item in enumerate(entries, 1):
        ident = str(item.get("id", ""))
        if not ident:
            errors.append(f"#{index}: missing id")
        elif ident in seen:
            errors.append(f"{ident}: duplicate id")
        seen.add(ident)
        english = str(item.get("en", ""))
        polish = str(item.get("pl", ""))
        if not polish.strip():
            errors.append(f"{ident}: empty Polish translation")
        if token_multiset(english) != token_multiset(polish):
            errors.append(f"{ident}: placeholder/token mismatch")
        if tag_signature(english) != tag_signature(polish):
            errors.append(f"{ident}: markup tag mismatch")
        if english.count("\n") != polish.count("\n"):
            errors.append(f"{ident}: newline count mismatch")
        if english == polish and polish not in UNCHANGED_ALLOWLIST:
            warnings.append(f"{ident}: Polish is unchanged from English")
        if len(english) > 20 and len(polish) > len(english) * 2.0:
            warnings.append(f"{ident}: translation expands beyond 2x source length")
        if "\r" in polish:
            errors.append(f"{ident}: CRLF/control newline in translation")

    for message in warnings:
        print(f"WARN {message}")
    for message in errors:
        print(f"ERROR {message}")
    print(f"entries={len(entries)} warnings={len(warnings)} errors={len(errors)}")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())

