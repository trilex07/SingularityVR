"""Build Pocket-TTS test lines from user-supplied modulation WAVs in incoming_modulations/."""
from __future__ import annotations

import json
import sys
from datetime import date
from pathlib import Path

sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.pocket_tools")
sys.path.insert(0, str(Path(__file__).resolve().parent))

import numpy as np
from pocket_tts import TTSModel
from scipy.io import wavfile
from scipy.signal import resample_poly

ROOT = Path(__file__).resolve().parents[2]
INCOMING = ROOT / "audio/polish/user_reference/incoming_modulations"
OUT = ROOT / "audio/polish/user_reference" / f"modulation_candidates_{date.today().strftime('%Y%m%d')}"

TEST_LINES = [
    ("ui_loading", "Wczytywanie. Proszę czekać."),
    ("ui_options", "Opcje gry. Tryb wieloosobowy."),
    ("ui_quit", "Czy na pewno chcesz wyjść z meczu?"),
    ("story_calm", "Musimy działać szybko. Sprawdźmy laboratorium."),
    ("story_alert", "Słyszycie to? Wszyscy na stanowiska!"),
]


def load_mono_24k(path: Path) -> tuple[np.ndarray, int]:
    rate, data = wavfile.read(str(path))
    data = np.asarray(data)
    if data.ndim == 2:
        data = data.astype(np.float32).mean(axis=1)
    if np.issubdtype(data.dtype, np.integer):
        data = data.astype(np.float32) / max(
            abs(np.iinfo(data.dtype).min), np.iinfo(data.dtype).max
        )
    else:
        data = data.astype(np.float32)
    if rate != 24000:
        data = resample_poly(data, 24000, rate).astype(np.float32)
    return np.clip(data, -1.0, 1.0), 24000


def best_window(audio: np.ndarray, rate: int, target_s: float = 12.0) -> np.ndarray:
    win = int(target_s * rate)
    if audio.size <= win:
        return audio
    step = int(0.25 * rate)
    best_i, best_e = 0, -1.0
    for i in range(0, audio.size - win, step):
        chunk = audio[i : i + win]
        e = float(np.sqrt(np.mean(chunk * chunk)))
        if e > best_e:
            best_e, best_i = e, i
    return audio[best_i : best_i + win]


def write_wav(path: Path, audio: np.ndarray, rate: int = 24000) -> None:
    peak = float(np.max(np.abs(audio))) or 1.0
    audio = np.clip(audio * ((10 ** (-3 / 20)) / peak), -1.0, 1.0)
    wavfile.write(
        str(path),
        rate,
        np.asarray(np.rint(audio * 32767.0), dtype=np.int16),
    )


def main() -> None:
    if not INCOMING.is_dir():
        INCOMING.mkdir(parents=True, exist_ok=True)
        raise SystemExit(
            f"Brak próbek. Wrzuć WAV do:\n  {INCOMING}\nPatrz docs/USER_VOICE_SAMPLES_INTAKE.md"
        )
    wavs = sorted(INCOMING.glob("*.wav"))
    if not wavs:
        raise SystemExit(f"Brak plików .wav w {INCOMING}")

    OUT.mkdir(parents=True, exist_ok=True)
    refs = OUT / "references"
    refs.mkdir(exist_ok=True)
    demos = OUT / "pocket_tests"
    demos.mkdir(exist_ok=True)

    model = TTSModel.load_model(
        config="hf://shefowl/pocket-tts-polish-6l/config.yaml", temp=0.48, quantize=True
    )
    manifest: list[dict] = []
    for src in wavs:
        audio, rate = load_mono_24k(src)
        clip = best_window(audio, rate)
        ref_path = refs / f"{src.stem}_reference_pcm16.wav"
        write_wav(ref_path, clip, rate)
        state = model.get_state_for_audio_prompt(str(ref_path))
        for line_id, text in TEST_LINES:
            out = demos / f"{src.stem}__{line_id}.wav"
            gen = model.generate_audio(state, text, max_tokens=100, frames_after_eos=4)
            arr = gen.numpy().squeeze().astype(np.float32)
            write_wav(out, arr, model.sample_rate)
            manifest.append(
                {
                    "source_user_wav": str(src.resolve()),
                    "reference_clip": str(ref_path.resolve()),
                    "output": str(out.resolve()),
                    "line_id": line_id,
                    "text": text,
                }
            )

    (OUT / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({"output": str(OUT), "sources": len(wavs), "tests": len(manifest)}, ensure_ascii=False))


if __name__ == "__main__":
    main()
