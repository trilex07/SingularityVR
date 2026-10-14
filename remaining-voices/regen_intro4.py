import sys, os
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

OUT = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/out/intro'
ENB = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/story_en_reference/SP_VI_intro_Kismet_LOC_INT/SoundNodeWave'

JOBS = {
    'INTRO_1005_NIM': 'Zespół Spartan, tu Nimitz. Wkraczacie w rosyjską przestrzeń powietrzną. Zachować ostrożność.',
    'INTRO_1006_CP': 'Zrozumiałem, Nimitz, trzymamy kurs.',
    'INTRO_1010_CP': 'Zrozumiałem, Nimitz.',
    'INTRO_1016_CP': ['Dostaliśmy! Dostaliśmy! Mejday, mejday, mejday! Spadamy!',
                      'My... O mój Bo...'],
}

import miniaudio, array as pyarray

def ogg_to_wav(src, dst):
    dec = miniaudio.decode_file(src, output_format=miniaudio.SampleFormat.SIGNED16)
    x = np.array(pyarray.array('h', dec.samples), dtype=np.float64) / 32768.0
    if dec.nchannels == 2:
        x = x.reshape(-1, 2).mean(axis=1)
    if dec.sample_rate != 24000:
        idx = np.linspace(0, len(x) - 1, int(len(x) * 24000 / dec.sample_rate))
        x = np.interp(idx, np.arange(len(x)), x)
    import wave
    with wave.open(dst, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(24000)
        w.writeframes((np.clip(x, -1, 1) * 32767).astype(np.int16).tobytes())
    return dst

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading temp=0.5...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.5, quantize=True)
SR = 24000
for asset, text in JOBS.items():
    chunks = text if isinstance(text, list) else [text]
    ref = ogg_to_wav(os.path.join(ENB, asset + '.ogg'), os.path.join(OUT, '_ref2_' + asset + '.wav'))
    parts = []
    for ch in chunks:
        state = model.get_state_for_audio_prompt(ref)
        a = model.generate_audio(state, ch).numpy().squeeze()
        parts.append(np.atleast_1d(a).astype(np.float32))
    gap = np.zeros(int(SR * 0.15), dtype=np.float32)
    full = parts[0]
    for p in parts[1:]:
        full = np.concatenate([full, gap, p])
    full = full * (10 ** (-1 / 20) / max(1e-6, np.max(np.abs(full))))
    wavfile.write(os.path.join(OUT, asset + '.pl.wav'), SR, full)
    print('OK', asset, round(len(full) / SR, 1), 's', flush=True)
print('DONE')
