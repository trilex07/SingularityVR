import argparse, json, os
os.environ['HF_HUB_OFFLINE'] = '1'
import numpy as np, sys, re
sys.path.insert(0, r'C:\Users\Trilex\vr-mods\.pocket_tools')
from scipy.io import wavfile
from faster_whisper import WhisperModel
ap = argparse.ArgumentParser()
ap.add_argument('--dir', required=True)
ap.add_argument('--tag', required=True)
ap.add_argument('--out', required=True)
ap.add_argument('--manifest', default=None)
a = ap.parse_args()
REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
man = {}
for m in (json.loads(l) for l in open(
        a.manifest or REPO + r'\dubbing-corrections\resynth_manifest.jsonl',
        encoding='utf-8')):
    if 'FAIL' not in m:
        man[m['file']] = m.get('text_pl') or m.get('expected') or ''

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

model = WhisperModel('small', device='cpu', compute_type='int8')
files = sorted(f for f in os.listdir(a.dir) if f.endswith('.wav') and f in man)
print('tag=%s n=%d' % (a.tag, len(files)), flush=True)
with open(a.out, 'a', encoding='utf-8') as fh:
    for f in files:
        p = os.path.join(a.dir, f)
        sr, s = wavfile.read(p)
        dur = len(np.asarray(s)) / sr
        segs, _ = model.transcribe(wav_to_16k_mono(p), language='pl', beam_size=5)
        asr = ' '.join(x.text for x in segs).strip()
        at, et = toks(asr), toks(man[f])
        rec = round(len(set(at) & set(et)) / max(1, len(set(et))), 2)
        fh.write(json.dumps({'file': f, 'tag': a.tag, 'recall': rec,
                             'dur_s': round(dur, 2), 'asr': asr[:150]},
                            ensure_ascii=False) + '\n')
        print('%s %.2f %s' % (f, rec, asr[:70].encode('ascii', 'replace').decode()),
              flush=True)
