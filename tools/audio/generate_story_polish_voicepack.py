"""Generate a reviewable Polish voice pack for extracted story SoundNodeWaves.

The source game assets are never modified.  This pass deliberately keeps the
English transcript and machine translation in the manifest so a later human
review can replace individual lines before any runtime integration.
"""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.pocket_tools")
import numpy as np
from scipy.io import wavfile
from pocket_tts import TTSModel  # noqa: E402


ROOT = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-cutscene")
TRANSCRIPTS = ROOT / "audio" / "story_en_reference" / "story_transcripts_pl.jsonl"
OUT = ROOT / "audio" / "polish" / "story_voicepack"
REFS = ROOT / "audio" / "polish"
INTRO_REFERENCE = REFS / "repair_candidate_20261013" / "intro_reference_experiment" / "intro_narrator_reference.wav"
NARRATOR_REFERENCE = INTRO_REFERENCE if INTRO_REFERENCE.is_file() else REFS / "user_reference" / "spokoj_reference.wav"


def profile_for(row: dict) -> tuple[str, Path]:
    speaker = (row.get("speaker_guess") or "").lower()
    if speaker == "barisov":
        return "barisov_dmitri", REFS / "accent_reference" / "ru_dmitri_reference.wav"
    if speaker == "demichev":
        return "demichev_ruslan", REFS / "accent_reference" / "ru_ruslan_reference.wav"
    if speaker == "kathryn":
        return "kathryn_tetiana", REFS / "accent_reference" / "uk_tetiana_reference.wav"
    if speaker == "devlin":
        return "devlin_mykyta", REFS / "accent_reference" / "uk_mykyta_reference.wav"
    if speaker == "renko":
        return "renko_user", NARRATOR_REFERENCE
    return "narrator_user", NARRATOR_REFERENCE


def normalize(audio: np.ndarray) -> np.ndarray:
    audio = np.asarray(audio, dtype=np.float32).squeeze()
    peak = float(np.max(np.abs(audio))) if audio.size else 0.0
    if peak > 0:
        audio = audio * ((10.0 ** (-3.0 / 20.0)) / peak)
    return np.clip(audio, -1.0, 1.0)


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    manifest_path = OUT / "manifest.jsonl"
    done: set[str] = set()
    if manifest_path.exists():
        for line in manifest_path.read_text(encoding="utf-8").splitlines():
            if line.strip():
                try:
                    done.add(json.loads(line)["source"])
                except (KeyError, json.JSONDecodeError):
                    pass

    rows: list[dict] = []
    for line in TRANSCRIPTS.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        row = json.loads(line)
        text = (row.get("text_pl_machine") or "").strip()
        source = row.get("path") or ""
        if not text or not source or source in done:
            continue
        profile, reference = profile_for(row)
        if not reference.exists():
            print(f"ERROR missing reference for {profile}: {reference}", flush=True)
            continue
        row["_profile"] = profile
        row["_reference"] = str(reference)
        rows.append(row)

    if not rows:
        print("complete: nothing to generate", flush=True)
        return

    model = TTSModel.load_model(
        config="hf://shefowl/pocket-tts-polish-6l/config.yaml",
        temp=0.45,
        quantize=True,
    )
    references: dict[str, object] = {}
    for row in rows:
        profile = row["_profile"]
        if profile not in references:
            references[profile] = model.get_state_for_audio_prompt(row["_reference"])

    with manifest_path.open("a", encoding="utf-8") as manifest:
        for index, row in enumerate(rows, 1):
            source = row["path"]
            stem = hashlib.sha1(source.encode("utf-8")).hexdigest()[:16]
            out_path = OUT / f"{stem}.wav"
            try:
                audio = model.generate_audio(
                    references[row["_profile"]], row["text_pl_machine"]
                ).numpy()
                audio = normalize(audio)
                # PCM16 keeps the review files compatible with ordinary
                # Windows players and the game's legacy audio importer.
                wavfile.write(str(out_path), model.sample_rate, (audio * 32767.0).astype(np.int16))
                manifest.write(
                    json.dumps(
                        {
                            "source": source,
                            "package": row.get("package"),
                            "asset": row.get("asset"),
                            "speaker_guess": row.get("speaker_guess"),
                            "profile": row["_profile"],
                            "text_en": row.get("text_en"),
                            "text_pl_machine": row["text_pl_machine"],
                            "translation_status": row.get("translation_status"),
                            "source_duration_s": row.get("duration"),
                            "generated_duration_s": round(float(len(audio) / model.sample_rate), 3),
                            "file": out_path.name,
                            "sample_rate": model.sample_rate,
                            "peak_dbfs": -3.0,
                        },
                        ensure_ascii=False,
                    )
                    + "\n"
                )
                manifest.flush()
                if index == 1 or index % 25 == 0:
                    print(f"generated {index}/{len(rows)}", flush=True)
            except Exception as exc:
                print(f"ERROR {source}: {exc}", flush=True)
    print("complete", flush=True)


if __name__ == "__main__":
    main()
