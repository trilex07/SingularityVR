"""Prove three original cooked GPU streams against ActorX positions and bones.

Explicit private inputs/output only. No Steam access or game asset publication.
The SVNV fixture contains original bytes and must stay in this checkout's lab.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import numpy as np
from prepare_centurion import read_psk
from prepare_weapon_animation import SPECS, cooked_layout, inspect


def audit(kind, psk, data, start, end):
    spec = SPECS[kind]
    names, _, _, bind, points, weights, wedges, faces = psk
    if len(wedges) != spec['vertices'] or len(faces) != sum(spec['triangles']):
        raise ValueError('unreviewed topology')
    layout = cooked_layout(data, spec, start, end)
    expected = points[[w[0] for w in wedges]] * [1, -1, 1]
    needle = struct.pack('<3f', *expected[0])
    matches = []
    at = data.find(needle, start, end)
    while at >= 0:
        if at + len(wedges)*32 <= end:
            xyz = np.ndarray((len(wedges), 3), '<f4', data, at, (32, 4))
            if np.allclose(xyz, expected, rtol=0, atol=2e-5):
                matches.append(at)
        at = data.find(needle, at+1, end)
    if len(matches) != 1:
        raise ValueError('not one complete native position stream')
    native = data[matches[0]:matches[0]+len(wedges)*32]
    slots = np.ndarray((len(wedges), 4), 'u1', native, 20, (32, 1))
    blends = np.ndarray((len(wedges), 4), 'u1', native, 24, (32, 1))
    if not np.all(blends.sum(1) == 255):
        raise ValueError('native weights not normalized')
    rigid_clip = 0
    for chunk in layout['chunks']:
        palette = chunk['palette']
        for i in range(chunk['first'], chunk['first']+chunk['vertices']):
            if np.any(slots[i] >= len(palette)):
                raise ValueError('native bone slot outside chunk')
            actual = sorted((palette[int(b)], float(w)/255) for b, w in zip(slots[i], blends[i]) if w)
            source = sorted((b, float(w)) for w, b in weights[wedges[i][0]] if w)
            if len(actual) != len(source) or any(a[0] != b[0] or abs(a[1]-b[1]) > 1/255+1e-5
                                                for a, b in zip(actual, source)):
                raise ValueError('native/source blend disagreement')
            rigid_clip += actual == [(names.index('clip'), 1.)]
    # Section table -> INDEX16 element-size/count -> complete original IB.
    index_at = layout['offset']+4+struct.calcsize('<hhiH')*len(spec['triangles'])+8
    indices = data[index_at:index_at+layout['index_count']*2]
    triangles = np.frombuffer(indices, '<u2').reshape(-1, 3)
    if not np.array_equal(triangles, np.array([f[:3] for f in faces])[:, [1, 0, 2]]):
        raise ValueError('native/source winding disagreement')
    clip = names.index('clip')
    reflected = np.diag([1., -1., 1., 1.]) @ bind[clip] @ np.diag([1., -1., 1., 1.])
    report = dict(profile=kind, vertices=len(wedges), triangles=len(faces), rigid_clip_wedges=rigid_clip,
                  native_bind=bind[clip].tolist(), exported_bind=reflected.tolist(),
                  wrong_bind_origin_distance_uu=float(np.linalg.norm(bind[clip, 3, :3]-reflected[3, :3])),
                  native_position_file_offset=matches[0], index_fnv=layout['index_fnv'],
                  proof='all native positions = ActorX positions with Y negated; all weights/bone slots/indices match',
                  scope='offline original cooked bytes, not live shader matrices or PSVR2 acceptance')
    fixture = b'SVNV'+struct.pack('<3I', 1, len(wedges), len(indices)//2)+native+indices
    return fixture, report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--weapon', choices=SPECS, required=True)
    for key in ('psk', 'package', 'output'):
        parser.add_argument('--'+key, type=Path, required=True)
    args = parser.parse_args()
    allowed = (Path(__file__).resolve().parents[2]/'build/next-weapon-reload').resolve()
    output = args.output.resolve()
    if not output.is_relative_to(allowed) or output == allowed or output.exists():
        parser.error('output must be NEW child of own ignored lab')
    raw = args.package.read_bytes()
    package = inspect(args.package, data=raw, include_all=True)
    mesh = [e for e in package['exports'] if e['name'] == SPECS[args.weapon]['mesh'] and e['class_name'] == 'SkeletalMesh']
    if len(mesh) != 1:
        parser.error('not one named original mesh')
    mesh = mesh[0]
    fixture, report = audit(args.weapon, read_psk(args.psk), raw, mesh['property_end'], mesh['offset']+mesh['size'])
    report.update(psk_sha256=hashlib.sha256(args.psk.read_bytes()).hexdigest(),
                  package_sha256=hashlib.sha256(raw).hexdigest(), fixture_sha256=hashlib.sha256(fixture).hexdigest())
    output.mkdir(parents=True)
    (output/'native.svnv').write_bytes(fixture)
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, indent=2))
