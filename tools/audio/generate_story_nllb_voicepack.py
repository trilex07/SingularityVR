"""Generate the NLLB-translated story pack with isolated per-line states."""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.pocket_tools")
import numpy as np  # noqa: E402
from scipy.io import wavfile  # noqa: E402
from pocket_tts import TTSModel  # noqa: E402

ROOT = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-cutscene")
SOURCE = ROOT / "audio" / "story_en_reference" / "story_transcripts_pl.jsonl"
OUT = ROOT / "audio" / "polish" / "story_voicepack_nllb"
REFS = ROOT / "audio" / "polish"
PROFILE_REF = {
    "barisov": ("barisov_dmitri", REFS / "accent_reference" / "ru_dmitri_reference.wav"),
    "demichev": ("demichev_ruslan", REFS / "accent_reference" / "ru_ruslan_reference.wav"),
    "kathryn": ("kathryn_tetiana", REFS / "accent_reference" / "uk_tetiana_reference.wav"),
    "renko": ("renko_user", REFS / "user_reference" / "spokoj_reference.wav"),
}


def normalize(audio: np.ndarray) -> np.ndarray:
    audio = np.asarray(audio, dtype=np.float32).squeeze()
    peak = float(np.max(np.abs(audio))) if audio.size else 0.0
    if peak > 0:
        audio *= (10.0 ** (-3.0 / 20.0)) / peak
    return np.clip(audio, -1.0, 1.0)


def pieces(text: str) -> list[str]:
    text = re.sub(r"\bE99\b", "E dziewięćdziesiąt dziewięć", text, flags=re.I)
    text = re.sub(r"\bTMD\b", "te em de", text, flags=re.I)
    text = re.sub(r"\bKatorga[- ]12\b", "Katorga dwanaście", text, flags=re.I)
    text = re.sub(r"\s+", " ", text).strip()
    if len(re.findall(r"\w+", text, flags=re.UNICODE)) <= 42:
        return [text]
    return [p.strip() for p in re.split(r"(?<=[.!?])\s+", text) if p.strip()] or [text]


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--shard", type=int, default=0)
    ap.add_argument("--shards", type=int, default=1)
    args = ap.parse_args()
    rows = [json.loads(x) for x in SOURCE.read_text(encoding="utf-8").splitlines() if x.strip()]
    rows = [r for r in rows if r.get("text_pl_machine")]
    rows = [r for i, r in enumerate(rows) if i % args.shards == args.shard]
    OUT.mkdir(parents=True, exist_ok=True)
    model = TTSModel.load_model(config="hf://shefowl/pocket-tts-polish-6l/config.yaml", temp=0.55, quantize=True)
    refs: dict[str, object] = {}
    for profile, reference in {"narrator_user": REFS / "user_reference" / "spokoj_reference.wav", **{v[0]: v[1] for v in PROFILE_REF.values()}}.items():
        refs[profile] = model.get_state_for_audio_prompt(str(reference))
    manifest = OUT / f"manifest_{args.shard}.jsonl"
    with manifest.open("w", encoding="utf-8") as f:
        for index, row in enumerate(rows, 1):
            speaker = (row.get("speaker_guess") or "").lower()
            profile = PROFILE_REF.get(speaker, ("narrator_user", None))[0]
            name = hashlib.sha1(row["path"].encode("utf-8")).hexdigest()[:16] + ".wav"
            out = OUT / name
            generated = []
            for piece in pieces(row["text_pl_machine"]):
                audio = model.generate_audio(copy.deepcopy(refs[profile]), piece).numpy().squeeze()
                generated.append(normalize(audio))
            pause = np.zeros(int(model.sample_rate * 0.08), dtype=np.float32)
            audio = generated[0] if len(generated) == 1 else np.concatenate([x for p in generated for x in (p, pause)])[:-len(pause)]
            wavfile.write(str(out), model.sample_rate, (audio * 32767.0).astype(np.int16))
            f.write(json.dumps({**row, "profile": profile, "file": name, "pack": "story_nllb", "sample_rate": model.sample_rate, "generation_attempt": 1}, ensure_ascii=False) + "\n")
            if index == 1 or index % 25 == 0:
                print(f"generated {index}/{len(rows)}", flush=True)
    print("complete", flush=True)


if __name__ == "__main__":
    main()
