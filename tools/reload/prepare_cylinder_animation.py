"""Private original PSA -> bounded native palette animation. ZERO Steam.

Cooked chunk order is parsed, not equated with PSK bone numbers. No game-derived
animation keys or mesh assets belong in Git. Requires explicit offline inputs.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import numpy as np
from prepare_centurion import read_psk
from inspect_centurion_animation import read_psa


def fnv(data):
    h = 2166136261
    for b in data:
        h = ((h ^ b) * 16777619) & 0xffffffff
    return h


def cooked_section(data, faces):
    pattern = struct.pack('<ihhiH', 1, 0, 0, 0, faces)
    at = data.find(pattern)
    if at < 0 or data.find(pattern, at+1) != -1:
        raise ValueError('cooked section not unique')
    pos = at+len(pattern)

    def integer():
        nonlocal pos
        value, = struct.unpack_from('<i', data, pos)
        pos += 4
        return value

    def array(fmt):
        nonlocal pos
        n = integer()
        size = struct.calcsize(fmt)
        if not 0 <= n <= 100000 or pos+n*size > len(data):
            raise ValueError('cooked array bounds')
        result = [struct.unpack_from(fmt, data, pos+i*size)[0] for i in range(n)]
        pos += n*size
        return result

    if integer() != 2:
        raise ValueError('not 16-bit indices')
    indices = array('<H')
    if len(indices) != faces*3:
        raise ValueError('index count mismatch')
    array('<H'); array('<H'); array('<B')
    if integer() != 1:
        raise ValueError('not one cooked chunk')
    if integer() != 0 or integer() != 0 or integer() != 0:
        raise ValueError('unexpected chunk vertex origin/CPU data')
    palette = array('<H')
    rigid, soft, influences = integer(), integer(), integer()
    if rigid+soft != 5799 or not 1 <= influences <= 4:
        raise ValueError('unexpected vertex chunk')
    return palette, struct.pack('<'+'H'*len(indices), *indices)


def prepare(psk, psa, cooked):
    names, parents, _, bind, _, weights, wedges, faces = psk
    palette, indices = cooked_section(cooked, len(faces))
    expected = ['clip', 'cog_alt_pistol', 'hammer', 'Trigger', 'clip_release']
    if [names[b] for b in palette] != expected:
        raise ValueError('unknown GPU palette order')
    if {b for row in weights.values() for w, b in row} != set(palette):
        raise ValueError('PSK/cooked bone contributors disagree')
    if len(wedges) != 5799 or len(faces) != 6767:
        raise ValueError('unknown topology')
    pn, sequences, keys = read_psa(psa)
    reload = next(s for s in sequences if s['name'] == 'Reload')
    if reload['frames'] != 53 or reload['rate'] != 24:
        raise ValueError('unreviewed animation')
    parent = names.index('cog_alt_pistol')
    animated = ['clip', 'clip_release']
    bone_ids = [names.index(n) for n in animated]
    if any(parents[b] != parent for b in bone_ids):
        raise ValueError('unreviewed hierarchy')
    reflect = np.diag([1., -1., 1., 1.])
    raw_bind = reflect @ bind @ reflect
    matrices = [raw_bind[parent]] + [np.linalg.inv(raw_bind[b]) for b in bone_ids]
    track = keys[reload['first']:reload['first']+53,
                 [pn.index(n) for n in animated], :7]
    if not np.isfinite(track).all() or not all(np.isfinite(m).all() for m in matrices):
        raise ValueError('nonfinite animation')
    header = struct.pack('<4s10If', b'SVCA', 1, 5799, 0, 6767, fnv(indices), 5, 1, 0, 4, 53, 24.)
    data = header + b''.join(m.astype('<f4').tobytes() for m in matrices) + track.astype('<f4').tobytes()
    if len(data) != 3208:
        raise ValueError('cache size contract')
    return data, dict(palette=palette, palette_names=expected, clip_slot=0,
        release_slot=4, parent_slot=1, vertices=5799, triangles=6767,
        index_fnv=fnv(indices), frames=53, fps=24,
        output_sha256=hashlib.sha256(data).hexdigest(),
        note='Original Reload clip/release keys; render-only, no animation notify or ammo authority')


def preview(psk, psa, destination):
    """Actual original-mesh pose inspection, not a native lighting claim."""
    from PIL import Image, ImageDraw
    names, parents, _, bind, points, weights, wedges, faces = psk
    tracks, sequences, keys = read_psa(psa)
    seq = next(s for s in sequences if s['name'] == 'Reload')
    reflect = np.diag([1., -1., 1., 1.])
    raw_bind = reflect @ bind @ reflect
    animated = [names.index('clip'), names.index('clip_release')]
    frames = (0, 12, 19, 26, 33, 48)
    posed = []
    for frame in frames:
        transforms = {}
        for bone in animated:
            k = keys[seq['first']+frame, tracks.index(names[bone])].astype(float)
            x, y, z, w = k[3:7]*[1, -1, 1, -1]
            qn = np.linalg.norm([x, y, z, w]); x, y, z, w = np.array([x, y, z, w])/qn
            local = np.eye(4)
            local[:3, :3] = [[1-2*(y*y+z*z), 2*(x*y-w*z), 2*(x*z+w*y)],
                            [2*(x*y+w*z), 1-2*(x*x+z*z), 2*(y*z-w*x)],
                            [2*(x*z-w*y), 2*(y*z+w*x), 1-2*(x*x+y*y)]]
            local[3, :3] = k[:3]*[1, -1, 1]
            local = reflect @ local @ reflect
            transforms[bone] = np.linalg.inv(raw_bind[bone]) @ local @ raw_bind[parents[bone]]
        output = np.zeros_like(points)
        for p, rows in weights.items():
            for weight, bone in rows:
                output[p] += weight*((np.r_[points[p], 1.] @ transforms[bone])[:3] if bone in transforms else points[p])
        if frame == 0 and not np.allclose(output, points, rtol=0, atol=2e-5):
            raise ValueError('preview idle pose does not roundtrip to original')
        posed.append(output)
    # Common view/scale for all frames: movement must not be hidden by recentering.
    axes = np.argsort(np.ptp(points, axis=0))[-2:]
    all_xy = np.concatenate([p[:, axes] for p in posed])
    centre = (all_xy.min(0)+all_xy.max(0))*.5
    scale = min(420/np.ptp(all_xy[:, 0]), 300/np.ptp(all_xy[:, 1]))
    image = Image.new('RGB', (1440, 760), '#10171b'); draw = ImageDraw.Draw(image)
    for panel, (frame, p) in enumerate(zip(frames, posed)):
        xy = (p[:, axes]-centre)*[scale, -scale]+[240+(panel%3)*480, 190+(panel//3)*380]
        for face in faces:
            ids = [wedges[w][0] for w in face[:3]]
            moved = any(any(b in animated for _, b in weights[i]) for i in ids)
            coords = [tuple(xy[i]) for i in ids]
            draw.line(coords+[coords[0]], fill='#3c9ca4' if moved else '#6b7075', width=1)
        draw.text((20+(panel%3)*480, 15+(panel//3)*380), f'Original Reload frame {frame} / geometry only', fill='white')
    image.save(destination)


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    for n in ('psk', 'psa', 'cooked', 'output'):
        ap.add_argument('--'+n, type=Path, required=True)
    ap.add_argument('--preview', action='store_true')
    args = ap.parse_args()
    data, report = prepare(read_psk(args.psk), args.psa.read_bytes(), args.cooked.read_bytes())
    for name in ('psk', 'psa', 'cooked'):
        report[name+'_sha256'] = hashlib.sha256(getattr(args, name).read_bytes()).hexdigest()
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output/'centurion_reload.svca').write_bytes(data)
    (args.output/'animation-report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    if args.preview:
        preview(read_psk(args.psk), args.psa.read_bytes(), args.output/'animation-geometry.png')
    print(json.dumps(report, indent=2))
