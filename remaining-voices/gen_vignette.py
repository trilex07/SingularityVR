"""Punkt 1b: 6 kwestii Vignette (SC1/SC2) - czyste teksty, dawcy wlasne EN, temp 0.5."""
import sys, os
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

EN_BASE = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/story_en_reference'
OUT = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/out/vignette'
os.makedirs(OUT, exist_ok=True)

JOBS = [
    ('RT99_0105_SC1', 'SP_RT3_Vignette_LOC_INT/SoundNodeWave/RT99_0105_SC1.ogg',
     'Bo to nie ma sensu, Michale! Narażasz tych ludzi bez powodu!'),
    ('RT99_0106_SC2', 'SP_RT3_Vignette_LOC_INT/SoundNodeWave/RT99_0106_SC2.ogg',
     'A ty tylko czekasz na śmierć.'),
    ('RT99_0107_SC1', 'SP_RT3_Vignette_LOC_INT/SoundNodeWave/RT99_0107_SC1.ogg',
     'Ale tam jest zbyt niebezpiecznie!'),
    ('RT99_0108_SC2', 'SP_RT3_Vignette_LOC_INT/SoundNodeWave/RT99_0108_SC2.ogg',
     'Posłuchaj mnie. Kanały doprowadzą nas do parowozowni. Stamtąd weźmiemy lokomotywę do doków i uciekniemy.'),
    ('RT99_0111_SC1', 'SP_RT3_Vignette_LOC_INT/SoundNodeWave/RT99_0111_SC1.ogg',
     'Nie. To samobójstwo.'),
    ('RT99_0112_SC2', 'SP_RT3_Vignette_LOC_INT/SoundNodeWave/RT99_0112_SC2.ogg',
     'Bardzo dobrze. Życzę ci wszystkiego najlepszego, przyjacielu. Mam nadzieję, że zobaczymy się na stałym lądzie, w otoczeniu rodziny, towarzyszu.'),
]

import miniaudio, array as pyarray

def ogg_to_wav(src, dst):
    dec = miniaudio.decode_file(src, output_format=miniaudio.SampleFormat.SIGNED16)
    x = np.array(pyarray.array('h', dec.samples), dtype=np.float64) / 32768.0
    if dec.nchannels == 2:
        x = x.reshape(-1, 2).mean(axis=1)  # downmix stereo SC1 -> mono
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
print('loading...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.5, quantize=True)
for asset, ogg, text in JOBS:
    ref = ogg_to_wav(os.path.join(EN_BASE, ogg), os.path.join(OUT, '_ref_' + asset + '.wav'))
    state = model.get_state_for_audio_prompt(ref)
    a = model.generate_audio(state, text).numpy().squeeze()
    wavfile.write(os.path.join(OUT, asset + '.pl.wav'), 24000, np.atleast_1d(a).astype(np.float32))
    print('OK', asset, flush=True)
print('DONE')
