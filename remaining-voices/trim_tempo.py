import os, struct, io
import numpy as np

D = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/out/skipped'

def load(p):
    with open(p, 'rb') as fh:
        raw = fh.read()
    pos = 12; sr = 0; data = None
    while pos + 8 <= len(raw):
        cid = raw[pos:pos+4]; sz = struct.unpack('<I', raw[pos+4:pos+8])[0]
        body = raw[pos+8:pos+8+sz]
        if cid == b'fmt ':
            sr = struct.unpack('<HHI', body[:8])[2]
        elif cid == b'data':
            data = body
        pos += 8 + sz + (sz & 1)
    n = len(data) // 4
    return np.array(struct.unpack('<%df' % n, data[:n*4])), sr

import json
rows = json.loads(io.open(os.path.join(D, 'skipped_results.json'), encoding='utf-8', errors='replace').read())
by_asset = {r['asset']: r for r in rows if isinstance(r, dict) and r.get('asset')}

out = io.open('C:/Users/Trilex/vr-mods/trim_tempo.txt', 'w', encoding='utf-8')
for f in sorted(os.listdir(D)):
    if not f.endswith('.pl.wav'):
        continue
    s, sr = load(os.path.join(D, f))
    win = sr // 100
    env = np.array([np.sqrt(np.mean(s[i:i+win]**2)) for i in range(0, len(s), win)])
    loud = np.where(env > 0.03)[0]
    if len(loud) == 0:
        out.write('%s PUSTY\n' % f)
        continue
    a = max(0, loud[0] * win - int(sr * 0.04))
    b = min(len(s), (loud[-1] + 1) * win + int(sr * 0.04))
    trimmed = (b - a) / sr
    asset = f[:-len('.pl.wav')]
    r = by_asset.get(asset, {})
    budget = r.get('budget', 0) or 0
    tempo = trimmed / budget if budget else 0
    out.write('%s trim=%.2f budzet=%.2f tempo=%.2f %s\n'
              % (asset, trimmed, budget, tempo,
                 'OK<=1.3' if tempo <= 1.3 else ('REVIEW<=1.85' if tempo <= 1.85 else 'NADAL_ZA_DLUGIE')))
out.close()
print('ok')
