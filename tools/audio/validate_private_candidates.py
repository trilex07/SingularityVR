#!/usr/bin/env python3
"""Read-only QA for the eight private SIDE audio comparisons.

This intentionally does not package WAVs or patch UE3 SoundNodeWave objects.
Pass the ignored ``audio-remaster-private/audition`` directory as --root.
"""
from __future__ import annotations

import argparse
import array
import hashlib
import json
import math
import sys
import wave
from pathlib import Path

TARGETS = (
    "01_weapon_centurion_revolver",
    "02_weapon_kasimov_sniper",
    "03_e99_tmd_impulse",
    "04_weapon_walkiria_ar9",
    "05_weapon_spikeshot_shotgun",
    "06_music_explore_drone",
    "07_music_combat_boss_grom",
    "08_ambience_ocean_storm",
)
VARIANTS = (
    ("original.wav", "orig_meas", "rms_db", "peak_db"),
    ("restored_conservative.wav", "cons_meas", "proc_rms_db", "sample_peak_db"),
    ("remaster_enhanced.wav", "enh_meas", "proc_rms_db", "sample_peak_db"),
)


def measure(path: Path) -> dict:
    digest = hashlib.sha256()
    peak = 0
    squares = 0
    samples = 0
    with wave.open(str(path), "rb") as wav:
        channels, rate, width, frames = (
            wav.getnchannels(), wav.getframerate(), wav.getsampwidth(), wav.getnframes()
        )
        if width != 2 or channels not in (1, 2, 4, 6) or rate not in (22050, 24000, 32000, 44100, 48000):
            raise ValueError(f"{path}: unexpected PCM layout: {channels}ch/{rate}Hz/{width * 8}bit")
        while True:
            chunk = wav.readframes(32768)
            if not chunk:
                break
            digest.update(chunk)
            pcm = array.array("h")
            pcm.frombytes(chunk)
            if sys.byteorder != "little":
                pcm.byteswap()
            samples += len(pcm)
            for value in pcm:
                peak = max(peak, abs(value))
                squares += value * value
    if samples != frames * channels:
        raise ValueError(f"truncated PCM: {samples} samples, expected {frames * channels}")
    rms = math.sqrt(squares / samples) / 32768 if samples else 0
    return {
        "channels": channels,
        "sample_rate": rate,
        "frames": frames,
        "pcm_sha256": digest.hexdigest().upper(),
        "rms_db": 20 * math.log10(rms) if rms else float("-inf"),
        "sample_peak_db": 20 * math.log10(peak / 32768) if peak else float("-inf"),
    }


def validate(root: Path) -> list[dict]:
    result = []
    for target in TARGETS:
        directory = root / target
        meta = json.loads((directory / "metadata.json").read_text(encoding="utf-8"))
        if meta.get("id") != target:
            raise ValueError(f"{target}: metadata identity mismatch")
        rows = []
        for filename, key, rms_key, peak_key in VARIANTS:
            actual = measure(directory / filename)
            reported = meta[key]
            if (actual["channels"], actual["sample_rate"], actual["frames"]) != (
                meta["orig_meas"]["channels"], meta["orig_meas"]["sample_rate"],
                meta["orig_meas"]["frames"]
            ):
                raise ValueError(f"{target}/{filename}: channels/rate/length changed")
            if abs(actual["rms_db"] - reported[rms_key]) > 0.30:
                raise ValueError(f"{target}/{filename}: RMS disagrees with SIDE metadata")
            if abs(actual["sample_peak_db"] - reported[peak_key]) > 0.30:
                raise ValueError(f"{target}/{filename}: peak disagrees with SIDE metadata")
            rows.append({"file": filename, **actual})
        result.append({"id": target, "package": meta["package"], "asset": meta["asset"],
                       "cue": meta["cue"], "cue_confidence": meta.get("confidence"), "variants": rows})
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    args = parser.parse_args()
    try:
        rows = validate(args.root)
    except (OSError, KeyError, ValueError, wave.Error) as exc:
        parser.exit(1, f"Audio QA FAIL: {exc}\n")
    for row in rows:
        identity = "PASS" if row["cue_confidence"] == "VERIFIED_REFERENCE" else "CUE UNPROVEN"
        print(f"{identity} {row['id']}: {row['package']}::{row['asset']} cue={row['cue']}")
        for variant in row["variants"]:
            print(f"  {variant['file']}: {variant['channels']}ch "
                  f"{variant['sample_rate']}Hz {variant['frames']}f "
                  f"RMS={variant['rms_db']:.2f}dB peak={variant['sample_peak_db']:.2f}dB "
                  f"PCM-SHA256={variant['pcm_sha256']}")
    print("True peak, phase and UE3 cue replacement: NOT verified by this PCM-header check")
