"""Read-only original PSK/PSA evidence for shared native hand animations.
No Steam, animation or pose writes. Output only source hashes and measurements.
"""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from prepare_weapon_animation import read_psk, read_psa, key_local


def measure(psk, psa):
    names, parents, _, bind, *_ = read_psk(psk)
    tracks, sequences, keys = read_psa(psa.read_bytes())
    wrist = names.index('l_arm_wrist')
    forearm = names.index('l_arm_forearm')
    hardware = names.index('tmd_base')
    right = names.index('r_arm_wrist')
    gun = names.index('cog_gun')
    assert parents[hardware] == forearm and parents[forearm] == parents[wrist]
    assert parents[gun] == right
    reflect = np.diag([1., -1., 1., 1.])
    joint_ids = [wrist, forearm, hardware, right, gun]
    measurements = []
    for seq in sequences:
        if seq['name'].lower() not in ('idle', 'up', 'down', 'impulse', 'impulse_small', 'age_deconstruct', 'age_reconstruct'):
            continue
        positions = []
        for frame in range(seq['first'], seq['first'] + seq['frames']):
            absolute = []
            for i, name in enumerate(names):
                # PSA parent fields are placeholders. Only original PSK hierarchy owns ancestry.
                local = reflect @ key_local(keys[frame, tracks.index(name)]) @ reflect
                absolute.append(local if i == 0 else local @ absolute[parents[i]])
            positions.append(np.array([absolute[i][3, :3] for i in joint_ids]))
        a = np.array(positions)
        measurements.append(dict(sequence=seq['name'], frames=seq['frames'],
            first_positions=a[0].tolist(), last_positions=a[-1].tolist(),
            displacement_from_first_max_uu=np.linalg.norm(a-a[0], axis=2).max(axis=0).tolist()))
    assert any(m['sequence'].lower()=='up' for m in measurements)
    return dict(inputs=[dict(file=p.name, sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in (psk, psa)],
        joint_order=[names[i] for i in joint_ids],
        parent_chain={names[i]: names[parents[i]] for i in joint_ids},
        convention='row vectors; native Y reflection; local @ parent; PSK ancestry, PSA names',
        sequences=measurements,
        limitation='Original source animation evidence, not a captured blended GPU pose or headset proof')

if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--psk', required=True, type=Path)
    p.add_argument('--psa', required=True, type=Path)
    p.add_argument('--report', required=True, type=Path)
    a=p.parse_args()
    result=measure(a.psk,a.psa)
    a.report.write_text(json.dumps(result,indent=2)+'\n',encoding='utf8')
    for s in result['sequences']:
        print(s['sequence'], 'max displacement UU by joint:',s['displacement_from_first_max_uu'])
