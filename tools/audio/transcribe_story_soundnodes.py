from __future__ import annotations

import csv
import json
import os
import sys
from pathlib import Path

sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.whisper_tools')
from faster_whisper import WhisperModel
import faster_whisper.audio as fw_audio

# PyAV 19 removed the metadata_errors keyword while faster-whisper still
# passes it. Keep the decoder local and compatible with both APIs.
_av_open = fw_audio.av.open
def _compat_open(input_file, mode='r', metadata_errors=None, **kwargs):
    return _av_open(input_file, mode=mode, **kwargs)
fw_audio.av.open = _compat_open

ROOT = Path(r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene')
CSV_PATH = ROOT / 'audio' / 'story_en_reference' / 'soundnodes_manifest.csv'
OUT = ROOT / 'audio' / 'story_en_reference' / 'story_transcripts.jsonl'


def main() -> None:
    done = set()
    if OUT.exists():
        for line in OUT.read_text(encoding='utf-8').splitlines():
            if line.strip():
                item = json.loads(line)
                if 'error' not in item:
                    done.add(item['path'])

    rows = []
    with CSV_PATH.open(encoding='utf-8') as f:
        for row in csv.DictReader(f):
            asset = row['asset'].upper()
            if not asset.startswith(('RL', 'VI', 'WL', 'FR', 'RT')):
                continue
            if row['path'] in done:
                continue
            rows.append(row)

    limit = os.environ.get('TRANSCRIBE_LIMIT')
    if limit:
        rows = rows[:int(limit)]

    print('loading Whisper base.en', flush=True)
    model = WhisperModel('base.en', device='cpu', compute_type='int8')
    with OUT.open('a', encoding='utf-8') as out:
        for i, row in enumerate(rows, 1):
            path = ROOT / 'audio' / 'story_en_reference' / Path(row['path'])
            try:
                segments, info = model.transcribe(
                    str(path),
                    beam_size=3,
                    vad_filter=True,
                    condition_on_previous_text=False,
                    initial_prompt='Singularity, Katorga-12, E99, TMD, Barisov, Demichev, Kathryn, Devlin, Renko.',
                )
                segments = list(segments)
                text = ' '.join(s.text.strip() for s in segments if s.text.strip()).strip()
                result = {
                    'path': row['path'],
                    'package': row['package'],
                    'asset': row['asset'],
                    'speaker_guess': row['speaker_guess'],
                    'language': info.language,
                    'duration': info.duration,
                    'text_en': text,
                    'segments': [{'start': s.start, 'end': s.end, 'text': s.text.strip()} for s in segments],
                }
                out.write(json.dumps(result, ensure_ascii=False) + '\n')
                out.flush()
                if i == 1 or i % 25 == 0:
                    print(f'transcribed {i}/{len(rows)}', flush=True)
            except Exception as exc:
                out.write(json.dumps({'path': row['path'], 'asset': row['asset'], 'error': str(exc)}, ensure_ascii=False) + '\n')
                out.flush()
                print('ERROR', row['path'], exc, flush=True)
    print('complete', flush=True)


if __name__ == '__main__':
    main()
