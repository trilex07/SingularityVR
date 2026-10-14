import io, json

CP1250 = bytes(range(256)).decode('cp1250', errors='replace')

for P in ['bark-pl.json', 'mp-pl.json']:
    P = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/' + P
    try:
        raw = io.open(P, 'rb').read()
        raw.decode('utf-8')
        print(P.split('/')[-1], 'czysty')
        continue
    except UnicodeDecodeError:
        pass
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
    d = json.loads(text)
    io.open(P, 'w', encoding='utf-8').write(json.dumps(d, ensure_ascii=False, indent=1))
    print(P.split('/')[-1], 'naprawiono bajtow:', fixed, 'wpisow:', len(d))
