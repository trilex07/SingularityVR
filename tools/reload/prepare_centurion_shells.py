"""Offline original handgun particle asset -> private SVRK/SVRT. ZERO Steam.

Requires an explicit decompressed package and its UEViewer static-mesh export.
No game-derived bytes belong in Git. Records the actual particle reference chain.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import numpy as np
from PIL import Image
from prepare_centurion import diffuse_cache, textured_preview
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'title'))
from ue3_layout import inspect
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'knife'))
from prepare_knife import parse


def particle_chain(pkg, data):
    exports = {e['index']: e for e in pkg['exports']}

    def unique(name, kind):
        rows = [e for e in exports.values() if e['name'] == name and e['class_name'] == kind]
        if len(rows) != 1 or rows[0].get('parse_error'):
            raise ValueError('missing/ambiguous particle asset: '+name)
        return rows[0]

    def refs(e, name, array=False):
        p = [p for p in e['properties'] if p['name'] == name]
        if len(p) != 1 or p[0]['type'] != ('ArrayProperty' if array else 'ObjectProperty'):
            raise ValueError('missing particle reference: '+name)
        p = p[0]
        raw = data[p['tag_offset']+24:p['tag_offset']+p['tag_bytes']]
        if array:
            if len(raw) < 4:
                raise ValueError('short reference array')
            n, = struct.unpack_from('<i', raw)
            if not 0 < n <= 128 or len(raw) != 4+n*4:
                raise ValueError('invalid reference array')
            return list(struct.unpack_from('<'+'i'*n, raw, 4))
        if len(raw) != 4:
            raise ValueError('invalid object reference')
        return [struct.unpack('<i', raw)[0]]

    system = unique('FX_handgunShellsEject', 'ParticleSystem')
    emitters = refs(system, 'Emitters', True)
    if len(emitters) != 1:
        raise ValueError('unexpected emitter count')
    emitter = exports[emitters[0]]
    lods = [exports[i] for i in refs(emitter, 'LODLevels', True)]
    meshes, materials = set(), set()
    for lod in lods:
        mesh_type = exports[refs(lod, 'TypeDataModule')[0]]
        if mesh_type['class_name'] != 'ParticleModuleTypeDataMesh':
            raise ValueError('not original mesh emitter')
        meshes.update(refs(mesh_type, 'Mesh'))
        required = exports[refs(lod, 'RequiredModule')[0]]
        materials.update(refs(required, 'Material'))
    mesh = unique('shell_rifle', 'StaticMesh')
    material = unique('shells', 'MaterialInstanceConstant')
    texture = unique('shell_rifle_d', 'Texture2D')
    if meshes != {mesh['index']} or materials != {material['index']} or \
            texture['index'] not in refs(material, 'ReferencedTextures', True):
        raise ValueError('particle mesh/material/texture chain changed')
    return dict(system=system['index'], emitter=emitter['index'],
                lods=[e['index'] for e in lods], mesh=mesh['index'],
                material=material['index'], diffuse=texture['index'])


def bake(data, units=52.5):
    if not np.isfinite(units) or not 10 <= units <= 1000:
        raise ValueError('invalid units')
    chunks = parse(data)

    def records(name, stride, fmt):
        size, count, raw = chunks[name]
        if size != stride or not 0 < count <= 65535:
            raise ValueError('unsupported static mesh chunk')
        return [struct.unpack_from(fmt, raw, i*stride) for i in range(count)]

    # Refuse skinning/animation; this is the exact rigid particle mesh.
    if chunks['REFSKELT'][1] or chunks['RAWWEIGHTS'][1]:
        raise ValueError('not a rigid static mesh')
    points = np.array(records('PNTS0000', 12, '<3f'))
    wedges = records('VTXW0000', 16, '<IffBBH')
    faces = records('FACE0000', 12, '<3HBBI')
    if not np.isfinite(points).all():
        raise ValueError('nonfinite points')
    centre = (points.min(0)+points.max(0))*.5
    vertices = []
    for p, u, v, material, _, _ in wedges:
        if p >= len(points) or not np.isfinite([u, v]).all() or material:
            raise ValueError('invalid wedge/material')
        xyz = (points[p]-centre)/units
        if np.linalg.norm(xyz) > .20:
            raise ValueError('oversized particle')
        vertices.append((*xyz, 0xffffffff, u, v))
    indices = []
    for a, b, c, material, _, _ in faces:
        if max(a, b, c) >= len(vertices) or material:
            raise ValueError('invalid face/material')
        indices.extend((a, b, c))
    cache = struct.pack('<4sIII', b'SVRK', 2, len(vertices), len(indices))
    cache += b''.join(struct.pack('<3fI2f', *v) for v in vertices)
    cache += struct.pack('<'+'H'*len(indices), *indices)
    encoded = np.array([struct.unpack_from('<3f', cache, 16+24*i) for i in range(len(vertices))])
    if not np.allclose(encoded*units+centre, points[[w[0] for w in wedges]], rtol=0, atol=2e-6):
        raise ValueError('shell position roundtrip failed')
    return cache, dict(points=len(points), vertices=len(vertices), triangles=len(faces),
                       original_centre_uu=centre.tolist(), units_per_metre=units,
                       dimensions_metres=(np.ptp(points, axis=0)/units).tolist(),
                       geometry='original points/UV/faces; centre translated for spin, no shape edits',
                       roundtrip='all FLOAT32 cache vertices -> original positions <=2e-6 UU',
                       output_sha256=hashlib.sha256(cache).hexdigest())


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--package', type=Path, required=True)
    ap.add_argument('--mesh', type=Path, required=True)
    ap.add_argument('--diffuse', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--preview', action='store_true')
    args = ap.parse_args()
    package = args.package.read_bytes()
    if hashlib.sha256(package).hexdigest().upper() != '0534C5C9A5878D280294168BD12711B99CB3948503D4ED53608A0316479D1B18':
        ap.error('requires audited original Centurion package copy')
    if args.mesh.name != 'shell_rifle.pskx' or args.diffuse.name != 'shell_rifle_d.dds':
        ap.error('requires actual FX_handgunShellsEject exports')
    if hashlib.sha256(args.mesh.read_bytes()).hexdigest() != '0b270e50b804ffb86bab067b2301dde69132a3d710c37f76894d67479e452642' or \
            hashlib.sha256(args.diffuse.read_bytes()).hexdigest() != 'a8379d67d5c5c2f15fc1bda427aa99a7fef2d692fbb90e5b0bda87eff355a6bd':
        ap.error('original export hash mismatch; review rather than silently accepting other art')
    chain = particle_chain(inspect(args.package, data=package, include_all=True), package)
    mesh, report = bake(args.mesh.read_bytes())
    if (report['points'], report['vertices'], report['triangles']) != (105, 136, 100):
        ap.error('unexpected original shell topology')
    with Image.open(args.diffuse) as image:
        texture = diffuse_cache(image)
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output/'centurion_shell.svrk').write_bytes(mesh)
    (args.output/'centurion_shell.svrt').write_bytes(texture)
    report.update(chain=chain, package_sha256=hashlib.sha256(package).hexdigest(),
                  source_sha256=hashlib.sha256(args.mesh.read_bytes()).hexdigest(),
                  texture_source_sha256=hashlib.sha256(args.diffuse.read_bytes()).hexdigest(),
                  texture_sha256=hashlib.sha256(texture).hexdigest(),
                  scope='original asset/albedo; new cosmetic flight, NOT native particle simulation/material/collision/audio')
    (args.output/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    if args.preview:
        textured_preview(mesh, texture, args.output/'shell-original.png', 'Original FX_handgunShellsEject case')
    print(json.dumps(report, indent=2))
