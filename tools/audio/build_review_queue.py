"""Export a compact, prioritized human listening queue from the final QA JSON."""
from __future__ import annotations

import json
from collections import Counter
from pathlib import Path

ROOT = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-cutscene")
REPORT = ROOT / "research" / "localization"


def main() -> None:
    payload = json.loads((REPORT / "polish_dubbing_qa_final.json").read_text(encoding="utf-8"))
    queue = []
    for row in payload.get("results", []):
        if row.get("qa_status") != "REVIEW_REQUIRED":
            continue
        pack = row.get("pack")
        notes = row.get("notes", [])
        if pack == "story_nllb":
            priority = "P1_story_translation_and_names"
        elif any(n in notes for n in ("REVIEW_REQUIRED", "FAIL_LANGUAGE")):
            priority = "P1_localization_language"
        elif any(n in notes for n in ("FAIL_GIBBERISH", "FAIL_TRUNCATED")):
            priority = "P1_localization_audio"
        else:
            priority = "P2_short_or_inflected_ui"
        queue.append({
            "priority": priority,
            "pack": pack,
            "line_id": row.get("line_id"),
            "profile": row.get("profile"),
            "expected_pl": row.get("expected_pl"),
            "asr": row.get("asr", {}).get("text", ""),
            "detected_language": row.get("asr", {}).get("language"),
            "language_probability": row.get("asr", {}).get("language_probability"),
            "duration_s": row.get("audio", {}).get("duration_s"),
            "notes": notes,
            "audio_path": row.get("audio_path"),
        })
    order = {"P1_story_translation_and_names": 0, "P1_localization_language": 1, "P1_localization_audio": 2, "P2_short_or_inflected_ui": 3}
    queue.sort(key=lambda item: (order.get(item["priority"], 9), item["pack"], item["line_id"] or ""))
    json_out = REPORT / "polish_dubbing_review_queue.json"
    json_out.write_text(json.dumps(queue, ensure_ascii=False, indent=2), encoding="utf-8")
    counts = Counter(item["priority"] for item in queue)
    lines = [
        "# Polish dubbing human review queue",
        "",
        f"Review items: **{len(queue)}**",
        "",
        "The queue contains only lines classified `REVIEW_REQUIRED`; generated audio remains available at each `audio_path`.",
        "",
        "## Priority counts",
        "",
    ]
    for key in order:
        lines.append(f"- {key}: **{counts.get(key, 0)}**")
    lines += [
        "",
        "## Review order",
        "",
        "1. Story translation, names and pronunciation (`story_nllb`).",
        "2. Localization lines with language or audio warnings.",
        "3. Short UI commands and inflected Polish strings where Whisper is conservative.",
        "",
        "Full per-line data is in `polish_dubbing_review_queue.json`.",
    ]
    (REPORT / "POLISH_DUBBING_REVIEW_QUEUE.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(json.dumps({"review_items": len(queue), "counts": dict(counts), "json": str(json_out)}, ensure_ascii=False))


if __name__ == "__main__":
    main()
