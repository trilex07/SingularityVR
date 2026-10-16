"""Uniwersalne ASR-QA (faster-whisper-small): manifest [{file, text_pl|expected}],
wavdir, out jsonl {file, asr, recall, loop}. Bramka: recall>=0.5 i brak petli."""
import argparse, json, os, re
ap = argparse.ArgumentParser()
ap.add_argument('--manifest', required=True)
ap.add_argument('--wavdir', required=True)
ap.add_argument('--out', required=True)
ap.add_argument('--text-key', default='text_pl')
a = ap.parse_args()
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np, sys
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
from scipy.io import wavfile
from faster_whisper import WhisperModel

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

man = [json.loads(l) for l in open(a.manifest, encoding='utf-8') if l.strip()]
man = [m for m in man if 'FAIL' not in m]
model = WhisperModel('small', device='cpu', compute_type='int8')
print('files: %d' % len(man), flush=True)
out = []
for m in man:
    exp = m.get(a.text_key) or m.get('expected') or ''
    segs, _ = model.transcribe(
        wav_to_16k_mono(os.path.join(a.wavdir, m['file'])), language='pl', beam_size=5)
    asr = ' '.join(x.text for x in segs).strip()
    at, et = toks(asr), toks(exp)
    rec = round(len(set(at) & set(et)) / max(1, len(set(et))), 2)
    loop = bool(re.search(r'(\b\w+\b)(,? \1){2,}', asr, re.IGNORECASE))
    out.append({'file': m['file'], 'asset': m.get('asset'), 'asr': asr,
                'recall': rec, 'loop': loop, 'expected': exp,
                'pass': bool(rec >= 0.5 and not loop)})
    print('%s rec=%.2f loop=%s %s' % (m.get('asset', m['file']), rec, loop,
                                      'PASS' if rec >= 0.5 and not loop else 'FAIL'),
          flush=True)
with open(a.out, 'w', encoding='utf-8') as fh:
    for r in out:
        fh.write(json.dumps(r, ensure_ascii=False) + '\n')
print('pass=%d/%d' % (sum(1 for r in out if r['pass']), len(out)), flush=True)
