"""Build the local corpus inventory, flagged listening reel and QA report."""
from __future__ import annotations

import hashlib
import json
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'audio/polish/repair_candidate_20261013'
BASELINE = ROOT / 'research/localization/polish_dubbing_qa_final.json'


def read_jsonl(path: Path) -> list[dict]:
    return [json.loads(line) for line in path.read_text(encoding='utf-8').splitlines()
            if line.strip()]


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def pcm16_24k(path: str) -> np.ndarray:
    rate, audio = wavfile.read(path)
    audio = np.asarray(audio)
    if audio.ndim == 2:
        audio = audio.astype(np.float32).mean(axis=1)
    if np.issubdtype(audio.dtype, np.integer):
        audio = audio.astype(np.float32) / np.iinfo(audio.dtype).max
    else:
        audio = audio.astype(np.float32)
    if rate != 24000:
        audio = resample_poly(audio, 24000, rate)
    return (np.clip(audio, -1.0, 1.0) * 32767).astype(np.int16)


def main() -> None:
    qa_path = OUT / 'full_audit/qa.json'
    manifest_path = OUT / 'full_audit/manifest.jsonl'
    qa = json.loads(qa_path.read_text(encoding='utf-8'))
    candidates = { (row['pack'], row['line_id']): row for row in qa }
    baseline = json.loads(BASELINE.read_text(encoding='utf-8'))['results']

    inventory = []
    for row in baseline:
        candidate = candidates.get((row['pack'], row['line_id']))
        identity = row.get('profile') or 'unknown'
        scene = row.get('package') or (row.get('line_id', '').split('/')[0]
                                       if row['pack'] == 'story_nllb'
                                       else row.get('record', 'unknown'))
        inventory.append({
            'pack': row['pack'], 'line_id': row['line_id'], 'scene_or_record': scene,
            'asset': row.get('asset') or row.get('key'),
            'speaker': row.get('speaker_guess', 'localization_voice'),
            'voice_identity': identity,
            'emotion_direction': ('not_annotated_in_source; requires scene/performance review'
                                  if row['pack'] == 'story_nllb'
                                  else 'neutral/informational (provisional)'),
            'source_en': row.get('text_en') or row.get('en'),
            'polish_text': row.get('expected_pl'),
            'candidate_polish_text': candidate.get('corrected_polish') if candidate else None,
            'translation_status': row.get('translation_status'),
            'current_audio': row.get('audio_path'),
            'candidate_audio': candidate.get('file') if candidate else None,
            'candidate_voice_engine': candidate.get('candidate_voice_engine') if candidate else None,
            'candidate_qa_status': candidate.get('qa_status') if candidate else None,
            'candidate_qa_flags': candidate.get('qa_flags', []) if candidate else [],
            'mapping_status': ('candidate_replacement' if candidate
                               else 'retain_existing_pending_review'),
        })
    inventory_path = OUT / 'full_corpus_inventory_20261013.jsonl'
    inventory_path.write_text(''.join(json.dumps(row, ensure_ascii=False) + '\n'
                                       for row in inventory), encoding='utf-8')

    flagged = [row for row in qa if row.get('qa_flags')]
    silence = np.zeros(24000, dtype=np.int16)
    parts, index = [], []
    for number, row in enumerate(flagged, 1):
        audio = pcm16_24k(row['file'])
        parts.extend((silence, audio))
        index.append({'index': number, 'pack': row['pack'], 'line_id': row['line_id'],
                      'expected_polish': row['corrected_polish'], 'asr': row['qa_asr'],
                      'flags': row['qa_flags'], 'candidate_audio': row['file']})
    reel = np.concatenate(parts) if parts else np.zeros(1, dtype=np.int16)
    reel_path = OUT / 'targeted_listening_set_20261013.wav'
    wavfile.write(str(reel_path), 24000, reel)
    index_path = OUT / 'targeted_listening_set_20261013.json'
    index_path.write_text(json.dumps(index, ensure_ascii=False, indent=2), encoding='utf-8')

    statuses = Counter(row['qa_status'] for row in qa)
    manifest = read_jsonl(manifest_path)
    corrected = [row for row in manifest if row.get('correction_applied')]
    story_count = sum(row['pack'] == 'story_nllb' for row in inventory)
    report = [
        '# Polish dubbing repair candidate — 2026-10-13', '',
        'Offline QA/staging only. Originals and the Steam installation were not modified.',
        'Do not integrate until the targeted listening review is complete.', '',
        '## Coverage', '',
        f'- Inventory: **{len(inventory)} recordings** ({story_count} story / '
        f'{len(inventory) - story_count} UI/localization).',
        f'- Hard-failure rows regenerated: **{len(qa)}** '
        f'({sum(row["pack"] == "story_nllb" for row in qa)} story / '
        f'{sum(row["pack"] == "full_localization" for row in qa)} UI).',
        f'- Text corrections applied: **{len(corrected)} occurrences / '
        f'{len(set(row["line_id"] for row in corrected))} unique line IDs**.',
        f'- Automated ASR screen: **{statuses["PASS_AUTOMATED_SCREEN"]} pass**, '
        f'**{statuses["REVIEW_OR_REPAIR"]} require listening/review**.',
        '- Candidate files are 24 kHz mono PCM16; the QA screen found no technical flags.',
        '- Piper Polish medium was the available local replacement voice; it does not preserve '
        'the original Pocket-TTS/user-reference identities. Acting and voice identity remain pending.',
        '- No source emotion metadata was available for story lines; no performance intent was guessed.', '',
        '## Local files', '',
        '- Replacements: `full_audit/` (`manifest.jsonl`, `qa.json`, WAVs).',
        '- Full line mapping: `full_corpus_inventory_20261013.jsonl`.',
        f'- Flagged listening reel: `targeted_listening_set_20261013.wav` ({len(flagged)} clips).',
        '- Original voicepacks remain untouched; integration is a reversible manifest-based overlay.', '',
        '## Limits', '',
        'Whisper and waveform measurements do not validate acting, timbre, emotion, stress, or '
        'translation fidelity. The short reel covers only remaining automated flags and does not '
        'replace perceptual review of the candidates. No 100% quality claim is made.', '',
        '## Provenance', '',
        '- Generator: `tools/audio/repair_full_corpus_failed_lines_20261013.py`.',
        '- QA: `tools/audio/qa_repair_candidates.py`.',
        '- Finalizer: `tools/audio/finalize_repair_candidate.py`.',
        f'- Candidate manifest SHA-256: `{sha256(manifest_path)}`.',
        f'- QA SHA-256: `{sha256(qa_path)}`.',
        f'- Inventory SHA-256: `{sha256(inventory_path)}`.',
        f'- Listening reel SHA-256: `{sha256(reel_path)}`.', '',
        '## Status', '', '**CANDIDATE — targeted human listening required before MAIN integration.**',
    ]
    report_path = OUT / 'REPORT.md'
    report_path.write_text('\n'.join(report) + '\n', encoding='utf-8')
    print(json.dumps({'inventory': len(inventory), 'candidates': len(qa),
                      'statuses': dict(statuses), 'corrected_occurrences': len(corrected),
                      'listening_clips': len(flagged), 'reel_seconds': round(len(reel) / 24000, 2),
                      'report': str(report_path)}, ensure_ascii=False))


if __name__ == '__main__':
    main()
