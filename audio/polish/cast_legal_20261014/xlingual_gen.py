import sys, os
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import miniaudio, array as pyarray
import numpy as np

REF_DIR = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/story_en_reference'
OUT = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/cast_legal_20261014/casting_final/role_xlingual'
os.makedirs(OUT, exist_ok=True)

def ogg_to_wav(src, dst, rate=24000):
    dec = miniaudio.decode_file(src, output_format=miniaudio.SampleFormat.SIGNED16)
    x = np.array(pyarray.array('h', dec.samples), dtype=np.float64) / 32768.0
    if dec.nchannels == 2:
        x = x.reshape(-1, 2).mean(axis=1)
    if dec.sample_rate != rate:
        idx = np.linspace(0, len(x) - 1, int(len(x) * rate / dec.sample_rate))
        x = np.interp(idx, np.arange(len(x)), x)
    import wave
    with wave.open(dst, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
        w.writeframes((np.clip(x, -1, 1) * 32767).astype(np.int16).tobytes())
    print('ref ready:', dst, '%.1fs' % (len(x) / rate), flush=True)

jobs = [
    ('XL_demichev_en_voice_pl_text.wav',
     REF_DIR + '/SP_RL2_kismet05_1950_LOC_INT/SoundNodeWave/RL3_2009_DEMY.ogg',
     'Próbuję być miły. Ale nie łudź się, zabiję cię, jeśli nie powiesz mi, gdzie jest TMD.'),
    ('XL_kathryn_en_voice_pl_text.wav',
     REF_DIR + '/SP_Freight_Script_StreamBridge_LOC_INT/SoundNodeWave/FR3_5505_KAT.ogg',
     'Renko, jesteśmy na doku, ale ludzie Demicheva są wszędzie. Miej się na baczności!'),
]

refs = []
for out_name, ogg, text in jobs:
    ref_wav = os.path.join(OUT, 'ref_' + out_name.replace('.wav', '_en.wav'))
    ogg_to_wav(ogg, ref_wav)
    refs.append((out_name, ref_wav, text))

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading model...', flush=True)
model = TTSModel.load_model(
    config='hf://shefowl/pocket-tts-polish-6l/config.yaml',
    temp=0.65,
    quantize=True,
)
print('model loaded, generating...', flush=True)
for out_name, ref_wav, text in refs:
    state = model.get_state_for_audio_prompt(ref_wav)
    audio = model.generate_audio(state, text)
    wavfile.write(os.path.join(OUT, out_name), model.sample_rate, audio.numpy().squeeze())
    print('generated', out_name, flush=True)
print('DONE')
