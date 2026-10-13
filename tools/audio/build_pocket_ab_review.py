"""Make a Polish-only A/B reel: Polish voice sample, previous, Pocket-TTS repair."""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.pocket_tools")
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly

ROOT = Path(__file__).resolve().parents[2]
REPAIR = ROOT / "audio/polish/repair_candidate_20261013"
REFERENCES = {
    # These are finished Polish character-sample renders. The Russian and
    # Ukrainian WAVs used to condition Pocket-TTS are intentionally excluded
    # from a Polish listening reel: they are not Polish dialogue examples.
    "narrator_user": ROOT / "audio/polish/character_samples_12/01_renko_neutral.wav",
    "renko_user": ROOT / "audio/polish/character_samples_12/01_renko_neutral.wav",
    "barisov_dmitri": ROOT / "audio/polish/character_samples_12/03_barisov_command.wav",
    "demichev_ruslan": ROOT / "audio/polish/character_samples_12/06_demichev_cold.wav",
    "kathryn_tetiana": ROOT / "audio/polish/character_samples_12/09_kathryn_scientific.wav",
    "devlin_mykyta": ROOT / "audio/polish/character_samples_12/11_devlin_restrained.wav",
}
FFMPEG = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-polish-integration\tools\ffmpeg\ffmpeg-9.0.2-essentials_build\bin\ffmpeg.exe")


def load_mono(path: Path) -> np.ndarray:
    try:
        rate, audio = wavfile.read(path)
    except ValueError:
        # Some historical full_voicepack entries have MPEG audio bytes despite
        # their .wav suffix. Decode them for comparison without changing source.
        if not FFMPEG.is_file():
            raise RuntimeError(f"Non-WAV previous candidate needs local ffmpeg: {path}")
        decoded = subprocess.run(
            [str(FFMPEG), "-v", "error", "-i", str(path), "-f", "s16le", "-acodec", "pcm_s16le",
             "-ac", "1", "-ar", "24000", "pipe:1"], check=True, capture_output=True)
        return np.frombuffer(decoded.stdout, dtype=np.int16).astype(np.float32) / 32768.0
    audio = np.asarray(audio)
    if audio.ndim == 2:
        audio = audio.astype(np.float32).mean(axis=1)
    if np.issubdtype(audio.dtype, np.integer):
        audio = audio.astype(np.float32) / np.iinfo(audio.dtype).max
    else:
        audio = audio.astype(np.float32)
    if rate != 24000:
        audio = resample_poly(audio, 24000, rate)
    audio = np.nan_to_num(audio)
    peak = float(np.max(np.abs(audio))) if audio.size else 0.0
    if peak > 0:
        audio *= (10 ** (-3 / 20)) / peak
    return np.clip(audio, -1.0, 1.0)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--candidate-dir", type=Path, default=REPAIR / "full_audit_pocket")
    ap.add_argument("--output", type=Path, default=REPAIR / "pocket_voice_ab_review_20261013.wav")
    args = ap.parse_args()
    candidate_dir = args.candidate_dir.resolve()
    qa_by_id = {}
    qa_paths = [candidate_dir / name for name in ("qa.json", "qa_base.json", "qa_small.json")
                if (candidate_dir / name).is_file()]
    for qa_path in qa_paths:
        for row in json.loads(qa_path.read_text(encoding="utf-8")):
            current = qa_by_id.get(row["line_id"], row)
            current["qa_flags"] = sorted(set(current.get("qa_flags", [])) | set(row.get("qa_flags", [])))
            qa_by_id[row["line_id"]] = current
    qa = list(qa_by_id.values())
    audio_manifest = {x["line_id"]: x for x in
                     (json.loads(line) for line in (candidate_dir / "manifest.jsonl").read_text(encoding="utf-8").splitlines() if line.strip())}

    # Include every flagged Pocket output, then one clean output per profile and
    # pack where available. This covers all original voices and the UI pipeline.
    selected: dict[tuple[str, str], dict] = {}
    for row in qa:
        if row.get("qa_flags"):
            selected[(row["pack"], row["line_id"])] = row
    seen_clean = set()
    for row in qa:
        if row.get("qa_flags"):
            continue
        profile = row.get("profile_before", "unknown")
        key = (row["pack"], profile)
        if key not in seen_clean:
            selected[(row["pack"], row["line_id"])] = row
            seen_clean.add(key)

    chunks: list[np.ndarray] = []
    marks: list[dict] = []
    cursor = 0
    gap = np.zeros(24000 // 2, dtype=np.float32)

    def append(label: str, path: Path, **metadata: object) -> None:
        nonlocal cursor
        audio = load_mono(path)
        start = cursor / 24000
        chunks.append(audio)
        cursor += len(audio)
        marks.append({"label": label, "start_s": round(start, 3),
                      "end_s": round(cursor / 24000, 3),
                      "source_file": str(path.resolve()), **metadata})
        chunks.append(gap)
        cursor += len(gap)

    profiles = sorted({row.get("profile_before", "unknown") for row in selected.values()})
    for profile in profiles:
        ref = REFERENCES.get(profile)
        if ref and ref.is_file():
            append(f"REFERENCE {profile}", ref, profile=profile, kind="reference")
    for (pack, line_id), row in selected.items():
        entry = audio_manifest[line_id]
        append(f"PREVIOUS {pack} {line_id}", Path(entry["previous_audio"]),
               pack=pack, line_id=line_id, profile=row.get("profile_before"),
               kind="previous", qa_flags=row.get("qa_flags", []))
        append(f"POCKET {pack} {line_id}", Path(entry["file"]),
               pack=pack, line_id=line_id, profile=row.get("profile_before"),
               kind="pocket_candidate", qa_flags=row.get("qa_flags", []))

    args.output.parent.mkdir(parents=True, exist_ok=True)
    wavfile.write(str(args.output), 24000,
                  np.asarray(np.clip(np.concatenate(chunks), -1, 1) * 32767, dtype=np.int16))
    sidecar = args.output.with_suffix(".json")
    sidecar.write_text(json.dumps({"sample_rate": 24000, "channels": 1,
                                   "duration_s": round(cursor / 24000, 3),
                                   "segments": marks}, ensure_ascii=False, indent=2), encoding="utf-8")
    guide = args.output.with_suffix(".md")
    guide.write_text("# Pocket-TTS dubbing A/B review\n\n"
                     "All voice samples and dialogue in this reel are Polish. The first clips are existing Polish character samples (not the Russian/Ukrainian conditioning WAVs); listen to each `PREVIOUS` / `POCKET` pair.\n\n"
                     + "\n".join(f"- {m['start_s']:.2f}–{m['end_s']:.2f}s — {m['label']}"
                                  for m in marks) + "\n", encoding="utf-8")
    print(json.dumps({"segments": len(marks), "duration_s": round(cursor / 24000, 2),
                      "output": str(args.output.resolve()), "index": str(sidecar.resolve()),
                      "guide": str(guide.resolve())}, ensure_ascii=False))


if __name__ == "__main__":
    main()
