import io, json
import ftfy

P = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/intro-pl.json'
raw = io.open(P, encoding='utf-8', errors='surrogateescape').read()
d = json.loads(raw)
log = io.open('C:/Users/Trilex/vr-mods/intro_pl_check.txt', 'w', encoding='utf-8')
fixed = 0
for r in d:
    s = r.get('pl', '')
    try:
        b = s.encode('utf-8', errors='surrogateescape')
    except Exception:
        b = s.encode('utf-8', errors='replace')
    s2 = ftfy.fix_text(b.decode('cp1252', errors='replace'))
    if s2 != s:
        fixed += 1
    r['pl'] = s2
    log.write('%s :: %s\n  PL: %s\n' % (r['asset'], r['en'][:90], r['pl']))
io.open(P, 'w', encoding='utf-8').write(json.dumps(d, ensure_ascii=False, indent=1))
log.write('\nnaprawiono: %d\n' % fixed)
log.close()
print('ok', len(d), 'fixed', fixed)
