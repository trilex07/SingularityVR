"""Intro retry: receptura produkcyjna gen_intro (single chunk, temp 0.5, own donor).
Akceptacja per plik: recall >= 0.5 i brak petli, inaczej brak dostawy (oddanie
istniejacego packa + teksty w jsonl na przyszlosc)."""
import json, os, re, sys
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections\dubbing-corrections')
from resynth_corrected import clean, ogg_to_wav_mono24

REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
OUT = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\resynth_intro'
ENB = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference\SP_VI_intro_Kismet_LOC_INT\SoundNodeWave'
DON = os.path.join(OUT, '_donors_en')

pl = {r['asset']: r['pl'] for r in
      json.load(open(REPO + r'\remaining-voices\intro-pl.json', encoding='utf-8'))}
ASSETS = ['INTRO_1005_NIM', 'INTRO_1006_CP', 'INTRO_1011_GST',
          'INTRO_1012_NIM', 'INTRO_1014_GST', 'INTRO_1016_CP']

from pocket_tts import TTSModel
from scipy.io import wavfile
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml',
                            temp=0.5, quantize=True)
SR = model.sample_rate

import sys as _s
_s.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
from faster_whisper import WhisperModel
asr = WhisperModel('small', device='cpu', compute_type='int8')

def wav_to_16k_mono(path):
    sr, s = wavfile.read(path)
    s = np.asarray(s, dtype=np.float64)
    if s.ndim > 1:
        s = s.mean(axis=1)
    if np.issubdtype(s.dtype, np.integer):
        s = s / 32768.0
    idx = np.linspace(0, len(s) - 1, int(len(s) * 16000 / sr))
    return np.interp(idx, np.arange(len(s)), s).astype(np.float32)

def toks(s):
    return re.findall(r'[a-ząćęłńóśźż]+', s.lower())

log = []
for a in ASSETS:
    text = clean(pl[a])
    ogg = os.path.join(ENB, a + '.ogg')
    dw = os.path.join(DON, a + '_en.wav')
    if not os.path.exists(dw):
        ogg_to_wav_mono24(ogg, dw)
    st = model.get_state_for_audio_prompt(dw)
    au = model.generate_audio(st, text).numpy().squeeze()
    full = np.atleast_1d(au).astype(np.float32)
    peak = float(np.max(np.abs(full))) or 1e-6
    g = (10 ** (-1.0 / 20)) / peak
    cand = os.path.join(OUT, a + '.single.wav')
    wavfile.write(cand, SR, (np.clip(full * g, -1, 1) * 32767).astype(np.int16))
    segs, _ = asr.transcribe(wav_to_16k_mono(cand), language='pl', beam_size=5)
    txt = ' '.join(x.text for x in segs).strip()
    at, et = toks(txt), toks(text)
    rc = round(len(set(at) & set(et)) / max(1, len(set(et))), 2)
    # petla? ten sam token 3x z rzedu
    loop = bool(re.search(r'(\b\w+\b)(,? \1){2,}', txt, re.IGNORECASE))
    win = 'CAND' if (rc >= 0.5 and not loop) else 'REJECT'
    if win == 'CAND':
        os.replace(cand, os.path.join(OUT, a + '.pl.wav'))
    else:
        os.remove(cand)
    log.append({'asset': a, 'recall_single': rc, 'loop': loop,
                'asr_single': txt[:120], 'winner': win})
    print('%s rec=%.2f loop=%s WIN=%s' % (a, rc, loop, win), flush=True)

with open(REPO + r'\dubbing-corrections\resynth_intro_retry_log.jsonl', 'w', encoding='utf-8') as fh:
    for r in log:
        fh.write(json.dumps(r, ensure_ascii=False) + '\n')
print('DONE', flush=True)
