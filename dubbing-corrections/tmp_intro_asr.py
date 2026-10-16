import json, os, re, shutil
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np, struct, wave
SRC = r'C:\Users\Trilex\vr-mods\SingularityVR-polish-remaining-voices\remaining-voices\out\intro'
DST = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\intro_asr'
os.makedirs(DST, exist_ok=True)
for f in sorted(os.listdir(SRC)):
    if f.startswith('INTRO_') and f.endswith('.pl.wav'):
        shutil.copyfile(os.path.join(SRC, f), os.path.join(DST, f))
print('copied', len(os.listdir(DST)), flush=True)

import sys
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
from scipy.io import wavfile

def wav_to_16k_mono(path):
    sr, s = wavfile.read(path)
    s = np.asarray(s, dtype=np.float64)
    if s.ndim > 1:
        s = s.mean(axis=1)
    if np.issubdtype(np.asarray(s).dtype, np.integer):
        s = s / 32768.0
    idx = np.linspace(0, len(s) - 1, int(len(s) * 16000 / sr))
    return np.interp(idx, np.arange(len(s)), s).astype(np.float32)

from faster_whisper import WhisperModel
model = WhisperModel('small', device='cpu', compute_type='int8')
for f in sorted(os.listdir(DST)):
    segs, _ = model.transcribe(wav_to_16k_mono(os.path.join(DST, f)),
                               language='pl', beam_size=5)
    print('%s | %s' % (f, ' '.join(x.text for x in segs).strip()[:130]), flush=True)
