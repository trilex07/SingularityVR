import sys, os
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

OUT = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/out/intro'
os.makedirs(OUT, exist_ok=True)
ENB = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/story_en_reference/SP_VI_intro_Kismet_LOC_INT/SoundNodeWave'

# recznie wyczyszczone (1016: naprawa zapetlenia; 1006/1007/1008/1005: sens)
JOBS = {
    'INTRO_1005_NIM': 'Zespół Spartan, Nimitz Actual, wkraczacie w rosyjską przestrzeń powietrzną. Zachować ostrożność.',
    'INTRO_1006_CP': 'Zrozumiałem, Nimitz, trzymamy kurs.',
    'INTRO_1007_GST': 'Sztab musi mieć pełne portki, skoro dla wysłania nas ryzykuje międzynarodowy incydent.',
    'INTRO_1008_GST': 'Wywiad mówi, że ta placówka jest opuszczona od pięćdziesięciu lat, ale wiecie, co mówią o wojsku.',
    'INTRO_1009_NIM': 'Zespół Spartan, utrzymać prędkość i kurs.',
    'INTRO_1010_CP': 'Zrozumiałem, Nimitz.',
    'INTRO_1011_GST': 'Cholera... Widzisz to?',
    'INTRO_1012_NIM': 'Spartan dwa-jeden, Spartan dwa-jeden, odczytujemy skok promieniowania na waszym wektorze.',
    'INTRO_1014_GST': 'Trzymaj się, Renko, czeka nas jazda!',
    'INTRO_1016_CP': ['Dostaliśmy! Dostaliśmy! Mayday, mayday, mayday! Spadamy!',
                      'My... O mój Bo...'],
}
# INTRO_1015_GST (Aaaaaaagh!) - niewerbalne, zostaje EN.

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
    ref = ogg_to_wav(os.path.join(ENB, asset + '.ogg'), os.path.join(OUT, '_ref_' + asset + '.wav'))
    try:
        parts = []
        for ch in chunks:
            state = model.get_state_for_audio_prompt(ref)
            a = model.generate_audio(state, ch).numpy().squeeze()
            parts.append(np.atleast_1d(a).astype(np.float32))
        gap = np.zeros(int(SR * 0.15), dtype=np.float32)
        full = parts[0]
        for p in parts[1:]:
            full = np.concatenate([full, gap, p])
        # POZIOM EMISYJNY: peak -1 dBFS (poprzednik byl za cichy)
        full = full * (10 ** (-1 / 20) / max(1e-6, np.max(np.abs(full))))
        wavfile.write(os.path.join(OUT, asset + '.pl.wav'), SR, full)
        print('OK', asset, round(len(full) / SR, 1), 's', flush=True)
    except Exception as ex:
        print('FAIL', asset, type(ex).__name__, str(ex)[:100], flush=True)
print('DONE')
