import sys
from pathlib import Path

sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
from pocket_tts import TTSModel
from scipy.io import wavfile

root = Path(r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\user_reference')
model = TTSModel.load_model(
    config='hf://shefowl/pocket-tts-polish-6l/config.yaml',
    temp=0.65,
    quantize=True,
)

jobs = [
    ('side_user_command', root / 'agresja_reference.wav',
     'Stać! Odłóż broń i wykonuj moje rozkazy.'),
    ('side_user_cold', root / 'spokoj_reference.wav',
     'Nie ma już odwrotu. Uruchom procedurę i zamknij sektor.'),
]

for name, ref, text in jobs:
    state = model.get_state_for_audio_prompt(str(ref))
    audio = model.generate_audio(state, text)
    wavfile.write(str(root / (name + '.wav')), model.sample_rate, audio.numpy().squeeze())
    print('generated', name, flush=True)
