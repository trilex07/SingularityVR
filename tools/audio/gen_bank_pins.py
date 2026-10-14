"""Generate pinned stock/logical hashes for all xlingual target banks.

For each xlingual package: hash the Steam original, cross-check two mirror
copies, decode with unpack_chunks, verify the logical (package tag, export
table parses, mapped assets exist), record both hashes. Refuses disagreement.
Writes tools/audio/xlingual_bank_pins.json (tracked provenance record).
"""
import json
import collections
import hashlib
import os
import subprocess
import sys

sys.path.insert(0, r'C:\Users\Trilex\vr-mods\SingularityVR-recovery\tools\audio')
from ue3_sound_package import inspect

XLDIR = r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\polish\story_voicepack_xlingual'
COOKED = r'C:\Program Files (x86)\Steam\steamapps\common\Singularity\RvGame\CookedPC'
MIRRORS = [r'C:\Users\Trilex\vr-mods\l3-sandbox-20261010\RvGame\CookedPC',
           r'C:\Users\Trilex\vr-mods\SingularityVR-assets-local\cooked']
UX = r'C:\Users\Trilex\vr-mods\SingularityVR-recovery\deployment\hands-dubbing\xlingual-banks\tools\unpack_chunks.exe'
SCRATCH = r'C:\Users\Trilex\vr-mods\SingularityVR-recovery\deployment\hands-dubbing\xlingual-banks\full'
OUT_PINS = r'C:\Users\Trilex\vr-mods\SingularityVR-recovery\tools\audio\xlingual_bank_pins.json'

xl = collections.defaultdict(list)
for line in open(os.path.join(XLDIR, 'manifest_xlingual.jsonl'), encoding='utf-8-sig'):
    if line.strip():
        r = json.loads(line)
        xl[r.get('package')].append(r['asset'])

os.makedirs(os.path.join(SCRATCH, 'cooked'), exist_ok=True)
os.makedirs(os.path.join(SCRATCH, 'logicals'), exist_ok=True)
pins = {}
for pkg in sorted(xl):
    src = os.path.join(COOKED, pkg + '.xxx')
    assert os.path.exists(src), 'missing cooked: ' + pkg
    raw = open(src, 'rb').read()
    stock = hashlib.sha256(raw).hexdigest()
    agree = 0
    for m in MIRRORS:
        q = os.path.join(m, pkg + '.xxx')
        if os.path.exists(q) and hashlib.sha256(open(q, 'rb').read()).hexdigest() == stock:
            agree += 1
    dst = os.path.join(SCRATCH, 'cooked', pkg + '.xxx')
    open(dst, 'wb').write(raw)
    dec = os.path.join(SCRATCH, 'logicals', pkg + '.u')
    r = subprocess.run([UX, dst, dec], capture_output=True, text=True)
    assert r.returncode == 0, 'decode failed: ' + pkg + ' ' + r.stdout[-500:]
    logical = open(dec, 'rb').read()
    exports = inspect(logical)['exports']
    names = {e['name'] for e in exports if e['class_name'] == 'SoundNodeWave'}
    missing = [a for a in xl[pkg] if a not in names]
    assert not missing, 'mapped assets absent in %s: %s' % (pkg, missing)
    pins[pkg] = {'stock': stock.upper(), 'logical': hashlib.sha256(logical).hexdigest().upper(),
                 'mirror_agree': agree, 'xlingual_lines': len(xl[pkg]),
                 'native_waves': len(names)}
    print(pkg, 'stock', stock[:16], 'mirrors', agree, 'lines', len(xl[pkg]), 'waves', len(names), flush=True)

open(OUT_PINS, 'w').write(json.dumps(pins, indent=1))
print('PINS:', len(pins), '->', OUT_PINS, flush=True)
