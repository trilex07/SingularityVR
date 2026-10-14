import json, io
rows = json.loads(io.open('C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/out/skipped/skipped_results.json',
                          encoding='utf-8', errors='replace').read())
nllb = {}
raw = io.open('C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/story_voicepack_nllb/manifest.jsonl',
              encoding='utf-8', errors='replace').read()
for line in raw.splitlines():
    if line.strip():
        d = json.loads(line)
        nllb[(d['package'], d['asset'])] = d.get('text_en', '')
out = io.open('C:/Users/Trilex/vr-mods/skip_analysis.txt', 'w', encoding='utf-8')
from collections import Counter
out.write(str(Counter(r.get('verdict') for r in rows)) + '\n')
en = [r for r in rows if r.get('verdict') == 'STAYS_EN']
out.write('tempo min/max: %.2f %.2f\n' % (min(r['tempo'] for r in en), max(r['tempo'] for r in en)))
out.write('--- STAYS_EN: budzet vs EN vs PL ---\n')
for r in en:
    e = nllb.get((r.get('package'), r.get('asset')), '')
    out.write('%.2f budzet=%.2f EN(%dsl): %s || PL(%dsl): %s\n'
              % (r['tempo'], r['budget'], len(e.split()), e[:80], len(r['text'].split()), r['text'][:80]))
out.close()
print('ok')
