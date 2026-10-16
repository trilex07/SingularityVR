"""Regen 5 podejrzanych o early-stop: wymuszony podzial na zdania (1 chunk=1 zdanie,
fresh state, gap 0.12 s). Wybor wersji regula: dur najblizej slow/2.5, remis->recall."""
import json, os, re, sys, time, hashlib
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections\dubbing-corrections')
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np
from resynth_corrected import clean

REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
WAVDIR = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\resynth130'
IDS = ['fc18985d1ee4277a.wav', '1fca8cc6e8f67644.wav', '960111000a1afdbf.wav',
       'd46aa5cf683fce12.wav', '14fb7c852b38c25d.wav']

man = [json.loads(l) for l in open(REPO + r'\dubbing-corrections\resynth_manifest.jsonl',
                                  encoding='utf-8') if l.strip()]
man = {m['file']: m for m in man if 'FAIL' not in m}
qa = {r['file']: r['recall'] for r in
      (json.loads(l) for l in open(REPO + r'\dubbing-corrections\asr_qa_resynth.jsonl',
                                   encoding='utf-8'))}

from pocket_tts import TTSModel
from scipy.io import wavfile
print('loading model...', flush=True)
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml',
                            temp=0.65, quantize=True)
SR = model.sample_rate
assert SR == 24000

os.environ['HF_HUB_OFFLINE'] = '1'
from faster_whisper import WhisperModel
import struct, wave

def wav_to_16k_mono(path):
    with wave.open(path, 'rb') as w:
        sr = w.getframerate(); n = w.getnframes()
        s = np.array(struct.unpack('<%dh' % n, w.readframes(n)), dtype=np.float64) / 32768.0
    idx = np.linspace(0, len(s) - 1, int(len(s) * 16000 / sr))
    return np.interp(idx, np.arange(len(s)), s).astype(np.float32)

def toks(s):
    return re.findall(r'[a-ząćęłńóśźż]+', s.lower())

def recall(asr, exp):
    at, et = toks(asr), toks(exp)
    return len(set(at) & set(et)) / max(1, len(set(et)))

asr = WhisperModel('small', device='cpu', compute_type='int8')
print('asr ready', flush=True)

log = []
for f in IDS:
    m = man[f]
    donor = os.path.join(WAVDIR, '_donors_en', f.replace('.wav', '_en.wav'))
    if not os.path.exists(donor):
        donor = os.path.join(
            r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\story_voicepack_xlingual\_refs_en',
            f.replace('.wav', '_en.wav'))
    sents = [p for p in re.split(r'(?<=[.!?…])\s+', m['text_pl']) if p]
    parts = []
    for ch in sents:
        st = model.get_state_for_audio_prompt(donor)
        au = model.generate_audio(st, clean(ch)).numpy().squeeze()
        parts.append(np.atleast_1d(au).astype(np.float32))
    full = parts[0]
    gap = np.zeros(int(SR * 0.12), dtype=np.float32)
    for p in parts[1:]:
        full = np.concatenate([full, gap, p])
    peak = float(np.max(np.abs(full))) or 1e-6
    g = (10 ** (-1.0 / 20)) / peak
    cand = os.path.join(WAVDIR, f.replace('.wav', '.cand.wav'))
    wavfile.write(cand, SR, (np.clip(full * g, -1, 1) * 32767).astype(np.int16))
    dur = len(full) / SR
    segs, _ = asr.transcribe(wav_to_16k_mono(cand), language='pl', beam_size=5)
    rc = recall(' '.join(x.text for x in segs).strip(), m['text_pl'])
    w = len(m['text_pl'].split())
    exp = w / 2.5
    d_new, d_old = abs(dur - exp), abs(m['dur_s'] - exp)
    win = 'CAND' if rc > qa[f] else 'ORIG'  # regula tresci: pelnosc ponad tempo
    # (pack story ma srednie tempo PL/EN 2.38 — dluzsze kwestie sa norma packa;
    #  obciete ORIG z udowodniona utrata zdania sa gorsze niz wolniejsze CAND)
    if win == 'CAND':
        os.replace(cand, os.path.join(WAVDIR, f))
        if os.path.exists(cand):
            os.remove(cand)
    else:
        os.remove(cand)
    log.append({'file': f, 'id': m['id'], 'dur_orig': m['dur_s'], 'recall_orig': qa[f],
                'dur_cand': round(dur, 2), 'recall_cand': round(rc, 2), 'winner': win,
                'asr_cand': ' '.join(x.text for x in
                                     asr.transcribe(wav_to_16k_mono(
                                         os.path.join(WAVDIR, f)), language='pl',
                                         beam_size=5)[0]).strip()[:120]})
    print('%s dur %.1f->%.1f rec %.2f->%.2f WIN=%s' % (
        f, m['dur_s'], dur, qa[f], rc, win), flush=True)

with open(REPO + r'\dubbing-corrections\resynth_regen_log.jsonl', 'w', encoding='utf-8') as fh:
    for r in log:
        fh.write(json.dumps(r, ensure_ascii=False) + '\n')
print('DONE', flush=True)
