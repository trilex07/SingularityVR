import sys, os
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

OUTDIR = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/story_voicepack_xlingual'
REFDIR = os.path.join(OUTDIR, '_refs_en')

jobs = {
    '825b608f491f738b.wav': ['Pomóż mi z sireny,', 'a potem eksplodowała,', 'a pani Barisov krzyknęła i uciekła z klasy.'],
    'db5a9bb47d436a5a.wav': ['Jak wiele jeszcze mogę znieść?', 'Singularność wybuchła dwa dni temu, a od tego czasu nie odpoczywałam.',
                              'Kilku pracowników medycznych było w stanie otworzyć triaż w tych tunelach, ale bez względu na to, ilu nas pracuje, pojawia się jeszcze więcej chorych i rannych.'],
}

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.65, quantize=True)
SR = 24000
for fname, chunks in jobs.items():
    ref = os.path.join(REFDIR, fname.replace('.wav', '_en.wav'))
    parts = []
    for i, ch in enumerate(chunks):
        state = model.get_state_for_audio_prompt(ref)  # SWIEZY state na kazdy chunk
        print('gen', fname, i, flush=True)
        a = model.generate_audio(state, ch).numpy().squeeze()
        parts.append(np.atleast_1d(a).astype(np.float32))
    gap = np.zeros(int(SR * 0.12), dtype=np.float32)
    full = parts[0]
    for p in parts[1:]:
        full = np.concatenate([full, gap, p])
    wavfile.write(os.path.join(OUTDIR, fname), SR, full)
    print('FIXED', fname, round(len(full) / SR, 1), 's', flush=True)
print('DONE')
