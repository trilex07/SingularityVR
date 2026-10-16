import os, sys
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.xtts_tools')
# UWAGA: nie importowac resynth_corrected (wciaga .pocket_tools na przod sys.path
# i jego tokenizers 0.23.2 psuje check wersji transformers). Lokalny decode ogg:
import miniaudio
import array as pyarray
import numpy as _np

def ogg_to_wav_mono24(src, dst):
    dec = miniaudio.decode_file(src, output_format=miniaudio.SampleFormat.SIGNED16)
    x = _np.array(pyarray.array('h', dec.samples), dtype=_np.float64) / 32768.0
    if dec.nchannels == 2:
        x = x.reshape(-1, 2).mean(axis=1)
    if dec.sample_rate != 24000:
        idx = _np.linspace(0, len(x) - 1, int(len(x) * 24000 / dec.sample_rate))
        x = _np.interp(idx, _np.arange(len(x)), x)
    import wave
    with wave.open(dst, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(24000)
        w.writeframes((_np.clip(x, -1, 1) * 32767).astype(_np.int16).tobytes())
    return dst
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
ENB = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference'
src = os.path.join(ENB, r'SP_VI5_kismet03_LOC_INT\SoundNodeWave\SD_SPT3_MOVE_IN4.ogg'.replace('\\', os.sep))
donor = os.path.join(OUT, 'shout_donor_en.wav')
if not os.path.exists(donor):
    ogg_to_wav_mono24(src, donor)
pl = {r['asset']: r['pl'] for r in
      json.load(open(REPO + r'\remaining-voices\intro-pl.json', encoding='utf-8'))}
from TTS.api import TTS
print('loading xtts...', flush=True)
tts = TTS('tts_models/multilingual/multi-dataset/xtts_v2', gpu=False)
p = os.path.join(OUT, 'INTRO_1016_CP.shout.wav')
tts.tts_to_file(pl['INTRO_1016_CP'], speaker_wav=donor, language='pl',
                file_path=p, temperature=0.85)
print('SYN ok', p, flush=True)
