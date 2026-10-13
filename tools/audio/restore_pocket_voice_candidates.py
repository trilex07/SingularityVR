"""Regenerate the existing failed-line set with its original Pocket-TTS voices.

Reads the already corrected repair manifest and never edits original voicepacks.
Generated audio and manifests belong in the ignored local repair-candidate tree.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.pocket_tools")
import numpy as np
from pocket_tts import TTSModel
from scipy.io import wavfile

ROOT = Path(__file__).resolve().parents[2]
REPAIR = ROOT / "audio/polish/repair_candidate_20261013"
INPUT = REPAIR / "full_audit/manifest.jsonl"
OUT = REPAIR / "full_audit_pocket"
PROFILE_REFERENCES = {
    "narrator_user": ROOT / "audio/polish/user_reference/spokoj_reference.wav",
    "renko_user": ROOT / "audio/polish/user_reference/spokoj_reference.wav",
    "barisov_dmitri": ROOT / "audio/polish/accent_reference/ru_dmitri_reference.wav",
    "demichev_ruslan": ROOT / "audio/polish/accent_reference/ru_ruslan_reference.wav",
    "kathryn_tetiana": ROOT / "audio/polish/accent_reference/uk_tetiana_reference.wav",
    "devlin_mykyta": ROOT / "audio/polish/accent_reference/uk_mykyta_reference.wav",
}


def pieces(text: str) -> list[str]:
    text = re.sub(r"\s+", " ", text).strip()
    word_count = len(re.findall(r"\w+", text, flags=re.UNICODE))
    if word_count <= 14:
        return [text]
    # Pocket-TTS can drop words on long unsegmented strings. Split only at
    # natural punctuation, retaining the punctuation with each spoken piece.
    parts = [part.strip() for part in re.split(r"(?<=[.!?;,:])\s+", text) if part.strip()]
    if len(parts) <= 1 and word_count > 32:
        parts = [part.strip() for part in re.split(r"(?<=\s—)\s*|\s+(?=and\b|or\b|but\b|aby\b|żeby\b)", text, flags=re.I) if part.strip()]
    return parts or [text]


def normalize(audio: np.ndarray) -> np.ndarray:
    audio = np.asarray(audio, dtype=np.float32).squeeze()
    peak = float(np.max(np.abs(audio))) if audio.size else 0.0
    if peak:
        audio *= (10.0 ** (-3.0 / 20.0)) / peak
    return np.clip(audio, -1.0, 1.0)


def trim_padding(audio: np.ndarray, sample_rate: int) -> np.ndarray:
    """Remove only silent model padding; retain a short natural release tail."""
    active = np.flatnonzero(np.abs(audio) >= 0.002)
    if not active.size:
        return audio
    start = max(0, int(active[0]) - int(0.03 * sample_rate))
    end = min(len(audio), int(active[-1]) + int(0.07 * sample_rate))
    trimmed = audio[start:end].copy()
    fade = min(int(0.005 * sample_rate), len(trimmed) // 2)
    if fade:
        trimmed[:fade] *= np.linspace(0.0, 1.0, fade, dtype=np.float32)
        trimmed[-fade:] *= np.linspace(1.0, 0.0, fade, dtype=np.float32)
    return trimmed


def trim_pcm16_padding(audio: np.ndarray, sample_rate: int) -> np.ndarray:
    """Apply the silence limits after PCM16 quantization, matching automated QA."""
    active = np.flatnonzero(np.abs(audio.astype(np.int32)) >= 66)  # >= 0.002 at PCM16
    if not active.size:
        return audio
    start = max(0, int(active[0]) - int(0.03 * sample_rate))
    end = min(len(audio), int(active[-1]) + int(0.07 * sample_rate))
    trimmed = audio[start:end].astype(np.float32)
    fade = min(int(0.005 * sample_rate), len(trimmed) // 2)
    if fade:
        trimmed[:fade] *= np.linspace(0.0, 1.0, fade, dtype=np.float32)
        trimmed[-fade:] *= np.linspace(1.0, 0.0, fade, dtype=np.float32)
    return np.asarray(np.clip(np.rint(trimmed), -32768, 32767), dtype=np.int16)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--force", action="store_true", help="regenerate existing Pocket candidates")
    parser.add_argument("--trim-existing", action="store_true",
                        help="trim only model-added silence from the already generated Pocket set")
    parser.add_argument("--output-dir", type=Path, default=OUT,
                        help="local staging directory for a separate candidate variant")
    parser.add_argument("--retry-qa", type=Path,
                        help="regenerate only line IDs flagged by this QA JSON, retaining other outputs")
    parser.add_argument("--retry-pack", action="append", default=[],
                        help="regenerate every row in this pack (repeatable; retains other outputs)")
    parser.add_argument("--retry-line-id", action="append", default=[],
                        help="regenerate this exact line ID (repeatable; retains other outputs)")
    args = parser.parse_args()
    output_dir = args.output_dir.resolve()
    if not INPUT.is_file():
        raise SystemExit(f"Missing existing repair manifest: {INPUT}")
    rows = [json.loads(line) for line in INPUT.read_text(encoding="utf-8").splitlines() if line.strip()]
    if len(rows) != 205:
        raise SystemExit(f"Expected the preserved 205-row failure set, found {len(rows)}")
    if args.trim_existing:
        manifest_path = output_dir / "manifest.jsonl"
        if not manifest_path.is_file():
            raise SystemExit(f"No Pocket candidate manifest at {manifest_path}")
        candidates = [json.loads(line) for line in manifest_path.read_text(encoding="utf-8").splitlines()
                      if line.strip()]
        if len(candidates) != 205:
            raise SystemExit(f"Expected 205 generated candidates, found {len(candidates)}")
        for item in candidates:
            path = Path(item["file"])
            rate, audio = wavfile.read(path)
            if rate != 24000 or audio.dtype != np.int16 or audio.ndim != 1:
                raise SystemExit(f"Unexpected WAV format at {path}: {rate} Hz, {audio.dtype}, shape={audio.shape}")
            trimmed = trim_pcm16_padding(audio, rate)
            wavfile.write(str(path), rate, trimmed)
            item["generated_duration_s"] = round(len(trimmed) / rate, 3)
            item["edge_padding_trimmed_after_pcm16"] = True
        manifest_path.write_text("\n".join(json.dumps(item, ensure_ascii=False) for item in candidates) + "\n",
                                encoding="utf-8")
        print(json.dumps({"trimmed": len(candidates), "out": str(OUT.resolve())}, ensure_ascii=False))
        return
    for row in rows:
        profile = row.get("profile_before")
        ref = PROFILE_REFERENCES.get(profile)
        if ref is None or not ref.is_file():
            raise SystemExit(f"Missing authorized reference for {profile}: {ref}")
        if not Path(row["previous_audio"]).is_file():
            raise SystemExit(f"Missing previous candidate for {row['line_id']}")
        if not row.get("tts_text"):
            raise SystemExit(f"Missing preserved corrected TTS text for {row['line_id']}")

    output_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = output_dir / "manifest.jsonl"
    existing = {}
    if manifest_path.is_file() and not args.force:
        for line in manifest_path.read_text(encoding="utf-8").splitlines():
            if not line.strip():
                continue
            item = json.loads(line)
            local_copy = output_dir / Path(item["file"]).name
            if local_copy.is_file():
                item["file"] = str(local_copy)
            existing[item["line_id"]] = item

    retry_ids = set()
    if args.retry_qa:
        qa_rows = json.loads(args.retry_qa.read_text(encoding="utf-8"))
        retry_ids = {row["line_id"] for row in qa_rows if row.get("qa_flags")}
        if not retry_ids:
            raise SystemExit(f"No flagged lines in QA file: {args.retry_qa}")
    retry_ids.update(row["line_id"] for row in rows if row["pack"] in set(args.retry_pack))
    retry_ids.update(args.retry_line_id)
    unknown_retry_ids = retry_ids - {row["line_id"] for row in rows}
    if unknown_retry_ids:
        raise SystemExit(f"Unknown requested line IDs: {sorted(unknown_retry_ids)}")
    selected_rows = [row for row in rows if not retry_ids or row["line_id"] in retry_ids]
    completed = [existing[row["line_id"]] for row in rows
                 if row["line_id"] in existing and row["line_id"] not in retry_ids]

    # Match the source pipeline's settings by pack: NLLB story was generated at
    # temp=.55; localization/UI was generated at temp=.45; both used quantization.
    models = {
        "story_nllb": TTSModel.load_model(
            config="hf://shefowl/pocket-tts-polish-6l/config.yaml", temp=0.55, quantize=True),
        "full_localization": TTSModel.load_model(
            config="hf://shefowl/pocket-tts-polish-6l/config.yaml", temp=0.45, quantize=True),
    }
    states = {pack: {} for pack in models}
    for pack, model in models.items():
        for profile, ref in PROFILE_REFERENCES.items():
            if any(row.get("profile_before") == profile and
                   (row["pack"] == pack or row["line_id"] in retry_ids) for row in rows):
                # Prompt states are built by the same model instance that will
                # synthesize them; do not pass state across separately loaded models.
                states[pack][profile] = model.get_state_for_audio_prompt(str(ref))

    for index, row in enumerate(selected_rows, 1):
        old = existing.get(row["line_id"])
        outname = hashlib.sha1((row["pack"] + row["line_id"]).encode()).hexdigest()[:16] + ".wav"
        outpath = output_dir / outname
        if old and row["line_id"] not in retry_ids and Path(old.get("file", "")).is_file():
            completed.append(old)
            continue
        profile = row["profile_before"]
        model = models["story_nllb" if row["pack"] == "story_nllb" else "full_localization"]
        audio_pieces = [normalize(model.generate_audio(copy.deepcopy(states[row["pack"]][profile]), part).numpy())
                        for part in pieces(row["tts_text"])]
        pause = np.zeros(int(model.sample_rate * 0.08), dtype=np.float32)
        audio = (audio_pieces[0] if len(audio_pieces) == 1 else
                 np.concatenate([part for piece in audio_pieces for part in (piece, pause)])[:-len(pause)])
        audio = trim_padding(audio, model.sample_rate)
        pcm16 = np.asarray(np.clip(audio, -1.0, 1.0) * 32767.0, dtype=np.int16)
        pcm16 = trim_pcm16_padding(pcm16, model.sample_rate)
        wavfile.write(str(outpath), model.sample_rate, pcm16)
        item = {
            **row,
            "file": str(outpath.resolve()),
            "voice_engine": "Pocket-TTS Polish 6L",
            "reference_profile": profile,
            "reference_file": str(PROFILE_REFERENCES[profile].resolve()),
            "generation_temperature": 0.55 if row["pack"] == "story_nllb" else 0.45,
            "quantize": True,
            "sample_rate": model.sample_rate,
            "generated_duration_s": round(len(audio) / model.sample_rate, 3),
        }
        completed.append(item)
        if index == 1 or index % 10 == 0:
            print(f"Pocket-TTS regenerated {index}/{len(selected_rows)}", flush=True)
        # Persist each result so an interrupted batch resumes safely.
        manifest_path.write_text("\n".join(json.dumps(x, ensure_ascii=False) for x in completed) + "\n",
                                encoding="utf-8")

    by_id = {item["line_id"]: item for item in completed}
    ordered = [by_id[row["line_id"]] for row in rows if row["line_id"] in by_id]
    manifest_path.write_text("\n".join(json.dumps(x, ensure_ascii=False) for x in ordered) + "\n",
                            encoding="utf-8")
    print(json.dumps({"rows": len(ordered), "out": str(output_dir),
                      "profiles": {p: sum(x['profile_before'] == p for x in rows)
                                   for p in sorted({x['profile_before'] for x in rows})}}, ensure_ascii=False))


if __name__ == "__main__":
    main()
