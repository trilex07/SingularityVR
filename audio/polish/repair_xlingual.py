"""NAPRAWA oflagowanych: tekst na 2 czesci, generacja tym samym dawca EN, sklejka.
Backup starych do _backup_before_split/. Log: REPAIR_LOG.json."""
import sys, os, time, json, re
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'

AUDIO = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish'
MANIFEST = AUDIO + '/story_voicepack_nllb/manifest.jsonl'
OUTDIR = AUDIO + '/story_voicepack_xlingual'
REFDIR = os.path.join(OUTDIR, '_refs_en')
BAKDIR = os.path.join(OUTDIR, '_backup_before_split')
os.makedirs(BAKDIR, exist_ok=True)

with open(os.path.join(OUTDIR, 'QA_FLAGS.json'), encoding='utf-8') as f:
    flags = json.load(f)
targets = set()
for k in ('long_token', 'abrupt_tail', 'too_short'):
    for row in flags.get(k, []):
        targets.add(row[0])
print('do naprawy:', len(targets), flush=True)

entries = {}
with open(MANIFEST, encoding='utf-8') as f:
    for line in f:
        if line.strip():
            d = json.loads(line)
            entries[d['file']] = d

def split_pl(text):
    parts = re.split(r'(?<=[.!?…])\s+', text.strip())
    parts = [p for p in parts if p]
    if len(parts) < 2:
        parts = re.split(r'(?<=[,;:])\s+', text.strip())
        parts = [p for p in parts if p]
    if len(parts) < 2:
        words = text.split()
        parts = [' '.join(words[:len(words)//2]), ' '.join(words[len(words)//2:])]
    # balans: lacz w 2 polowy o zblizonej liczbie slow
    if len(parts) > 2:
        total = sum(len(p.split()) for p in parts)
        acc, h1 = 0, []
        for p in parts:
            h1.append(p)
            acc += len(p.split())
            if acc >= total / 2:
                break
        h2 = parts[len(h1):]
        parts = [' '.join(h1), ' '.join(h2)]
    return parts[0], parts[1]

import numpy as np
from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading model...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.65, quantize=True)
print('ready', flush=True)

SR = 24000
log = []
for fname in sorted(targets):
    e = entries.get(fname)
    if not e:
        print('SKIP brak wpisu:', fname, flush=True)
        continue
    ref = os.path.join(REFDIR, fname.replace('.wav', '_en.wav'))
    if not os.path.exists(ref):
        print('SKIP brak ref:', fname, flush=True)
        continue
    outp = os.path.join(OUTDIR, fname)
    if os.path.exists(outp):
        os.replace(outp, os.path.join(BAKDIR, fname))
    p1, p2 = split_pl(e.get('text_pl_machine', ''))
    try:
        state = model.get_state_for_audio_prompt(ref)
        a1 = model.generate_audio(state, p1).numpy().squeeze()
        a2 = model.generate_audio(state, p2).numpy().squeeze()
        gap = np.zeros(int(SR * 0.12), dtype=np.float32)
        full = np.concatenate([np.atleast_1d(a1).astype(np.float32), gap, np.atleast_1d(a2).astype(np.float32)])
        wavfile.write(outp, SR, full)
        log.append({'file': fname, 'part1': p1, 'part2': p2, 'dur_s': round(len(full) / SR, 1)})
        print('FIXED', fname, round(len(full) / SR, 1), 's', flush=True)
    except Exception as ex:
        print('FAIL', fname, type(ex).__name__, str(ex)[:150], flush=True)
with open(os.path.join(OUTDIR, 'REPAIR_LOG.json'), 'w', encoding='utf-8') as f:
    json.dump(log, f, ensure_ascii=False, indent=1)
print('DONE fixed=%d' % len(log), flush=True)
