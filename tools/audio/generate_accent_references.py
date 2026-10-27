from pathlib import Path
import sys

sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.piper_tools')
from piper import PiperVoice, SynthesisConfig

root = Path(r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio')
models = root / 'models' / 'piper'
out = root / 'polish' / 'accent_reference'
out.mkdir(parents=True, exist_ok=True)

jobs = [
    (
        'ru_dmitri_reference.wav',
        models / 'ru_RU-dmitri-medium' / 'ru_RU-dmitri-medium.onnx',
        'Мы продолжаем работу. Сектор будет закрыт до дальнейшего распоряжения.',
    ),
    (
        'ru_ruslan_reference.wav',
        models / 'ru_RU-ruslan-medium' / 'ru_RU-ruslan-medium.onnx',
        'Мы продолжаем работу. Сектор будет закрыт до дальнейшего распоряжения.',
    ),
    (
        'uk_tetiana_reference.wav',
        models / 'uk_UA-ukrainian_tts-medium' / 'uk_UA-ukrainian_tts-medium.onnx',
        'ми продовжуємо роботу. сектор буде закритий до подальшого наказу.',
    ),
    (
        'uk_mykyta_reference.wav',
        models / 'uk_UA-ukrainian_tts-medium' / 'uk_UA-ukrainian_tts-medium.onnx',
        'ми продовжуємо роботу. сектор буде закритий до подальшого наказу.',
    ),
]

for index, (name, model, text) in enumerate(jobs):
    voice = PiperVoice.load(model)
    with open(out / name, 'wb') as f:
        import wave
        with wave.open(f, 'wb') as wav:
            speaker_id = None
            if name == 'uk_tetiana_reference.wav':
                speaker_id = 2
            elif name == 'uk_mykyta_reference.wav':
                speaker_id = 1
            voice.synthesize_wav(text, wav, SynthesisConfig(speaker_id=speaker_id))
    print('generated', name, flush=True)
