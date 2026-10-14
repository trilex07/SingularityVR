import sys, os
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'

OUTDIR = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/story_voicepack_xlingual'
REFDIR = os.path.join(OUTDIR, '_refs_en')
TESTDIR = os.path.join(OUTDIR, '_donor_test')
os.makedirs(TESTDIR, exist_ok=True)

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.65, quantize=True)

# A: podejrzany dawca (KID) + PROSTY tekst kontrolny
# B: dobry dawca (Demichev RL3_2009) + problematyczny tekst
import shutil
good_donor = REFDIR + '/425e388a18d457a5.wav'.replace('.wav', '_en.wav')
print('good donor exists:', os.path.exists(good_donor), flush=True)

tests = [
    ('A_kid_donor_prosty_tekst.wav', REFDIR + '/825b608f491f738b_en.wav', 'Dzień dobry, jak się masz?'),
    ('B_dobry_dawca_trudny_tekst.wav', good_donor, 'Pomóż mi z sireny, a potem eksplodowała.'),
]
for name, ref, text in tests:
    state = model.get_state_for_audio_prompt(ref)
    a = model.generate_audio(state, text).numpy().squeeze()
    import numpy as np
    wavfile.write(os.path.join(TESTDIR, name), 24000, np.atleast_1d(a).astype(np.float32))
    print('OK', name, flush=True)
print('DONE')
