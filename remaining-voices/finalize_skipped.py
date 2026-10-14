"""Finalizacja punktu 2: wariant RT4B_1512_6_KAT (sp_rt3_katya) + trim + tempo + wiersze manifestu."""
import sys, os, json, io, re, struct
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

BASE = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices'
OUT = BASE + '/out/skipped'
EN_BASE = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/story_en_reference'

import ftfy

nllb = {}
raw = io.open('C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/story_voicepack_nllb/manifest.jsonl',
              encoding='utf-8', errors='replace').read()
for line in raw.splitlines():
    if line.strip():
        d = json.loads(line)
        nllb[(d['package'], d['asset'])] = d

b = json.load(open('C:/Users/Trilex/vr-mods/SingularityVR-recovery/deployment/hands-dubbing/xlingual-banks/full/out/xlingual-banks.json', encoding='utf-8'))
jobs = {}  # (package, asset) -> {budget, old_tempo}
for e in b:
    for s in e.get('skipped', []):
        jobs[(e['package'], s['asset'])] = {'budget': s['native_duration'], 'old_tempo': s.get('tempo')}

# 1) wariant sp_rt3_katya
d = nllb.get(('sp_rt3_katya_LOC_INT', 'RT4B_1512_6_KAT'))
print('wariant tekst:', (d['text_pl_machine'] if d else None), flush=True)

import miniaudio, array as pyarray

def ogg_to_wavmono(src):
    dec = miniaudio.decode_file(src, output_format=miniaudio.SampleFormat.SIGNED16)
    x = np.array(pyarray.array('h', dec.samples), dtype=np.float64) / 32768.0
    if dec.nchannels == 2:
        x = x.reshape(-1, 2).mean(axis=1)
    if dec.sample_rate != 24000:
        idx = np.linspace(0, len(x) - 1, int(len(x) * 24000 / dec.sample_rate))
        x = np.interp(idx, np.arange(len(x)), x)
    return x

def load_float(p):
    with open(p, 'rb') as fh:
        raw = fh.read()
    pos = 12; sr = 0; data = None
    while pos + 8 <= len(raw):
        cid = raw[pos:pos+4]; sz = struct.unpack('<I', raw[pos+4:pos+8])[0]
        body = raw[pos+8:pos+8+sz]
        if cid == b'fmt ':
            sr = struct.unpack('<HHI', body[:8])[2]
        elif cid == b'data':
            data = body
        pos += 8 + sz + (sz & 1)
    n = len(data) // 4
    return np.array(struct.unpack('<%df' % n, data[:n*4])), sr

def trim(x, sr, thr=0.03, guard_ms=40):
    win = sr // 100
    env = np.array([np.sqrt(np.mean(x[i:i+win]**2)) for i in range(0, len(x), win)])
    loud = np.where(env > thr)[0]
    if len(loud) == 0:
        return x, 0.0
    a = max(0, loud[0] * win - int(sr * guard_ms / 1000))
    b = min(len(x), (loud[-1] + 1) * win + int(sr * guard_ms / 1000))
    return x[a:b], (b - a) / sr

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading temp=0.35...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.35, quantize=True)

# wariant katya
KKEY = ('sp_rt3_katya_LOC_INT', 'RT4B_1512_6_KAT')
kd = nllb.get(KKEY)
if kd:
    text = ftfy.fix_text(kd['text_pl_machine'])
    ref = ogg_to_wavmono(os.path.join(EN_BASE, kd['source']))
    import wave
    with wave.open(os.path.join(OUT, '_kref.wav'), 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(24000)
        w.writeframes((np.clip(ref, -1, 1) * 32767).astype(np.int16).tobytes())
    state = model.get_state_for_audio_prompt(os.path.join(OUT, '_kref.wav'))
    a = model.generate_audio(state, text).numpy().squeeze()
    t, dur = trim(np.atleast_1d(a).astype(np.float32), 24000)
    wavfile.write(os.path.join(OUT, 'sp_rt3_katya__RT4B_1512_6_KAT.pl.wav'), 24000, t)
    print('wariant katya: %.2fs' % dur, flush=True)

# 2) trim + tempo dla wszystkich (klucz: package__asset, pliki po asset)
import hashlib
rows = []
for (pkg, asset), j in sorted(jobs.items()):
    f = os.path.join(OUT, asset + '.pl.wav')
    special = os.path.join(OUT, pkg + '__' + asset + '.pl.wav')
    if pkg == 'sp_rt3_katya_LOC_INT' and asset == 'RT4B_1512_6_KAT':
        f = special
    if not os.path.exists(f):
        rows.append({'package': pkg, 'asset': asset, 'verdict': 'MISSING_FILE'})
        continue
    x, sr = load_float(f)
    t, dur = trim(x, sr)
    tempo = dur / j['budget']
    h = hashlib.sha256(open(f, 'rb').read()).hexdigest()
    peak = float(np.max(np.abs(t)))
    import math
    rows.append({'package': pkg, 'asset': asset, 'budget': round(j['budget'], 2),
                 'old_tempo': j.get('old_tempo'), 'dur': round(dur, 2),
                 'tempo': round(tempo, 3), 'peak_db': round(20 * math.log10(max(peak, 1e-9)), 1),
                 'sha256': h[:16],
                 'verdict': 'CANDIDATE' if tempo <= 1.3 else ('REVIEW' if tempo <= 1.85 else 'STAYS_EN')})
json.dump(rows, open(os.path.join(OUT, 'skipped_final.json'), 'w'), ensure_ascii=False, indent=1)
from collections import Counter
print(Counter(r['verdict'] for r in rows), flush=True)
print('DONE')
