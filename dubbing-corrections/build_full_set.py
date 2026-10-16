import json, os, shutil, hashlib
REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-dubbing-corrections'
CUT = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\story_voicepack_xlingual'
RVO = r'C:\Users\Trilex\vr-mods\SingularityVR-polish-remaining-voices\remaining-voices\out'
DST = r'C:\Dubbing-aktualny'
log = []

def put(src, sub, name=None, origin='pack'):
    name = name or os.path.basename(src)
    dst = os.path.join(DST, sub, name)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copyfile(src, dst)
    log.append({'pack': sub, 'file': name, 'origin': origin,
                'sha16': hashlib.sha256(open(dst, 'rb').read()).hexdigest()[:16]})

# 1) story: caly pack 992 z cutscene, potem overlay moich 130+29
n0 = 0
for f in sorted(os.listdir(CUT)):
    if f.endswith('.wav') and not f.startswith('_'):
        put(os.path.join(CUT, f), 'story', origin='xlingual-pack'); n0 += 1
man = [json.loads(l) for l in open(REPO + r'\dubbing-corrections\deploy_manifest.jsonl',
                                  encoding='utf-8') if l.strip()]
n1 = 0
for r in man:
    if r['pack'] != 'story':
        continue
    put(r['src'], 'story', r['target'], origin='side-resynth')
    n1 += 1

# 2) intro: istniejace (bez _ref) + overlay 6 moich
Existing = [f for f in sorted(os.listdir(RVO + r'\intro'))
            if f.endswith('.pl.wav') and not f.startswith('_ref')]
for f in Existing:
    put(os.path.join(RVO, 'intro', f), 'intro', origin='intro-pack')
for r in man:
    if r['pack'] != 'intro':
        continue
    put(r['src'], 'intro', r['target'], origin='side-new')

# 3) reszta packow bez zmian (z remaining-voices/out, tylko pliki kandydujace)
def load(p):
    import json as j
    raw = open(p, 'rb').read()
    for enc in ('utf-8-sig', 'utf-8', 'cp1250'):
        try:
            return j.loads(raw.decode(enc))
        except Exception:
            pass
    raise ValueError(p)
for pack, rows, fn in [
        ('barks', load(REPO + r'\remaining-voices\out\barks\bark_rows.json'),
         lambda r: r['asset'] + '.pl.wav'),
        ('mp', load(REPO + r'\remaining-voices\out\mp\mp_rows.json'),
         lambda r: r['package'] + '__' + r['asset'] + '.pl.wav'),
        ('skipped', load(REPO + r'\remaining-voices\out\skipped\skipped_final.json'),
         lambda r: r['asset'] + '.pl.wav'),
        ('vignette', load(REPO + r'\remaining-voices\out\vignette\manifest_rows.json'),
         lambda r: r['source_wav'])]:
    seen, n = set(), 0
    for r in rows:
        f = fn(r)
        if f in seen:
            continue
        seen.add(f)
        put(os.path.join(RVO, pack, f), pack, origin='remaining-voices')
        n += 1

with open(REPO + r'\dubbing-corrections\deploy_full_manifest.jsonl', 'w', encoding='utf-8') as fh:
    for r in log:
        fh.write(json.dumps(r, ensure_ascii=False) + '\n')
from collections import Counter
print('story pack:', n0, '+ overlay:', n1, flush=True)
print('total:', len(log), Counter(r['pack'] for r in log), flush=True)
