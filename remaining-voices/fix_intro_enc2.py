import io

P = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/intro-pl.json'
raw = io.open(P, 'rb').read()

CP1250 = bytes(range(256)).decode('cp1250', errors='replace')

out = []
i = 0
fixed = 0
while i < len(raw):
    b = raw[i]
    if b < 0x80:
        out.append(chr(b))
        i += 1
    elif 0xC2 <= b <= 0xDF and i + 1 < len(raw) and 0x80 <= raw[i+1] <= 0xBF:
        out.append(bytes([b, raw[i+1]]).decode('utf-8'))
        i += 2
    elif 0xE0 <= b <= 0xEF and i + 2 < len(raw) and all(0x80 <= raw[i+k] <= 0xBF for k in (1, 2)):
        out.append(bytes([b, raw[i+1], raw[i+2]]).decode('utf-8'))
        i += 3
    elif 0xF0 <= b <= 0xF4 and i + 3 < len(raw) and all(0x80 <= raw[i+k] <= 0xBF for k in (1, 2, 3)):
        out.append(bytes([b, raw[i+1], raw[i+2], raw[i+3]]).decode('utf-8'))
        i += 4
    else:
        out.append(CP1250[b])
        fixed += 1
        i += 1

text = ''.join(out)
import json
d = json.loads(text)  # walidacja
io.open(P, 'w', encoding='utf-8').write(json.dumps(d, ensure_ascii=False, indent=1))

log = io.open('C:/Users/Trilex/vr-mods/intro_pl_check.txt', 'w', encoding='utf-8')
for r in d:
    log.write('%s :: %s\n  PL: %s\n' % (r['asset'], r['en'][:90], r['pl']))
log.close()
print('naprawiono bajtow:', fixed, 'wpisow:', len(d))
