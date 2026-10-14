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

out = io.open('C:/Users/Trilex/vr-mods/silence_check.txt', 'w', encoding='utf-8')
files = sorted(f for f in os.listdir(D) if f.endswith('.pl.wav'))
out.write('plikow: %d\n' % len(files))
for f in files:
    s, sr = load(os.path.join(D, f))
    win = sr // 100
    env = np.array([np.sqrt(np.mean(s[i:i+win]**2)) for i in range(0, len(s), win)])
    speech = np.sum(env > 0.03) / 10.0
    total = len(s) / sr
    out.write('%s total=%.2f mowa=%.2f cisza=%.2f (%.0f%%)\n' % (f, total, speech, total - speech, 100*(total-speech)/total))
out.close()
print('ok', len(files))
