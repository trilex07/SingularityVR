"""Private, source-exact Seeker rig and detachable rear magazine GLBs.

The animation is a reference for mechanical motion. It never commits ammo or
drives the tracked weapon root. Input PSK/PSA must be original private exports.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'reload'))
from prepare_centurion import read_psk
from inspect_centurion_animation import read_psa
from build_volk_mechanical_glb import Buffer, quaternion
from prepare_weapon_animation import key_local

UNITS = 52.5
ROOT = 53
CLIP = 57
EXPECTED_PSK = '9ac2af6884610789cab9d65e685222a04f33d51d9eca6bfa04c62977d7947816'
EXPECTED_PSA = '0fe784a10c05fe32729a4385275d66730c256809f86a72feddcac2134f6bb460'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def encode(doc, buf, path):
    doc.update(buffers=[{'byteLength': len(buf.data)}], bufferViews=buf.views, accessors=buf.accessors)
    js = json.dumps(doc, separators=(',', ':')).encode()
    js += b' ' * (-len(js) % 4)
    binary = bytes(buf.data) + b'\0' * (-len(buf.data) % 4)
    blob = (struct.pack('<4sII', b'glTF', 2, 12 + 8 + len(js) + 8 + len(binary)) +
            struct.pack('<I4s', len(js), b'JSON') + js +
            struct.pack('<I4s', len(binary), b'BIN\0') + binary)
    path.write_bytes(blob)
    return hashlib.sha256(blob).hexdigest()


def mesh_parts(psk, selected):
    names, _, _, bind, points, weights, wedges, faces = psk
    reflect = np.diag([1., -1., 1., 1.])
    inverse = {b: np.linalg.inv(reflect @ bind[b] @ reflect) for b in selected}
    rows = {(b, m): [] for b in selected for m in (0, 1)}
    indices = {key: [] for key in rows}
    remaps = {key: {} for key in rows}
    for face in faces:
        bones = set()
        for wedge in face[:3]:
            w = weights[wedges[wedge][0]]
            if len(w) != 1 or abs(w[0][0] - 1) > 1e-6:
                raise ValueError('nonrigid Seeker source point')
            bones.add(w[0][1])
        if len(bones) != 1:
            raise ValueError('cross-bone Seeker source triangle')
        bone = bones.pop()
        key = bone, face[3]
        if key not in rows:
            raise ValueError('unexpected bone/material')
        for wedge in face[:3]:
            point, u, v = wedges[wedge]
            corner = point, u, v
            if corner not in remaps[key]:
                xyz = (np.r_[points[point], 1.] @ inverse[bone])[:3] / UNITS
                remaps[key][corner] = len(rows[key])
                rows[key].append([*xyz, u, v])
            indices[key].append(remaps[key][corner])
    if sum(len(x) for x in indices.values()) != len(faces) * 3:
        raise ValueError('incomplete Seeker mesh split')
    return {k: (np.asarray(v, dtype='<f4').reshape(-1, 5),
                np.asarray(indices[k], dtype='<u2')) for k, v in rows.items() if v}


def add_pbr(doc, buf, texture_dir):
    paths = {name: texture_dir / f'seeker_{name}_4k.png' for name in ('albedo', 'normal_dx', 'roughness', 'metallic')}
    for path in paths.values():
        if not path.is_file() or Image.open(path).size != (4096, 2048):
            raise ValueError('missing/unreviewed 4K PBR map')
    rough = np.asarray(Image.open(paths['roughness']).convert('L'))
    metal = np.asarray(Image.open(paths['metallic']).convert('L'))
    packed = np.zeros((2048, 4096, 3), dtype=np.uint8)
    packed[:, :, 1] = rough; packed[:, :, 2] = metal
    n = np.asarray(Image.open(paths['normal_dx']).convert('RGB')).copy()
    n[:, :, 1] = 255 - n[:, :, 1]  # glTF tangent normal is OpenGL Y.
    import io
    blobs = []
    for value in (Image.open(paths['albedo']).convert('RGB'), Image.fromarray(n), Image.fromarray(packed)):
        stream = io.BytesIO(); value.save(stream, format='PNG'); blobs.append(stream.getvalue())
    images = [];textures = []
    for label, blob in zip(('albedo', 'normal_gl', 'metallic_roughness'), blobs):
        while len(buf.data) % 4: buf.data.append(0)
        offset = len(buf.data); buf.data.extend(blob)
        images.append({'name':label,'mimeType':'image/png','bufferView':len(buf.views)})
        buf.views.append({'buffer':0,'byteOffset':offset,'byteLength':len(blob)})
        textures.append({'source':len(images)-1})
    doc['images'] = images; doc['textures'] = textures
    doc['materials'] = [
        {'name':'e99_steerable_hd','pbrMetallicRoughness':{
            'baseColorTexture':{'index':0},'metallicRoughnessTexture':{'index':2}},
         'normalTexture':{'index':1}},
        {'name':'e99_steerable_lens','pbrMetallicRoughness':{
            'baseColorFactor':[.12,.4,.45,1.], 'metallicFactor':.15,'roughnessFactor':.24},
         'emissiveFactor':[.04,.23,.3]}]


def build(psk_path, psa_path, output, textures=None):
    if digest(psk_path) != EXPECTED_PSK or digest(psa_path) != EXPECTED_PSA:
        raise ValueError('source identity differs from audited Seeker exports')
    psk = read_psk(psk_path)
    names, parents, local = psk[:3]
    if len(names) != 85 or names[ROOT] != 'cog_e99_steerable' or names[CLIP] != 'clip':
        raise ValueError('unexpected Seeker rig')
    subtree = {CLIP}
    for i, p in enumerate(parents):
        if i and p in subtree:
            subtree.add(i)
    weapon = {ROOT}
    for i, p in enumerate(parents):
        if i and p in weapon:
            weapon.add(i)
    parts = mesh_parts(psk, weapon)
    if sum(len(v[1]) for v in parts.values()) != 44988:
        raise ValueError('source triangle count mismatch')
    clip_triangles = sum(len(v[1]) // 3 for k, v in parts.items() if k[0] in subtree)
    if clip_triangles != 410:
        raise ValueError('rear magazine source topology mismatch')
    psa_names, sequences, keys = read_psa(psa_path.read_bytes())
    reloads = [s for s in sequences if s['name'] == 'Reload']
    if len(reloads) != 1 or reloads[0]['frames'] != 54 or reloads[0]['rate'] != 24:
        raise ValueError('unexpected Seeker reload')
    seq = reloads[0]
    source = keys[seq['first']:seq['first'] + 54]
    output.mkdir(parents=True, exist_ok=True)
    report = {'psk_sha256': digest(psk_path), 'psa_sha256': digest(psa_path),
              'source_bones': len(names), 'frames': 54, 'fps': 24,
              'root_bone': names[ROOT], 'root_policy': 'static; controller drives weapon',
              'magazine_bone': names[CLIP], 'magazine_subtree': [names[i] for i in sorted(subtree)],
              'magazine_triangles': clip_triangles, 'units_per_metre': UNITS,
              'coordinates': 'ActorX raw PSK; do not reflect native cooked vertices twice',
              'bind_socket': (np.diag([1.,-1.,1.,1.]) @ psk[3][CLIP] @
                              np.diag([1.,-1.,1.,1.])).tolist(),
              'parts': {}, 'outputs': {}}
    for detached in (False, True):
        reflect = np.diag([1., -1., 1., 1.])
        nodes = []
        for i, name in enumerate(names):
            mat = reflect @ local[i] @ reflect
            nodes.append({'name': name, 'translation': (mat[3, :3] / UNITS).tolist(),
                          'rotation': quaternion(mat).tolist(), 'extras': {'sourceBoneIndex': i}})
        for i in range(1, len(names)):
            nodes[parents[i]].setdefault('children', []).append(i)
        if detached:
            # A tracked hand owns this standalone object. Preserve descendants
            # but make the source clip pivot its local, origin-centered root.
            nodes[CLIP]['translation'] = [0., 0., 0.]
            nodes[CLIP]['rotation'] = [0., 0., 0., 1.]
        buf = Buffer()
        meshes = []
        chosen = {k: v for k, v in parts.items() if not detached or k[0] in subtree}
        for (bone, material), (rows, indices) in chosen.items():
            if len(rows) > 65535:
                raise ValueError('16-bit index budget exceeded')
            pos = buf.add(rows[:, :3].astype('<f4'), 'POSITION', 34962)
            uv = buf.add(rows[:, 3:5].astype('<f4'), 'TEXCOORD_0', 34962)
            idx = buf.add(indices.astype('<u2'), 'indices', 34963)
            label = ('seeker_magazine' if bone == CLIP else f'seeker_{names[bone].lower()}') + f'_mat{material}'
            mesh_index = len(meshes)
            meshes.append({'name': label, 'primitives': [{'attributes':
                {'POSITION': pos, 'TEXCOORD_0': uv}, 'indices': idx, 'mode': 4,
                'material': material}]})
            node_index = len(nodes)
            nodes.append({'name': label, 'mesh': mesh_index,
                          'extras': {'sourceBone': names[bone], 'sourceMaterial': material,
                                     'detachable': bone in subtree}})
            nodes[bone].setdefault('children', []).append(node_index)
            report['parts'][label] = {'sourceBone': names[bone], 'triangles': len(indices)//3}
        animations = []
        if not detached:
            # Original source mechanical descendants only. No tracked-hand animation.
            channels, samplers = [], []
            times = buf.add((np.arange(54, dtype='<f4') / 24), 'times')
            for bone in sorted(weapon - {ROOT}):
                if names[bone] not in psa_names:
                    raise ValueError('missing source PSA bone')
                track = source[:, psa_names.index(names[bone]), :7]
                matrices = [key_local(k) for k in track]
                positions = np.array([m[3, :3] / UNITS for m in matrices], dtype='<f4')
                rotations = np.array([quaternion(m) for m in matrices], dtype='<f4')
                for i in range(1, len(rotations)):
                    if np.dot(rotations[i-1], rotations[i]) < 0:
                        rotations[i] *= -1
                for path, values in (('translation', positions), ('rotation', rotations)):
                    value = buf.add(values, path)
                    channels.append({'sampler': len(samplers), 'target': {'node': bone, 'path': path}})
                    samplers.append({'input': times, 'output': value, 'interpolation': 'LINEAR'})
            animations.append({'name': 'source_Reload_reference', 'channels': channels,
                               'samplers': samplers, 'extras': {'sourceFrames': [0, 53],
                               'sourceFPS': 24, 'nativeAmmoTransaction': False}})
        doc = {'asset': {'version': '2.0', 'generator': 'build_seeker_source_glb.py'},
               'scene': 0, 'scenes': [{'nodes': [CLIP if detached else 0]}], 'nodes': nodes, 'meshes': meshes,
               'materials': [{'name': 'e99_steerable_body'}, {'name': 'e99_steerable_lens'}],
               'animations': animations, 'extras': {'sourcePSK': report['psk_sha256'],
               'sourcePSA': report['psa_sha256'], 'textureBinding':
               'embedded 4K PBR' if textures else 'external maps in private package'}}
        if textures:
            add_pbr(doc, buf, textures)
        name = (('seeker_hd_rear_magazine.glb' if detached else 'seeker_hd_animated.glb') if textures else
                ('seeker_rear_magazine.glb' if detached else 'seeker_source_animated.glb'))
        report['outputs'][name] = encode(doc, buf, output / name)
    (output / ('hd_rig_manifest.json' if textures else 'source_rig_manifest.json')).write_text(json.dumps(report, indent=2) + '\n')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for arg in ('psk', 'psa', 'output'):
        parser.add_argument('--' + arg, required=True, type=Path)
    parser.add_argument('--textures', type=Path)
    args = parser.parse_args()
    print(json.dumps(build(args.psk, args.psa, args.output, args.textures), indent=2))
