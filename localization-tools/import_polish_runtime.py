#!/usr/bin/env python3
"""Build a safe, external Polish runtime pack.

This tool deliberately does not touch the game directory, Coalesced_INT.bin,
UE3 packages, or Singularity.exe.  It validates the text draft and optional
voice manifests, then writes a versioned package containing JSONL data and
relative audio references.  A future runtime redirect/importer can consume the
manifest after the binary format is proven safe.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import sys
from pathlib import Path
from typing import Iterable

TOKEN_RE = re.compile(r"\[[A-Za-z0-9_]+\]|%[-+0-9.]*[A-Za-z]|\{\d+\}|</?[A-Za-z][^>]*>")


def read_jsonl(path: Path) -> list[dict]:
    rows: list[dict] = []
    for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip():
            continue
        try:
            value = json.loads(line)
        except json.JSONDecodeError as exc:
            raise ValueError(f"{path}:{line_no}: invalid JSON: {exc}") from exc
        if not isinstance(value, dict):
            raise ValueError(f"{path}:{line_no}: expected an object")
        rows.append(value)
    return rows


def token_signature(text: str) -> list[str]:
    return TOKEN_RE.findall(text or "")


def validate_text(rows: Iterable[dict], allow_unreviewed: bool = False) -> list[str]:
    errors: list[str] = []
    seen: set[str] = set()
    for n, row in enumerate(rows, 1):
        ident = str(row.get("id", ""))
        if not ident:
            errors.append(f"text #{n}: missing id")
        elif ident in seen:
            errors.append(f"text #{n}: duplicate id {ident}")
        seen.add(ident)
        en, pl = str(row.get("en", "")), str(row.get("pl", ""))
        status = row.get("status")
        if not pl.strip() and status not in {"skip", "empty"}:
            errors.append(f"{ident}: empty Polish text")
        if token_signature(en) != token_signature(pl) and (not allow_unreviewed or status == "translated"):
            errors.append(f"{ident}: token/markup signature differs")
        if en.count("\n") != pl.count("\n"):
            errors.append(f"{ident}: newline count differs")
        if status not in {"translated", "skip", "empty", "machine_draft", "needs_review", None}:
            errors.append(f"{ident}: unknown status {row.get('status')!r}")
    return errors


def validate_voice(rows: Iterable[dict], audio_root: Path | None) -> list[str]:
    errors: list[str] = []
    seen: set[str] = set()
    for n, row in enumerate(rows, 1):
        ident = str(row.get("id") or row.get("source") or row.get("file") or "")
        if not ident:
            errors.append(f"voice #{n}: missing id/source/file")
        if ident in seen:
            errors.append(f"voice #{n}: duplicate {ident}")
        seen.add(ident)
        file_name = row.get("file")
        if audio_root and file_name:
            # Manifests use basenames; reject traversal before resolving.
            candidate = (audio_root / str(file_name)).resolve()
            if audio_root.resolve() not in candidate.parents:
                errors.append(f"{ident}: audio path escapes audio root")
            elif not candidate.is_file():
                errors.append(f"{ident}: missing audio {file_name}")
    return errors


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def write_jsonl(path: Path, rows: Iterable[dict]) -> None:
    with path.open("w", encoding="utf-8", newline="\n") as handle:
        for row in rows:
            handle.write(json.dumps(row, ensure_ascii=False, sort_keys=True) + "\n")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--draft", type=Path, required=True, help="polish/full_draft.jsonl")
    ap.add_argument("--voice-manifest", type=Path, action="append", default=[])
    ap.add_argument("--audio-root", type=Path, help="directory containing manifest WAV files")
    ap.add_argument("--output", type=Path, required=True, help="new external package directory")
    ap.add_argument("--copy-audio", action="store_true", help="copy referenced WAVs into output/audio")
    ap.add_argument("--allow-unreviewed", action="store_true",
                    help="package machine_draft/needs_review/empty rows and record them as warnings")
    args = ap.parse_args()

    if args.output.exists() and any(args.output.iterdir()):
        print(f"ERROR output is non-empty: {args.output}", file=sys.stderr)
        return 2
    try:
        text_rows = read_jsonl(args.draft)
        voice_rows: list[dict] = []
        for manifest in args.voice_manifest:
            voice_rows.extend(read_jsonl(manifest))
        errors = validate_text(text_rows, args.allow_unreviewed) + validate_voice(voice_rows, args.audio_root)
    except (OSError, UnicodeError, ValueError) as exc:
        print(f"ERROR {exc}", file=sys.stderr)
        return 2
    if errors:
        for error in errors[:80]:
            print(f"ERROR {error}", file=sys.stderr)
        if len(errors) > 80:
            print(f"ERROR ... {len(errors) - 80} more", file=sys.stderr)
        return 1

    args.output.mkdir(parents=True, exist_ok=True)
    write_jsonl(args.output / "polish_text.jsonl", text_rows)
    if voice_rows:
        write_jsonl(args.output / "polish_voice.jsonl", voice_rows)
    copied = 0
    if args.copy_audio:
        if not args.audio_root:
            print("ERROR --copy-audio requires --audio-root", file=sys.stderr)
            return 2
        audio_out = args.output / "audio"
        audio_out.mkdir()
        for row in voice_rows:
            name = row.get("file")
            if not name:
                continue
            src = (args.audio_root / str(name)).resolve()
            dst = audio_out / Path(str(name)).name
            shutil.copy2(src, dst)
            copied += 1
    manifest = {
        "format": "singularityvr-polish-runtime-pack-1",
        "safe_external_only": True,
        "game_binary_modified": False,
        "coalesced_bin_modified": False,
        "text_source": str(args.draft.resolve()),
        "text_entries": len(text_rows),
        "voice_entries": len(voice_rows),
        "audio_files_copied": copied,
        "files": {"text": "polish_text.jsonl", "voice": "polish_voice.jsonl" if voice_rows else None},
        "sha256": {"polish_text.jsonl": sha256(args.output / "polish_text.jsonl")},
        "integration": "external manifest only; requires a separately proven UE3 runtime redirect",
    }
    if voice_rows:
        manifest["sha256"]["polish_voice.jsonl"] = sha256(args.output / "polish_voice.jsonl")
    (args.output / "polish_runtime_manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    (args.output / "README.txt").write_text(
        "External Polish runtime pack. This package is intentionally not installed into Singularity.\n"
        "Do not rename it to Coalesced_INT.bin or copy it into the game until a safe UE3\n"
        "repack/runtime redirect is proven.\n", encoding="utf-8"
    )
    print(json.dumps({"output": str(args.output.resolve()), "text": len(text_rows),
                      "voice": len(voice_rows), "audio_copied": copied}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
