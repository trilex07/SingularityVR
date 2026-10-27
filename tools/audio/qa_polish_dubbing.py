"""Full local QA pass for generated Polish dubbing packs.

The report is intentionally conservative: uncertain language detection,
machine-draft translation, weak ASR agreement and suspicious timing are marked
REVIEW_REQUIRED instead of being silently accepted.
"""
from __future__ import annotations

import difflib
import json
import math
import re
import sys
import argparse
from collections import Counter
from pathlib import Path

sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.whisper_tools")
sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.pocket_tools")

import numpy as np  # noqa: E402
from scipy.io import wavfile  # noqa: E402

import faster_whisper.audio as fw_audio  # noqa: E402
from faster_whisper import WhisperModel  # noqa: E402


ROOT = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-cutscene")
REPORT_DIR = ROOT / "research" / "localization"
REPORT_DIR.mkdir(parents=True, exist_ok=True)
JSON_OUT = REPORT_DIR / "polish_dubbing_qa.json"
MD_OUT = REPORT_DIR / "POLISH_DUBBING_QA.md"

# PyAV 19 removed metadata_errors, while faster-whisper still passes it.
_av_open = fw_audio.av.open


def _compat_open(input_file, mode="r", metadata_errors=None, **kwargs):
    return _av_open(input_file, mode=mode, **kwargs)


fw_audio.av.open = _compat_open


def tokens(value: str) -> list[str]:
    return re.findall(r"[a-ząćęłńóśźż0-9]+", (value or "").lower())


def wer(expected: list[str], actual: list[str]) -> float:
    if not expected:
        return 0.0 if not actual else 1.0
    row = list(range(len(actual) + 1))
    for i, left in enumerate(expected, 1):
        nxt = [i]
        for j, right in enumerate(actual, 1):
            nxt.append(min(nxt[-1] + 1, row[j] + 1, row[j - 1] + (left != right)))
        row = nxt
    return row[-1] / len(expected)


def similarity(expected: str, actual: str) -> tuple[float, float]:
    et, at = tokens(expected), tokens(actual)
    if not et:
        return (1.0 if not at else 0.0, 0.0)
    seq = difflib.SequenceMatcher(None, et, at).ratio()
    return seq, wer(et, at)


def read_audio(path: Path) -> tuple[dict, str | None]:
    try:
        rate, data = wavfile.read(str(path))
        arr = np.asarray(data)
        if arr.ndim == 2:
            arr = arr.mean(axis=1)
        if np.issubdtype(arr.dtype, np.integer):
            scale = float(np.iinfo(arr.dtype).max)
            x = arr.astype(np.float32) / max(scale, 1.0)
        else:
            x = arr.astype(np.float32)
        x = np.nan_to_num(x)
        duration = float(len(x) / rate) if rate else 0.0
        rms = float(np.sqrt(np.mean(np.square(x)))) if len(x) else 0.0
        peak = float(np.max(np.abs(x))) if len(x) else 0.0
        clipping = float(np.mean(np.abs(x) >= 0.999)) if len(x) else 1.0
        silent = float(np.mean(np.abs(x) < 0.002)) if len(x) else 1.0
        return {
            "sample_rate": int(rate),
            "channels": int(data.shape[1]) if data.ndim == 2 else 1,
            "duration_s": round(duration, 3),
            "rms": round(rms, 6),
            "peak": round(peak, 6),
            "clipping_fraction": round(clipping, 6),
            "silence_fraction": round(silent, 6),
            "audio_format_status": "PASS" if rate == 24000 and data.ndim in (1, 2) and len(x) else "FAIL_AUDIO",
        }, None
    except Exception as exc:
        return {}, str(exc)


def load_retry_map() -> dict[tuple[str, str], str]:
    retry_map: dict[tuple[str, str], str] = {}
    for manifest in (REPORT_DIR).glob("polish_dubbing_retry_manifest_*.json"):
        try:
            payload = json.loads(manifest.read_text(encoding="utf-8"))
            for row in payload.get("rows", []):
                if row.get("audio_path") and Path(row["audio_path"]).exists():
                    retry_map[(row.get("pack", ""), row.get("line_id", ""))] = row["audio_path"]
        except Exception:
            continue
    return retry_map


def load_inventory(use_retry: bool = False, nllb_only: bool = False, final_mix: bool = False) -> list[dict]:
    rows: list[dict] = []
    packs = [
        ("story", ROOT / "audio" / "polish" / "story_voicepack" / "manifest.jsonl"),
        ("full_localization", ROOT / "audio" / "polish" / "full_voicepack" / "manifest.jsonl"),
    ]
    if nllb_only:
        packs = [("story_nllb", ROOT / "audio" / "polish" / "story_voicepack_nllb" / "manifest.jsonl")]
    if final_mix:
        packs = [("story_nllb", ROOT / "audio" / "polish" / "story_voicepack_nllb" / "manifest.jsonl"), ("full_localization", ROOT / "audio" / "polish" / "full_voicepack" / "manifest.jsonl")]
    retry_map = load_retry_map() if use_retry else {}
    for pack, manifest in packs:
        if not manifest.exists():
            continue
        for line_no, line in enumerate(manifest.read_text(encoding="utf-8").splitlines(), 1):
            if not line.strip():
                continue
            row = json.loads(line)
            row["pack"] = pack
            row["manifest_line"] = line_no
            row["audio_path"] = str(manifest.parent / row["file"])
            if pack in {"story", "story_nllb"}:
                row["line_id"] = row.get("source")
                row["expected_pl"] = row.get("text_pl_machine", "")
                row["translation_status"] = "machine_draft"
            else:
                row["line_id"] = row.get("id")
                row["expected_pl"] = row.get("tts_text") or row.get("pl", "")
                row["translation_status"] = "draft_without_explicit_review"
            retry_path = retry_map.get((pack, row.get("line_id", "")))
            if retry_path:
                row["original_audio_path"] = row["audio_path"]
                row["audio_path"] = retry_path
                row["generation_attempt"] = 2
            else:
                row["generation_attempt"] = 1
            rows.append(row)
    return rows


def classify_translation(expected: str, pack: str) -> str:
    words = tokens(expected)
    if not words:
        return "FAIL_TRANSLATION"
    # A strict automated language sanity check, not a semantic approval.
    english_markers = {"the", "and", "you", "your", "file", "loading", "press", "hold", "new", "match", "online", "public", "private", "play", "settings", "weapon", "health", "primary", "secondary", "attack", "description", "disabled", "service", "resolution", "fullscreen", "sudden", "death", "overtime", "save", "continue", "back", "cancel"}
    en = sum(w in english_markers for w in words)
    if en >= 1 and (len(words) <= 4 or en / len(words) > 0.25):
        return "FAIL_TRANSLATION"
    # Story translations are explicitly machine drafts and therefore cannot be
    # called fully reviewed by this automated pass.
    if pack in {"story", "story_nllb"}:
        return "REVIEW_REQUIRED"
    return "AUTOMATED_SANITY_OK"


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--retry", action="store_true")
    ap.add_argument("--nllb", action="store_true")
    ap.add_argument("--final", action="store_true")
    ap.add_argument("--shard", type=int, default=0)
    ap.add_argument("--shards", type=int, default=1)
    ap.add_argument("--delta", action="store_true")
    ap.add_argument("--ids-file", type=Path, help="Optional JSON/JSONL list of pack+line_id pairs to scan")
    args = ap.parse_args()
    rows = load_inventory(use_retry=args.retry, nllb_only=args.nllb, final_mix=args.final)
    if args.delta:
        piper_ids = set()
        piper_manifest = REPORT_DIR / "piper_fallback_manifest.jsonl"
        if piper_manifest.exists():
            for line in piper_manifest.read_text(encoding="utf-8").splitlines():
                if line.strip():
                    item = json.loads(line)
                    piper_ids.add((item.get("pack"), item.get("line_id")))
        rows = [row for row in rows if (row.get("pack"), row.get("line_id")) in piper_ids]
    if args.ids_file:
        selected = set()
        raw = args.ids_file.read_text(encoding="utf-8")
        try:
            parsed = json.loads(raw)
            items = parsed if isinstance(parsed, list) else parsed.get("results", [])
        except json.JSONDecodeError:
            items = [json.loads(line) for line in raw.splitlines() if line.strip()]
        for item in items:
            if isinstance(item, dict):
                selected.add((item.get("pack"), item.get("line_id") or item.get("id")))
        rows = [row for row in rows if (row.get("pack"), row.get("line_id")) in selected]
    if args.shards > 1:
        rows = [row for index, row in enumerate(rows) if index % args.shards == args.shard]
    model = WhisperModel("base", device="cpu", compute_type="int8")
    results: list[dict] = []
    for index, row in enumerate(rows, 1):
        path = Path(row["audio_path"])
        result = dict(row)
        stats, error = read_audio(path)
        result["audio"] = stats
        result.setdefault("generation_attempt", 1)
        result["asr"] = {}
        result["notes"] = []
        result["qa_status"] = "PASS"
        if error:
            result["qa_status"] = "FAIL_AUDIO"
            result["notes"].append(error)
            results.append(result)
            continue

        expected = row["expected_pl"]
        try:
            segments, info = model.transcribe(
                str(path), beam_size=2, vad_filter=False, condition_on_previous_text=False
            )
            segs = list(segments)
            actual = " ".join(s.text.strip() for s in segs).strip()
            language = getattr(info, "language", None)
            language_probability = float(getattr(info, "language_probability", 0.0) or 0.0)
            result["asr"] = {
                "language": language,
                "language_probability": round(language_probability, 4),
                "text": actual,
                "segments": [{"start": s.start, "end": s.end, "text": s.text.strip()} for s in segs],
            }
        except Exception as exc:
            actual, language, language_probability = "", None, 0.0
            result["notes"].append(f"ASR error: {exc}")

        seq, error_rate = similarity(expected, actual)
        result["text_similarity"] = {"token_sequence": round(seq, 4), "wer": round(error_rate, 4)}
        result["translation_quality_status"] = classify_translation(expected, row["pack"])
        expected_words = len(tokens(expected))
        duration = stats.get("duration_s", 0.0)
        speech_rate = expected_words / duration if duration > 0 else math.inf
        result["speech_rate_expected_words_per_s"] = round(speech_rate, 3) if math.isfinite(speech_rate) else None

        statuses: list[str] = []
        if stats.get("audio_format_status") != "PASS" or stats.get("clipping_fraction", 1) > 0.005:
            statuses.append("FAIL_AUDIO")
        if language and language != "pl" and language_probability >= 0.55:
            # Short Polish commands, names and repeated negatives are often
            # classified as Russian/Czech/English by Whisper even when the
            # waveform is intelligible. Require a longer, confident foreign
            # result before calling it a language failure.
            if expected_words <= 4:
                statuses.append("REVIEW_REQUIRED")
            else:
                statuses.append("FAIL_LANGUAGE")
        elif language != "pl" or language_probability < 0.45:
            statuses.append("REVIEW_REQUIRED")
        if not actual or len(tokens(actual)) < max(1, int(expected_words * 0.35)):
            statuses.append("FAIL_GIBBERISH" if actual else "FAIL_TRUNCATED")
        elif seq < 0.35:
            # Whisper's language classifier is unreliable on very short Polish
            # utterances and names (e.g. "Mam go!" may be tagged Japanese or
            # Slovak). Treat low-confidence short-line mismatches as review
            # items; reserve FAIL_GIBBERISH for confidently foreign/unrelated
            # speech.
            if expected_words <= 4 or language_probability < 0.55:
                statuses.append("REVIEW_REQUIRED")
            elif language == "pl" and language_probability >= 0.70 and len(tokens(actual)) >= 2:
                statuses.append("FAIL_GIBBERISH")
            else:
                statuses.append("REVIEW_REQUIRED")
        elif seq < 0.58:
            statuses.append("REVIEW_REQUIRED")
        if expected_words >= 4 and (speech_rate < 0.8 or speech_rate > 7.5):
            statuses.append("REVIEW_REQUIRED")
        if stats.get("silence_fraction", 1.0) > 0.65:
            statuses.append("REVIEW_REQUIRED")
        if row["pack"] in {"story", "story_nllb"}:
            statuses.append("REVIEW_REQUIRED")
        if result["translation_quality_status"] == "FAIL_TRANSLATION":
            statuses.append("FAIL_TRANSLATION")
        elif result["translation_quality_status"] == "REVIEW_REQUIRED":
            statuses.append("REVIEW_REQUIRED")
        result["qa_status"] = next((s for s in ["FAIL_AUDIO", "FAIL_LANGUAGE", "FAIL_TRUNCATED", "FAIL_GIBBERISH", "FAIL_PRONUNCIATION", "FAIL_TRANSLATION", "FAIL_WRONG_VOICE", "REVIEW_REQUIRED"] if s in statuses), "PASS")
        result["notes"].extend(sorted(set(statuses)))
        results.append(result)
        if index == 1 or index % 25 == 0:
            print(f"qa {index}/{len(rows)}", flush=True)

    counts = Counter(r["qa_status"] for r in results)
    payload = {
        "generated_at": "2026-10-08",
        "model": "Systran/faster-whisper-base",
        "expected_language": "pl",
        "total_lines": len(results),
        "counts": dict(sorted(counts.items())),
        "speaker_counts": dict(sorted(Counter(r.get("profile", "unknown") for r in results).items())),
        "results": results,
    }
    base_json = "polish_dubbing_qa_final.json" if args.final else ("polish_dubbing_qa_nllb.json" if args.nllb else "polish_dubbing_qa.json")
    base_md = "POLISH_DUBBING_QA_FINAL.md" if args.final else ("POLISH_DUBBING_QA_NLLB.md" if args.nllb else "POLISH_DUBBING_QA.md")
    if args.shards > 1:
        suffix = f"_delta_shard{args.shard}" if args.delta else f"_shard{args.shard}"
        base_json = base_json.replace(".json", suffix + ".json")
        base_md = base_md.replace(".md", suffix + ".md")
    elif args.delta:
        base_json = base_json.replace(".json", "_delta.json")
        base_md = base_md.replace(".md", "_delta.md")
    json_out = REPORT_DIR / base_json
    md_out = REPORT_DIR / base_md
    json_out.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")

    order = ["PASS", "REVIEW_REQUIRED", "FAIL_LANGUAGE", "FAIL_GIBBERISH", "FAIL_PRONUNCIATION", "FAIL_TRANSLATION", "FAIL_TRUNCATED", "FAIL_AUDIO", "FAIL_WRONG_VOICE", "MISSING_AUDIO"]
    lines = [
        "# Polish dubbing QA",
        "",
        "Automated full pass over both generated packs. Whisper language detection was not forced to Polish. Story translations remain machine drafts and are therefore conservatively marked `REVIEW_REQUIRED` even when audio agreement is good.",
        "",
        f"Model: `Systran/faster-whisper-base`  ",
        f"Total lines: **{len(results)}**  ",
        f"Audio files found: **{sum(1 for r in results if r.get('audio_path') and Path(r['audio_path']).exists())}**",
        "",
        "## QA counts",
        "",
    ]
    for key in order:
        lines.append(f"- {key}: **{counts.get(key, 0)}**")
    lines += ["", "## Speaker/profile counts", ""]
    for key, value in sorted(Counter(r.get("profile", "unknown") for r in results).items()):
        lines.append(f"- `{key}`: {value}")
    lines += ["", "## Review focus", "", "- All story lines are machine-translated and require human meaning/naturalness review.", "- `polish_dubbing_qa.json` retains ASR text, detected language, probabilities, timing and per-line notes.", "- No automatic embedding-based wrong-speaker classifier is available locally; voice mapping is checked against the manifest profile and remains a manual listening item."]
    md_out.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(json.dumps({"total": len(results), "counts": dict(counts)}, ensure_ascii=False), flush=True)


if __name__ == "__main__":
    main()
