import io, json
import ftfy

P = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/bark-speech.json'
raw = io.open(P, encoding='utf-8', errors='surrogateescape').read()
d = json.loads(raw)
log = io.open('C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/bark_fix_log.txt',
              'w', encoding='utf-8')
fixed = 0
for k, v in d.items():
    s = v.get('spoken') or ''
    # surowe bajty cp1252 (np. 0x92 = ') ukryte jako surrogateescape
    try:
        b = s.encode('utf-8', errors='surrogateescape')
    except Exception:
        b = s.encode('utf-8', errors='replace')
    s2 = ftfy.fix_text(b.decode('cp1252', errors='replace'))
    if s2 != s:
        log.write('%s :: %r -> %r\n' % (k, s[:80], s2[:80]))
        v['spoken'] = s2
        fixed += 1
io.open(P, 'w', encoding='utf-8').write(json.dumps(d, ensure_ascii=False, indent=1))
log.write('naprawiono: %d\n' % fixed)
log.close()
print('ok', fixed)
