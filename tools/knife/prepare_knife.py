"""Offline ActorX knife -> bounded, local-only SVRK mesh. Never reads Steam.

Usage: prepare_knife.py --mesh <2010_knife.pskx> --output <outside-Steam dir>
No original assets are committed. Export with UEViewer from an offline copy.
Neutral steel/handle vertex colours deliberately do not claim to be the native
material: source texture TFC may be absent in an offline research copy.
"""
import argparse
import hashlib
import json
import math
import struct
from pathlib import Path


def parse(data):
    p = 0
    chunks = {}
    while p < len(data):
        if p+32 > len(data):
            raise ValueError('short chunk header')
        raw, _, size, count = struct.unpack_from('<20siii', data, p)
        p += 32
        name = raw.split(b'\0')[0].decode('ascii')
        if size < 0 or count < 0 or count > 1000000 or p+size*count > len(data) or name in chunks:
            raise ValueError('chunk bounds/duplicate')
        chunks[name] = (size, count, data[p:p+size*count])
        p += size*count
    return chunks


def bake(data, length=.30):
    c = parse(data)
    if not .15 <= length <= .50:
        raise ValueError('physical length outside bounded calibration range')
    def records(name, size, fmt):
        stride, count, raw = c[name]
        if stride != size or not 0 < count <= 65535:
            raise ValueError('unsupported ActorX layout')
        return [struct.unpack_from(fmt, raw, i*stride) for i in range(count)]
    points = records('PNTS0000', 12, '<3f')
    wedges = records('VTXW0000', 16, '<IffBBH')
    faces = records('FACE0000', 12, '<3HBBI')
    if any(not all(math.isfinite(v) for v in p) for p in points):
        raise ValueError('nonfinite point')
    bounds = [[min(p[j] for p in points), max(p[j] for p in points)] for j in range(3)]
    if bounds[1][1]-bounds[1][0] < 1:
        raise ValueError('longitudinal -Y axis not established')
    scale = length/(bounds[1][1]-bounds[1][0])
    verts = []
    for point, u, v, _, _, _ in wedges:
        if point >= len(points) or not math.isfinite(u+v):
            raise ValueError('bad wedge')
        x, y, z = points[point]
        # Measured source -Y = blade, X = thickness, Z = blade width.
        # Tracking local -Z = blade, +X = width, +Y = thickness; grip at
        # original mesh origin (authored hilt) with no invented UE3 bone offset.
        verts.append((z*scale, x*scale, y*scale))
    indices = []
    colours = [0xff9ba6af if points[w[0]][1] < -7 else 0xff34383b for w in wedges]
    for a, b, d, material, _, _ in faces:
        if max(a,b,d) >= len(verts) or material != 0:
            raise ValueError('bad face/material')
        indices.extend((a,b,d))
    output = struct.pack('<4sIII', b'SVRK', 1, len(verts), len(indices))
    output += b''.join(struct.pack('<3fI', *v, col) for v,col in zip(verts,colours))
    output += struct.pack('<'+'H'*len(indices), *indices)
    report = dict(source_sha256=hashlib.sha256(data).hexdigest(), source_points=len(points),
                  vertices=len(verts), triangles=len(indices)//3, bounds_source=bounds,
                  length_metres=length, metres_per_source_unit=scale,
                  material='neutral vertex colours, not native texture',
                  output_sha256=hashlib.sha256(output).hexdigest())
    return output, report


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('--mesh', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--length', type=float, default=.30)
    args = ap.parse_args()
    data, report = bake(args.mesh.read_bytes(), args.length)
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output/'knife.svrk').write_bytes(data)
    (args.output/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, indent=2))
