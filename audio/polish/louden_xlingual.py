"""Glosniej: float32 -> PCM16 mono 24kHz, peak -1 dBFS. W miejscu (in-place).
Deterministyczne, do powtorzenia po kazdej regeneracji."""
import os, struct, math

OUTDIR = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/story_voicepack_xlingual'
TARGET_PEAK = 10 ** (-1.0 / 20)

def read_float_wav(p):
    with open(p, 'rb') as fh:
        raw = fh.read()
    assert raw[0:4] == b'RIFF' and raw[8:12] == b'WAVE'
    pos = 12; sr = ch = bits = 0; data = None
    while pos + 8 <= len(raw):
        cid = raw[pos:pos+4]; sz = struct.unpack('<I', raw[pos+4:pos+8])[0]
        body = raw[pos+8:pos+8+sz]
        if cid == b'fmt ':
            tag, ch, sr, _, _, bits = struct.unpack('<HHIIHH', body[:16])
        elif cid == b'data':
            data = body
        pos += 8 + sz + (sz & 1)
    assert tag == 3 and bits == 32, (p, tag, bits)
    n = len(data) // 4
    s = struct.unpack('<%df' % n, data[:n*4])
    return sr, ch, s

def write_pcm16(p, sr, s):
    import array
    body = array.array('h', [max(-32768, min(32767, int(round(x * 32767)))) for x in s]).tobytes()
    with open(p, 'wb') as fh:
        fh.write(b'RIFF' + struct.pack('<I', 36 + len(body)) + b'WAVEfmt ')
        fh.write(struct.pack('<IHHIIHH', 16, 1, 1, sr, sr * 2, 2, 16))
        fh.write(b'data' + struct.pack('<I', len(body)))
        fh.write(body)

files = sorted(f for f in os.listdir(OUTDIR) if f.endswith('.wav'))
print('plikow:', len(files), flush=True)
for i, f in enumerate(files):
    p = os.path.join(OUTDIR, f)
    try:
        sr, ch, s = read_float_wav(p)
    except Exception:
        continue  # juz PCM16 albo inny format - skip
    if ch == 2:
        s = [(a + b) / 2 for a, b in zip(s[0::2], s[1::2])]
    if sr != 24000:
        idx = [i * (len(s) - 1) / (int(len(s) * 24000 / sr) - 1) for i in range(int(len(s) * 24000 / sr))]
        s = [s[int(k)] for k in idx]
        sr = 24000
    peak = max(abs(x) for x in s) or 1e-6
    g = TARGET_PEAK / peak
    write_pcm16(p, 24000, [x * g for x in s])
    if (i + 1) % 200 == 0:
        print('postep %d/%d' % (i + 1, len(files)), flush=True)
print('DONE', flush=True)
