"""Punkt 2: 184 pominiętych (tempo) - regeneracja tym samym dawca, szybsza dykcja.
Cele: tempo<=1.3, twardy limit 1.85 (inaczej 'zostaje EN'). Bez ciec mowy.
Donor = wlasny oryginal EN (metoda xlingual). Temp 0.35, chunki zdaniowe, pauzy 80ms."""
import sys, os, re, json
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np

AUDIO = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio'
EN_BASE = AUDIO + '/story_en_reference'
OUT = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/out/skipped'
REFD = os.path.join(OUT, '_refs')
os.makedirs(OUT, exist_ok=True)
os.makedirs(REFD, exist_ok=True)

nllb = {}
with open(AUDIO + '/polish/story_voicepack_nllb/manifest.jsonl', encoding='utf-8') as f:
    for line in f:
        if line.strip():
            d = json.loads(line)
            nllb[(d['package'], d['asset'])] = d

b = json.load(open('C:/Users/Trilex/vr-mods/SingularityVR-recovery/deployment/hands-dubbing/xlingual-banks/full/out/xlingual-banks.json', encoding='utf-8'))
jobs = []
for e in b:
    for s in e.get('skipped', []):
        d = nllb.get((e['package'], s['asset']))
        if d:
            jobs.append({'package': e['package'], 'asset': s['asset'],
                         'budget': s['native_duration'], 'old_tempo': s.get('tempo'),
                         'text': d['text_pl_machine'], 'source': d['source'],
                         'profile': d.get('profile')})
print('zadan:', len(jobs), flush=True)

import miniaudio, array as pyarray

def ogg_to_ref(src, dst):
    if os.path.exists(dst):
        return dst
    dec = miniaudio.decode_file(src, output_format=miniaudio.SampleFormat.SIGNED16)
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
    parts = [p for p in re.split(r'(?<=[.!?…])\s+', text.strip()) if p]
    return parts if parts else [text]


def clean_text(text):
    # NLLB artefakty: wiodace myslniki, zdublowane zdania, doklejone halucynacje
    t = text.strip()
    t = re.sub(r'^[\-\u2013\u2014\u2022>]+\s*', '', t)
    parts = [p for p in re.split(r'(?<=[.!?…])\s+', t) if p]
    dedup = []
    for p in parts:
        pn = re.sub(r'\s+', ' ', p).strip().lower()
        if not any(re.sub(r'\s+', ' ', q).strip().lower() == pn for q in dedup):
            dedup.append(p)
    t = ' '.join(dedup)
    t = re.sub(r'\s+', ' ', t).strip()
    return t

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading temp=0.35...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml', temp=0.35, quantize=True)
SR = 24000
rows = []
for i, j in enumerate(jobs):
    outp = os.path.join(OUT, j['asset'] + '.pl.wav')
    if os.path.exists(outp):
        rows.append({'asset': j['asset'], 'note': 'exists-skip'})
        continue
    try:
        ref = ogg_to_ref(os.path.join(EN_BASE, j['source']),
                         os.path.join(REFD, j['asset'] + '_en.wav'))
        state = model.get_state_for_audio_prompt(ref)
        chunks = split_sent(clean_text(j['text']))
        parts = []
        for ch in chunks:
            st = model.get_state_for_audio_prompt(ref)
            a = model.generate_audio(st, ch).numpy().squeeze()
            parts.append(np.atleast_1d(a).astype(np.float32))
        gap = np.zeros(int(SR * 0.08), dtype=np.float32)
        full = parts[0]
        for p in parts[1:]:
            full = np.concatenate([full, gap, p])
        dur = len(full) / SR
        tempo = dur / j['budget']
        rec = {'package': j['package'], 'asset': j['asset'], 'budget': round(j['budget'], 2),
               'dur': round(dur, 2), 'tempo': round(tempo, 3), 'profile': j['profile'],
               'text': j['text'][:120]}
        wavfile.write(outp, SR, full)  # zapis ZAWSZE; decyzja po trim+ASR, nie po surowej dlugosci
        if tempo > 1.85:
            rec['verdict'] = 'NEEDS_TRIM_OR_TEXTFIX'
        else:
            rec['verdict'] = 'CANDIDATE'
            rec['qa_note'] = 'REVIEW: fitting bez ciecia, tempo<=1.85; ASR do potwierdzenia'
        rows.append(rec)
        if (i + 1) % 20 == 0:
            print('postep %d/%d' % (i + 1, len(jobs)), flush=True)
    except Exception as ex:
        rows.append({'asset': j['asset'], 'verdict': 'FAIL', 'err': type(ex).__name__ + str(ex)[:100]})
        print('FAIL', j['asset'], str(ex)[:100], flush=True)
json.dump(rows, open(os.path.join(OUT, 'skipped_results_r2.json'), 'w'), ensure_ascii=False, indent=1)
ok = [r for r in rows if r.get('verdict') == 'CANDIDATE']
en = [r for r in rows if r.get('verdict') == 'STAYS_EN']
print('DONE kandydaci=%d zostaje_EN=%d fail=%d' % (len(ok), len(en), len(rows) - len(ok) - len(en)), flush=True)
