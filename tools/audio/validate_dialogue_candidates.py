"""Offline transcription/quality evidence, never native-game acceptance."""
import argparse
import json
import os
import sys
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument('--input', type=Path, required=True)
ap.add_argument('--dependencies', type=Path, required=True)
args = ap.parse_args()
os.environ['HF_HUB_OFFLINE'] = '1'
os.environ['TRANSFORMERS_OFFLINE'] = '1'
sys.path.insert(0, str(args.dependencies))
from faster_whisper import WhisperModel
import faster_whisper.audio as fw_audio
av_open = fw_audio.av.open
fw_audio.av.open = lambda file, mode='r', metadata_errors=None, **kw: av_open(file, mode=mode, **kw)
model = WhisperModel('small', device='cpu', compute_type='int8', cpu_threads=4,
                     local_files_only=True)
results = []
for row in json.loads(args.input.read_text(encoding='utf-8')):
    segments, info = model.transcribe(row['file'], language='pl', beam_size=3,
                                      condition_on_previous_text=False)
    result = dict(asset=row['asset'], expected=row['text_pl'],
                  detected=' '.join(s.text.strip() for s in segments),
                  duration=info.duration, status='ASR_only_not_auditioned')
    results.append(result)
    print(json.dumps(result, ensure_ascii=False), flush=True)
    args.input.with_name('asr-review.json').write_text(json.dumps(results, ensure_ascii=False,
                                                               indent=2), encoding='utf-8')
