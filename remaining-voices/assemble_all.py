import os, json, hashlib, struct
import numpy as np

CP1250 = bytes(range(256)).decode('cp1250', errors='replace')


def load_json(path, **kw):
    try:
        with open(path, encoding='utf-8') as f:
            return json.load(f)
    except UnicodeDecodeError:
        pass
    with open(path, 'rb') as fh:
        raw = fh.read()
    out = []
    i = 0
    while i < len(raw):
        b = raw[i]
        if b < 0x80:
            out.append(chr(b)); i += 1
        elif 0xC2 <= b <= 0xDF and i+1 < len(raw) and 0x80 <= raw[i+1] <= 0xBF:
            out.append(bytes([b, raw[i+1]]).decode('utf-8')); i += 2
        elif 0xE0 <= b <= 0xEF and i+2 < len(raw) and all(0x80 <= raw[i+k] <= 0xBF for k in (1, 2)):
            out.append(bytes([b, raw[i+1], raw[i+2]]).decode('utf-8')); i += 3
        else:
            out.append(CP1250[b]); i += 1
    return json.loads(''.join(out))

BASE = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices'

def wav_dur_peak(p):
    with open(p, 'rb') as fh:
        raw = fh.read()
    pos = 12; sr = 0; data = None; bits = 16
    while pos + 8 <= len(raw):
        cid = raw[pos:pos+4]; sz = struct.unpack('<I', raw[pos+4:pos+8])[0]
        body = raw[pos+8:pos+8+sz]
        if cid == b'fmt ':
            sr = struct.unpack('<HHI', body[:8])[2]; bits = struct.unpack('<H', body[14:16])[0]
        elif cid == b'data':
            data = body
        pos += 8 + sz + (sz & 1)
    if bits == 16:
        n = len(data) // 2
        s = np.array(struct.unpack('<%dh' % n, data[:n*2]), dtype=np.float32) / 32768.0
    else:
        n = len(data) // 4
        s = np.array(struct.unpack('<%df' % n, data[:n*4]), dtype=np.float32)
    import math
    return len(s) / sr, 20 * math.log10(max(float(np.max(np.abs(s))), 1e-9))

def sha16(p):
    return hashlib.sha256(open(p, 'rb').read()).hexdigest()[:16]

all_rows = {'vignette': [], 'skipped': [], 'barks': [], 'intro': [], 'mp': [], 'single_825b': []}

# 1) vignette (gotowe manifest_rows)
all_rows['vignette'] = load_json(BASE + '/out/vignette/manifest_rows.json')

# 2) skipped final
sk = load_json(BASE + '/out/skipped/skipped_final.json')
for r in sk:
    if r.get('verdict') in ('CANDIDATE', 'REVIEW'):
        f = BASE + '/out/skipped/' + r['asset'] + '.pl.wav'
        kk = r['package'] + '__' + r['asset']
        alt = BASE + '/out/skipped/' + r['package'] + '__' + r['asset'] + '.pl.wav'
        if r['package'] == 'sp_rt3_katya_LOC_INT' and os.path.exists(alt):
            f = alt
        if os.path.exists(f):
            dur, peak = wav_dur_peak(f)
            r2 = dict(r)
            r2.update({'source_wav': os.path.basename(f), 'source_sha256': sha16(f),
                       'true_peak_dbtp': round(peak, 2), 'qa_flagged': r['verdict'] == 'REVIEW'})
            all_rows['skipped'].append(r2)
print('skipped rows:', len(all_rows['skipped']))

# 3) barks
brows = load_json(BASE + '/out/barks/bark_rows.json')
for r in brows:
    if r.get('verdict') in ('CANDIDATE', 'CANDIDATE_645B_DONOR', 'REVIEW_TEMPO'):
        f = BASE + '/out/barks/' + r['asset'] + '.pl.wav'
        if os.path.exists(f):
            dur, peak = wav_dur_peak(f)
            r2 = dict(r)
            r2.update({'source_wav': r['asset'] + '.pl.wav', 'source_sha256': sha16(f),
                       'true_peak_dbtp': round(peak, 2),
                       'qa_flagged': r['verdict'] == 'REVIEW_TEMPO'})
            all_rows['barks'].append(r2)
print('bark rows:', len(all_rows['barks']))

# 4) intro (10 narracji)
intro_inv = {r['asset']: r for r in
             load_json(BASE + '/intro-speech.json')}
intro_pl = {r['asset']: r['pl'] for r in
            load_json(BASE + '/intro-pl.json')}
for asset in sorted(intro_pl):
    f = BASE + '/out/intro/' + asset + '.pl.wav'
    if os.path.exists(f):
        dur, peak = wav_dur_peak(f)
        iv = intro_inv.get(asset, {})
        all_rows['intro'].append({
            'asset': asset, 'english': iv.get('spoken', ''), 'polish': intro_pl[asset],
            'source_wav': asset + '.pl.wav', 'source_sha256': sha16(f),
            'true_peak_dbtp': round(peak, 2), 'tempo': round(dur / iv['dur'], 3) if iv.get('dur') else 0,
            'guard_ms': 150, 'source_duration': round(dur, 2),
            'native_frames': None, 'rate': iv.get('rate'),
            'qa_flagged': True, 'qa_note': 'REVIEW: rezyser po sluchu (callsigns respelling)'})
print('intro rows:', len(all_rows['intro']))

# 5) mp
mrows = load_json(BASE + '/out/mp/mp_rows.json')
for r in mrows:
    if r.get('verdict') in ('CANDIDATE', 'REVIEW_TEMPO'):
        all_rows['mp'].append(dict(r, qa_flagged=r['verdict'] == 'REVIEW_TEMPO'))
print('mp rows:', len(all_rows['mp']))

# 6) 825b
f = BASE + '/out/825b/825b608f491f738b.pl.wav'
if os.path.exists(f):
    dur, peak = wav_dur_peak(f)
    all_rows['single_825b'] = [{'asset': '825b608f491f738b', 'source_wav': '825b608f491f738b.pl.wav',
                                'source_sha256': sha16(f), 'true_peak_dbtp': round(peak, 2),
                                'source_duration': round(dur, 2),
                                'qa_note': 'REVIEW: srodek kwestii do potwierdzenia uchem'}]

json.dump(all_rows, open(BASE + '/ALL_MANIFEST_ROWS.json', 'w'), ensure_ascii=False, indent=1)
tot = sum(len(v) if isinstance(v, list) else 1 for v in all_rows.values())
print('RAZEM wierszy:', tot)
