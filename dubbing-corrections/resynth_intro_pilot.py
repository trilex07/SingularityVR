import json, os, re, sys
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np
from pocket_tts import TTSModel
from scipy.io import wavfile
from faster_whisper import WhisperModel

REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
OUT = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\resynth_intro_pilot'
os.makedirs(OUT, exist_ok=True)
DONOR = r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\pilot_pl_donor.wav'
pl = {r['asset']: r['pl'] for r in
      json.load(open(REPO + r'\remaining-voices\intro-pl.json', encoding='utf-8'))}
JOBS = ['INTRO_1006_CP', 'INTRO_1016_CP']

model = TTSModel.load_model(config='hf://shefowl/pocket-tts-polish-6l/config.yaml',
                            temp=0.65, quantize=True)
asr = WhisperModel('small', device='cpu', compute_type='int8')

def wav_to_16k_mono(path):
    sr, s = wavfile.read(path)
    s = np.asarray(s, dtype=np.float64)
    idx = np.linspace(0, len(s) - 1, int(len(s) * 16000 / sr))
    return np.interp(idx, np.arange(len(s)), s).astype(np.float32)

def toks(s):
    return re.findall(r'[a-ząćęłńóśźż]+', s.lower())

man = []
for a in JOBS:
    text = pl[a]
    st = model.get_state_for_audio_prompt(DONOR)
    au = model.generate_audio(st, text).numpy().squeeze()
    full = np.atleast_1d(au).astype(np.float32)
    peak = float(np.max(np.abs(full))) or 1e-6
    g = (10 ** (-1.0 / 20)) / peak
    p = os.path.join(OUT, a + '.pl.wav')
    wavfile.write(p, 24000, (np.clip(full * g, -1, 1) * 32767).astype(np.int16))
    recs, txts = [], []
    for _ in range(2):
        segs, _ = asr.transcribe(wav_to_16k_mono(p), language='pl', beam_size=5)
        txt = ' '.join(x.text for x in segs).strip()
        at, et = toks(txt), toks(text)
        recs.append(round(len(set(at) & set(et)) / max(1, len(set(et))), 2))
        txts.append(txt[:100])
    ok = sum(1 for r in recs if r >= 0.5) >= 1
    man.append({'file': a + '.pl.wav', 'asset': a, 'donor': 'mailabs-pl-m',
                'text_pl': text, 'dur_s': round(len(full) / 24000, 2),
                'recalls': recs, 'pass': ok})
    print('%s dur=%.1f draws=%s %s' % (a, len(full) / 24000, recs,
                                      'PASS' if ok else 'FAIL'), flush=True)
    for t in txts:
        print('   ', t[:100], flush=True)

with open(REPO + r'\dubbing-corrections\resynth_manifest_intro_pilot.jsonl', 'w', encoding='utf-8') as fh:
    for m in man:
        fh.write(json.dumps(m, ensure_ascii=False) + '\n')
print('DONE', flush=True)
