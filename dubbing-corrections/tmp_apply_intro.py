import json
REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
SRC = REPO + r'\remaining-voices\intro-pl.json'
COR = REPO + r'\dubbing-corrections\corrections_intro.jsonl'
NEW_STATUS = 'side_text_review_applied'

d = json.loads(open(SRC, 'rb').read().decode('utf-8'))
idx = {r['asset']: r for r in d}
cor = [json.loads(l) for l in open(COR, encoding='utf-8') if l.strip()]
changed = 0
for c in cor:
    asset = c['id'].split(':')[-1]
    assert asset in idx, asset
    r = idx[asset]
    assert r.get('status') == 'machine_draft', (asset, r.get('status'))
    if r['pl'] == c['proposed_pl']:
        continue
    r['pl'] = c['proposed_pl']
    r['status'] = NEW_STATUS
    changed += 1
print('matched: %d changed: %d' % (len(cor), changed), flush=True)
open(SRC, 'wb').write(json.dumps(d, ensure_ascii=False, indent=1).replace('\n', '\r\n').encode('utf-8'))

d2 = json.loads(open(SRC, 'rb').read().decode('utf-8'))
cmap = {c['id'].split(':')[-1]: c['proposed_pl'] for c in cor}
ok = sum(1 for r in d2 if r['asset'] in cmap and r['pl'] == cmap[r['asset']]
         and r['status'] == NEW_STATUS)
assert ok == changed == 6, (ok, changed)
from collections import Counter
print(Counter(r['status'] for r in d2), flush=True)
print('OK', flush=True)
