import os, sys, wave, subprocess
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.xtts_tools')
import numpy as np, soundfile as sf
import torch, torchaudio

def _load_via_soundfile(uri, *a, **k):
    data, sr = sf.read(uri, always_2d=False)
    arr = np.asarray(data, dtype=np.float32)
    t = torch.from_numpy(arr).unsqueeze(0) if arr.ndim == 1 else torch.from_numpy(arr.T.copy())
    return t, sr

torchaudio.load = _load_via_soundfile
os.environ['COQUI_TOS_AGREED'] = '1'

from TTS.api import TTS
tts = TTS('tts_models/multilingual/multi-dataset/xtts_v2', gpu=False)

donor = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\mailabs_m_loud.wav'
out_wav = r'C:\Users\Trilex\AppData\Local\Temp\xtts_smooth_test.wav'

t = 'Zespół Spartan, tu dowódca Nimitz. Wkraczacie w rosyjską przestrzeń powietrzną. Zachować szczególną ostrożność.'
tts.tts_to_file(text=t, speaker_wav=donor, language='pl', file_path=out_wav, temperature=0.25, split_sentences=False)

with wave.open(out_wav, 'rb') as w:
    s = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(np.float32) / 32768.0

diff = np.abs(np.diff(s))
bad = np.where(diff > 0.35)[0]
print(f'Done! Dur={len(s)/24000:.2f}s, Peak={20*np.log10(np.max(np.abs(s))):.1f}dB, RMS={20*np.log10(np.sqrt(np.mean(s**2))):.1f}dB, Max step={np.max(diff):.3f}, Severe discontinuities: {len(bad)}', flush=True)
