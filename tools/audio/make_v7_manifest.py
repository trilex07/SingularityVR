"""Generate the v7 install manifest: remaining xlingual banks + story flags.

Install set = OFFLINE-status banks minus the 8 VI2 banks already on Steam.
3 FAILED banks stay stock (recorded, not installed).
"""
import json
import hashlib
import os

REPO = r'C:\Users\Trilex\vr-mods\SingularityVR-recovery'
OUT = os.path.join(REPO, 'deployment', 'hands-dubbing', 'xlingual-banks', 'full', 'out')
PINS = json.load(open(os.path.join(REPO, 'tools', 'audio', 'xlingual_bank_pins.json'), encoding='utf-8-sig'))
SEP = chr(92)

VI2_DONE = {'SP_VI2_kismet01_LOC_INT', 'SP_VI2_kismet01_a_LOC_INT', 'SP_VI2_kismet01_b_LOC_INT',
            'SP_VI2_kismet02_LOC_INT', 'SP_VI2_kismet03_LOC_INT', 'SP_VI2_kismet04_LOC_INT',
            'SP_VI2_zone02a_LOC_INT', 'SP_VI2_zone02a_2010_LOC_INT'}

report = json.load(open(os.path.join(OUT, 'xlingual-banks.json'), encoding='utf-8-sig'))
files = []
failed = []
for b in report:
    pkg = b['package']
    if pkg in VI2_DONE:
        continue
    if not b.get('status', '').startswith('OFFLINE'):
        failed.append(pkg)
        continue
    cand = os.path.join(OUT, pkg, 'candidate.xxx')
    h = hashlib.sha256(open(cand, 'rb').read()).hexdigest().upper()
    assert h == b['candidate_sha256'].upper(), 'candidate changed: ' + pkg
    files.append({'RelativePath': SEP.join(['RvGame', 'CookedPC', pkg + '.xxx']),
                  'Source': pkg + SEP + 'candidate.xxx',
                  'BaselineSha256': PINS[pkg]['stock'].upper(),
                  'CandidateSha256': h})

story_rel = SEP.join(['Binaries', 'vr-assets', 'localization', 'pl', 'polish_story_voice.jsonl'])
story_src = os.path.join(OUT, 'story', 'polish_story_voice.jsonl')
story_cand = hashlib.sha256(open(story_src, 'rb').read()).hexdigest().upper()
GAME_STORY = r'C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\vr-assets\localization\pl\polish_story_voice.jsonl'
story_base = hashlib.sha256(open(GAME_STORY, 'rb').read()).hexdigest().upper()
assert story_base == '22B755EBA43D1003E188722AC53620757019DEE6C97C8F879BAFA09E7023C315', 'story manifest moved'
files.append({'RelativePath': story_rel, 'Source': 'story' + SEP + 'polish_story_voice.jsonl',
              'BaselineSha256': story_base, 'CandidateSha256': story_cand})

manifest = {'Kind': 'SingularityVR xlingual full-game bank delta v7',
            'Mode': 'MAIN-only indistinguishable-lease',
            'Files': sorted(files, key=lambda f: f['RelativePath']),
            'StartRequired': False, 'SteamWritePerformed': False, 'PhysicalAcceptance': 'PENDING',
            'FontEnabled': False,
            'Notes': 'remaining xlingual banks + story flags; VI2-done excluded; 3 failed banks stay stock'}
open(os.path.join(OUT, 'bank-manifest-v7.json'), 'w').write(json.dumps(manifest, indent=1))
print('V7 FILES:', len(files), 'banks:', len(files) - 1, 'failed-excluded:', failed, flush=True)
