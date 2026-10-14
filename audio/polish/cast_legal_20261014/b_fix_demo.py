import json, wave, struct, os, math

base = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/repair_candidate_20261013/narrator_ui_lektor_20261014'
with open(os.path.join(base, 'narrator_lektor_ab_pilot.json'), 'r', encoding='utf-8') as f:
    d = json.load(f)

outdir = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/cast_legal_20261014/casting_final/role_lektor/B_fix_demo'
os.makedirs(outdir, exist_ok=True)

def load(p):
    with wave.open(p, 'rb') as w:
        pr = (w.getnchannels(), w.getsampwidth(), w.getframerate())
        n = w.getnframes()
        data = w.readframes(n)
    s = list(struct.unpack('<%dh' % (len(data)//2), data))
    return pr, s

def save(p, pr, s):
    with wave.open(p, 'wb') as w:
        w.setnchannels(pr[0]); w.setsampwidth(pr[1]); w.setframerate(pr[2])
        w.writeframes(struct.pack('<%dh' % len(s), *s))

report = []
for e in d:
    if e.get('label') != 'B_current_intro_assembled':
        continue
    pr, s = load(e['path'])
    rate = pr[2]
    # 10ms bins
    win = rate // 100
    bins = []
    for i in range(0, len(s), win):
        seg = s[i:i+win]
        rms = (sum(x*x for x in seg)/len(seg)) ** 0.5
        # zero-crossing rate
        zc = sum(1 for a, b in zip(seg, seg[1:]) if (a < 0) != (b < 0)) / len(seg)
        bins.append((rms, zc))
    # cut leading bins: silence (<300) or breath (rms<1200 and zcr>0.25)
    cut = 0
    for rms, zc in bins:
        if rms < 300 or (rms < 1200 and zc > 0.25):
            cut += 1
        else:
            break
    cut_samp = min(cut * win, int(rate * 0.6))
    fixed = s[cut_samp:]
    # 15ms fade-in
    fl = int(rate * 0.015)
    for i in range(min(fl, len(fixed))):
        fixed[i] = int(fixed[i] * (i / fl))
    fn = 'BFIX_' + os.path.basename(e['path'])
    save(os.path.join(outdir, fn), pr, fixed)
    report.append('%s | ciecie=%dms | %s' % (os.path.basename(e['path']), cut_samp * 1000 // rate, e.get('line_id')))

with open(os.path.join(outdir, 'RAPORT.txt'), 'w', encoding='utf-8') as f:
    f.write('B-fix demo: obciety oddech/cisza z poczatku + 15ms fade-in\n')
    f.write('\n'.join(report) + '\n')
print('\n'.join(report))
print('saved to', outdir)
