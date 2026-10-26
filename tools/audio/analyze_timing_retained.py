"""Analyze the 542 timing-retained takes from final-dubbing-main-build-20261026.json.

Correlate with polish_story_voice.jsonl to get the English and Polish text, speaker profile,
and calculate the required budget reduction.
"""
import json
from pathlib import Path

ROOT = Path("C:/Users/Trilex/vr-mods/SingularityVR-final-completion")
BUILD_PATH = ROOT / "docs/handoffs/final-dubbing-main-build-20261026.json"
LOOKUP_PATH = ROOT / "localization/pl/polish_story_voice.jsonl"


def main():
    with BUILD_PATH.open("r", encoding="utf-8") as f:
        build_data = json.load(f)

    with LOOKUP_PATH.open("r", encoding="utf-8-sig") as f:
        lookup_rows = [json.loads(s) for s in f if s.strip()]

    lookup_by_id = {(r["package"].casefold(), r["asset"]): r for r in lookup_rows}

    retained = build_data["retained"]
    timing_cases = [r for r in retained if "budget mismatch" not in r.get("reason", "")]
    codec_cases = [r for r in retained if "budget mismatch" in r.get("reason", "")]

    print(f"Total retained: {len(retained)}")
    print(f"Timing cases:   {len(timing_cases)}")
    print(f"Codec cases:    {len(codec_cases)}")

    # Correlate timing cases
    annotated = []
    missing_lookup = 0
    for tc in timing_cases:
        key = (tc["package"].casefold(), tc["asset"])
        lk = lookup_by_id.get(key)
        if not lk:
            missing_lookup += 1
            continue
        if "native_duration" in tc:
            native_dur = tc["native_duration"]
        elif "native_frames" in tc and "rate" in tc:
            native_dur = tc["native_frames"] / tc["rate"]
        else:
            raise ValueError(f"Unknown duration format in {tc}")
        source_dur = tc["source_duration"]
        # Max source duration allowed to achieve tempo <= 1.35 with 40ms guards (80ms total)
        # native_voice_fit: guarded = source_dur + 0.08; tempo = guarded / native_dur
        # For tempo <= 1.35: (source_dur + 0.08) <= 1.35 * native_dur
        # source_dur <= 1.35 * native_dur - 0.08
        max_allowed_source_dur = max(0.0, 1.35 * native_dur - 0.08)
        dur_delta = source_dur - max_allowed_source_dur

        annotated.append({
            "package": tc["package"],
            "asset": tc["asset"],
            "speaker": lk.get("speaker_guess", "unknown"),
            "profile": lk.get("profile", "unknown"),
            "text_en": lk.get("text_en", ""),
            "text_pl": lk.get("text_pl", ""),
            "tts_text": lk.get("tts_text", ""),
            "native_duration": native_dur,
            "source_duration": source_dur,
            "current_tempo": tc["tempo"],
            "max_allowed_source_duration": max_allowed_source_dur,
            "duration_overshoot_s": dur_delta,
            "tempo_category": "UNSAFE_OVER_185" if tc["tempo"] > 1.85 else "MODERATE_135_TO_185"
        })

    print(f"Annotated {len(annotated)} timing cases (missing from lookup: {missing_lookup})")

    # Speaker distribution among timing cases
    speakers = {}
    for a in annotated:
        spk = a["speaker"]
        speakers[spk] = speakers.get(spk, 0) + 1
    print("\nSpeakers in timing cases:")
    for spk, c in sorted(speakers.items(), key=lambda x: -x[1]):
        print(f"  {spk}: {c}")

    # Duration stats
    dur_deltas = [a["duration_overshoot_s"] for a in annotated]
    print(f"\nOvershoot stats (seconds over max allowed):")
    print(f"  Min overshoot: {min(dur_deltas):.2f}s")
    print(f"  Max overshoot: {max(dur_deltas):.2f}s")
    print(f"  Avg overshoot: {sum(dur_deltas)/len(dur_deltas):.2f}s")

    under_1s = sum(1 for d in dur_deltas if d <= 1.0)
    between_1_2s = sum(1 for d in dur_deltas if 1.0 < d <= 2.0)
    over_2s = sum(1 for d in dur_deltas if d > 2.0)
    print(f"  Overshoot <= 1.0s: {under_1s} (easily fixed by silence trimming / modest phrasing)")
    print(f"  Overshoot 1.0-2.0s: {between_1_2s} (needs concise translation + TTS speed)")
    print(f"  Overshoot > 2.0s: {over_2s} (major translation reduction needed)")

    # Save detailed JSON plan for SIDE work
    out_path = ROOT / "docs/handoffs/final-dubbing-timing-plan-20261026.json"
    with out_path.open("w", encoding="utf-8") as f:
        json.dump({
            "schema": "singularityvr-dubbing-timing-plan-v1",
            "summary": {
                "total_retained": len(retained),
                "timing_cases": len(timing_cases),
                "codec_cases": len(codec_cases),
                "tempo_over_185": sum(1 for a in annotated if a["tempo_category"] == "UNSAFE_OVER_185"),
                "tempo_135_to_185": sum(1 for a in annotated if a["tempo_category"] == "MODERATE_135_TO_185"),
                "overshoot_under_1s": under_1s,
                "overshoot_1_to_2s": between_1_2s,
                "overshoot_over_2s": over_2s,
            },
            "timing_cases": annotated,
            "codec_cases": codec_cases
        }, f, indent=2, ensure_ascii=False)
    print(f"\nWrote timing plan to {out_path}")


if __name__ == "__main__":
    main()
