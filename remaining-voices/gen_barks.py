"""TTS 268 okrzykow werbalnych. Dawca = najdluzszy okrzyk w grupie frakcyjnej (stabilny prompt).
Temp 0.3, normalizacja -1dBFS PCM16, wiersze manifestu z tempem (REVIEW przy >2.0)."""
import sys, os, json, re
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

BASE = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices'
ENB = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/story_en_reference'
OUT = BASE + '/out/barks'
os.makedirs(OUT, exist_ok=True)

inv = json.load(open(BASE + '/bark-speech.json', encoding='utf-8'))
verb = set(json.load(open(BASE + '/bark-class.json', encoding='utf-8'))['verbal'])
pl = {r['asset']: r for r in json.load(open(BASE + '/bark-pl.json', encoding='utf-8'))}

def fac(name):
    m = re.match(r'SD_[A-Z0-9]+_([A-Z]+)', name)
    return m.group(1) if m else 'OTHER'

groups = {}
for n in sorted(verb):
    groups.setdefault(fac(n), []).append(n)
print('grup frakcyjnych:', len(groups), flush=True)

import miniaudio, array as pyarray

def ogg_dur(path):
    dec = miniaudio.decode_file(path, output_format=miniaudio.SampleFormat.SIGNED16)
    n = len(pyarray.array('h', dec.samples))
    ch = dec.nchannels
    return (n // ch) / dec.sample_rate

donors = {}
for f, names in groups.items():
    best, bd = None, 0
    for n in names:
        p = os.path.join(ENB, inv[n].get('package', ''), 'SoundNodeWave', n + '.ogg')
        # package_hint to pojedynczy pakiet; ogg moze byc w innym - szukaj
        if not os.path.exists(p):
            import glob
            hits = glob.glob(os.path.join(ENB, '*/SoundNodeWave', n + '.ogg'))
            if not hits:
                continue
            p = hits[0]
        try:
            d = ogg_dur(p)
        except Exception:
            continue
        if d > bd:
            bd, best = d, p
    donors[f] = (best, round(bd, 2))
    print('dawca', f, (os.path.basename(best) if best else None), bd, flush=True)

json.dump({k: v[0] for k, v in donors.items()},
          open(os.path.join(OUT, 'donors.json'), 'w'), ensure_ascii=False, indent=1)

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading temp=0.3...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.3, quantize=True)
SR = 24000

def ogg_to_ref(src, dst):
    dec = miniaudio.decode_file(src, output_format=miniaudio.SampleFormat.SIGNED16)
    x = np.array(pyarray.array('h', dec.samples), dtype=np.float64) / 32768.0
    if dec.nchannels == 2:
        x = x.reshape(-1, 2).mean(axis=1)
    if dec.sample_rate != SR:
        idx = np.linspace(0, len(x) - 1, int(len(x) * SR / dec.sample_rate))
        x = np.interp(idx, np.arange(len(x)), x)
    import wave
    with wave.open(dst, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes((np.clip(x, -1, 1) * 32767).astype(np.int16).tobytes())
    return dst

import math
rows = []
refcache = {}
n = 0
for f, names in sorted(groups.items()):
    donor, _ = donors[f]
    if donor is None:
        for x in names:
            rows.append({'asset': x, 'verdict': 'NO_DONOR'})
        continue
    rk = donor
    if rk not in refcache:
        refcache[rk] = ogg_to_ref(donor, os.path.join(OUT, '_donor_%s.wav' % f))
    ref = refcache[rk]
    for name in names:
        t = pl.get(name, {}).get('pl', '')
        if not t:
            rows.append({'asset': name, 'verdict': 'NO_TEXT'})
            continue
        try:
            state = model.get_state_for_audio_prompt(ref)
            a = model.generate_audio(state, t).numpy().squeeze()
            full = np.atleast_1d(a).astype(np.float32)
            full = full * (10 ** (-1 / 20) / max(1e-6, np.max(np.abs(full))))
            dur = len(full) / SR
            budget = inv[name]['dur'] or dur
            tempo = dur / budget if budget else 0
            import array
            outp = os.path.join(OUT, name + '.pl.wav')
            with open(outp, 'wb') as fh:
                body = array.array('h', (np.clip(full, -1, 1) * 32767).astype(np.int16)).tobytes()
                fh.write(b'RIFF' + __import__('struct').pack('<I', 36 + len(body)) + b'WAVEfmt ')
                fh.write(__import__('struct').pack('<IHHIIHH', 16, 1, 1, SR, SR * 2, 2, 16))
                fh.write(b'data' + __import__('struct').pack('<I', len(body)))
                fh.write(body)
            rows.append({'asset': name, 'en': inv[name]['spoken'], 'pl': t,
                         'donor_fac': f, 'dur': round(dur, 2), 'budget': budget,
                         'tempo': round(tempo, 3),
                         'verdict': 'CANDIDATE' if tempo <= 2.0 else 'REVIEW_TEMPO'})
            n += 1
            if n % 40 == 0:
                print('postep %d' % n, flush=True)
        except Exception as ex:
            rows.append({'asset': name, 'verdict': 'FAIL', 'err': str(ex)[:100]})
json.dump(rows, open(os.path.join(OUT, 'bark_rows.json'), 'w'), ensure_ascii=False, indent=1)
from collections import Counter
print(Counter(r['verdict'] for r in rows), flush=True)
print('DONE')
