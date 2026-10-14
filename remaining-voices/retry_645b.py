"""Ostatnia dzwignia dla zapetlonych: dawca 645b (zweryfikowany), temp 0.3.
Jesli tempo nadal >3.5 -> STAYS_EN (koniec, bez dalszych rund)."""
import sys, os, json
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

BASE = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices'
OUT = BASE + '/out/barks'
REF645 = ('C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/'
          'story_voicepack_xlingual/_refs_en/645b6de719763bc4_en.wav')

rows = json.load(open(OUT + '/bark_rows.json', encoding='utf-8'))
todo = [r for r in rows if r.get('retry_t02', {}).get('tempo', 0) > 3.5 or
        (r.get('verdict') == 'REVIEW_TEMPO' and r.get('tempo', 0) > 3.5 and 'retry_t02' not in r)]
print('kandydaci:', len(todo), flush=True)

pl = {}
for p in (BASE + '/bark-pl.json',):
    for e in json.load(open(p, encoding='utf-8')):
        pl[e['asset']] = e['pl']
inv = json.load(open(BASE + '/bark-speech.json', encoding='utf-8'))

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading temp=0.3...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.3, quantize=True)
SR = 24000
import array, struct
for r in todo:
    t = pl.get(r['asset'], '')
    if not t:
        r['verdict'] = 'NO_TEXT'
        continue
    try:
        state = model.get_state_for_audio_prompt(REF645)
        a = model.generate_audio(state, t).numpy().squeeze()
        full = np.atleast_1d(a).astype(np.float32)
        full = full * (10 ** (-1 / 20) / max(1e-6, np.max(np.abs(full))))
        dur = len(full) / SR
        budget = inv[r['asset']]['dur'] or dur
        tempo = dur / budget if budget else 0
        outp = os.path.join(OUT, r['asset'] + '.pl.wav')
        with open(outp, 'wb') as fh:
            body = array.array('h', (np.clip(full, -1, 1) * 32767).astype(np.int16)).tobytes()
            fh.write(b'RIFF' + struct.pack('<I', 36 + len(body)) + b'WAVEfmt ')
            fh.write(struct.pack('<IHHIIHH', 16, 1, 1, SR, SR * 2, 2, 16))
            fh.write(b'data' + struct.pack('<I', len(body)))
            fh.write(body)
        if tempo <= 3.5:
            r['retry_645b'] = {'dur': round(dur, 2), 'tempo': round(tempo, 3)}
            r['verdict'] = 'CANDIDATE_645B_DONOR'
        else:
            r['retry_645b'] = {'dur': round(dur, 2), 'tempo': round(tempo, 3)}
            r['verdict'] = 'STAYS_EN'
        print('%s %.2f -> %.2f %s' % (r['asset'], r.get('retry_t02', {}).get('tempo', r.get('tempo', 0)), tempo, r['verdict']), flush=True)
    except Exception as ex:
        r['verdict'] = 'STAYS_EN'
        r['retry_645b'] = {'err': str(ex)[:80]}
json.dump(rows, open(OUT + '/bark_rows.json', 'w'), ensure_ascii=False, indent=1)
from collections import Counter
print(Counter(r['verdict'] for r in rows), flush=True)
print('DONE')
