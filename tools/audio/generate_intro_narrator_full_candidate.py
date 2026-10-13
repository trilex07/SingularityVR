"""Regenerate the available Polish dubbing corpus into a separate Pocket-TTS candidate.

Narrator/Renko use the approved Polish intro voice. Named characters retain
their existing distinct Polish character references. Original WAVs are read
only; all new 24 kHz mono PCM16 files go under the ignored repair candidate.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.pocket_tools")
sys.path.insert(0, str(Path(__file__).resolve().parent))

import numpy as np
from pocket_tts import TTSModel
from scipy.io import wavfile
from scipy.signal import resample_poly

from tts_pronunciation import to_spoken_polish

ROOT = Path(__file__).resolve().parents[2]
AUDIO = ROOT / "audio/polish"
INTRO = AUDIO / "repair_candidate_20261013/intro_reference_experiment/intro_narrator_reference.wav"
OUT = AUDIO / "repair_candidate_20261013/full_revoice_intro"
CORRECTIONS = AUDIO / "repair_candidate_20261013/corrections.local.json"

CHARACTER_REFERENCES = {
    "narrator_user": INTRO,
    "renko_user": INTRO,
    "barisov_dmitri": AUDIO / "character_samples_12/03_barisov_command.wav",
    "demichev_ruslan": AUDIO / "character_samples_12/06_demichev_cold.wav",
    "kathryn_tetiana": AUDIO / "character_samples_12/09_kathryn_scientific.wav",
    "devlin_mykyta": AUDIO / "character_samples_12/11_devlin_restrained.wav",
}
TOKEN_WORDS = {
    "use": "użycia", "fire": "strzału", "grab": "chwytania", "age": "TMD",
    "impulse": "impulsu", "flashlight": "latarki", "aim": "celowania",
    "jump": "skoku", "crouch": "kucania", "dash": "biegu",
    "reload": "przeładowania", "switchwpn": "zmiany broni", "health": "zdrowia",
}
TEXT_OVERRIDES = {
    r"..\RvGame\Localization\INT\RvGame.int|General|LevelIntroProcessing":
        "Kompleks przetwarzania E99 na Katorgze dwanaście, 27 października 2010 roku.",
    r"..\RvGame\Localization\INT\RvGame.int|Achievements|kills_shotgun_DESC":
        "Zabij trzydziestu wrogów strzelbą Wołk es cztery w kampanii jednoosobowej.",
    r"..\RvGame\Localization\INT\RvGame.int|Achievements|kills_sniper_slomo_DESC":
        "Zabij dwudziestu pięciu wrogów karabinem snajperskim Kasimowa, spowalniając czas, w kampanii jednoosobowej.",
    r"..\RvGame\Localization\INT\RvGame.int|Achievements|kills_impulse_DESC":
        "Zabij dwudziestu wrogów mocą Impulsu w kampanii jednoosobowej.",
    r"..\RvGame\Localization\INT\RvGame.int|Achievements|chrono_notes_DESC":
        "Użyj TMD, by znaleźć piętnaście notatek Chrono w kampanii jednoosobowej.",
    r"..\RvGame\Localization\INT\RvGame.int|RvUI3D|ConsoleControls":
        "Strzał. Użyj lub przeładuj. Chrono Ping. Impuls. Chwyt. Postarzanie. Zmień broń. Kucnij. Celuj. Sprint. Skocz. Użyj apteczki.",
    r"..\RvGame\Localization\INT\RvGame.int|RvUI3D|ConsoleControls_MP_Soldiers":
        "Strzał. Przeładuj. Moc TMD. Chwyt. Postarzanie. Zmień broń. Kucnij. Celuj. Sprint. Skocz.",
    r"..\RvGame\Localization\INT\RvGame.int|RvUI|MPHitSound":
        "Dźwięk trafienia w trybie wieloosobowym.",
    r"..\RvGame\Localization\INT\RvGame.int|RvUI|ScienceToWeaponAnnounce":
        "Wymieniono dziesięć tysięcy jednostek technologii E99 na dwa moduły ulepszeń broni.",
    r"..\RvGame\Localization\INT\RvGame.int|Perk|TMDE_FieldDoctor_Desc":
        "Odzyskuj dwadzieścia pięć procent zdrowia poza walką.",
    r"..\RvGame\Localization\INT\Subtitles.int|Subtitles|Tip_009":
        "Metal E99 zawiera oczyszczony izotop, który nadaje mu wyjątkowe właściwości.",
    "SP_RL2_kismet05_1950_LOC_INT/SoundNodeWave/RL3_5215_BARY.ogg":
        "Weź to. To prototypowa broń na E99. Może być niestabilna, ale pomoże nam przechylić szalę zwycięstwa.",
    "SP_VI2_kismet01_LOC_INT/SoundNodeWave/VI1_0507_NIM.ogg":
        "Tajtan jeden, tu aktualny. Kapitanie, jaki jest wasz status?",
    "SP_VI2_kismet01_LOC_INT/SoundNodeWave/VI1_0114_CP.ogg":
        "Tu Katorga dwanaście. E99 i TMD. Nawiązano łączność radiową z Rią Stadless.",
    "SP_VI2_kismet01_LOC_INT/SoundNodeWave/VI2_9015_PA.ogg":
        "Na końcu E99 trafia do kalutronu albo pieca, gdzie zostaje oczyszczony i wzbogacony. Po zakończeniu tego etapu jest gotowy do użycia.",
    "Sp_vi3_kismet02_c_LOC_INT/SoundNodeWave/VI99_0616_ANN.ogg":
        "Możliwości E99 wydają się nieograniczone.",
    "SP_vi4_kismet01_c_LOC_INT/SoundNodeWave/VI99_0707_ANN.ogg":
        "Dziś ponownie porozmawiamy o bezpieczeństwie. Pomoże nam w tym doktor E99.",
    "SP_Freight_Script_1950Dock_LOC_INT/SoundNodeWave/FR3_2808_SPT3.ogg":
        "Nazywam się Katorga dwanaście. Jestem tu dziś z wami.",
    "SP_VI4_kismet01_a_LOC_INT/SoundNodeWave/VI4_4411_GST.ogg":
        "Zostanę tutaj, gdy będziecie otwierać drzwi!",
    "SP_vi4_kismet01_c_LOC_INT/SoundNodeWave/VI99_0708_ANN.ogg":
        "Katorga dwanaście to wspaniałe miejsce do życia. Jest tu mnóstwo atrakcji dla ciebie i twoich przyjaciół.",
    "SP_VI4_kismet01_LOC_INT/SoundNodeWave/VI3_6112_GST.ogg":
        "Renko, bierz karabin maszynowy, zanim się wedrą!",
    "SP_VI5_kismet02_LOC_INT/SoundNodeWave/VI5_4013_GST.ogg":
        "Powtórzę. Tu Devlin, cztery-jeden.",
    "SP_RT3_zone04_LOC_INT/SoundNodeWave/RT99_1405_SC2.ogg":
        "Szybko ubywa nam ludzi. Musimy wydostać się z tych cholernych kanałów i dotrzeć do torów. Tam znajdziemy lokomotywę i zabierzemy ją do doków. Musimy jednak iść powoli. Wielu z nas jest rannych, a te stworzenia są silne i przebiegłe.",
    "SP_VI2_kismet01_LOC_INT/SoundNodeWave/VI1_0115_NIM.ogg":
        "Wszystkie samoloty LERC pięć wystartowały i są już w powietrzu.",
    "SP_VI2_zone03_LOC_INT/SoundNodeWave/VI99_0505_ADM.ogg":
        "Wszyscy nowo przybyli do Katorgi dwanaście muszą zgłosić się do rejestracji. Osoby już zarejestrowane mogą wejść przez północną bramę.",
    "SP_VI5_kismet02_LOC_INT/SoundNodeWave/VI5_4009_GST.ogg":
        "Tajtan jeden, tu Devlin cztery-jeden. Odbiór?",
    "SP_VI5_kismet03_LOC_INT/SoundNodeWave/VI5_4009_GST.ogg":
        "Tajtan jeden, tu Devlin cztery-jeden. Odbiór?",
    "SP_VI5_kismet03_LOC_INT/SoundNodeWave/VI5_4013_GST.ogg":
        "Powtórzę. Tu Devlin, cztery-jeden.",
    "SP_VI3_kismet01_LOC_INT/SoundNodeWave/VI2_7507_GST.ogg":
        "Tu sekcja ósma. Wiem, do cholery.",
    "Sp_vi3_kismet02_b_LOC_INT/SoundNodeWave/VI99_0408_TCH.ogg":
        "Dzieci, natychmiast schowajcie się pod biurkiem i połóżcie ręce na głowie. Spokojnie, wszystko będzie dobrze...",
    "SP_VI5_kismet02_LOC_INT/SoundNodeWave/VI5_1805_GST.ogg":
        "W porządku, Mike. Wracaj na moje stanowisko.",
}


def clean_localization_text(text: str) -> str:
    text = re.sub(r"<[^>]+>", " ", text or "")
    # Turn UI action aliases into natural Polish instructions before the
    # generic token-name fallback below ("Naciśnij kucania" is ungrammatical).
    action_phrases = {
        "fire": "przycisk strzału", "use": "przycisk użycia",
        "grab": "przycisk chwytu", "age": "TMD", "impulse": "Impulsu",
        "flashlight": "przycisk latarki", "aim": "przycisk celowania",
        "jump": "przycisk skoku", "crouch": "przycisk kucania",
        "dash": "przycisk sprintu", "reload": "przycisk przeładowania",
        "switchwpn": "przycisk zmiany broni",
    }
    for token, spoken in action_phrases.items():
        token_re = re.escape(f"[{token}]")
        text = re.sub(rf"\b(naciśnij|nacisnij|press)\s+{token_re}",
                      lambda m: ("Naciśnij przycisk Impulsu" if token == "impulse" else
                                 "Naciśnij " + spoken), text, flags=re.IGNORECASE)
        text = re.sub(rf"\b(przytrzymaj|press and hold)\s+{token_re}",
                      ("Przytrzymaj przycisk Impulsu" if token == "impulse" else
                       "Przytrzymaj " + spoken), text, flags=re.IGNORECASE)
    text = re.sub(r"\[([^\]]+)\]", lambda m: TOKEN_WORDS.get(m.group(1).lower(), m.group(1)), text)
    text = text.replace("`~", " ").replace("\\n", " ")
    return to_spoken_polish(re.sub(r"\s+", " ", text).strip())


def spoken_pieces(text: str) -> list[str]:
    text = (text or "").replace("\r\n", "\n").replace("\r", "\n")
    # Multi-line tutorials and control lists must be synthesized as distinct
    # utterances; otherwise Pocket-TTS may stop partway through the list.
    lines = [re.sub(r"\s+", " ", part).strip() for part in text.split("\n")]
    lines = [part for part in lines if part]
    if not lines:
        return []
    pieces: list[str] = []
    for line in lines:
        sentences = [part.strip() for part in re.split(r"(?<=[.!?])\s+", line) if part.strip()]
        if len(sentences) > 1 or len(re.findall(r"\w+", line, flags=re.UNICODE)) <= 14:
            pieces.extend(sentences or [line])
            continue
        clauses = [part.strip() for part in re.split(r"(?<=[,;:])\s+", line) if part.strip()]
        pieces.extend(clauses or [line])
    return pieces


def trim_model_padding(audio: np.ndarray, rate: int) -> np.ndarray:
    active = np.flatnonzero(np.abs(audio) >= 0.002)
    if not active.size:
        return audio
    # Drop the model's low-level pre-roll instead of preserving 30 ms before
    # the speech threshold; these prompts often contain a long noise floor
    # below -54 dBFS. A short fade protects the first consonant from clicks.
    start = max(0, int(active[0]) - int(0.005 * rate))
    end = min(len(audio), int(active[-1]) + int(0.07 * rate))
    audio = audio[start:end].copy()
    fade = min(int(0.005 * rate), len(audio) // 2)
    if fade:
        audio[:fade] *= np.linspace(0.0, 1.0, fade, dtype=np.float32)
        audio[-fade:] *= np.linspace(1.0, 0.0, fade, dtype=np.float32)
    return audio


def read_jsonl(path: Path) -> list[dict]:
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]


def pcm16_reference(source: Path, output_dir: Path, profile: str) -> Path:
    """Pocket-TTS' WAV reader accepts PCM, so stage float demo WAVs as PCM16."""
    rate, audio = wavfile.read(source)
    audio = np.asarray(audio)
    if audio.ndim == 2:
        audio = audio.astype(np.float32).mean(axis=1)
    if np.issubdtype(audio.dtype, np.integer):
        audio = audio.astype(np.float32) / max(abs(np.iinfo(audio.dtype).min), np.iinfo(audio.dtype).max)
    else:
        audio = audio.astype(np.float32)
    if rate != 24000:
        audio = resample_poly(audio, 24000, rate)
        rate = 24000
    output_dir.mkdir(parents=True, exist_ok=True)
    path = output_dir / f"{profile}_reference_pcm16.wav"
    wavfile.write(str(path), rate,
                  np.asarray(np.rint(np.clip(audio, -1.0, 1.0) * 32767.0), dtype=np.int16))
    return path


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, default=OUT)
    parser.add_argument("--limit", type=int, default=0, help="for pipeline smoke checks; 0 processes all rows")
    parser.add_argument("--pack", choices=("all", "full_localization", "story_nllb"), default="all")
    parser.add_argument("--force", action="store_true", help="regenerate selected rows even when a candidate WAV already exists")
    parser.add_argument("--ids-file", type=Path, help="JSON array of line IDs to generate")
    parser.add_argument("--text-overrides-json", type=Path,
                        help="optional local JSON object mapping exact manifest line_id to corrected Polish text")
    parser.add_argument("--profile-overrides-json", type=Path,
                        help="optional local JSON object mapping exact manifest line_id to a verified voice profile")
    args = parser.parse_args()
    if not INTRO.is_file():
        raise SystemExit(f"Prepare the approved intro reference first: {INTRO}")
    if not CORRECTIONS.is_file():
        raise SystemExit(f"Missing preserved local correction map: {CORRECTIONS}")
    corrections = json.loads(CORRECTIONS.read_text(encoding="utf-8"))
    exact_overrides = (json.loads(args.text_overrides_json.read_text(encoding="utf-8"))
                       if args.text_overrides_json else {})
    profile_overrides = (json.loads(args.profile_overrides_json.read_text(encoding="utf-8"))
                         if args.profile_overrides_json else {})
    ui_corrections = corrections.get("UI_CORRECTIONS", {})
    story_corrections = corrections.get("STORY_CORRECTIONS", {})

    full_manifest = AUDIO / "full_voicepack/manifest.jsonl"
    story_manifest = AUDIO / "story_voicepack_nllb/manifest.jsonl"
    inputs: list[tuple[str, Path, float, list[dict]]] = []
    if args.pack in ("all", "full_localization"):
        inputs.append(("full_localization", full_manifest, 0.45, read_jsonl(full_manifest)))
    if args.pack in ("all", "story_nllb"):
        inputs.append(("story_nllb", story_manifest, 0.55, read_jsonl(story_manifest)))

    jobs: list[dict] = []
    for pack, _, temperature, rows in inputs:
        for row in rows:
            if pack == "full_localization":
                line_id = row.get("id", "")
                profile = profile_overrides.get(line_id, row.get("profile", "narrator_user"))
                polish = exact_overrides.get(line_id, TEXT_OVERRIDES.get(
                    line_id, ui_corrections.get(line_id, row.get("pl", ""))))
                # `tts_text` in this manifest is an older, partly-English
                # synthesis string. Prefer the actual Polish draft unless a
                # targeted correction overrides it.
                text = clean_localization_text(polish)
                original = AUDIO / "full_voicepack" / row.get("file", "")
                english = row.get("en", "")
            else:
                line_id = row.get("source", "")
                profile = profile_overrides.get(line_id, row.get("profile", "narrator_user"))
                polish = exact_overrides.get(line_id, TEXT_OVERRIDES.get(
                    line_id, story_corrections.get(row.get("asset", ""), row.get("text_pl_machine", ""))))
                text = to_spoken_polish(polish)
                original = AUDIO / "story_voicepack_nllb" / row.get("file", "")
                english = row.get("text_en", "")
            if not line_id or not text:
                continue
            reference = CHARACTER_REFERENCES.get(profile)
            if reference is None or not reference.is_file():
                raise SystemExit(f"Missing voice reference for profile {profile}: {reference}")
            jobs.append({"pack": pack, "line_id": line_id, "profile": profile,
                         "temperature": temperature, "text": text, "polish": polish,
                         "english": english, "original": str(original.resolve()),
                         "reference": str(reference.resolve())})
    if args.limit > 0:
        jobs = jobs[:args.limit]
    if args.ids_file:
        wanted = set(json.loads(args.ids_file.read_text(encoding="utf-8")))
        unknown = wanted - {job["line_id"] for job in jobs}
        if unknown:
            raise SystemExit(f"Unknown candidate line IDs: {len(unknown)}")
        jobs = [job for job in jobs if job["line_id"] in wanted]

    out = args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=True)
    reference_dir = out / "references"
    reference_paths = {profile: pcm16_reference(path, reference_dir, profile)
                       for profile, path in CHARACTER_REFERENCES.items() if path.is_file()}
    for job in jobs:
        job["reference"] = str(reference_paths[job["profile"]])
    manifest_path = out / "manifest.jsonl"
    existing = {item["pack"] + "\0" + item["line_id"]: item
                for item in read_jsonl(manifest_path)} if manifest_path.is_file() else {}
    models: dict[tuple[str, float], object] = {}
    states: dict[tuple[str, float, str], object] = {}
    pause = None

    for index, job in enumerate(jobs, 1):
        key = job["pack"] + "\0" + job["line_id"]
        old = existing.get(key)
        if old and Path(old["file"]).is_file() and not args.force:
            continue
        model_key = (job["pack"], job["temperature"])
        if model_key not in models:
            models[model_key] = TTSModel.load_model(
                config="hf://shefowl/pocket-tts-polish-6l/config.yaml",
                temp=job["temperature"], quantize=True)
        model = models[model_key]
        state_key = (*model_key, job["profile"])
        if state_key not in states:
            states[state_key] = model.get_state_for_audio_prompt(job["reference"])
        parts = spoken_pieces(job["text"])
        rendered = [model.generate_audio(
            states[state_key], part, max_tokens=100, frames_after_eos=4
        ).numpy().squeeze().astype(np.float32) for part in parts]
        pause = np.zeros(int(model.sample_rate * 0.08), dtype=np.float32)
        audio = np.concatenate([piece for i, piece in enumerate(rendered)
                                for piece in ((piece, pause) if i < len(rendered) - 1 else (piece,))])
        peak = float(np.max(np.abs(audio))) if audio.size else 0.0
        if not peak:
            raise SystemExit(f"Pocket-TTS generated silence for {job['line_id']}")
        audio = np.clip(audio * ((10 ** (-3 / 20)) / peak), -1.0, 1.0)
        audio = trim_model_padding(audio, model.sample_rate)
        filename = hashlib.sha1(key.encode("utf-8")).hexdigest()[:16] + ".wav"
        path = out / filename
        wavfile.write(str(path), model.sample_rate,
                      np.asarray(np.rint(audio * 32767.0), dtype=np.int16))
        item = {
            "pack": job["pack"], "line_id": job["line_id"],
            "asset_or_key": job.get("asset", ""), "source_english": job["english"],
            "corrected_polish": job["polish"], "tts_text": job["text"],
            "spoken_tts_text": job["text"], "previous_audio": job["original"],
            "file": str(path), "profile_before": job["profile"],
            "reference_file": job["reference"], "voice_engine": "Pocket-TTS Polish 6L",
            "generation_temperature": job["temperature"], "sample_rate": model.sample_rate,
            "generated_duration_s": round(len(audio) / model.sample_rate, 3),
        }
        existing[key] = item
        manifest_path.write_text("\n".join(json.dumps(x, ensure_ascii=False)
                                              for x in existing.values()) + "\n", encoding="utf-8")
        if index == 1 or index % 25 == 0:
            print(f"generated {index}/{len(jobs)}", flush=True)

    print(json.dumps({"rows": len(existing), "requested": len(jobs),
                      "output": str(out)}, ensure_ascii=False))


if __name__ == "__main__":
    main()
