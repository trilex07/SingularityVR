"""TTS 409 MP. Dawca = wlasny oryginal EN z bulka pakietu (ogg->miniaudio).
Temp 0.3, chunki zdaniowe, -1dBFS PCM16. REVIEW przy tempie >2.0."""
import sys, os, json, re
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
sys.path.insert(0, 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/tools/audio')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

BASE = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices'
OUT = BASE + '/remaining-voices/out/mp'
os.makedirs(OUT, exist_ok=True)
UDIR = BASE + '/_work_u/mp'

mp = json.load(open(BASE + '/remaining-voices/mp-pl.json', encoding='utf-8'))
print('zadan:', len(mp), flush=True)

from ue3_sound_package import inspect
from patch_opening_dialogue import bulks
import miniaudio, array as pyarray

PKG = {}

def ogg_bytes(pkg, asset):
    if pkg not in PKG:
        PKG[pkg] = inspect(open(os.path.join(UDIR, pkg + '.u'), 'rb').read())
    for e in PKG[pkg]['exports']:
        if e['name'] == asset and e['class_name'] == 'SoundNodeWave':
            data = open(os.path.join(UDIR, pkg + '.u'), 'rb').read()
            return bulks(data, e)[1][1]
    return None

def ogg_to_ref(ob, dst):
    import tempfile
    with tempfile.NamedTemporaryFile(suffix='.ogg', delete=False) as t:
        t.write(ob)
        tmp = t.name
    dec = miniaudio.decode_file(tmp, output_format=miniaudio.SampleFormat.SIGNED16)
    os.unlink(tmp)
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

def split_sent(text):
    import re as _re
    parts = [p for p in _re.split(r'(?<=[.!?…])\s+', text.strip()) if p]
    return parts if parts else [text]

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading temp=0.3...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.3, quantize=True)
SR = 24000
rows = []
for i, r in enumerate(mp):
    pkg, asset = r['pkg'], r['asset']
    outp = os.path.join(OUT, pkg + '__' + asset + '.pl.wav')
    if os.path.exists(outp):
        rows.append({'package': pkg, 'asset': asset, 'note': 'exists-skip'})
        continue
    try:
        ob = ogg_bytes(pkg, asset)
        if not ob:
            rows.append({'package': pkg, 'asset': asset, 'verdict': 'NO_SOURCE'})
            continue
        ref = ogg_to_ref(ob, os.path.join(OUT, '_ref_%s_%s.wav' % (pkg, asset)))
        parts = []
        for ch in split_sent(r['pl']):
            state = model.get_state_for_audio_prompt(ref)
            a = model.generate_audio(state, ch).numpy().squeeze()
            parts.append(np.atleast_1d(a).astype(np.float32))
        gap = np.zeros(int(SR * 0.1), dtype=np.float32)
        full = parts[0]
        for p in parts[1:]:
            full = np.concatenate([full, gap, p])
        full = full * (10 ** (-1 / 20) / max(1e-6, np.max(np.abs(full))))
        dur = len(full) / SR
        tempo = dur / r['dur'] if r['dur'] else 0
        import array, struct
        with open(outp, 'wb') as fh:
            body = array.array('h', (np.clip(full, -1, 1) * 32767).astype(np.int16)).tobytes()
            fh.write(b'RIFF' + struct.pack('<I', 36 + len(body)) + b'WAVEfmt ')
            fh.write(struct.pack('<IHHIIHH', 16, 1, 1, SR, SR * 2, 2, 16))
            fh.write(b'data' + struct.pack('<I', len(body)))
            fh.write(body)
        rows.append({'package': pkg, 'asset': asset, 'en': r['en'], 'pl': r['pl'],
                     'dur': round(dur, 2), 'budget': r['dur'], 'tempo': round(tempo, 3),
                     'verdict': 'CANDIDATE' if tempo <= 2.0 else 'REVIEW_TEMPO'})
        if (i + 1) % 25 == 0:
            json.dump(rows, open(os.path.join(OUT, 'mp_rows.json'), 'w'), ensure_ascii=False, indent=1)
            print('postep %d/%d' % (i + 1, len(mp)), flush=True)
    except Exception as ex:
        rows.append({'package': pkg, 'asset': asset, 'verdict': 'FAIL', 'err': str(ex)[:100]})
json.dump(rows, open(os.path.join(OUT, 'mp_rows.json'), 'w'), ensure_ascii=False, indent=1)
from collections import Counter
print(Counter(r['verdict'] for r in rows), flush=True)
print('DONE')
