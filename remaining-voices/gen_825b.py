import sys, os
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

OUT = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/out/825b'
os.makedirs(OUT, exist_ok=True)
REF = ('C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/'
       'story_voicepack_xlingual/_refs_en/645b6de719763bc4_en.wav')

CHUNKS = ['Pomocy! Syreny! A potem nastąpił wybuch, pani Barisow krzyknęła i wybiegła z klasy.',
          'Zostawiła nas tam.']

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading temp=0.3...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.3, quantize=True)
SR = 24000
parts = []
for ch in CHUNKS:
    state = model.get_state_for_audio_prompt(REF)
    a = model.generate_audio(state, ch).numpy().squeeze()
    parts.append(np.atleast_1d(a).astype(np.float32))
gap = np.zeros(int(SR * 0.3), dtype=np.float32)
full = np.concatenate([parts[0], gap, parts[1]])
wavfile.write(os.path.join(OUT, '825b608f491f738b.pl.wav'), SR, full)
print('OK %.1fs' % (len(full) / SR), flush=True)
