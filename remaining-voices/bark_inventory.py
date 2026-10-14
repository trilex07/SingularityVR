import sys, os, json, subprocess
sys.path.insert(0, 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/tools/audio')
from ue3_sound_package import inspect

G = 'C:/Program Files (x86)/Steam/steamapps/common/Singularity/RvGame/CookedPC'
W = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/_work_u/barks'
os.makedirs(W, exist_ok=True)
EXE = 'C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/tools/uedecompress/unpack_chunks.exe'
cov = json.load(open('C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/bark_coverage.json'))
inv = {}
for pkg, gain in cov['order']:
    u = os.path.join(W, pkg + '.u')
    if not os.path.exists(u):
        r = subprocess.run([EXE, os.path.join(G, pkg + '.xxx'), u], capture_output=True, text=True)
        if r.returncode != 0:
            print('DECODE FAIL', pkg, r.stdout[-200:] + r.stderr[-200:], flush=True)
            continue
    try:
        ex = inspect(open(u, 'rb').read())['exports']
    except Exception as e:
        print('INSPECT FAIL', pkg, str(e)[:120], flush=True)
        continue
    n = 0
    for e in ex:
        if e['class_name'] == 'SoundNodeWave' and e['name'].startswith('SD_'):
            p = {x['name']: x.get('value') for x in e['properties']}
            if e['name'] not in inv:
                inv[e['name']] = {'spoken': p.get('SpokenText') or '',
                                  'rate': p.get('SampleRate'), 'ch': p.get('NumChannels'),
                                  'dur': round(float(p.get('Duration') or 0), 2),
                                  'package': pkg}
                n += 1
    print('OK', pkg, 'nowych:', n, 'razem:', len(inv), flush=True)

json.dump(inv, open('C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/bark-speech.json', 'w'),
          ensure_ascii=False, indent=1)
# klasyfikacja
verbal, nonver = [], []
for name, r in inv.items():
    w = (r['spoken'] or '').split()
    if len(w) >= 2:
        verbal.append(name)
    else:
        nonver.append(name)
print('unikalnych:', len(inv), 'werbalnych(2+ slow):', len(verbal), 'niewerbalnych:', len(nonver))
json.dump({'verbal': sorted(verbal), 'nonverbal': sorted(nonver)},
          open('C:/Users/Trilex/vr-mods/SingularityVR-polish-remaining-voices/remaining-voices/bark-class.json', 'w'))
print('DONE')
