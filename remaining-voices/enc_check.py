import json, re, io
raw = io.open('C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/story_voicepack_nllb/manifest.jsonl',
              encoding='utf-8', errors='replace').read()
bad = []
for line in raw.splitlines():
    if line.strip():
        d = json.loads(line)
        t = d.get('text_pl_machine', '')
        if re.search(r'[\x00-\x08\x0b\x0c\x0e-\x1f\x7f-\x9f\ufeff\ufffd]', t):
            bad.append((d['package'], d['asset'], t[:100]))
out = io.open('C:/Users/Trilex/vr-mods/nllb_enc_check.txt', 'w', encoding='utf-8')
out.write('podejrzanych: %d\n' % len(bad))
for p, a, t in bad[:30]:
    out.write('%s %s :: %s\n' % (p, a, t))
out.close()
print('ok', len(bad))
