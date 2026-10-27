"""Restore UI/menu/loading narrator from spokoj_reference; keep intro voice elsewhere.

Reads the frozen assembled_reviewed candidate, replaces only full_localization
narrator_user rows. Unchanged synthesis text reuses full_voicepack audio;
changed text is regenerated locally with Pocket-TTS + spokoj_reference.wav.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import sys
import wave
from pathlib import Path

sys.path.insert(0, r"C:\Users\Trilex\vr-mods\.pocket_tools")
sys.path.insert(0, str(Path(__file__).resolve().parent))

import numpy as np
from pocket_tts import TTSModel
from scipy.io import wavfile
from scipy.signal import resample_poly

from generate_intro_narrator_full_candidate import (
    CORRECTIONS,
    TEXT_OVERRIDES,
    clean_localization_text,
    pcm16_reference,
    spoken_pieces,
    trim_model_padding,
)
from tts_pronunciation import to_spoken_polish

ROOT = Path(__file__).resolve().parents[2]
AUDIO = ROOT / "audio/polish"
BASE_MANIFEST = (
    AUDIO
    / "repair_candidate_20261013/full_revoice_final_pass/assembled_reviewed/voicepack/manifest.jsonl"
)
BASE_VOICEPACK = BASE_MANIFEST.parent
SPOKOJ = AUDIO / "user_reference/spokoj_reference.wav"
INTRO_REF = AUDIO / "repair_candidate_20261013/intro_reference_experiment/intro_narrator_reference.wav"
FULL_VOICE_MANIFEST = AUDIO / "full_voicepack/manifest.jsonl"
DEFAULT_OUT = AUDIO / "repair_candidate_20261013/narrator_ui_lektor_20261014"
FFMPEG = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-polish-integration\tools\ffmpeg\ffmpeg-9.0.2-essentials_build\bin\ffmpeg.exe")

PILOT_LINE_IDS = [
    r"..\Engine\Localization\INT\Engine.int|Progress|Loading",
    r"..\Engine\Localization\INT\Engine.int|Progress|Saving",
    r"..\RvGame\Localization\INT\Subtitles.int|Subtitles|Loading",
    r"..\RvGame\Localization\INT\RvGame.int|General|LevelIntroProcessing",
    r"..\RvGame\Localization\INT\RvGame.int|RvUI3D|ConsoleControls",
    r"..\RvGame\Localization\INT\RvGame.int|Achievements|kills_impulse_DESC",
    r"..\RvGame\Localization\INT\MP.int|General|PlayOnline",
    r"..\RvGame\Localization\INT\MP.int|MPPreGame|GameOptions",
    r"..\Engine\Localization\INT\Core.int|General|Yes",
    r"..\Engine\Localization\INT\Core.int|General|No",
    r"..\RvGame\Localization\INT\RvGame.int|RvUI|MPHitSound",
    r"..\RvGame\Localization\INT\RvGame.int|RvUI|ScienceToWeaponAnnounce",
]


def read_jsonl(path: Path) -> list[dict]:
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]


def is_ui_narrator(row: dict) -> bool:
    return row.get("pack") == "full_localization" and row.get("profile_before") == "narrator_user"


def stage_file(source: Path, target: Path) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists():
        return
    try:
        os.link(source, target)
    except OSError:
        shutil.copy2(source, target)


def is_pcm_wav(path: Path) -> bool:
    try:
        with wave.open(str(path), "rb") as wf:
            wf.getnframes()
        return True
    except wave.Error:
        return False


def validate_wav(path: Path) -> dict:
    with wave.open(str(path), "rb") as wf:
        return {
            "rate": wf.getframerate(),
            "channels": wf.getnchannels(),
            "sample_width": wf.getsampwidth(),
            "frames": wf.getnframes(),
        }


def normalize_peak(audio: np.ndarray, peak_dbfs: float = -3.0) -> np.ndarray:
    peak = float(np.max(np.abs(audio))) if audio.size else 0.0
    if peak <= 0:
        return audio
    return np.clip(audio * ((10 ** (peak_dbfs / 20)) / peak), -1.0, 1.0)


def synthesize_line(
    model: TTSModel,
    state,
    text: str,
    *,
    reference_label: str,
    temperature: float,
) -> np.ndarray:
    parts = spoken_pieces(text)
    pause = np.zeros(int(model.sample_rate * 0.08), dtype=np.float32)
    rendered = [
        model.generate_audio(state, part, max_tokens=100, frames_after_eos=4)
        .numpy()
        .squeeze()
        .astype(np.float32)
        for part in parts
    ]
    audio = np.concatenate(
        [
            piece
            for i, piece in enumerate(rendered)
            for piece in ((piece, pause) if i < len(rendered) - 1 else (piece,))
        ]
    )
    audio = normalize_peak(audio)
    return trim_model_padding(audio, model.sample_rate)


def write_wav(path: Path, audio: np.ndarray, rate: int = 24000) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    wavfile.write(
        str(path),
        rate,
        np.asarray(np.rint(np.clip(audio, -1.0, 1.0) * 32767.0), dtype=np.int16),
    )


def legacy_tts_for_row(fv_row: dict) -> str:
    return clean_localization_text(
        fv_row.get("pl", "") or fv_row.get("tts_text", "") or ""
    )


def spoken_for_assembled_row(row: dict) -> str:
    return row.get("spoken_tts_text") or row.get("tts_text") or row.get("corrected_polish") or ""


def classify_ui_rows(base_rows: list[dict], fv_by_id: dict[str, dict]) -> tuple[list[dict], list[dict]]:
    restore: list[dict] = []
    regen: list[dict] = []
    for row in base_rows:
        if not is_ui_narrator(row):
            continue
        lid = row["line_id"]
        fv = fv_by_id.get(lid)
        if not fv:
            regen.append(row)
            continue
        old_spoken = legacy_tts_for_row(fv).strip()
        new_spoken = spoken_for_assembled_row(row).strip()
        prev_path = AUDIO / "full_voicepack" / fv.get("file", "")
        if old_spoken == new_spoken and prev_path.is_file() and is_pcm_wav(prev_path):
            restore.append(row)
        else:
            regen.append(row)
    return restore, regen


def run_regeneration(
    rows: list[dict],
    out_dir: Path,
    *,
    reference: Path,
    temperature: float,
    profile_label: str,
    force: bool,
) -> list[dict]:
    if not rows:
        return []
    ref_pcm = pcm16_reference(reference, out_dir / "references", profile_label)
    model = TTSModel.load_model(
        config="hf://shefowl/pocket-tts-polish-6l/config.yaml",
        temp=temperature,
        quantize=True,
    )
    state = model.get_state_for_audio_prompt(str(ref_pcm))
    manifest_path = out_dir / "manifest.jsonl"
    existing = {r["line_id"]: r for r in read_jsonl(manifest_path)} if manifest_path.is_file() else {}
    outputs: list[dict] = []
    for index, row in enumerate(rows, 1):
        lid = row["line_id"]
        if not force and lid in existing and Path(existing[lid]["file"]).is_file():
            outputs.append(existing[lid])
            continue
        text = spoken_for_assembled_row(row)
        audio = synthesize_line(model, state, text, reference_label=profile_label, temperature=temperature)
        key = f"full_localization\0{lid}\0{profile_label}\0{temperature}"
        filename = hashlib.sha1(key.encode("utf-8")).hexdigest()[:16] + ".wav"
        path = out_dir / filename
        write_wav(path, audio, model.sample_rate)
        item = {
            "line_id": lid,
            "file": str(path.resolve()),
            "reference_file": str(ref_pcm.resolve()),
            "voice_engine": "Pocket-TTS Polish 6L",
            "generation_temperature": temperature,
            "spoken_tts_text": text,
            "source_row_file": row.get("file"),
            "previous_audio_intro_voice": row.get("file"),
            "restored_from_full_voicepack": row.get("previous_audio"),
        }
        existing[lid] = item
        outputs.append(item)
        manifest_path.write_text(
            "\n".join(json.dumps(x, ensure_ascii=False) for x in existing.values()) + "\n",
            encoding="utf-8",
        )
        if index == 1 or index % 20 == 0:
            print(f"regenerated {index}/{len(rows)}", flush=True)
    return outputs


def assemble_voicepack(
    base_rows: list[dict],
    out_voicepack: Path,
    restore_rows: list[dict],
    regen_map: dict[str, dict],
    decision_manifest: Path,
) -> list[dict]:
    out_voicepack.mkdir(parents=True, exist_ok=True)
    refs_src = BASE_VOICEPACK / "references"
    if refs_src.is_dir():
        shutil.copytree(refs_src, out_voicepack / "references", dirs_exist_ok=True)
    decisions: list[dict] = []
    staged: list[dict] = []
    for row in base_rows:
        new_row = dict(row)
        target_name = Path(row["file"]).name
        target = out_voicepack / target_name
        action = "unchanged_intro_candidate"
        if is_ui_narrator(row):
            lid = row["line_id"]
            if lid in regen_map:
                source = Path(regen_map[lid]["file"])
                action = "regenerated_spokoj"
                ref = regen_map[lid].get("reference_file")
                temp = regen_map[lid].get("generation_temperature")
            else:
                source = Path(row["previous_audio"])
                action = "restored_full_voicepack_spokoj"
                ref = str(SPOKOJ.resolve())
                temp = 0.45
            stage_file(source, target)
            decisions.append(
                {
                    "line_id": lid,
                    "action": action,
                    "source_wav": str(source.resolve()),
                    "target_wav": str(target.resolve()),
                    "intro_voice_wav": row.get("file"),
                    "reference_file": ref,
                    "generation_temperature": temp,
                }
            )
        else:
            stage_file(Path(row["file"]), target)
        new_row["file"] = str(target.resolve())
        new_row["narrator_ui_restore"] = action
        staged.append(new_row)
    decision_manifest.parent.mkdir(parents=True, exist_ok=True)
    decision_manifest.write_text(
        "\n".join(json.dumps(x, ensure_ascii=False) for x in decisions) + "\n",
        encoding="utf-8",
    )
    write_jsonl(out_voicepack / "manifest.jsonl", staged)
    return staged


def write_jsonl(path: Path, rows: list[dict]) -> None:
    path.write_text("\n".join(json.dumps(r, ensure_ascii=False) for r in rows) + "\n", encoding="utf-8")


def load_mono_normalized(path: Path, peak_dbfs: float = -3.0) -> np.ndarray:
    try:
        rate, data = wavfile.read(str(path))
    except ValueError:
        if not FFMPEG.is_file():
            raise
        import subprocess

        decoded = subprocess.run(
            [
                str(FFMPEG),
                "-v",
                "error",
                "-i",
                str(path),
                "-f",
                "s16le",
                "-acodec",
                "pcm_s16le",
                "-ac",
                "1",
                "-ar",
                "24000",
                "pipe:1",
            ],
            check=True,
            capture_output=True,
        )
        data = np.frombuffer(decoded.stdout, dtype=np.int16).astype(np.float32) / 32768.0
        peak = float(np.max(np.abs(data))) if data.size else 0.0
        if peak > 0:
            data *= (10 ** (peak_dbfs / 20)) / peak
        return np.clip(data, -1.0, 1.0)
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
    peak = float(np.max(np.abs(data))) if data.size else 0.0
    if peak > 0:
        data *= (10 ** (peak_dbfs / 20)) / peak
    return np.clip(data, -1.0, 1.0)


def build_ab_reel(
    out_wav: Path,
    timeline_json: Path,
    segments: list[dict],
) -> None:
    silence = np.zeros(round(24000 * 0.65), dtype=np.float32)
    blocks: list[np.ndarray] = []
    cursor = 0.0
    timeline: list[dict] = []
    for index, seg in enumerate(segments, 1):
        for label, path in seg["clips"]:
            audio = load_mono_normalized(Path(path))
            start = cursor
            blocks.append(audio)
            cursor += len(audio) / 24000
            timeline.append(
                {
                    "index": index,
                    "label": label,
                    "line_id": seg["line_id"],
                    "path": path,
                    "start_s": round(start, 3),
                    "end_s": round(cursor, 3),
                }
            )
            blocks.append(silence)
            cursor += len(silence) / 24000
    if not blocks:
        raise SystemExit("No audio blocks for A/B reel")
    merged = np.concatenate(blocks)
    write_wav(out_wav, merged, 24000)
    timeline_json.write_text(json.dumps(timeline, ensure_ascii=False, indent=2), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUT)
    parser.add_argument("--skip-regen", action="store_true")
    parser.add_argument("--skip-assemble", action="store_true")
    parser.add_argument("--force-regen", action="store_true")
    parser.add_argument("--pilot-only", action="store_true")
    args = parser.parse_args()

    if not BASE_MANIFEST.is_file():
        raise SystemExit(f"Missing base manifest: {BASE_MANIFEST}")
    if not SPOKOJ.is_file():
        raise SystemExit(f"Missing spokoj reference: {SPOKOJ}")

    out = args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=True)
    base_rows = read_jsonl(BASE_MANIFEST)
    fv_by_id = {r["id"]: r for r in read_jsonl(FULL_VOICE_MANIFEST)}
    restore_rows, regen_rows = classify_ui_rows(base_rows, fv_by_id)

    inventory = {
        "base_manifest": str(BASE_MANIFEST),
        "previous_lektor_voicepack": str(FULL_VOICE_MANIFEST.parent),
        "previous_lektor_reference": str(SPOKOJ.resolve()),
        "previous_lektor_engine": "Pocket-TTS Polish 6L (full_voicepack generation)",
        "current_ui_lektor_reference": str(
            (BASE_VOICEPACK / "references/narrator_user_reference_pcm16.wav").resolve()
        ),
        "current_ui_lektor_engine": "Pocket-TTS Polish 6L + intro_narrator_reference.wav @ 0.45",
        "ui_narrator_line_count": len(restore_rows) + len(regen_rows),
        "restore_unchanged_text_from_full_voicepack": len(restore_rows),
        "regenerate_corrected_text_spokoj": len(regen_rows),
        "intro_reference_sha256_prefix": hashlib.sha256(INTRO_REF.read_bytes()).hexdigest()[:16]
        if INTRO_REF.is_file()
        else None,
        "spokoj_reference_sha256_prefix": hashlib.sha256(SPOKOJ.read_bytes()).hexdigest()[:16],
    }
    (out / "inventory.json").write_text(json.dumps(inventory, ensure_ascii=False, indent=2), encoding="utf-8")

    regen_map: dict[str, dict] = {}
    if not args.skip_regen:
        regen_targets = regen_rows
        if args.pilot_only:
            wanted = set(PILOT_LINE_IDS)
            regen_targets = [r for r in base_rows if r["line_id"] in wanted]
        regen_out = out / "regenerated_spokoj"
        regen_items = run_regeneration(
            regen_targets,
            regen_out,
            reference=SPOKOJ,
            temperature=0.45,
            profile_label="narrator_spokoj_ui",
            force=args.force_regen,
        )
        regen_map = {item["line_id"]: item for item in regen_items}

        pilot_out = out / "pilot_assertive_spokoj_055"
        pilot_items = run_regeneration(
            [r for r in base_rows if r["line_id"] in PILOT_LINE_IDS],
            pilot_out,
            reference=SPOKOJ,
            temperature=0.55,
            profile_label="narrator_spokoj_assertive",
            force=args.force_regen,
        )
        (out / "pilot_assertive_manifest.json").write_text(
            json.dumps(pilot_items, ensure_ascii=False, indent=2), encoding="utf-8"
        )

    if args.pilot_only:
        print(json.dumps({"pilot": True, "output": str(out)}, ensure_ascii=False))
        return

    if not args.skip_assemble and not args.pilot_only:
        if regen_rows and not regen_map:
            existing = {
                r["line_id"]: r
                for r in read_jsonl(out / "regenerated_spokoj/manifest.jsonl")
            }
            regen_map = existing
        voicepack = out / "voicepack"
        assemble_voicepack(
            base_rows,
            voicepack,
            restore_rows,
            regen_map,
            out / "narrator_ui_restore_decisions.jsonl",
        )
        bad = []
        for wav in voicepack.glob("*.wav"):
            if not is_pcm_wav(wav):
                bad.append((wav.name, "not_pcm_wav"))
                continue
            meta = validate_wav(wav)
            if meta["rate"] != 24000 or meta["channels"] != 1 or meta["sample_width"] != 2:
                bad.append((wav.name, meta))
        if bad:
            raise SystemExit(f"WAV format failures: {bad[:5]}")

    # A/B reel: A=full_voicepack (previous), B=intro assembled, C=restored candidate (if assembled)
    ab_segments: list[dict] = []
    pilot_ids = [lid for lid in PILOT_LINE_IDS if any(r["line_id"] == lid for r in base_rows)]
    for lid in pilot_ids:
        row = next(r for r in base_rows if r["line_id"] == lid)
        clips = [
            ("A_previous_spokoj_full_voicepack", row["previous_audio"]),
            ("B_current_intro_assembled", row["file"]),
        ]
        restored = out / "voicepack" / Path(row["file"]).name
        if restored.is_file():
            clips.append(("C_restored_ui_voicepack", str(restored)))
        pilot_055 = out / "pilot_assertive_spokoj_055"
        for item in read_jsonl(pilot_055 / "manifest.jsonl") if (pilot_055 / "manifest.jsonl").is_file() else []:
            if item["line_id"] == lid:
                clips.append(("D_pilot_spokoj_055", item["file"]))
                break
        ab_segments.append({"line_id": lid, "clips": clips})

    build_ab_reel(out / "narrator_lektor_ab_pilot.wav", out / "narrator_lektor_ab_pilot.json", ab_segments)
    print(json.dumps({"output": str(out), "inventory": inventory}, ensure_ascii=False))


if __name__ == "__main__":
    main()
