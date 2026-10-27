"""Create stable Polish fallback audio only for lines that failed Whisper QA.

Pocket-TTS attempts remain untouched. Piper is used as an explicit fallback,
with the selected Piper profile recorded in the manifest for review.
"""
from __future__ import annotations

import io
import json
import re
import sys
import wave
from pathlib import Path

sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.piper_tools")
sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.pocket_tools")
import numpy as np  # noqa: E402
from scipy.io import wavfile  # noqa: E402
from scipy.signal import resample_poly  # noqa: E402
from piper import PiperVoice  # noqa: E402

ROOT = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-cutscene")
REPORT = ROOT / "research" / "localization"
VOICE_ROOT = ROOT / "audio" / "models" / "piper"
VOICES = {
    "darkman": VOICE_ROOT / "pl_PL-darkman-medium" / "pl_PL-darkman-medium.onnx",
    "mc_speech": VOICE_ROOT / "pl_PL-mc_speech-medium" / "pl_PL-mc_speech-medium.onnx",
}


def spoken(text: str) -> str:
    text = (text or "").replace("`~", " ").replace("\\n", " ")
    text = re.sub(r"\bE99\b", "E dziewięćdziesiąt dziewięć", text, flags=re.I)
    text = re.sub(r"\bTMD\b", "te em de", text, flags=re.I)
    text = re.sub(r"\bKatorga[- ]12\b", "Katorga dwanaście", text, flags=re.I)
    text = re.sub(r"Demichew", "Demichev", text, flags=re.I)
    text = re.sub(r"\s+", " ", text).strip()
    return text


def load_failed(report_file: Path, pack: str) -> list[dict]:
    data = json.loads(report_file.read_text(encoding="utf-8"))
    return [r for r in data["results"] if r.get("pack") == pack and r.get("qa_status") in {"FAIL_LANGUAGE", "FAIL_GIBBERISH", "FAIL_TRUNCATED", "FAIL_AUDIO", "FAIL_TRANSLATION"}]


def main() -> None:
    rows = load_failed(REPORT / "polish_dubbing_qa_nllb.json", "story_nllb")
    rows += load_failed(REPORT / "polish_dubbing_qa.json", "full_localization")
    unique = {(r["pack"], r["line_id"]): r for r in rows}
    rows = list(unique.values())
    voices = {name: PiperVoice.load(str(path)) for name, path in VOICES.items()}
    output_manifest = REPORT / "piper_fallback_manifest.jsonl"
    output_manifest.parent.mkdir(parents=True, exist_ok=True)
    with output_manifest.open("w", encoding="utf-8") as f:
        for index, row in enumerate(rows, 1):
            original = Path(row["audio_path"])
            # Keep all retries beside the corresponding pack, never overwrite
            # an original before the fallback has passed a second QA run.
            base = original.parent
            while base.name == "attempt2" or base.name == "qa_attempts":
                base = base.parent
            out_dir = base / "qa_attempts" / "piper"
            out_dir.mkdir(parents=True, exist_ok=True)
            out_path = out_dir / original.name
            profile = row.get("profile", "narrator_user")
            voice_name = "darkman" if profile == "demichev_ruslan" else "mc_speech"
            text = spoken(row.get("expected_pl", ""))
            try:
                if out_path.exists() and out_path.stat().st_size > 0:
                    f.write(json.dumps({
                        "pack": row["pack"], "line_id": row["line_id"],
                        "source_file": row.get("source") or row.get("id"),
                        "original_audio_path": str(original), "file": str(out_path),
                        "profile_before": profile, "profile_fallback": f"piper_{voice_name}",
                        "text_display": row.get("expected_pl", ""), "text_tts": text,
                        "previous_qa_status": row.get("qa_status"), "reused": True,
                    }, ensure_ascii=False) + "\n")
                    continue
                buf = io.BytesIO()
                with wave.open(buf, "wb") as wav:
                    voices[voice_name].synthesize_wav(text, wav)
                buf.seek(0)
                rate, audio = wavfile.read(buf)
                audio = np.asarray(audio)
                if audio.ndim == 2:
                    audio = audio.mean(axis=1)
                if np.issubdtype(audio.dtype, np.integer):
                    audio = audio.astype(np.float32) / np.iinfo(audio.dtype).max
                else:
                    audio = audio.astype(np.float32)
                if rate != 24000:
                    audio = resample_poly(audio, 24000, rate)
                audio = np.clip(audio, -1, 1)
                wavfile.write(str(out_path), 24000, (audio * 32767).astype(np.int16))
                f.write(json.dumps({
                    "pack": row["pack"],
                    "line_id": row["line_id"],
                    "source_file": row.get("source") or row.get("id"),
                    "original_audio_path": str(original),
                    "file": str(out_path),
                    "profile_before": profile,
                    "profile_fallback": f"piper_{voice_name}",
                    "text_display": row.get("expected_pl", ""),
                    "text_tts": text,
                    "previous_qa_status": row.get("qa_status"),
                }, ensure_ascii=False) + "\n")
            except Exception as exc:
                f.write(json.dumps({"pack": row["pack"], "line_id": row["line_id"], "error": str(exc)}, ensure_ascii=False) + "\n")
            if index == 1 or index % 50 == 0:
                print(f"generated {index}/{len(rows)}", flush=True)
    print(f"complete {len(rows)}", flush=True)


if __name__ == "__main__":
    main()
