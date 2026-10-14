import sys, os, time
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'

REF = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/story_en_reference/SP_RL2_kismet05_1950_LOC_INT/SoundNodeWave/RL3_2009_DEMY.ogg'

from pocket_tts import TTSModel
t0 = time.time()
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.65, quantize=True)
print('load: %.1fs' % (time.time() - t0), flush=True)

import miniaudio, array as pyarray
import numpy as np
dec = miniaudio.decode_file(REF, output_format=miniaudio.SampleFormat.SIGNED16)
x = np.array(pyarray.array('h', dec.samples), dtype=np.float64) / 32768.0
if dec.nchannels == 2:
    x = x.reshape(-1, 2).mean(axis=1)
import wave
with wave.open('_tref.wav', 'wb') as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(24000)
    w.writeframes((np.clip(x, -1, 1) * 32767).astype(np.int16).tobytes())

t0 = time.time()
state = model.get_state_for_audio_prompt('_tref.wav')
print('prompt-state: %.1fs' % (time.time() - t0), flush=True)
t0 = time.time()
audio = model.generate_audio(state, 'Próbuję być miły. Ale nie łudź się, zabiję cię.')
print('generate (1 krotka linia): %.1fs' % (time.time() - t0), flush=True)
os.remove('_tref.wav')
