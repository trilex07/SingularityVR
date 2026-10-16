"""ASR-QA resyntezy: faster-whisper-small (cache), jezyk pl, recall tokenowy
(application jak produkcyjne ASR_QA.json; prog watch <0.5 jak rundy repair)."""
import argparse, json, os, re
ap = argparse.ArgumentParser()
ap.add_argument('--manifest', default=None)
ap.add_argument('--wavdir', default=None)
ap.add_argument('--out', default=None)
a = ap.parse_args()

REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
os.environ['HF_HUB_OFFLINE'] = '1'
WAVDIR = a.wavdir or r'C:\Users\Trilex\AppData\Local\Temp\opencode\dubbing-qa\resynth130'
MAN = a.manifest or REPO + r'\dubbing-corrections\resynth_manifest.jsonl'
OUT = a.out or REPO + r'\dubbing-corrections\asr_qa_resynth.jsonl'

def toks(s):
    return re.findall(r'[a-ząćęłńóśźż]+', s.lower())

man = [json.loads(l) for l in open(MAN, encoding='utf-8') if l.strip()]
man = [m for m in man if 'FAIL' not in m]
print('files: %d' % len(man), flush=True)

from faster_whisper import WhisperModel
import numpy as np
import struct
import wave

def wav_to_16k_mono(path):
    with wave.open(path, 'rb') as w:
        assert w.getnchannels() == 1 and w.getsampwidth() == 2
        sr = w.getframerate()
        n = w.getnframes()
        s = np.array(struct.unpack('<%dh' % n, w.readframes(n)), dtype=np.float64) / 32768.0
    if sr != 16000:
        idx = np.linspace(0, len(s) - 1, int(len(s) * 16000 / sr))
        s = np.interp(idx, np.arange(len(s)), s)
    return s.astype(np.float32)

model = WhisperModel('small', device='cpu', compute_type='int8')
print('model ready', flush=True)

out = []
for n, m in enumerate(man, 1):
    p = os.path.join(WAVDIR, m['file'])
    try:
        segs, _ = model.transcribe(wav_to_16k_mono(p), language='pl', beam_size=5)
        asr = ' '.join(s.text for s in segs).strip()
    except Exception as ex:
        asr = 'DECODE-FAIL %s' % type(ex).__name__
    et, at = toks(m['text_pl']), toks(asr)
    rec = len(set(at) & set(et)) / max(1, len(set(et)))
    out.append({'file': m['file'], 'id': m['id'], 'asr': asr,
                'recall': round(rec, 2), 'expected': m['text_pl']})
    if n % 20 == 0:
        print('asr %d/%d' % (n, len(man)), flush=True)

with open(OUT, 'w', encoding='utf-8') as fh:
    for r in out:
        fh.write(json.dumps(r, ensure_ascii=False) + '\n')
import statistics
rs = [r['recall'] for r in out]
bad = [r for r in out if r['recall'] < 0.5]
print('mean=%.2f min=%.2f n<0.5=%d' % (
    statistics.mean(rs), min(rs), len(bad)), flush=True)
for r in sorted(bad, key=lambda x: x['recall'])[:15]:
    print('WATCH %.2f %s | %s' % (r['recall'], r['file'], r['asr'][:90]), flush=True)
