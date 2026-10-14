import faster_whisper.audio as fwa, faster_whisper.transcribe as fwt
import numpy as np, struct, json, os

def my_decode(path, sampling_rate=16000):
    with open(path, 'rb') as fh:
        raw = fh.read()
    pos = 12; sr = 0; ch = 1; data = None; bits = 16
    while pos + 8 <= len(raw):
        cid = raw[pos:pos+4]; sz = struct.unpack('<I', raw[pos+4:pos+8])[0]
        body = raw[pos+8:pos+8+sz]
        if cid == b'fmt ':
            tag, ch, sr = struct.unpack('<HHI', body[:8]); bits = struct.unpack('<H', body[14:16])[0]
        elif cid == b'data':
            data = body
        pos += 8 + sz + (sz & 1)
    if bits == 16:
        n = len(data) // 2
        x = np.array(struct.unpack('<%dh' % n, data[:n*2]), dtype=np.float32) / 32768.0
    else:
        n = len(data) // 4
        x = np.array(struct.unpack('<%df' % n, data[:n*4]), dtype=np.float32)
    if ch == 2:
        x = x.reshape(-1, 2).mean(axis=1)
    if sr != sampling_rate:
        idx = np.linspace(0, len(x) - 1, int(len(x) * sampling_rate / sr))
        x = np.interp(idx, np.arange(len(x)), x).astype(np.float32)
    return x

fwa.decode_audio = my_decode
fwt.decode_audio = my_decode
os.environ['HF_HUB_OFFLINE'] = '1'

AUDIO = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish'
OUTDIR = AUDIO + '/story_voicepack_xlingual'
MANIFEST = AUDIO + '/story_voicepack_nllb/manifest.jsonl'

texts = {}
with open(MANIFEST, encoding='utf-8') as f:
    for line in f:
        if line.strip():
            d = json.loads(line)
            texts[d['file']] = d.get('text_pl_machine', '')
with open(os.path.join(OUTDIR, '_asr_queue.txt')) as f:
    queue = [l.strip() for l in f if l.strip()]

from faster_whisper import WhisperModel
model = WhisperModel('small', device='cpu', compute_type='int8')
print('model ready, files:', len(queue), flush=True)

def norm_words(s):
    import re
    return re.findall(r'[a-ząćęłńóśźż]+', s.lower())

results = []
for i, fname in enumerate(queue):
    p = os.path.join(OUTDIR, fname)
    if not os.path.exists(p):
        results.append({'file': fname, 'asr': '', 'recall': 0.0, 'note': 'missing'})
        continue
    try:
        segs, _ = model.transcribe(p, language='pl')
        asr = ' '.join(s.text for s in segs)
    except Exception as ex:
        results.append({'file': fname, 'asr': '', 'recall': 0.0, 'note': str(ex)[:100]})
        continue
    exp = set(norm_words(texts.get(fname, '')))
    got = set(norm_words(asr))
    recall = len(exp & got) / max(1, len(exp))
    results.append({'file': fname, 'asr': asr[:200], 'recall': round(recall, 2),
                    'expected': texts.get(fname, '')[:120]})
    print('%d/%d %s recall=%.2f' % (i + 1, len(queue), fname, recall), flush=True)

with open(os.path.join(OUTDIR, 'ASR_QA.json'), 'w', encoding='utf-8') as f:
    json.dump(results, f, ensure_ascii=False, indent=1)
bad = [r for r in results if r['recall'] < 0.5]
print('DONE checked=%d bad(recall<0.5)=%d' % (len(results), len(bad)), flush=True)
for r in bad:
    print('BAD', r['file'], r['recall'], '|', r['asr'][:100], flush=True)
