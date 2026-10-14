"""RUNDA 3: 13 wyczyszczonych tekstow (CLEAN13.json), dawcy z puli, temp 0.5, fresh state."""
import sys, os, json
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

AUDIO = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish'
OUTDIR = AUDIO + '/story_voicepack_xlingual'
REFDIR = os.path.join(OUTDIR, '_refs_en')
BAK3 = os.path.join(OUTDIR, '_backup_r3')
os.makedirs(BAK3, exist_ok=True)

POOL = {
    'barisov_dmitri': ['691d44b2b46e8adc_en.wav', '64f99c874ecc2128_en.wav'],
    'demichev_ruslan': ['425e388a18d457a5_en.wav', 'f1e4518cef6740f1_en.wav'],
    'kathryn_tetiana': ['b34e99453cc1f788_en.wav', '38f168ed9f807bf5_en.wav'],
    'narrator_user': ['5339b0068ab0840e_en.wav', '645b6de719763bc4_en.wav', '1f71c047ef2edb87_en.wav'],
    'renko_user': ['29aff4a4dd00f402_en.wav'],
}
MANIFEST = AUDIO + '/story_voicepack_nllb/manifest.jsonl'
prof = {}
with open(MANIFEST, encoding='utf-8') as f:
    for line in f:
        if line.strip():
            d = json.loads(line)
            prof[d['file']] = d.get('profile', 'narrator_user')

with open(os.path.join(AUDIO, 'CLEAN13.json'), encoding='utf-8') as f:
    clean = json.load(f)

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.5, quantize=True)
SR = 24000
log = []
for fname, chunks in clean.items():
    pool = POOL.get(prof.get(fname, 'narrator_user'), POOL['narrator_user'])
    ref = os.path.join(REFDIR, pool[0])
    outp = os.path.join(OUTDIR, fname)
    if os.path.exists(outp):
        os.replace(outp, os.path.join(BAK3, fname))
    try:
        parts = []
        for ch in chunks:
            state = model.get_state_for_audio_prompt(ref)
            a = model.generate_audio(state, ch).numpy().squeeze()
            parts.append(np.atleast_1d(a).astype(np.float32))
        gap = np.zeros(int(SR * 0.12), dtype=np.float32)
        full = parts[0]
        for p in parts[1:]:
            full = np.concatenate([full, gap, p])
        wavfile.write(outp, SR, full)
        log.append({'file': fname, 'donor': pool[0], 'dur': round(len(full)/SR, 1)})
        print('R3', fname, round(len(full)/SR, 1), 's', flush=True)
    except Exception as ex:
        print('FAIL', fname, type(ex).__name__, str(ex)[:120], flush=True)
with open(os.path.join(OUTDIR, 'REPAIR3_LOG.json'), 'w', encoding='utf-8') as f:
    json.dump(log, f, ensure_ascii=False, indent=1)
print('DONE r3=%d' % len(log), flush=True)
