"""Retry 31 zapetlonych (tempo>4) z temp 0.2, ten sam dawca frakcyjny."""
import sys, os, json
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

BASE = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices'
OUT = BASE + '/out/barks'
rows = json.load(open(OUT + '/bark_rows.json', encoding='utf-8'))
todo = [r for r in rows if r.get('verdict') == 'REVIEW_TEMPO' and r['tempo'] > 4.0]
print('retry:', len(todo), flush=True)
donors = json.load(open(OUT + '/donors.json', encoding='utf-8'))
inv = json.load(open(BASE + '/bark-speech.json', encoding='utf-8'))
import re

def fac(name):
    m = re.match(r'SD_[A-Z0-9]+_([A-Z]+)', name)
    return m.group(1) if m else 'OTHER'

# donor per asset = ten sam co poprzednio (z donors.json po frakcji) - odtworz
fac_of = {}
for r in rows:
    fac_of[r['asset']] = fac(r['asset'])

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading temp=0.2...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.2, quantize=True)
SR = 24000

def ogg_to_ref(src, dst):
    import miniaudio, array as pyarray
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

import glob, math
ENB = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/story_en_reference'
for i, r in enumerate(todo):
    f = fac_of[r['asset']]
    donor = donors.get(f)
    if not donor or not os.path.exists(donor):
        hits = glob.glob(os.path.join(ENB, '*/SoundNodeWave', r['asset'] + '.ogg'))
        if not hits:
            r['retry'] = 'NO_SOURCE'
            continue
        # dawca frakcyjny: najdluzszy ogg z grupy
        cands = []
        for rr in rows:
            if fac_of.get(rr['asset']) == f:
                for h in glob.glob(os.path.join(ENB, '*/SoundNodeWave', rr['asset'] + '.ogg')):
                    cands.append(h)
                    break
        donor = cands[0] if cands else hits[0]
    ref = os.path.join(OUT, '_retry_ref_%s.wav' % f)
    ogg_to_wav = ogg_to_wav if False else None
    try:
        from gen_barks import ogg_to_ref as _  # noqa
    except Exception:
        pass
    # lokalna konwersja (nie importujemy gen_barks zeby nie odpalic batcha)
    import miniaudio, array as pyarray
    dec = miniaudio.decode_file(donor, output_format=miniaudio.SampleFormat.SIGNED16)
    x = np.array(pyarray.array('h', dec.samples), dtype=np.float64) / 32768.0
    if dec.nchannels == 2:
        x = x.reshape(-1, 2).mean(axis=1)
    if dec.sample_rate != SR:
        idx = np.linspace(0, len(x) - 1, int(len(x) * SR / dec.sample_rate))
        x = np.interp(idx, np.arange(len(x)), x)
    import wave
    with wave.open(ref, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes((np.clip(x, -1, 1) * 32767).astype(np.int16).tobytes())
    # tekst z bark-pl
    t = None
    for cand in (BASE + '/bark-pl.json',):
        for e in json.load(open(cand, encoding='utf-8')):
            if e['asset'] == r['asset']:
                t = e['pl']
                break
    try:
        state = model.get_state_for_audio_prompt(ref)
        a = model.generate_audio(state, t).numpy().squeeze()
        full = np.atleast_1d(a).astype(np.float32)
        full = full * (10 ** (-1 / 20) / max(1e-6, np.max(np.abs(full))))
        dur = len(full) / SR
        budget = inv[r['asset']]['dur'] or dur
        tempo = dur / budget if budget else 0
        import array
        outp = os.path.join(OUT, r['asset'] + '.pl.wav')
        with open(outp, 'wb') as fh:
            body = array.array('h', (np.clip(full, -1, 1) * 32767).astype(np.int16)).tobytes()
            fh.write(b'RIFF' + __import__('struct').pack('<I', 36 + len(body)) + b'WAVEfmt ')
            fh.write(__import__('struct').pack('<IHHIIHH', 16, 1, 1, SR, SR * 2, 2, 16))
            fh.write(b'data' + __import__('struct').pack('<I', len(body)))
            fh.write(body)
        r['retry_t02'] = {'dur': round(dur, 2), 'tempo': round(tempo, 3)}
        print('%d/%d %s tempo %.2f -> %.2f' % (i + 1, len(todo), r['asset'], r['tempo'], tempo), flush=True)
    except Exception as ex:
        r['retry'] = 'FAIL ' + str(ex)[:80]
json.dump(rows, open(OUT + '/bark_rows.json', 'w'), ensure_ascii=False, indent=1)
print('DONE')
