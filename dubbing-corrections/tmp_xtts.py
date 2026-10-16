import os, sys
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.xtts_tools')
# torchcodec (backend torchaudio) nie ma DLL-i FFmpeg na tym Windows:
# podczyt audio przez soundfile zamiast niego.
import torchaudio
import soundfile as _sf
import torch as _torch
import numpy as _np
def _load_via_soundfile(uri, *a, **k):
    data, sr = _sf.read(uri, always_2d=False)
    arr = _np.asarray(data, dtype=_np.float32)
    if arr.ndim == 1:
        t = _torch.from_numpy(arr).unsqueeze(0)
    else:
        t = _torch.from_numpy(arr.T.copy())
    return t, sr
torchaudio.load = _load_via_soundfile
os.environ['HF_HUB_OFFLINE'] = '0'
os.environ['COQUI_TOS_AGREED'] = '1'
OUT = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\xtts_test'
os.makedirs(OUT, exist_ok=True)
DONOR = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\mailabs_m_loud.wav'
from TTS.api import TTS
print('loading xtts...', flush=True)
tts = TTS('tts_models/multilingual/multi-dataset/xtts_v2', gpu=False)
print('model ready', flush=True)
p = os.path.join(OUT, 'INTRO_1006_CP.xtts.wav')
tts.tts_to_file('Siema. Trzymamy kurs. Do celu trzy minuty.',
                speaker_wav=DONOR, language='pl', file_path=p)
print('SYN ok', p, flush=True)
