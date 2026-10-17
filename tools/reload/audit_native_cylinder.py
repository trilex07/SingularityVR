"""Offline exact cooked GPU positions/indices versus ActorX export.

Inputs must be the user's previously extracted original files. Native bytes and
JSON reports stay private; no install detection or Steam operations.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import numpy as np
from prepare_centurion import read_psk
from prepare_cylinder_animation import cooked_section


def audit(psk, cooked):
    names, parents, _, bind, points, weights, wedges, faces = psk
    if len(wedges) != 5799 or len(faces) != 6767 or names.index('clip') != 57:
        raise ValueError('unreviewed original mesh')
    palette, indices = cooked_section(cooked, len(faces))
    if [names[i] for i in palette] != ['clip', 'cog_alt_pistol', 'hammer', 'Trigger', 'clip_release']:
        raise ValueError('palette identity changed')
    expected = points[[w[0] for w in wedges]] * [1, -1, 1]
    # Prove the complete stream, not a common float pattern/one point.
    needle = struct.pack('<3f', *expected[0])
    candidates = []
    at = cooked.find(needle)
    while at >= 0:
        if at+len(wedges)*32 <= len(cooked):
            xyz = np.ndarray((len(wedges),3),'<f4',cooked,at,(32,4))
            if np.allclose(xyz, expected, rtol=0, atol=2e-5):
                candidates.append(at)
        at = cooked.find(needle, at+1)
    if len(candidates) != 1:
        raise ValueError('not one complete original GPU stream')
    position = candidates[0]
    native = cooked[position:position+32*len(wedges)]
    slots = np.ndarray((len(wedges),4),'u1',native,20,(32,1))
    blends = np.ndarray((len(wedges),4),'u1',native,24,(32,1))
    if not np.all(blends.sum(1) == 255) or np.any(slots >= len(palette)):
        raise ValueError('native blend weights not normalized')
    for i, (point, _, _) in enumerate(wedges):
        actual = sorted((palette[int(b)],float(w)/255) for b,w in zip(slots[i],blends[i]) if w)
        expected_weights = sorted((b,float(w)) for w,b in weights[point] if w)
        if len(actual) != len(expected_weights) or any(a[0] != b[0] or abs(a[1]-b[1]) > 1/255+.00001 for a,b in zip(actual,expected_weights)):
            raise ValueError('native blend slot/weight differs from source')
    triangles = np.frombuffer(indices,'<u2').reshape(-1,3)
    if not np.array_equal(triangles, np.array([f[:3] for f in faces])[:,[1,0,2]]):
        raise ValueError('cooked winding differs from full export')
    raw_bind = np.diag([1.,-1.,1.,1.]) @ bind @ np.diag([1.,-1.,1.,1.])
    clip = names.index('clip')
    delta = np.linalg.norm(bind[clip,3,:3]-raw_bind[clip,3,:3])
    report = dict(vertices=len(wedges),triangles=len(faces),stride=32,position_offset=0,
                  indices_offset=20,weights_offset=24,uv_offset=28,
                  complete_native_vertex_blend_index_roundtrip=True,
                  rigid_clip_gpu_wedges=int(np.count_nonzero((slots[:,0] == 0) & (blends[:,0] == 255))),
                  exported_position_convention='ActorX reflected Y',native_position_convention='UE3 unreflected Y',
                  native_position_file_offset=position,
                  clip_native_bind_position=bind[clip,3,:3].tolist(),
                  clip_exported_bind_position=raw_bind[clip,3,:3].tolist(),
                  wrong_bind_origin_distance_uu=float(delta),
                  scope='original cooked bytes; not a current shader/content or headset proof')
    return b'SVNV'+struct.pack('<3I',1,len(wedges),len(indices)//2)+native+indices, report


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--psk',type=Path,required=True);p.add_argument('--cooked',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    allowed=(Path(__file__).resolve().parents[2]/'build/centurion-reload').resolve()
    output=a.output.resolve()
    if not output.is_relative_to(allowed) or output==allowed or output.exists():
        p.error('output must be NEW child of own ignored lab')
    raw=a.cooked.read_bytes();data,report=audit(read_psk(a.psk),raw)
    report.update(psk_sha256=hashlib.sha256(a.psk.read_bytes()).hexdigest(),
                  cooked_sha256=hashlib.sha256(raw).hexdigest(),fixture_sha256=hashlib.sha256(data).hexdigest())
    output.mkdir(parents=True);(output/'native.svnv').write_bytes(data)
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))
