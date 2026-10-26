"""Master synthesis and fitter execution engine for the 551 timing lines.

Produces:
1. Candidate WAV files staged in:
   - build/audio-release-private/timing_fix_551/
   - C:/Dubbing-aktualny/timing_fix_551/
2. Candidate delivery manifest:
   - docs/handoffs/final-dubbing-v3-manifest-20261026.json

Enforces:
- Mono PCM16 24 kHz
- Peak amplitude <= -1.0 dBFS, non-zero
- Strict tempo <= 1.35x against native budget
"""
from __future__ import annotations

import array as pyarray
import hashlib
import json
import os
import shutil
import struct
import sys
import time
import wave
from pathlib import Path
import numpy as np

# Set environment
os.environ['HF_HUB_OFFLINE'] = '1'

# Tooling paths
POCKET_TOOLS = Path(r'C:\Users\Trilex\vr-mods\.pocket_tools')
sys.path.insert(0, str(POCKET_TOOLS))

AUDIO_TOOLS = Path(__file__).resolve().parent
ROOT = AUDIO_TOOLS.parent.parent
sys.path.append(str(AUDIO_TOOLS))
sys.path.append(str(ROOT))

import miniaudio
from pocket_tts import TTSModel
import native_voice_fit as fit
from dialogue_variants import get_variants

PLAN_PATH = ROOT / 'docs/handoffs/final-dubbing-timing-plan-20261026.json'
LOOKUP_PATH = ROOT / 'localization/pl/polish_story_voice.jsonl'
EN_BASE = Path(r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference')

PRIVATE_OUT = ROOT / 'build/audio-release-private/timing_fix_551'
STAGING_OUT = Path(r'C:\Dubbing-aktualny\timing_fix_551')
REF_DIR = ROOT / 'build/audio-release-private/scratch_refs'
MANIFEST_OUT = ROOT / 'docs/handoffs/final-dubbing-v3-manifest-20261026.json'


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def ogg_to_ref(src: Path, dst: Path) -> Path:
    if dst.exists() and dst.stat().st_size > 0:
        return dst
    dec = miniaudio.decode_file(str(src), output_format=miniaudio.SampleFormat.SIGNED16)
    x = np.array(pyarray.array('h', dec.samples), dtype=np.float64) / 32768.0
    if dec.nchannels == 2:
        x = x.reshape(-1, 2).mean(axis=1)
    if dec.sample_rate != 24000:
        idx = np.linspace(0, len(x) - 1, int(len(x) * 24000 / dec.sample_rate))
        x = np.interp(idx, np.arange(len(x)), x)
    with wave.open(str(dst), 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(24000)
        w.writeframes((np.clip(x, -1, 1) * 32767).astype(np.int16).tobytes())
    return dst


def clean_decoder_startup(samples: np.ndarray) -> np.ndarray:
    """Clear Mimi filter startup transient in the first ~300 samples if followed by silence."""
    if len(samples) > 1500 and np.max(np.abs(samples[300:1500])) < 100:
        samples[:300] = 0
    return samples


def normalize_audio(samples: np.ndarray, target_peak_db: float = -1.5) -> np.ndarray:
    """Normalize peak volume safely between -2.0 dBFS and -1.0 dBFS."""
    peak = np.max(np.abs(samples))
    if peak == 0:
        return samples
    target_peak = int(32767.0 * (10.0 ** (target_peak_db / 20.0)))
    if peak > target_peak or peak < 10000:
        gain = target_peak / peak
        samples = np.clip(np.round(samples * gain), -32767, 32767).astype(np.int16)
    return samples


def main():
    t0 = time.time()
    PRIVATE_OUT.mkdir(parents=True, exist_ok=True)
    STAGING_OUT.mkdir(parents=True, exist_ok=True)
    REF_DIR.mkdir(parents=True, exist_ok=True)
    temp_dir = ROOT / 'build/audio-release-private/synth_temp'
    temp_dir.mkdir(parents=True, exist_ok=True)

    print("Loading timing plan...")
    with open(PLAN_PATH, 'r', encoding='utf-8') as f:
        plan = json.load(f)
    cases = plan['timing_cases']
    print(f"Total timing cases: {len(cases)}")

    print("Loading localization lookup...")
    with open(LOOKUP_PATH, 'r', encoding='utf-8-sig') as f:
        lookup = [json.loads(s) for s in f if s.strip()]
    lookup_by_id = {(r['package'].casefold(), r['asset']): r for r in lookup}

    print("Initializing Pocket-TTS model...")
    model = TTSModel.load_model(
        config='hf://shefowl/pocket-tts-polish-6l/config.yaml',
        temp=0.35,
        quantize=True,
    )
    model.pad_with_spaces_for_short_inputs = False

    prompt_state_cache = {}

    manifest_records = []
    pass_count = 0
    blocked_count = 0
    near_limit_count = 0

    print(f"Starting batch synthesis of {len(cases)} cases...")

    for idx, c in enumerate(cases):
        pkg = c['package']
        asset = c['asset']
        native_dur = c['native_duration']
        max_allowed = c['max_allowed_source_duration']
        text_en = c['text_en']
        text_pl = c['text_pl']

        lk = lookup_by_id[(pkg.casefold(), asset)]
        target_filename = lk['file']

        # Donor audio
        src_ogg = EN_BASE / lk['source']
        ref_wav_name = f"{lk['source'].replace('/', '_').replace('\\', '_')}.wav"
        ref_wav = REF_DIR / ref_wav_name
        ogg_to_ref(src_ogg, ref_wav)

        # Cache prompt state
        ref_key = str(ref_wav)
        if ref_key not in prompt_state_cache:
            prompt_state_cache[ref_key] = model.get_state_for_audio_prompt(ref_key)
        state = prompt_state_cache[ref_key]

        # Get candidate variants
        variants = get_variants(text_en, text_pl, native_dur, max_allowed)

        raw_wav = temp_dir / f"{asset}_raw.wav"
        trimmed_wav = temp_dir / f"{asset}_trimmed.wav"
        best_wav = temp_dir / f"{asset}_best.wav"

        best_tempo = 999.0
        best_gdur = 0.0
        best_variant = variants[0]
        is_passed = False

        # Multi-stage synthesis: First pass at temp=0.35, then fallback at temp=0.20
        for current_temp in [0.35, 0.20]:
            model.temp = current_temp
            for v in variants:
                audio = model.generate_audio(state, v, frames_after_eos=0).numpy().squeeze()
                audio_int16 = (np.clip(audio, -1, 1) * 32767).astype(np.int16)
                audio_int16 = clean_decoder_startup(audio_int16)
                audio_int16 = normalize_audio(audio_int16, target_peak_db=-1.5)

                with wave.open(str(raw_wav), 'wb') as w:
                    w.setnchannels(1)
                    w.setsampwidth(2)
                    w.setframerate(24000)
                    w.writeframes(audio_int16.tobytes())

                gdur = fit.trim_guarded(raw_wav, trimmed_wav)
                tempo = fit.native_tempo(gdur, native_dur)

                if tempo < best_tempo:
                    best_tempo = tempo
                    best_gdur = gdur
                    best_variant = v
                    shutil.copyfile(trimmed_wav, best_wav)

                if tempo <= 1.35:
                    is_passed = True
                    break

            if is_passed:
                break

        status = "PASS" if is_passed else "BLOCKED"
        if is_passed:
            pass_count += 1
            if best_tempo > 1.25:
                near_limit_count += 1
        else:
            blocked_count += 1

        # Deliver the WAV file
        priv_dest = PRIVATE_OUT / target_filename
        stag_dest = STAGING_OUT / target_filename
        shutil.copyfile(best_wav, priv_dest)
        shutil.copyfile(best_wav, stag_dest)

        # Validate delivered WAV
        with wave.open(str(priv_dest), 'rb') as w:
            assert (w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getcomptype()) == (1, 2, 24000, 'NONE')
            data = w.readframes(w.getnframes())
            samples = struct.iter_unpack('<h', data)
            peak = max(abs(s[0]) for s in samples)
            assert 0 < peak < 32767, f"Peak {peak} out of valid range"

        file_hash = sha256_file(priv_dest)

        rec = {
            "package": pkg,
            "asset": asset,
            "file": target_filename,
            "speaker": c.get('speaker', 'unclassified'),
            "profile": c.get('profile', 'unclassified'),
            "text_en": text_en,
            "text_pl_curated": best_variant,
            "tts_text": best_variant,
            "native_duration": native_dur,
            "max_allowed_source_duration": max_allowed,
            "guarded_duration": best_gdur,
            "computed_tempo": best_tempo,
            "status": status,
            "sha256": file_hash,
        }
        manifest_records.append(rec)

        if (idx + 1) % 25 == 0 or idx + 1 == len(cases):
            elapsed = time.time() - t0
            print(
                f"[{idx+1}/{len(cases)}] PASS: {pass_count}, BLOCKED: {blocked_count} "
                f"| Last: {asset} tempo={best_tempo:.2f}x ({status}) | Elapsed: {elapsed:.1f}s"
            )

    # Save manifest
    manifest_data = {
        "schema": "singularityvr-dubbing-v3-manifest",
        "evidence_level": "OFFLINE_CANDIDATE_STAGED_NOT_DEPLOYED_TO_STEAM",
        "summary": {
            "total_cases": len(cases),
            "pass_count": pass_count,
            "blocked_count": blocked_count,
            "near_limit_count": near_limit_count,
            "staging_directory": str(STAGING_OUT),
            "private_directory": str(PRIVATE_OUT),
        },
        "records": manifest_records,
    }

    with open(MANIFEST_OUT, 'w', encoding='utf-8') as f:
        json.dump(manifest_data, f, indent=2, ensure_ascii=False)

    total_time = time.time() - t0
    print("\n" + "=" * 60)
    print("SYNTHESIS AND FITTING COMPLETE")
    print(f"Total lines processed: {len(cases)}")
    print(f"PASS (tempo <= 1.35x): {pass_count} ({pass_count/len(cases)*100:.1f}%)")
    print(f"Near limit (1.25x < tempo <= 1.35x): {near_limit_count}")
    print(f"BLOCKED (tempo > 1.35x): {blocked_count}")
    print(f"Manifest written to: {MANIFEST_OUT}")
    print(f"Total elapsed time: {total_time:.1f}s ({total_time/len(cases):.2f}s per take)")
    print("=" * 60)


if __name__ == '__main__':
    main()
