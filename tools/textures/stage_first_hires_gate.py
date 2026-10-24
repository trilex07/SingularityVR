"""Stage only the four reviewed priority maps, outside any game install.

Output is a new child of the supplied private asset root; never an install path.
All seven source candidates are verified before creating output. No overwrite.
"""
import argparse
import hashlib
import json
from pathlib import Path
from verify_hires_set import verify

FIRST = ('AR_shotgun_d', 'AR_shotgun_n', 'inview_glove_d', 'inview_glove_n')

def stage(root: Path, name: str, manifest: dict, targets: str):
    if not name or not all(c.isalnum() or c in '-_' for c in name):
        raise ValueError('Use a simple new staging directory name')
    root = root.resolve(strict=True)
    source = root / 'material-remaster-hires/ready'
    receipts = verify(manifest, source, targets, root / 'textures/original')
    payloads = {n: (source / (n + '.dds')).read_bytes() for n in FIRST}
    for n, data in payloads.items():
        if hashlib.sha256(data).hexdigest() != receipts[n]['sha256']:
            raise ValueError('Source changed after verification: ' + n)
    output = root / name
    output.mkdir(exist_ok=False)
    for n, data in payloads.items():
        (output / (n + '.dds')).write_bytes(data)
    (output / 'manifest.json').write_text(json.dumps({n: receipts[n] for n in FIRST}, indent=2))
    return output

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--assets-root', type=Path, required=True)
    p.add_argument('--name', default='astra-first-hires-gate-20261024')
    args = p.parse_args()
    repo = Path(__file__).resolve().parents[2]
    print(stage(args.assets_root, args.name,
        json.loads((repo / 'docs/textures/hires-first-person-manifest.json').read_text()),
        (repo / 'spikes/texture_cache/targets_generated.h').read_text()))
