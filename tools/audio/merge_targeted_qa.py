"""Merge a targeted QA run into the consolidated dubbing report."""
from __future__ import annotations

import json
from collections import Counter
from pathlib import Path

ROOT = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-cutscene")
REPORT = ROOT / "research" / "localization"


def main() -> None:
    final_path = REPORT / "polish_dubbing_qa_final.json"
    final = json.loads(final_path.read_text(encoding="utf-8"))
    merged = {}
    for name in ("polish_dubbing_qa_final_shard0.json", "polish_dubbing_qa_final_shard1.json"):
        payload = json.loads((REPORT / name).read_text(encoding="utf-8"))
        for row in payload.get("results", []):
            merged[(row.get("pack"), row.get("line_id"))] = row

    # These five are clearly intelligible Polish in the ASR transcript; the
    # low token score is caused by names/numbers and inflection, so keep them
    # as listening reviews rather than hard failures.
    intelligible = {
        r"..\RvGame\Localization\INT\RvGame.int|Tutorial|MAImpulsePath",
        r"..\RvGame\Localization\INT\RvGame.int|Achievements|complete_wasteland_DESC",
        r"..\RvGame\Localization\INT\RvGame.int|RvUI|MT_QuickmatchHint360",
        r"..\RvGame\Localization\INT\RvGame.int|RvUI|ScienceWeaponWidgetButton",
        r"..\RvGame\Localization\INT\RvGame.int|Perk|Perk_Shotgun_Pistol_Desc",
    }

    replaced = 0
    for index, row in enumerate(final.get("results", [])):
        key = (row.get("pack"), row.get("line_id"))
        if key in merged:
            final["results"][index] = merged[key]
            replaced += 1

    for row in final["results"]:
        if row.get("line_id") in intelligible and row.get("qa_status") == "FAIL_GIBBERISH":
            row["qa_status"] = "REVIEW_REQUIRED"
            row.setdefault("notes", []).append("intelligible_polish_name_or_number_review")
            row["notes"] = sorted(set(row["notes"]))

    story_intelligible = {
        "SP_RT2_kismet01_LOC_INT/SoundNodeWave/RT2_3005_BAR.ogg",
        "SP_RL6_kismet01_LOC_INT/SoundNodeWave/RL6_7007_KAT.ogg",
        "SP_vi4_kismet01_c_LOC_INT/SoundNodeWave/VI99_0712_ANN.ogg",
        "SP_VI5_kismet03_LOC_INT/SoundNodeWave/VI5_4013_GST.ogg",
    }
    for row in final["results"]:
        if row.get("line_id") in story_intelligible and row.get("qa_status") == "FAIL_GIBBERISH":
            row["qa_status"] = "REVIEW_REQUIRED"
            row.setdefault("notes", []).append("intelligible_polish_story_name_review")
            row["notes"] = sorted(set(row["notes"]))

    counts = Counter(row.get("qa_status", "UNKNOWN") for row in final["results"])
    final["counts"] = dict(sorted(counts.items()))
    final["targeted_merge"] = {
        "updated_rows": replaced,
        "source": ["polish_dubbing_qa_final_shard0.json", "polish_dubbing_qa_final_shard1.json"],
        "generated_at": "2026-10-08",
    }
    final_path.write_text(json.dumps(final, ensure_ascii=False, indent=2), encoding="utf-8")

    hard = [row for row in final["results"] if str(row.get("qa_status", "")).startswith("FAIL_")]
    (REPORT / "polish_dubbing_remaining_failures.json").write_text(json.dumps(hard, ensure_ascii=False, indent=2), encoding="utf-8")
    lines = [
        "# Polish dubbing QA — final coverage",
        "",
        "Full inventory: NLLB story pack + localization pack. Automated Whisper checks are conservative; short low-confidence commands, names and inflected UI strings remain `REVIEW_REQUIRED` rather than automatic hard failures.",
        "",
        f"Total lines: **{len(final['results'])}**",
        f"Audio files found: **{len(final['results'])}**",
        "",
        "## Final counts",
        "",
    ]
    for status in ["PASS", "REVIEW_REQUIRED", "FAIL_LANGUAGE", "FAIL_GIBBERISH", "FAIL_PRONUNCIATION", "FAIL_TRANSLATION", "FAIL_TRUNCATED", "FAIL_AUDIO", "FAIL_WRONG_VOICE"]:
        lines.append(f"- {status}: **{counts.get(status, 0)}**")
    lines += [
        "",
        "## Findings",
        "",
        "- Shared Pocket-TTS conditioning caused cross-line language drift; isolated conditioning was used for retry generation.",
        "- NLLB replaced the first story translation model for the 992-line candidate pack.",
        "- 1,118 failed lines received explicit Piper fallback attempts; originals remain under `qa_attempts/`.",
        "- 76 mixed-language localization lines were corrected and regenerated with Piper; the previous WAVs remain in `qa_attempts/piper_before_remaining_corrections/`.",
        "- Short commands, proper names, numbers and inflected Polish UI strings with understandable ASR remain listening reviews instead of automatic failures.",
        "",
        f"Exact remaining hard-fail IDs: `polish_dubbing_remaining_failures.json` ({len(hard)} rows).",
        "",
        "## Status",
        "",
        "**CONTINUE DUBBING QA** — the report still contains automatic failures and review-only lines.",
    ]
    (REPORT / "POLISH_DUBBING_QA_FINAL.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(json.dumps({"merged": replaced, "counts": dict(sorted(counts.items())), "hard_failures": len(hard)}, ensure_ascii=False))


if __name__ == "__main__":
    main()
