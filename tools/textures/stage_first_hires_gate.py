"""Stage one exact, independently gated material group outside any game install.

Output is a new child of the supplied private asset root; never an install path.
All seven source candidates are verified before creating output. No overwrite.
"""
import argparse
import hashlib
import json
from pathlib import Path
from verify_hires_set import verify

FIRST = ('AR_shotgun_d', 'AR_shotgun_n', 'inview_glove_d', 'inview_glove_n')
GROUPS = {'first': FIRST, 'valkyrie': ('AR_E99Rifle_alt_d', 'AR_E99Rifle_n')}

def stage(root: Path, name: str, manifest: dict, targets: str, group: str = 'first'):
    if group not in GROUPS:
        raise ValueError('Unknown reviewed material group')
    if not name or not all(c.isalnum() or c in '-_' for c in name):
        raise ValueError('Use a simple new staging directory name')
    root = root.resolve(strict=True)
    if any(part.lower() in {'steam', 'steamapps', 'binaries'} for part in root.parts):
        raise ValueError('Refusing game/Steam staging destination')
    source = root / 'material-remaster-hires/ready'
    receipts = verify(manifest, source, targets, root / 'textures/original')
    payloads = {n: (source / (n + '.dds')).read_bytes() for n in GROUPS[group]}
    for n, data in payloads.items():
        if hashlib.sha256(data).hexdigest() != receipts[n]['sha256']:
            raise ValueError('Source changed after verification: ' + n)
    output = root / name
    output.mkdir(exist_ok=False)
    for n, data in payloads.items():
        (output / (n + '.dds')).write_bytes(data)
    (output / 'manifest.json').write_text(json.dumps({n: receipts[n] for n in GROUPS[group]}, indent=2))
    (output / 'review.json').write_text(json.dumps({
        'group': group, 'physical_acceptance': 'PENDING', 'deployed': False,
        'ini_key': 'valkyrie_material_mode' if group == 'valkyrie' else 'priority_material_mode',
        'default': 0, 'modes': {'0': 'existing policy', '1': 'log only', '2': 'apply reviewed pair'},
        'dds_bytes': sum(len(data) for data in payloads.values()),
        'note': 'Staging does not enable replacement or establish visual approval.'
    }, indent=2))
    return output

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--assets-root', type=Path, required=True)
    p.add_argument('--name', default='astra-first-hires-gate-20261024')
    p.add_argument('--group', choices=GROUPS, default='first')
    args = p.parse_args()
    repo = Path(__file__).resolve().parents[2]
    print(stage(args.assets_root, args.name,
        json.loads((repo / 'docs/textures/hires-first-person-manifest.json').read_text()),
        (repo / 'spikes/texture_cache/targets_generated.h').read_text(), args.group))
