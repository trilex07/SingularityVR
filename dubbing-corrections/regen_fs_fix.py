"""Fix firstscene: 3 z udowodniona utrata tresci (loop No-no, 2x early-stop).
Regen zdanie-po-zdaniu (fresh state, gap 0.04, temp 0.5), ten sam dawca.
Regula: recall_cand > recall_orig -> CAND (plik podmieniony)."""
import json, os, re, sys, hashlib
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections\dubbing-corrections')
from resynth_corrected import clean

REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
WAVDIR = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\resynth_firstscene'
REFS = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\story_voicepack_xlingual\_refs_en'
FILES = ['72cf0068760218eb.wav', 'e102465ca633fb4e.wav', 'f340d133e2909639.wav']

man = {m['file']: m for m in
       (json.loads(l) for l in open(REPO + r'\dubbing-corrections\resynth_manifest_firstscene.jsonl',
                                   encoding='utf-8')) if 'FAIL' not in m.get('FAIL', '')}
qa = {r['file']: r['recall'] for r in
      (json.loads(l) for l in open(REPO + r'\dubbing-corrections\asr_qa_firstscene.jsonl',
                                   encoding='utf-8'))}

from pocket_tts import TTSModel
from scipy.io import wavfile
model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml',
                            temp=0.5, quantize=True)
SR = model.sample_rate
assert SR == 24000

import struct, wave
from faster_whisper import WhisperModel

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
    return round(len(set(at) & set(et)) / max(1, len(set(et))), 2)

asr = WhisperModel('small', device='cpu', compute_type='int8')
print('models ready', flush=True)
log = []
for f in FILES:
    m = man[f]
    donor = os.path.join(REFS, f.replace('.wav', '_en.wav'))
    if not os.path.exists(donor):
        donor = os.path.join(WAVDIR, '_donors_en', f.replace('.wav', '_en.wav'))
    assert os.path.exists(donor), f
    sents = [p for p in re.split(r'(?<=[.!?…])\s+', m['text_pl']) if p]
    parts = []
    for ch in sents:
        st = model.get_state_for_audio_prompt(donor)
        au = model.generate_audio(st, clean(ch)).numpy().squeeze()
        parts.append(np.atleast_1d(au).astype(np.float32))
    full = parts[0]
    gap = np.zeros(int(SR * 0.04), dtype=np.float32)
    for p in parts[1:]:
        full = np.concatenate([full, gap, p])
    peak = float(np.max(np.abs(full))) or 1e-6
    g = (10 ** (-1.0 / 20)) / peak
    cand = os.path.join(WAVDIR, f.replace('.wav', '.cand.wav'))
    wavfile.write(cand, SR, (np.clip(full * g, -1, 1) * 32767).astype(np.int16))
    segs, _ = asr.transcribe(wav_to_16k_mono(cand), language='pl', beam_size=5)
    txt = ' '.join(x.text for x in segs).strip()
    rc = recall(txt, m['text_pl'])
    win = 'CAND' if rc > qa[f] else 'ORIG'
    if win == 'CAND':
        os.replace(cand, os.path.join(WAVDIR, f))
    else:
        os.remove(cand)
    log.append({'file': f, 'dur_orig': m['dur_s'], 'recall_orig': qa[f],
                'dur_cand': round(len(full) / SR, 2), 'recall_cand': rc,
                'asr_cand': txt[:120], 'winner': win})
    print('%s rec %.2f->%.2f WIN=%s' % (f, qa[f], rc, win), flush=True)

with open(REPO + r'\dubbing-corrections\resynth_fs_fix_log.jsonl', 'w', encoding='utf-8') as fh:
    for r in log:
        fh.write(json.dumps(r, ensure_ascii=False) + '\n')
print('DONE', flush=True)
