import json, io, os
rows = json.loads(io.open('C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/out/skipped/skipped_results_r2.json',
                          encoding='utf-8', errors='replace').read())
from collections import Counter
print(Counter(r.get('verdict', 'NO_VERDICT') for r in rows))
D = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/out/skipped'
have = {f[:-len('.pl.wav')] for f in os.listdir(D) if f.endswith('.pl.wav')}
print('plikow:', len(have))
nover = [r['asset'] for r in rows if 'verdict' not in r]
print('bez verdictu:', len(nover), 'w tym z plikiem:', sum(1 for a in nover if a in have))
needs = [r for r in rows if r.get('verdict') == 'NEEDS_TRIM_OR_TEXTFIX']
print('NEEDS_TRIM:', len(needs), 'z plikiem:', sum(1 for r in needs if r['asset'] in have))
out = io.open('C:/Users/Trilex/vr-mods/r2_analysis.txt', 'w', encoding='utf-8')
for r in rows:
    if 'verdict' not in r:
        out.write('NO_VERDICT %s | plik=%s | %s\n' % (r['asset'], r['asset'] in have, r.get('note', '')))
out.close()
print('ok')
