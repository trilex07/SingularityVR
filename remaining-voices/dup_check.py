import json, io
from collections import defaultdict
raw = io.open('C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/story_voicepack_nllb/manifest.jsonl',
              encoding='utf-8', errors='replace').read()
by_asset = defaultdict(list)
for line in raw.splitlines():
    if line.strip():
        d = json.loads(line)
        by_asset[d['asset']].append((d['package'], d.get('text_pl_machine', ''), d.get('text_en', '')))
dups = {a: v for a, v in by_asset.items() if len(v) > 1}
out = io.open('C:/Users/Trilex/vr-mods/dup_assets.txt', 'w', encoding='utf-8')
out.write('assetow powtarzalnych: %d\n' % len(dups))
same_text = 0
for a, v in sorted(dups.items()):
    pls = {t for _, t, _ in v}
    ens = {t for _, _, t in v}
    if len(pls) == 1:
        same_text += 1
    else:
        out.write('\n%s ROZNE TEKSTY:\n' % a)
        for p, t, e in v:
            out.write('  [%s] EN:%s || PL:%s\n' % (p, e[:70], t[:70]))
out.write('\nduplikaty z tym samym tekstem: %d/%d\n' % (same_text, len(dups)))
out.close()
print('ok', len(dups), 'same:', same_text)
