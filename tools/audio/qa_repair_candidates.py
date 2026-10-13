"""Run objective format, waveform and ASR screening on staged Polish WAV replacements.

This is an automated screen only; it cannot judge acting, emotion, voice identity,
pronunciation quality or translation fidelity. It never edits original voicepacks.
"""
from __future__ import annotations

import json
import re
import sys
import argparse
from collections import Counter
from pathlib import Path

sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.whisper_tools')

import numpy as np
from faster_whisper import WhisperModel
from scipy.io import wavfile
from scipy.signal import resample_poly

ROOT = Path(__file__).resolve().parents[2]


def tokens(value: str) -> list[str]:
    return re.findall(r'[a-ząćęłńóśźż0-9]+', (value or '').lower())


def similarity(expected: str, heard: str) -> float:
    left, right = tokens(expected), tokens(heard)
    if not left:
        return 1.0 if not right else 0.0
    previous = list(range(len(right) + 1))
    for i, word in enumerate(left, 1):
        current = [i]
        for j, other in enumerate(right, 1):
            current.append(min(current[-1] + 1, previous[j] + 1,
                               previous[j - 1] + (word != other)))
        previous = current
    return 1.0 - previous[-1] / len(left)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument('--candidate-dir', type=Path,
                    default=ROOT / 'audio/polish/repair_candidate_20261013/full_audit')
    ap.add_argument('--asr-model', default='base', help='cached faster-whisper model name (default: base)')
    ap.add_argument('--line-id', action='append', default=[], help='screen only this line ID (repeatable)')
    ap.add_argument('--output-file', type=Path, help='QA JSON output path (default: candidate-dir/qa.json)')
    ap.add_argument('--merge', action='store_true', help='merge screened rows into existing output by line ID')
    args = ap.parse_args()
    candidate_dir = args.candidate_dir.resolve()
    manifest = candidate_dir / 'manifest.jsonl'
    rows = [json.loads(line) for line in manifest.read_text(encoding='utf-8').splitlines()
            if line.strip()]
    if args.line_id:
        wanted = set(args.line_id)
        unknown = wanted - {row['line_id'] for row in rows}
        if unknown:
            raise SystemExit(f'Unknown line IDs: {sorted(unknown)}')
        rows = [row for row in rows if row['line_id'] in wanted]
    model = WhisperModel(args.asr_model, device='cpu', compute_type='int8')
    screened = []
    for index, row in enumerate(rows, 1):
        rate, audio = wavfile.read(row['file'])
        audio = np.asarray(audio)
        channels = audio.shape[1] if audio.ndim == 2 else 1
        if audio.ndim == 2:
            audio = audio.astype(np.float32).mean(axis=1)
        if np.issubdtype(audio.dtype, np.integer):
            audio = audio.astype(np.float32) / np.iinfo(audio.dtype).max
        else:
            audio = audio.astype(np.float32)

        duration = len(audio) / rate
        peak = float(np.max(np.abs(audio))) if len(audio) else 0.0
        clipping = float(np.mean(np.abs(audio) >= .999)) if len(audio) else 1.0
        active = np.flatnonzero(np.abs(audio) >= .002)
        leading = active[0] / rate if len(active) else duration
        trailing = (len(audio) - 1 - active[-1]) / rate if len(active) else duration
        file_rate, file_audio = wavfile.read(row['file'])
        pcm16 = file_rate == 24000 and file_audio.dtype == np.int16

        samples = resample_poly(audio, 2, 3) if rate == 24000 else resample_poly(audio, 16000, rate)
        segments, info = model.transcribe(samples, beam_size=2, vad_filter=False,
                                          condition_on_previous_text=False)
        heard = ' '.join(segment.text.strip() for segment in segments).strip()
        expected_count, heard_count = len(tokens(row['tts_text'])), len(tokens(heard))
        score = similarity(row['tts_text'], heard)
        flags = []
        if (rate != 24000 or channels != 1 or not pcm16 or clipping > .001
                or leading > .12 or trailing > .25 or not np.isfinite(audio).all()):
            flags.append('TECHNICAL')
        if expected_count >= 4 and heard_count < max(2, int(expected_count * .45)):
            flags.append('POSSIBLE_TRUNCATION')
        if info.language != 'pl' and (info.language_probability or 0) >= .6 and expected_count > 3:
            flags.append('LANGUAGE_OR_PRONUNCIATION')
        if expected_count >= 5 and score < .35:
            flags.append('CONTENT_OR_PRONUNCIATION')

        row.update(qa_asr=heard, qa_language=info.language,
                   qa_language_probability=round(float(info.language_probability or 0), 4),
                   qa_similarity=round(score, 4), qa_expected_words=expected_count,
                   qa_heard_words=heard_count, qa_duration_s=round(duration, 3),
                   qa_peak=round(peak, 5), qa_clip_fraction=round(clipping, 6),
                   qa_leading_silence_s=round(leading, 3),
                   qa_trailing_silence_s=round(trailing, 3), qa_flags=flags,
                   qa_status='PASS_AUTOMATED_SCREEN' if not flags else 'REVIEW_OR_REPAIR')
        screened.append(row)
        if index == 1 or index % 50 == 0:
            print(f'ASR QA {index}/{len(rows)}', flush=True)

    output = (args.output_file or (candidate_dir / 'qa.json')).resolve()
    if args.merge and output.is_file():
        existing = {row['line_id']: row for row in json.loads(output.read_text(encoding='utf-8'))}
        existing.update({row['line_id']: row for row in screened})
        manifest_order = [json.loads(line)['line_id'] for line in manifest.read_text(encoding='utf-8').splitlines()
                          if line.strip()]
        screened = [existing[line_id] for line_id in manifest_order if line_id in existing]
    output.write_text(json.dumps(screened, ensure_ascii=False, indent=2), encoding='utf-8')
    counts = Counter(row['qa_status'] for row in screened)
    flag_counts = Counter(flag for row in screened for flag in row['qa_flags'])
    print(json.dumps({'total': len(screened), 'statuses': dict(counts),
                      'flags': dict(flag_counts), 'output': str(output)}, ensure_ascii=False))


if __name__ == '__main__':
    main()
