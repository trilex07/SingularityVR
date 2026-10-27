from pathlib import Path
import sys

sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
from pocket_tts import TTSModel
from scipy.io import wavfile
import numpy as np

root = Path(r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio')
out = root / 'polish' / 'character_samples_12'
out.mkdir(parents=True, exist_ok=True)

model = TTSModel.load_model(
    config='hf://shefowl/pocket-tts-polish-6l/config.yaml',
    temp=0.55,
    quantize=True,
)

jobs = [
    ('01_renko_neutral', 'Renko', root / 'polish' / 'user_reference' / 'spokoj_reference.wav',
     'Musimy działać szybko. Sprawdźmy, co dzieje się w laboratorium.'),
    ('02_renko_tense', 'Renko', root / 'polish' / 'user_reference' / 'agresja_reference.wav',
     'Słyszycie to? Wszyscy na stanowiska, natychmiast!'),
    ('03_barisov_command', 'Barisov', root / 'polish' / 'accent_reference' / 'ru_dmitri_reference.wav',
     'Stać! Odłożyć broń i wykonywać moje rozkazy.'),
    ('04_barisov_briefing', 'Barisov', root / 'polish' / 'accent_reference' / 'ru_dmitri_reference.wav',
     'Sektor jest zabezpieczony. Przechodzimy do następnego punktu.'),
    ('05_barisov_angry', 'Barisov', root / 'polish' / 'accent_reference' / 'ru_dmitri_reference.wav',
     'Nie będę powtarzał tego drugi raz!'),
    ('06_demichev_cold', 'Demichev', root / 'polish' / 'accent_reference' / 'ru_ruslan_reference.wav',
     'Nie ma już odwrotu. Procedura zostanie uruchomiona.'),
    ('07_demichev_technical', 'Demichev', root / 'polish' / 'accent_reference' / 'ru_ruslan_reference.wav',
     'Rdzeń temporalny osiągnął niestabilność. Odłączcie zasilanie.'),
    ('08_demichev_threat', 'Demichev', root / 'polish' / 'accent_reference' / 'ru_ruslan_reference.wav',
     'Twoje działania tylko pogarszają sytuację.'),
    ('09_kathryn_scientific', 'Kathryn', root / 'polish' / 'accent_reference' / 'uk_tetiana_reference.wav',
     'Potrzebuję jeszcze chwili. Dane są prawie kompletne.'),
    ('10_kathryn_urgent', 'Kathryn', root / 'polish' / 'accent_reference' / 'uk_tetiana_reference.wav',
     'Zatrzymajcie reaktor, zanim będzie za późno!'),
    ('11_devlin_restrained', 'Devlin', root / 'polish' / 'accent_reference' / 'uk_mykyta_reference.wav',
     'Droga jest czysta. Możemy ruszać dalej.'),
    ('12_devlin_urgent', 'Devlin', root / 'polish' / 'accent_reference' / 'uk_mykyta_reference.wav',
     'Renko, za tobą! Uważaj!'),
]

manifest = []
for stem, speaker, reference, text in jobs:
    state = model.get_state_for_audio_prompt(str(reference))
    audio = model.generate_audio(state, text).numpy().squeeze().astype(np.float32)
    peak = float(np.max(np.abs(audio)))
    if peak > 0:
        audio *= (10 ** (-3 / 20)) / peak
    filename = stem + '.wav'
    wavfile.write(str(out / filename), model.sample_rate, audio)
    manifest.append({
        'file': filename,
        'speaker': speaker,
        'reference': str(reference.relative_to(root)).replace('\\', '/'),
        'text': text,
        'engine': 'Pocket-TTS Polish 6L',
        'temperature': 0.55,
        'peak_dbfs': -3.0,
    })
    print('generated', filename, flush=True)

(out / 'manifest.json').write_text(
    __import__('json').dumps(manifest, ensure_ascii=False, indent=2) + '\n',
    encoding='utf-8',
)
