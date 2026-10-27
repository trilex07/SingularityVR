"""Regenerate only automated QA failures with fresh per-line conditioning.

Attempt 1 is never overwritten. Attempt 2 is written under qa_attempts and
is promoted only after a second full QA pass confirms the mapping and format.
"""
from __future__ import annotations

import json
import re
import shutil
import sys
import argparse
import copy
from pathlib import Path

sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.pocket_tools")
import numpy as np
from scipy.io import wavfile
from pocket_tts import TTSModel  # noqa: E402


ROOT = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-cutscene")
QA = ROOT / "research" / "localization" / "polish_dubbing_qa.json"
REFS = ROOT / "audio" / "polish"

REFERENCE_BY_PROFILE = {
    "narrator_user": REFS / "user_reference" / "spokoj_reference.wav",
    "renko_user": REFS / "user_reference" / "spokoj_reference.wav",
    "barisov_dmitri": REFS / "accent_reference" / "ru_dmitri_reference.wav",
    "demichev_ruslan": REFS / "accent_reference" / "ru_ruslan_reference.wav",
    "kathryn_tetiana": REFS / "accent_reference" / "uk_tetiana_reference.wav",
    "devlin_mykyta": REFS / "accent_reference" / "uk_mykyta_reference.wav",
}


def speech_text(text: str) -> str:
    """Keep display text unchanged while making common technical tokens speakable."""
    text = (text or "").replace("`~", " ").replace("\\n", " ")
    text = re.sub(r"\bE99\b", "E dziewięćdziesiąt dziewięć", text, flags=re.I)
    text = re.sub(r"\bTMD\b", "te em de", text, flags=re.I)
    text = re.sub(r"\bKatorga[- ]12\b", "Katorga dwanaście", text, flags=re.I)
    text = re.sub(r"\s+", " ", text).strip()
    return text


def chunks(text: str) -> list[str]:
    # Long machine translations frequently hit the model chunk limit. Generate
    # sentence-sized pieces with a fresh state and join them with a short pause.
    if len(re.findall(r"\w+", text, flags=re.UNICODE)) <= 42:
        return [text]
    parts = [p.strip() for p in re.split(r"(?<=[.!?])\s+", text) if p.strip()]
    return parts or [text]


def normalize(audio: np.ndarray) -> np.ndarray:
    audio = np.asarray(audio, dtype=np.float32).squeeze()
    peak = float(np.max(np.abs(audio))) if audio.size else 0.0
    if peak > 0:
        audio *= (10.0 ** (-3.0 / 20.0)) / peak
    return np.clip(audio, -1.0, 1.0)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--shard", type=int, default=0)
    ap.add_argument("--shards", type=int, default=1)
    args = ap.parse_args()
    qa = json.loads(QA.read_text(encoding="utf-8"))
    failed = [r for r in qa["results"] if r.get("qa_status") in {"FAIL_LANGUAGE", "FAIL_GIBBERISH", "FAIL_TRUNCATED", "FAIL_AUDIO"}]
    failed = [row for i, row in enumerate(failed) if i % args.shards == args.shard]
    model = TTSModel.load_model(
        config="hf://shefowl/pocket-tts-polish-6l/config.yaml",
        temp=0.55,
        quantize=True,
    )
    attempt_rows = []
    base_states: dict[str, object] = {}
    for profile, reference in REFERENCE_BY_PROFILE.items():
        if reference.exists():
            base_states[profile] = model.get_state_for_audio_prompt(str(reference))
    for index, row in enumerate(failed, 1):
        profile = row.get("profile", "narrator_user")
        reference = REFERENCE_BY_PROFILE.get(profile)
        if not reference or not reference.exists():
            row["retry_error"] = f"missing reference for {profile}"
            attempt_rows.append(row)
            continue
        original = Path(row["audio_path"])
        out_dir = original.parent / "qa_attempts" / "attempt2"
        out_dir.mkdir(parents=True, exist_ok=True)
        output = out_dir / original.name
        if output.exists() and output.stat().st_size > 0:
            retry = dict(row)
            retry["attempt"] = 2
            retry["audio_path"] = str(output)
            retry["original_audio_path"] = str(original)
            attempt_rows.append(retry)
            continue
        expected = speech_text(row.get("expected_pl", ""))
        try:
            pieces = []
            for piece in chunks(expected):
                # Crucial fix: get a fresh conditioning state for every line
                # (and every long-line sentence), avoiding cross-line language
                # and prompt leakage observed in the controlled experiment.
                # Deep-copying a pristine state is materially faster than
                # re-encoding the reference for every line, while preserving
                # the isolation proven by the controlled test.
                state = copy.deepcopy(base_states[profile])
                generated = model.generate_audio(state, piece).numpy().squeeze().astype(np.float32)
                pieces.append(normalize(generated))
            pause = np.zeros(int(model.sample_rate * 0.08), dtype=np.float32)
            audio = pieces[0] if len(pieces) == 1 else np.concatenate([x for p in pieces for x in (p, pause)])[:-len(pause)]
            wavfile.write(str(output), model.sample_rate, (np.clip(audio, -1, 1) * 32767.0).astype(np.int16))
            retry = dict(row)
            retry["attempt"] = 2
            retry["audio_path"] = str(output)
            retry["tts_text_attempt2"] = expected
            retry["original_audio_path"] = str(original)
            attempt_rows.append(retry)
        except Exception as exc:
            retry = dict(row)
            retry["attempt"] = 2
            retry["retry_error"] = str(exc)
            attempt_rows.append(retry)
        if index == 1 or index % 25 == 0:
            print(f"regenerated {index}/{len(failed)}", flush=True)

    out = ROOT / "research" / "localization" / f"polish_dubbing_retry_manifest_{args.shard}.json"
    out.write_text(json.dumps({"attempt": 2, "shard": args.shard, "shards": args.shards, "count": len(attempt_rows), "rows": attempt_rows}, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"complete {len(attempt_rows)}", flush=True)


if __name__ == "__main__":
    main()
