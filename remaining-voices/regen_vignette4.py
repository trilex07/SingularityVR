import sys, os
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

OUT = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/out/vignette'
REF = ('C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/'
       'story_voicepack_xlingual/_refs_en/645b6de719763bc4_en.wav')

JOBS = {
    'RT99_0105_SC1': ['Bo to nie ma sensu, Michale!', 'Narażasz tych ludzi bez powodu!'],
    'RT99_0108_SC2': ['Posłuchaj mnie.', 'Kanały doprowadzą nas do parowozowni.',
                      'Stamtąd weźmiemy lokomotywę do doków i uciekniemy.'],
    'RT99_0111_SC1': ['Nie.', 'To samobójstwo.'],
    'RT99_0112_SC2': ['Bardzo dobrze.', 'Życzę ci wszystkiego najlepszego, przyjacielu.',
                      'Mam nadzieję, że zobaczymy się na stałym lądzie, w otoczeniu rodziny, towarzyszu.'],
}

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading temp=0.3...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.3, quantize=True)
SR = 24000
for asset, chunks in JOBS.items():
    parts = []
    for ch in chunks:
        state = model.get_state_for_audio_prompt(REF)
        a = model.generate_audio(state, ch).numpy().squeeze()
        parts.append(np.atleast_1d(a).astype(np.float32))
    gap = np.zeros(int(SR * 0.2), dtype=np.float32)
    full = parts[0]
    for p in parts[1:]:
        full = np.concatenate([full, gap, p])
    wavfile.write(os.path.join(OUT, asset + '.pl.wav'), SR, full)
    print('OK', asset, round(len(full) / SR, 1), 's', flush=True)
print('DONE')
