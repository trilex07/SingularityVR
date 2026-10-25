"""Measure idle wrist attachment frames from private original PSK/PSA inputs.

JSON input: {"tmd":{"psk":...,"psa":...}, "5799":{...}, ...}.
Only two attachment frames per profile are emitted, never mesh/animation data.
PSK owns ancestry; PSA owns named tracks. Native cooked axes use reflected Y.
"""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from prepare_weapon_animation import read_psk, read_psa, key_local


def idle_frame(psk, psa, joint):
    names, parents, *_ = read_psk(psk)
    tracks, seqs, keys = read_psa(psa.read_bytes())
    idle = [s for s in seqs if s['name'].lower() == 'idle']
    if len(idle) != 1 or names.count(joint) != 1:
        raise ValueError('ambiguous source idle/joint')
    reflection = np.diag([1., -1., 1., 1.])
    chain = [names.index(joint)]
    while chain[-1] != 0:
        parent = parents[chain[-1]]
        if not 0 <= parent < chain[-1]:
            raise ValueError('invalid source hierarchy')
        chain.append(parent)
    absolute = {}
    for i in reversed(chain):
        name = names[i]
        if tracks.count(name) != 1:
            raise ValueError('missing/ambiguous source track: ' + name)
        local = reflection @ key_local(keys[idle[0]['first'], tracks.index(name)]) @ reflection
        # UEViewer exports the root quaternion conjugated (same reversal as
        # armbind::PskLocal(root=true)); non-root key_local cannot undo that.
        if i == 0:
            local[:3, :3] = local[:3, :3].T
        absolute[i] = local if i == 0 else local @ absolute[parents[i]]
    frame = absolute[names.index(joint)]
    np.testing.assert_allclose(frame[:3, :3] @ frame[:3, :3].T, np.eye(3), atol=1e-6)
    return frame


def generate(manifest, output, report):
    sources = json.loads(manifest.read_text(encoding='utf-8-sig'))
    lines = ['#pragma once', '#include "arm_bind.h"',
             '// Original idle attachment measurements. Reproduce with prepare_dual_hand_reference.py.',
             'namespace dualhandreference {']
    evidence = {}
    for profile, paths in sources.items():
        psk, psa = Path(paths['psk']), Path(paths['psa'])
        joint = 'l_arm_wrist' if profile == 'tmd' else 'r_arm_wrist'
        frame = idle_frame(psk, psa, joint)
        name = 'Left' if profile == 'tmd' else 'Right' + profile
        lines.append('inline constexpr armbind::Mat ' + name + '{{')
        def number(v):
            s = format(float(v), '.9g')
            return s + ('.0' if '.' not in s and 'e' not in s else '') + 'f'
        lines.extend('    {' + ','.join(number(v) for v in row) + '},' for row in frame)
        lines.append('}};')
        evidence[profile] = dict(joint=joint, frame=frame.tolist(),
            inputs=[dict(file=p.name, sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in (psk, psa)])
    lines += ['inline const armbind::Mat* Right(unsigned vertices) {', '    switch(vertices) {']
    for profile in sources:
        if profile != 'tmd':
            lines.append('    case ' + profile + ': return &Right' + profile + ';')
    lines += ['    default: return nullptr;', '    }', '}', '}']
    output.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    report.write_text(json.dumps(evidence, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    for arg in ('manifest', 'output', 'report'):
        ap.add_argument('--' + arg, type=Path, required=True)
    a = ap.parse_args()
    generate(a.manifest, a.output, a.report)
