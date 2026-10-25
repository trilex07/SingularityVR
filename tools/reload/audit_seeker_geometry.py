"""Measure original Seeker joints. Read-only private PSK/PSA/cooked package.

No mesh/texture/key data is redistributed. Output contains attachment facts only.
"""
import argparse
import hashlib
import json
import subprocess
import struct
from pathlib import Path
import numpy as np
from prepare_weapon_animation import read_psk, read_psa, cooked_layout, key_local, inspect


def audit(psk_path, psa_path, package_path):
    inputs = [Path(p) for p in (psk_path, psa_path, package_path)]
    raw = [p.read_bytes() for p in inputs]
    names, parents, _, exported, points, weights, wedges, faces = read_psk(inputs[0])
    tracks, sequences, keys = read_psa(raw[1])
    pkg = inspect(inputs[2], data=raw[2], include_all=True)
    weapon, = [e for e in pkg['exports'] if e['name']=='Shared_ARSteerable']
    assert weapon['class_name']=='RvWeaponShared_RocketLauncher'
    def prop(e,n):
        value, = [p for p in e['properties'] if p['name']==n]
        return value
    assert prop(weapon,'mbIsHeavyWeapon')['value'] is True
    ni,number=struct.unpack_from('<ii',raw[2],prop(weapon,'mWeaponName')['tag_offset']+24)
    assert pkg['names'][ni]=='ARSteerable' and number==0
    count,main,alternate=struct.unpack_from('<iii',raw[2],prop(weapon,'mProjectileArchetypes')['tag_offset']+24)
    assert count==2
    projectile_modes=[]
    for index,field in ((main,'mIsDrunk'),(alternate,'mSteerable')):
        e, = [e for e in pkg['exports'] if e['index']==index]
        assert e['class_name']=='RvProjectileShared_Rocket' and prop(e,field)['value'] is True
        projectile_modes.append(dict(archetype=e['name'],native_flag=field))
    mesh, = [e for e in pkg['exports'] if e['name'] == 'ch_inview_e99_steerable_mesh'
             and e['class_name'] == 'SkeletalMesh']
    assert len(wedges) == 12444 and len(faces) == 14996
    layout = cooked_layout(raw[2], dict(vertices=12444, triangles=[14990, 6]),
                           mesh['property_end'], mesh['offset'] + mesh['size'])
    root = names.index('cog_e99_steerable')
    wrist = names.index('l_arm_wrist')
    muzzle = names.index('muzzle_flash')
    assert root == 53 and wrist == 9 and muzzle == 56
    assert layout['chunks'][0]['palette'] == [64, 53, 54, 62, 57, 59, 60, 61, 55]
    assert layout['chunks'][1]['palette'] == [64]
    reflect = np.diag([1., -1., 1., 1.])
    # read_psk has ALREADY undone ActorX's Y reflection in its bone matrices.
    # Its PNTS0000 positions, unlike those matrices, are still ActorX-space.
    native = exported
    # Independently prove the convention against every packed native GPU vertex,
    # not a second computation that repeats the same reflection assumption.
    expected = points[[w[0] for w in wedges]] * [1., -1., 1.]
    first = struct.pack('<3f', *expected[0])
    candidates = []
    at = raw[2].find(first, mesh['property_end'], mesh['offset'] + mesh['size'])
    while at >= 0:
        if at + len(wedges)*32 <= mesh['offset'] + mesh['size']:
            gpu = np.ndarray((len(wedges), 3), '<f4', raw[2], at, strides=(32, 4))
            if np.array_equal(gpu, expected):
                candidates.append(at)
        at = raw[2].find(first, at + 1, mesh['offset'] + mesh['size'])
    assert len(candidates) == 1, 'native GPU positions must prove the bone-space convention'

    def pose(sequence):
        seq, = [s for s in sequences if s['name'].lower() == sequence]
        absolute = []
        for i, name in enumerate(names):
            if name in tracks:
                local = reflect @ key_local(keys[seq['first'], tracks.index(name)]) @ reflect
            else:
                local = native[i] if i == 0 else native[i] @ np.linalg.inv(native[parents[i]])
            absolute.append(local if i == 0 else local @ absolute[parents[i]])
        return absolute[wrist] @ np.linalg.inv(absolute[root])

    idle, fire = pose('idle'), pose('fire')
    assert np.max(np.abs(idle - fire)) < .001  # independently authored fire starts at this grip
    assert np.allclose(idle[:3, :3] @ idle[:3, :3].T, np.eye(3), atol=1e-5)
    origin = native[muzzle][3, :3]
    forward = native[muzzle][0, :3]  # original named socket local +X
    return dict(input_sha256=[hashlib.sha256(r).hexdigest() for r in raw],
                vertices=len(wedges), triangles=len(faces), layout=layout,
                root_bind=native[root].tolist(), left_idle_relative=idle.tolist(),
                muzzle_origin=origin.tolist(), muzzle_forward=forward.tolist(),
                idle_fire_max_difference=float(np.max(np.abs(idle-fire))),native_projectile_modes=projectile_modes,
                native_vertex_witness=dict(offset=candidates[0], stride=32,
                    vertices=len(wedges), max_position_error_uu=0,
                    bone_space='read_psk matrices are already native; PNTS0000 alone reflects Y'))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--psk', required=True)
    p.add_argument('--psa', required=True)
    p.add_argument('--package', required=True)
    p.add_argument('--report', required=True)
    p.add_argument('--test-exe', help='Compiled test-seeker.exe: compare its real constants against private source')
    p.add_argument('--index-output', help='Optional ignored/private original IB for own-process D3D9 test')
    a = p.parse_args()
    result = audit(a.psk, a.psa, a.package)
    if a.test_exe:
        values = np.fromstring(subprocess.check_output([a.test_exe, '--geometry'], text=True), sep=' ')
        expected = np.concatenate([np.asarray(result[k]).ravel() for k in
            ('root_bind', 'left_idle_relative', 'muzzle_origin', 'muzzle_forward')])
        assert values.shape == expected.shape and np.allclose(values, expected, rtol=1e-6, atol=2e-6)
        result['compiled_geometry_max_error'] = float(np.max(np.abs(values-expected)))
    if a.index_output:
        offset = result['layout']['offset'] + 4 + 2*10 + 4 + 4
        size = result['layout']['index_count']*2
        Path(a.index_output).write_bytes(Path(a.package).read_bytes()[offset:offset+size])
    Path(a.report).write_text(json.dumps(result, indent=2)+'\n', encoding='utf8')
    print(json.dumps(result, indent=2))
