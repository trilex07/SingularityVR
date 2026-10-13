"""Offline local-model voice candidates from an explicit private line manifest.

Does not install/download models, write to Steam or modify source audio. The
manifest owns translations and fictional-character reference clips outside Git.
"""
import argparse
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--manifest', type=Path, required=True)
    ap.add_argument('--dependencies', type=Path, required=True)
    ap.add_argument('--ffmpeg', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--reference-map', type=Path)
    args = ap.parse_args()
    os.environ['HF_HUB_OFFLINE'] = '1'
    os.environ['TRANSFORMERS_OFFLINE'] = '1'
    sys.path.insert(0, str(args.dependencies))
    import torch
    import numpy as np
    from scipy.io import wavfile
    from pocket_tts import TTSModel
    torch.set_num_threads(4)
    torch.manual_seed(12013)
    rows = json.loads(args.manifest.read_text(encoding='utf-8'))
    reference_map = json.loads(args.reference_map.read_text(encoding='utf-8')) if args.reference_map else {}
    assert rows and len({r['asset'] for r in rows}) == len(rows)
    args.out.mkdir(parents=True, exist_ok=True)
    model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml',
                               temp=0.45, quantize=True)
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
        audio = model.generate_audio(voices[str(reference)], row['text_pl']).numpy()
        audio = np.asarray(audio, dtype=np.float32).squeeze()
        assert audio.ndim == 1 and audio.size and np.isfinite(audio).all()
        peak = float(np.max(np.abs(audio)))
        assert peak > 0
        audio *= (10**(-3/20))/peak
        wavfile.write(target, model.sample_rate,
                      (np.clip(audio, -1, 1)*32767).astype(np.int16))
        result = dict(row, file=str(target), duration=len(audio)/model.sample_rate,
                      sha256=hashlib.sha256(target.read_bytes()).hexdigest(),
                      reference_sha256=hashlib.sha256(reference.read_bytes()).hexdigest(),
                      status='generated_candidate_not_auditioned')
        results.append(result)
        (args.out / 'generated.json').write_text(json.dumps(results, ensure_ascii=False,
                                                           indent=2), encoding='utf-8')
        print(row['asset'], round(result['duration'], 3), flush=True)


if __name__ == '__main__':
    main()
