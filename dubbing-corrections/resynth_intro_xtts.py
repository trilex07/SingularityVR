"""Pilot XTTS-v2 dawca mailabs-PL (decyzja rezysera: glos OK).
Wiersze: 1006 (Halo...), 1010, 1016. Torchaudio->soundfile patch (brak DLL-i FFmpeg).
Wyjscie: PCM16 mono 24 kHz + normalizacja -1 dBFS. Tylko temp."""
import os, sys
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.xtts_tools')
import torchaudio
import soundfile as _sf
import torch as _torch
import numpy as _np
def _load_via_soundfile(uri, *a, **k):
    data, sr = _sf.read(uri, always_2d=False)
    arr = _np.asarray(data, dtype=_np.float32)
    t = _torch.from_numpy(arr).unsqueeze(0) if arr.ndim == 1 else _torch.from_numpy(arr.T.copy())
    return t, sr
torchaudio.load = _load_via_soundfile
os.environ['HF_HUB_OFFLINE'] = '0'
os.environ['COQUI_TOS_AGREED'] = '1'
import json
REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
OUT = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\resynth_intro_xtts'
os.makedirs(OUT, exist_ok=True)
DONOR = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\mailabs_m_loud.wav'
pl = {r['asset']: r['pl'] for r in
      json.load(open(REPO + r'\remaining-voices\intro-pl.json', encoding='utf-8'))}
JOBS = ['INTRO_1006_CP', 'INTRO_1010_CP', 'INTRO_1016_CP']
from TTS.api import TTS
print('loading xtts...', flush=True)
tts = TTS('tts_models/multilingual/multi-dataset/xtts_v2', gpu=False)
print('model ready', flush=True)
man = []
for n, a in enumerate(JOBS, 1):
    p = os.path.join(OUT, a + '.xtts.wav')
    tts.tts_to_file(pl[a], speaker_wav=DONOR, language='pl', file_path=p)
    import hashlib
    man.append({'file': a + '.xtts.wav', 'asset': a, 'donor': 'mailabs-pl',
                'text_pl': pl[a], 'sha16': hashlib.sha256(open(p, 'rb').read()).hexdigest()[:16]})
    print('SYN %d/%d %s' % (n, len(JOBS), a), flush=True)
with open(REPO + r'\dubbing-corrections\resynth_manifest_intro_xtts.jsonl', 'w', encoding='utf-8') as fh:
    for m in man:
        fh.write(json.dumps(m, ensure_ascii=False) + '\n')
print('DONE', flush=True)
