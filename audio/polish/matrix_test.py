import sys, os
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

OUTDIR = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/story_voicepack_xlingual'
REFDIR = os.path.join(OUTDIR, '_refs_en')
TESTDIR = os.path.join(OUTDIR, '_matrix_test')
os.makedirs(TESTDIR, exist_ok=True)

TEXT = 'Pomocy! Syreny! A potem nastąpił wybuch.'
MATRIX = [
    ('t03_donor5339.wav', 0.3, '5339b0068ab0840e_en.wav'),
    ('t03_donor645b.wav', 0.3, '645b6de719763bc4_en.wav'),
    ('t05_donor5339.wav', 0.5, '5339b0068ab0840e_en.wav'),
    ('t05_donor645b.wav', 0.5, '645b6de719763bc4_en.wav'),
]

from pocket_tts import TTSModel
from scipy.io import wavfile
import importlib
import pocket_tts
print('start', flush=True)
for name, temp, ref in MATRIX:
    print('loading temp=', temp, flush=True)
    importlib.reload(pocket_tts)
    from pocket_tts import TTSModel as T2
    model = T2.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=temp, quantize=True)
    state = model.get_state_for_audio_prompt(os.path.join(REFDIR, ref))
    a = model.generate_audio(state, TEXT).numpy().squeeze()
    wavfile.write(os.path.join(TESTDIR, name), 24000, np.atleast_1d(a).astype(np.float32))
    print('OK', name, flush=True)
print('DONE')
