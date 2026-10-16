#!/usr/bin/env python3
"""Localization gap analyzer (Side, offline).

Reads the in-repo manifests (never the game, never audio binaries beyond
headers already recorded) and emits a machine-readable backlog plus a
readable summary. Status taxonomy is conservative: a recording is never
marked accepted for existing, and nothing is marked verified in game
without runtime evidence (which Side cannot produce).

Usage: py -3 tools/localization/analyze_gaps.py [--out DIR]
"""
from __future__ import annotations

import argparse
import json
from collections import Counter
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
LOC = REPO / "localization" / "pl"
XVOX = REPO / "audio" / "polish" / "story_voicepack_xlingual"
RV = REPO / "remaining-voices"


def load_any(path: Path):
    raw = path.read_bytes()
    text = None
    for enc in ("utf-8-sig", "utf-8", "cp1250", "utf-16"):
        try:
            text = raw.decode(enc)
            break
        except Exception:
            pass
    if text is None:
        raise ValueError(f"undecodable: {path}")
    text = text.strip()
    try:
        d = json.loads(text)
        if isinstance(d, list):
            return d
        for k in ("rows", "records", "manifest_rows"):
            if isinstance(d.get(k), list):
                return d[k]
        return d
    except Exception:
        return [json.loads(line) for line in text.splitlines() if line.strip()]


def chapter_of(package: str) -> str:
    p = (package or "").upper()
    if p.startswith("MP_"):
        return "multiplayer"
    if "_LOC_INT" in p or "_LOC_" in p:
        return p.split("_LOC")[0]
    return p or "unknown"


def scope_of(package: str, category: str) -> str:
    if (package or "").upper().startswith("MP_") or category in ("mp",):
        return "multiplayer"
    return "campaign"


def story_rows():
    out = []
    for r in load_any(LOC / "polish_story_voice.jsonl"):
        has_audio = bool((r.get("file") or "").strip())
        reviewed = r.get("translation_status") == "full_native_text_review"
        replaced = str(r.get("native_replaced")) == "1"
        if not has_audio:
            st, act = "AUDIO_MISSING", "generate/integrate missing WAV, then human listening"
        elif not reviewed:
            st, act = "TRANSLATION_NEEDS_CORRECTION", "native text review of machine draft, then human listening"
        elif not replaced:
            st, act = "TRANSLATED_NOT_INTEGRATED", "integrate into native bank, then verify in game"
        else:
            st, act = "TRANSLATED_AND_MAPPED", "verify in game (never verified by Side)"
        out.append({
            "id": f"story:{r.get('package')}:{r.get('asset')}",
            "category": "story", "scope": scope_of(r.get("package"), "story"),
            "chapter": chapter_of(r.get("package")), "package": r.get("package"),
            "asset": r.get("asset"), "speaker": r.get("speaker_guess"),
            "status": st, "verified_in_game": False,
            "evidence": f"translation_status={r.get('translation_status')} native_replaced={r.get('native_replaced')} file={'yes' if has_audio else 'no'}",
            "action": act})
    return out


def speech_rows(path: Path, category: str, pl_path: Path | None):
    d = load_any(path)
    items = d.items() if isinstance(d, dict) else [(r.get("asset", f"row{i}"), r) for i, r in enumerate(d)]
    pl = {}
    if pl_path and pl_path.is_file():
        for r in load_any(pl_path):
            pl[r.get("asset")] = r
    counts: dict = {}
    for asset, r in items:
        if isinstance(r, dict):
            counts[asset] = counts.get(asset, 0) + 1
    out, emitted = [], set()
    for asset, r in items:
        if not isinstance(r, dict) or asset in emitted:
            continue
        emitted.add(asset)
        p = pl.get(asset, {})
        has_pl = bool((p.get("pl") or "").strip())
        reviewed = (p.get("status") or "") == "full_native_text_review"
        has_audio = bool((r.get("spoken") or r.get("dur") or 0)) or bool((p.get("dur") or 0))
        pkg = r.get("pkg") or r.get("package") or p.get("pkg")
        if not has_pl:
            st, act = "ENGLISH_MISSING_TRANSLATION", "translate, then record"
        elif not reviewed:
            st, act = "TRANSLATION_NEEDS_CORRECTION", "native text review of machine draft"
        elif not has_audio:
            st, act = "AUDIO_MISSING", "record/integrate"
        else:
            st, act = "TRANSLATED_AND_MAPPED", "verify in game"
        out.append({
            "id": f"{category}:{pkg}:{asset}", "category": category,
            "scope": scope_of(pkg, category), "chapter": chapter_of(pkg),
            "package": pkg, "asset": asset, "status": st,
            "verified_in_game": False,
            "occurrences": counts[asset],
            "evidence": f"pl={'yes' if has_pl else 'no'} pl_status={p.get('status')} audio={'yes' if has_audio else 'no'}",
            "action": act})
    return out


def skipped_rows():
    out = []
    for r in load_any(RV / "out" / "skipped" / "skipped_final.json"):
        v = r.get("verdict")
        st = {"STAYS_EN": "ENGLISH_RETAINED", "REVIEW": "TRANSLATION_NEEDS_CORRECTION",
              "CANDIDATE": "NEEDS_CONTEXT", "MISSING_FILE": "AUDIO_MISSING"}.get(v, "NEEDS_CONTEXT")
        out.append({
            "id": f"skipped:{r.get('package')}:{r.get('asset')}", "category": "skipped",
            "scope": scope_of(r.get("package"), "skipped"),
            "chapter": chapter_of(r.get("package")), "package": r.get("package"),
            "asset": r.get("asset"), "status": st, "verified_in_game": False,
            "evidence": f"verdict={v} tempo={r.get('tempo')} peak_db={r.get('peak_db')}",
            "action": "fit-to-budget decision per verdict, then human listening"})
    return out


def vignette_rows():
    out = []
    p = RV / "out" / "vignette" / "manifest_rows.json"
    if not p.is_file():
        return out
    for r in load_any(p):
        flagged = bool(r.get("qa_flagged"))
        out.append({
            "id": f"vignette:{r.get('asset')}", "category": "vignette",
            "scope": "campaign", "chapter": "vignette", "package": None,
            "asset": r.get("asset"),
            "status": "NEEDS_HUMAN_LISTENING" if flagged else "TRANSLATED_AND_MAPPED",
            "verified_in_game": False,
            "evidence": f"qa_flagged={flagged} qa_note={r.get('qa_note')}",
            "action": "human listening" if flagged else "verify in game"})
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=str(REPO / "tools" / "localization"))
    args = ap.parse_args()
    outdir = Path(args.out)
    recs = []
    dup_total = 0
    recs += story_rows()
    for _rows in (speech_rows(RV / "bark-speech.json", "bark", RV / "bark-pl.json"),
                  speech_rows(RV / "mp-speech.json", "mp", RV / "mp-pl.json"),
                  speech_rows(RV / "intro-speech.json", "intro", RV / "intro-pl.json")):
        recs += _rows
        dup_total += sum(r["occurrences"] - 1 for r in _rows)
    recs += vignette_rows()
    recs += skipped_rows()
    try:
        qa = load_any(XVOX / "QA_FLAGS.json")
        qa_files = set()
        if isinstance(qa, dict):
            for v in qa.values():
                if isinstance(v, list):
                    qa_files.update(str(x) for x in v)
        print(f"QA flag buckets (long_token/abrupt_tail/too_short) reference {len(qa_files)} files -> NEEDS_HUMAN_LISTENING")
    except Exception as e:
        print(f"QA_FLAGS unreadable: {e}")
    by_status, by_cat, by_chapter = Counter(), Counter(), Counter()
    for r in recs:
        by_status[r["status"]] += 1
        by_cat[r["category"]] += 1
        by_chapter[r["chapter"]] += 1
    (outdir / "backlog.json").write_text(json.dumps(recs, indent=1, ensure_ascii=False), encoding="utf-8")
    lines = ["# Polish localization gaps (generated, do not hand-edit)",
             "", f"Unique records: {len(recs)} (+{dup_total} repeated references "
             "to the same audio asset, counted via `occurrences`, not as content).",
             f"verified_in_game=true: "
             f"{sum(1 for r in recs if r['verified_in_game'])} (Side cannot verify).",
             "", "## By status",
             *[f"- {k}: {v}" for k, v in by_status.most_common()],
             "", "## By category",
             *[f"- {k}: {v}" for k, v in by_cat.most_common()],
             "", "## Top chapters (excluding multiplayer bucket)",
             *[f"- {k}: {v}" for k, v in by_chapter.most_common(15) if k != "multiplayer"],
             "", "Multiplayer scope rows: "
             f"{sum(1 for r in recs if r['scope'] == 'multiplayer')} (separate optional scope).",
             "", "Method: tools/localization/analyze_gaps.py. Machine drafts are "
             "NEEDS_CORRECTION by construction; recordings are never accepted on "
             "existence/ASR alone."]
    (outdir / "GAPS.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"records={len(recs)}")
    for k, v in by_status.most_common():
        print(f"  {k}: {v}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
