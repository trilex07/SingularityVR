"""RUNDA 2: 17 BAD -> czysty dawca z puli (ten sam profil), tekst bez '/', temp 0.5,
swiezy state na chunk, split >25 slow. Backup juz istnieje (repair) / nowe do _backup_r2/."""
import sys, os, re, json
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

AUDIO = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish'
OUTDIR = AUDIO + '/story_voicepack_xlingual'
REFDIR = os.path.join(OUTDIR, '_refs_en')
BAK2 = os.path.join(OUTDIR, '_backup_r2')
os.makedirs(BAK2, exist_ok=True)
MANIFEST = AUDIO + '/story_voicepack_nllb/manifest.jsonl'

POOL = {
    'barisov_dmitri': ['691d44b2b46e8adc_en.wav', '64f99c874ecc2128_en.wav'],
    'demichev_ruslan': ['425e388a18d457a5_en.wav', 'f1e4518cef6740f1_en.wav'],
    'kathryn_tetiana': ['b34e99453cc1f788_en.wav', '38f168ed9f807bf5_en.wav'],
    'narrator_user': ['5339b0068ab0840e_en.wav', '645b6de719763bc4_en.wav', '1f71c047ef2edb87_en.wav'],
    'renko_user': ['29aff4a4dd00f402_en.wav'],
}

with open(os.path.join(OUTDIR, 'ASR_QA.json'), encoding='utf-8') as f:
    qa = json.load(f)
bad = [r['file'] for r in qa if r['recall'] < 0.5]
print('BAD:', len(bad), flush=True)

entries = {}
with open(MANIFEST, encoding='utf-8') as f:
    for line in f:
        if line.strip():
            d = json.loads(line)
            entries[d['file']] = d

def clean(t):
    t = t.replace('/', ' ')
    t = re.sub(r'\s+', ' ', t).strip()
    return t

def split25(t):
    if len(t.split()) <= 25:
        return [t]
    parts = re.split(r'(?<=[.!?…])\s+', t)
    parts = [p for p in parts if p]
    if len(parts) < 2:
        parts = [p for p in re.split(r'(?<=[,;:])\s+', t) if p]
    out, cur = [], ''
    for p in parts:
        if len((cur + ' ' + p).split()) <= 25 and cur:
            cur = cur + ' ' + p
        else:
            if cur:
                out.append(cur)
            cur = p
    if cur:
        out.append(cur)
    if len(out) < 2 and len(t.split()) > 25:
        w = t.split()
        out = [' '.join(w[:len(w)//2]), ' '.join(w[len(w)//2:])]
    return out

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.5, quantize=True)
SR = 24000
log = []
for fname in sorted(bad):
    e = entries.get(fname)
    if not e:
        continue
    pool = POOL.get(e.get('profile', ''), POOL['narrator_user'])
    ref = os.path.join(REFDIR, pool[0])
    if not os.path.exists(ref):
        print('SKIP brak ref', fname, flush=True)
        continue
    outp = os.path.join(OUTDIR, fname)
    if os.path.exists(outp):
        os.replace(outp, os.path.join(BAK2, fname))
    chunks = split25(clean(e.get('text_pl_machine', '')))
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
        log.append({'file': fname, 'donor': pool[0], 'chunks': len(chunks), 'dur': round(len(full)/SR, 1)})
        print('R2', fname, len(chunks), 'ch, donor', pool[0][:16], round(len(full)/SR, 1), 's', flush=True)
    except Exception as ex:
        print('FAIL', fname, type(ex).__name__, str(ex)[:120], flush=True)
with open(os.path.join(OUTDIR, 'REPAIR2_LOG.json'), 'w', encoding='utf-8') as f:
    json.dump(log, f, ensure_ascii=False, indent=1)
print('DONE r2=%d' % len(log), flush=True)
