import sys, os
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

OUT = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/out/intro'
REF = ('C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/'
       'story_voicepack_xlingual/_refs_en/645b6de719763bc4_en.wav')

JOBS = {
    'INTRO_1005_NIM': 'Zespół Spartan, mówi Nimitz. Wkraczacie w rosyjską przestrzeń powietrzną. Zachować ostrożność.',
    'INTRO_1006_CP': 'Zrozumiałem, trzymamy kurs.',
    'INTRO_1010_CP': 'Zrozumiałem.',
}

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading temp=0.3...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.3, quantize=True)
SR = 24000
for asset, text in JOBS.items():
    state = model.get_state_for_audio_prompt(REF)
    a = model.generate_audio(state, text).numpy().squeeze()
    full = np.atleast_1d(a).astype(np.float32)
    full = full * (10 ** (-1 / 20) / max(1e-6, np.max(np.abs(full))))
    wavfile.write(os.path.join(OUT, asset + '.pl.wav'), SR, full)
    print('OK', asset, round(len(full) / SR, 1), 's', flush=True)
print('DONE')
