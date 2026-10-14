"""Offline local-model voice candidates from an explicit private line manifest.

Does not install/download models, write to Steam or modify source audio. The
manifest owns translations and fictional-character reference clips outside Git.
"""
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path


def checked_segments(spoken, segments=None):
    segments = segments if segments is not None else [spoken]
    words = lambda value: re.findall(r'\w+', value.lower())
    if not segments or not all(segment.strip() for segment in segments):
        raise ValueError('empty voice segment')
    if words(' '.join(segments))!=words(spoken):
        raise ValueError('segments changed spoken content')
    return segments


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--manifest', type=Path, required=True)
    ap.add_argument('--dependencies', type=Path, required=True)
    ap.add_argument('--ffmpeg', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--reference-map', type=Path)
    ap.add_argument('--seed', type=int, default=12013)
    ap.add_argument('--temperature', type=float, default=0.45)
    ap.add_argument('--max-tokens', type=int, default=50)
    ap.add_argument('--frames-after-eos', type=int)
    args = ap.parse_args()
    os.environ['HF_HUB_OFFLINE'] = '1'
    os.environ['TRANSFORMERS_OFFLINE'] = '1'
    sys.path.insert(0, str(args.dependencies))
    import torch
    import numpy as np
    from scipy.io import wavfile
    from pocket_tts import TTSModel
    torch.set_num_threads(4)
    assert 0<args.temperature<=1 and 10<=args.max_tokens<=200
    assert args.frames_after_eos is None or 0<=args.frames_after_eos<=12
    torch.manual_seed(args.seed)
    rows = json.loads(args.manifest.read_text(encoding='utf-8'))
    reference_map = json.loads(args.reference_map.read_text(encoding='utf-8')) if args.reference_map else {}
    assert rows and len({r['asset'] for r in rows}) == len(rows)
    args.out.mkdir(parents=True, exist_ok=True)
    model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml',
                               temp=args.temperature, quantize=True)
    voices = {}
    results = []
    for row in rows:
        reference = Path(reference_map.get(row['role'], row['reference']))
        assert reference.is_file() and row['text_pl'].strip()
        if str(reference) not in voices:
            prompt = args.out / (row['role'] + '-reference.wav')
            subprocess.run([str(args.ffmpeg), '-v', 'error', '-n', '-i', str(reference),
                            '-ac', '1', '-ar', '24000', str(prompt)], check=True)
            voices[str(reference)] = model.get_state_for_audio_prompt(prompt)
        target = args.out / (row['asset'] + '.wav')
        if target.exists():
            raise ValueError('refuse overwrite: ' + str(target))
        segments = checked_segments(row.get('tts_text',row['text_pl']),row.get('tts_segments'))
        parts = [model.generate_audio(voices[str(reference)],segment,
            max_tokens=args.max_tokens,frames_after_eos=args.frames_after_eos).numpy().reshape(-1)
            for segment in segments]
        audio = np.concatenate([item for index,part in enumerate(parts) for item in
            ([part] if index==len(parts)-1 else [part,np.zeros(round(model.sample_rate*.04),dtype=np.float32)])])
        audio = np.asarray(audio, dtype=np.float32).squeeze()
        assert audio.ndim == 1 and audio.size and np.isfinite(audio).all()
        peak = float(np.max(np.abs(audio)))
        assert peak > 0
        audio *= (10**(-3/20))/peak
        wavfile.write(target, model.sample_rate,
                      (np.clip(audio, -1, 1)*32767).astype(np.int16))
        result = dict(row, reference=str(reference), source_reference=row['reference'],
                      generation_seed=args.seed,generation_temperature=args.temperature,
                      generation_segments=len(segments),generation_max_tokens=args.max_tokens,
                      generation_frames_after_eos=args.frames_after_eos,
                      file=str(target), duration=len(audio)/model.sample_rate,
                      sha256=hashlib.sha256(target.read_bytes()).hexdigest(),
                      reference_sha256=hashlib.sha256(reference.read_bytes()).hexdigest(),
                      status='generated_candidate_not_auditioned')
        results.append(result)
        (args.out / 'generated.json').write_text(json.dumps(results, ensure_ascii=False,
                                                           indent=2), encoding='utf-8')
        print(row['asset'], round(result['duration'], 3), flush=True)


if __name__ == '__main__':
    main()
