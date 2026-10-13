"""Extract a local Polish voice reference from the user's dubbed intro and synthesize test lines.

Source audio and generated WAVs stay in the ignored repair-candidate directory.
This does not modify or install any voicepack.
"""
from __future__ import annotations

import argparse
import hashlib
import json
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
DEFAULT_SOURCE = Path(r"C:\Users\Trilex\Music\SNG_intro_polish_voice.wav")
DEFAULT_OUT = ROOT / "audio/polish/repair_candidate_20261013/intro_reference_experiment"
DEFAULT_TEXTS = (
    "Musimy działać szybko. Sprawdźmy, co dzieje się w laboratorium.",
    "Rocket Launcher jest dostępny online.",
)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--start", type=float, default=8.0, help="reference start time in seconds")
    parser.add_argument("--end", type=float, default=18.0, help="reference end time in seconds")
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUT)
    parser.add_argument("--text", action="append", default=[], help="spoken test line; repeat to add lines")
    args = parser.parse_args()
    if not args.source.is_file():
        raise SystemExit(f"Intro voice reference not found: {args.source}")
    if args.start < 0 or args.end <= args.start:
        raise SystemExit("Expected 0 <= --start < --end")

    rate, source = wavfile.read(args.source)
    source = np.asarray(source)
    if source.ndim == 2:
        source = source.astype(np.float32).mean(axis=1)
    if np.issubdtype(source.dtype, np.integer):
        source = source.astype(np.float32) / max(abs(np.iinfo(source.dtype).min), np.iinfo(source.dtype).max)
    else:
        source = source.astype(np.float32)
    if args.end > len(source) / rate:
        raise SystemExit(f"Reference ends at {len(source) / rate:.2f}s, after requested {args.end:.2f}s")
    reference = source[int(args.start * rate):int(args.end * rate)]
    if rate != 24000:
        reference = resample_poly(reference, 24000, rate)
    peak = float(np.max(np.abs(reference))) if reference.size else 0.0
    if peak < 0.01:
        raise SystemExit("Selected intro segment is silent or too quiet")
    reference = np.clip(reference, -1.0, 1.0)

    out_dir = args.output_dir.resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    reference_path = out_dir / "intro_narrator_reference.wav"
    wavfile.write(str(reference_path), 24000,
                  np.asarray(np.rint(reference * 32767.0), dtype=np.int16))

    model = TTSModel.load_model(
        config="hf://shefowl/pocket-tts-polish-6l/config.yaml",
        temp=0.35,
        quantize=True,
    )
    state = model.get_state_for_audio_prompt(str(reference_path))
    manifest = {
        "source_file": str(args.source.resolve()),
        "source_sha256": hashlib.sha256(args.source.read_bytes()).hexdigest(),
        "reference_file": str(reference_path),
        "reference_start_s": args.start,
        "reference_end_s": args.end,
        "reference_sample_rate": 24000,
        "engine": "Pocket-TTS Polish 6L",
        "temperature": 0.35,
        "max_tokens": 100,
        "frames_after_eos": 4,
        "samples": [],
    }
    for index, text in enumerate(args.text or DEFAULT_TEXTS, 1):
        spoken = to_spoken_polish(text)
        audio = model.generate_audio(
            state, spoken, max_tokens=100, frames_after_eos=4
        ).numpy().squeeze().astype(np.float32)
        sample_peak = float(np.max(np.abs(audio))) if audio.size else 0.0
        if not sample_peak:
            raise SystemExit(f"Pocket-TTS generated silence for: {text}")
        audio *= (10.0 ** (-3.0 / 20.0)) / sample_peak
        audio = np.clip(audio, -1.0, 1.0)
        name = f"intro_voice_sample_{index:02d}.wav"
        path = out_dir / name
        wavfile.write(str(path), 24000,
                      np.asarray(np.rint(audio * 32767.0), dtype=np.int16))
        manifest["samples"].append({
            "file": str(path),
            "display_text": text,
            "spoken_tts_text": spoken,
            "duration_s": round(len(audio) / 24000, 3),
        })
    (out_dir / "manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print(json.dumps(manifest, ensure_ascii=False))


if __name__ == "__main__":
    main()
