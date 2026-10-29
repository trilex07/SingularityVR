"""Build a source-pose Volk transform rig and modular glTF animations.

Requires original PSK plus the validated SVWA cache; no Blender installation.
The glTF nodes are an animatable transform hierarchy. A Blender import retains
these as named empties and separate actions; the mesh is the original clip only.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

from prepare_centurion import extract, read_psk
from prepare_weapon_animation import key_local

FPS = 36.0
UNITS = 52.5
PALETTE = (55, 56, 53, 54, 57)
TRACKS = ((54, 3), (55, 0), (56, 1), (57, 4))
PARTS = ((53, 'volk_body'), (54, 'volk_reload_part_01'),
         (55, 'volk_magazine'), (56, 'volk_slide'), (57, 'volk_trigger'))
SEGMENTS = {
    'mag_release_state': (54, 0, 0),  # source release is static: state marker
    'mag_remove_reference': (55, 21, 27),
    'mag_insert_reference': (55, 39, 43),
    'mag_seat': (55, 68, 74),
    'slide_move': (56, 19, 24),
    'slide_return': (56, 74, 76),
}


def quaternion(m):
    """Column-vector rotation matrix to glTF xyzw, with deterministic sign."""
    r = m[:3, :3].T
    if np.linalg.det(r) < .999 or np.linalg.det(r) > 1.001:
        raise ValueError('nonrigid source rotation')
    q = np.empty(4, dtype=np.float64)
    trace = np.trace(r)
    if trace > 0:
        s = np.sqrt(trace + 1) * 2
        q[:] = [(r[2, 1]-r[1, 2])/s, (r[0, 2]-r[2, 0])/s,
                (r[1, 0]-r[0, 1])/s, s/4]
    else:
        i = int(np.argmax(np.diag(r)))
        if i == 0:
            s = np.sqrt(1+r[0, 0]-r[1, 1]-r[2, 2])*2
            q[:] = [s/4, (r[0, 1]+r[1, 0])/s, (r[0, 2]+r[2, 0])/s, (r[2, 1]-r[1, 2])/s]
        elif i == 1:
            s = np.sqrt(1+r[1, 1]-r[0, 0]-r[2, 2])*2
            q[:] = [(r[0, 1]+r[1, 0])/s, s/4, (r[1, 2]+r[2, 1])/s, (r[0, 2]-r[2, 0])/s]
        else:
            s = np.sqrt(1+r[2, 2]-r[0, 0]-r[1, 1])*2
            q[:] = [(r[0, 2]+r[2, 0])/s, (r[1, 2]+r[2, 1])/s, s/4, (r[1, 0]-r[0, 1])/s]
    q /= np.linalg.norm(q)
    return q


class Buffer:
    def __init__(self):
        self.data = bytearray()
        self.views = []
        self.accessors = []

    def add(self, array, kind, target=None):
        a = np.ascontiguousarray(array)
        while len(self.data) % 4:
            self.data.append(0)
        offset = len(self.data)
        self.data.extend(a.tobytes())
        view = {'buffer': 0, 'byteOffset': offset, 'byteLength': a.nbytes}
        if target:
            view['target'] = target
        self.views.append(view)
        width = 1 if a.ndim == 1 else a.shape[1]
        accessor = {'bufferView': len(self.views)-1, 'componentType':
                    {np.dtype('<f4'): 5126, np.dtype('<u2'): 5123}[a.dtype],
                    'count': len(a), 'type': {1:'SCALAR', 2:'VEC2', 3:'VEC3', 4:'VEC4'}[width]}
        if kind in ('POSITION', 'times'):
            accessor['min'] = [float(a.min())] if a.ndim == 1 else a.min(axis=0).tolist()
            accessor['max'] = [float(a.max())] if a.ndim == 1 else a.max(axis=0).tolist()
        self.accessors.append(accessor)
        return len(self.accessors)-1


def load_cache(path):
    data = path.read_bytes()
    h = struct.unpack_from('<4s10If', data)
    if h != (b'SVWA', 1, 2, 8297, 0, 10835, 754008012, 5, 2, 4, 108, FPS):
        raise ValueError('unreviewed SVWA header')
    offset = 48 + 64
    descriptors = []
    for _ in range(4):
        slot, parent = struct.unpack_from('<Ii', data, offset)
        descriptors.append((slot, parent))
        offset += 72
    if descriptors != [(3, -1), (0, -1), (1, -1), (4, -1)] or len(data) != offset + 108*4*7*4:
        raise ValueError('unreviewed track layout')
    keys = np.frombuffer(data, '<f4', 108*4*7, offset).reshape(108, 4, 7)
    if not np.isfinite(keys).all():
        raise ValueError('nonfinite key')
    return data, keys


def preview_magazine(rows, faces, keys, output):
    """Common-scale source-track wireframe, never a controller-space preview."""
    chosen = (21, 27, 39, 43, 68, 74)
    positions = []
    for frame in chosen:
        matrix = key_local(keys[frame, 1])
        xyz = np.c_[rows[:, :3]*UNITS, np.ones(len(rows))] @ matrix
        positions.append(xyz[:, :3]/UNITS)
    xy_all = np.concatenate([xyz[:, (0, 1)] for xyz in positions])
    center = (xy_all.min(0)+xy_all.max(0))*.5
    scale = 200/max(np.ptp(xy_all,axis=0).max(), .001)
    image = Image.new('RGB',(1440,640),'#10171b')
    draw = ImageDraw.Draw(image)
    for panel,(frame,xyz) in enumerate(zip(chosen,positions)):
        x0,y0 = 240+480*(panel%3), 160+320*(panel//3)
        xy = (xyz[:, (0,1)]-center)*[scale,-scale]+[x0,y0]
        for face in faces.reshape(-1,3):
            polygon=[tuple(xy[int(i)]) for i in face]
            draw.line(polygon+[polygon[0]],fill='#64aeb8',width=1)
        draw.text((x0-200,y0-140),f'clip source frame {frame} / 36 Hz',fill='white')
    draw.text((20,620),'Original clip local track in parent space; fixed view and scale; no hand/socket calibration',fill='white')
    image.save(output)


def rigid_parts(psk):
    """Split source triangles by their sole PSK bone; preserve UV and bind."""
    names, _, _, absolute, points, weights, wedges, faces = psk
    reflect = np.diag([1.,-1.,1.,1.])
    inverse = {b: np.linalg.inv(reflect @ absolute[b] @ reflect)
               for b,_ in PARTS}
    remaps = {b:{} for b,_ in PARTS}
    vertices = {b:[] for b,_ in PARTS}
    indices = {b:[] for b,_ in PARTS}
    for face in faces:
        bones = set()
        for wedge in face[:3]:
            point = wedges[wedge][0]
            rows = weights[point]
            if len(rows) != 1 or abs(rows[0][0]-1) > 1e-6:
                raise ValueError('source point is not rigid')
            bones.add(rows[0][1])
        if len(bones) != 1:
            raise ValueError('triangle crosses source-bone boundary')
        bone = bones.pop()
        if bone not in vertices or face[3] != 0:
            raise ValueError('unreviewed source part/material')
        for wedge in face[:3]:
            point,u,v = wedges[wedge]
            key = (point,u,v)
            if key not in remaps[bone]:
                xyz = (np.r_[points[point],1.] @ inverse[bone])[:3]/UNITS
                remaps[bone][key] = len(vertices[bone])
                vertices[bone].append((*xyz,u,v))
            indices[bone].append(remaps[bone][key])
    if sum(len(indices[b]) for b,_ in PARTS) != len(faces)*3:
        raise ValueError('source faces omitted')
    return {b:(np.array(vertices[b],dtype=np.float32),
               np.array(indices[b],dtype=np.uint16)) for b,_ in PARTS}


def build(psk_path, svwa_path, output):
    psk = read_psk(psk_path)
    names, parents, local = psk[:3]
    if len(names) != 79 or len(psk[6]) != 8297 or [names[i] for i in PALETTE] != [
        'clip', 'Slide', 'cog_shotgun', 'release', 'Trigger']:
        raise ValueError('unexpected Volk skeleton')
    cache, keys = load_cache(svwa_path)
    reflect = np.diag([1., -1., 1., 1.])
    nodes = []
    for i, name in enumerate(names):
        raw = reflect @ local[i] @ reflect
        node = {'name': name, 'translation': (raw[3, :3]/UNITS).tolist(),
                'rotation': quaternion(raw).tolist(), 'extras': {'pskBoneIndex': i}}
        nodes.append(node)
    for i in range(1, len(names)):
        nodes[parents[i]].setdefault('children', []).append(i)
    clip_cache, clip_report = extract(psk)
    parts = rigid_parts(psk)
    rows, faces = parts[55]
    count, index_count = len(rows), len(faces)
    if (count,index_count) != (242,594):
        raise ValueError('clip split disagrees with source extractor')
    buf = Buffer()
    meshes = []
    for bone,label in PARTS:
        part, triangles = parts[bone]
        pos = buf.add(part[:, :3].astype('<f4'), 'POSITION', 34962)
        uv = buf.add(part[:, 3:5].astype('<f4'), 'TEXCOORD_0', 34962)
        indices = buf.add(triangles.astype('<u2'), 'indices', 34963)
        meshes.append({'name':label, 'primitives':[{'attributes':
            {'POSITION':pos,'TEXCOORD_0':uv},'indices':indices,'mode':4}]})
        node_index = len(nodes)
        nodes.append({'name':label, 'mesh':len(meshes)-1,
                      'extras':{'parentSourceBone':names[bone],
                                'detachable':bone==55}})
        nodes[bone].setdefault('children', []).append(node_index)
    animations = []
    for title, (bone, first, last) in SEGMENTS.items():
        track = [b for b, _ in TRACKS].index(bone)
        sample = keys[first:last+1, track]
        times = (np.arange(len(sample), dtype=np.float32)/FPS).astype('<f4')
        matrices = [key_local(k) for k in sample]
        positions = np.array([m[3, :3]/UNITS for m in matrices], dtype='<f4')
        rotations = np.array([quaternion(m) for m in matrices], dtype='<f4')
        for i in range(1, len(rotations)):
            if np.dot(rotations[i-1], rotations[i]) < 0:
                rotations[i] *= -1
        ti = buf.add(times, 'times')
        pi = buf.add(positions, 'translation')
        qi = buf.add(rotations, 'rotation')
        animations.append({'name': title, 'samplers': [
            {'input': ti, 'output': pi, 'interpolation': 'LINEAR'},
            {'input': ti, 'output': qi, 'interpolation': 'LINEAR'}],
            'channels': [{'sampler': 0, 'target': {'node': bone, 'path': 'translation'}},
                         {'sampler': 1, 'target': {'node': bone, 'path': 'rotation'}}],
            'extras': {'sourceFramesInclusive': [first, last],
                       'sourceBone': names[bone], 'fps': FPS,
                       'referenceOnly': title in ('mag_remove_reference','mag_insert_reference')}})
    doc = {'asset': {'version':'2.0', 'generator':'build_volk_mechanical_glb.py'},
           'scene':0, 'scenes':[{'nodes':[0]}], 'nodes':nodes,
           'meshes':meshes,
           'animations':animations, 'buffers':[{'byteLength':len(buf.data)}],
           'bufferViews':buf.views, 'accessors':buf.accessors,
           'extras': {'coordinateSpace':'original PSK raw space; metres at 52.5 UU/m',
                      'rootPolicy':'all ancestor nodes static; controller drives root',
                      'sourcePSKSha256':hashlib.sha256(psk_path.read_bytes()).hexdigest(),
                      'sourceSVWASha256':hashlib.sha256(cache).hexdigest()}}
    js = json.dumps(doc, separators=(',',':')).encode()
    js += b' ' * ((-len(js)) % 4)
    binary = bytes(buf.data) + b'\0' * ((-len(buf.data)) % 4)
    glb = (struct.pack('<4sII',b'glTF',2,12+8+len(js)+8+len(binary))+
           struct.pack('<I4s',len(js),b'JSON')+js+
           struct.pack('<I4s',len(binary),b'BIN\0')+binary)
    output.mkdir(parents=True, exist_ok=True)
    path = output/'volk_mechanical_rig.glb'
    path.write_bytes(glb)
    preview_magazine(rows, faces, keys, output/'volk_mechanical_preview.png')
    report = {'source_psk_sha256':doc['extras']['sourcePSKSha256'],
              'source_svwa_sha256':doc['extras']['sourceSVWASha256'],
              'glb_sha256':hashlib.sha256(glb).hexdigest(), 'bones':len(names),
              'magazine_vertices':count, 'magazine_triangles':index_count//3,
              'parts': {label:{'bone':names[bone], 'vertices':len(parts[bone][0]),
                               'triangles':len(parts[bone][1])//3}
                        for bone,label in PARTS},
              'source_magazine':clip_report, 'animations':[
                  {'name':a['name'], **a['extras']} for a in animations],
              'source_bones': {names[i]: {'index':i, 'parent':names[parents[i]] if i else None,
                           'localTranslationMetres':nodes[i]['translation'],
                           'localRotationXYZW':nodes[i]['rotation']}
                           for i in (0,52,53,54,55,56,57,58)},
              'socket': {'parent':'cog_shotgun','sourceBone':'clip',
                         'transform':'original PSK clip bind; no extra offset',
                         'insertionAxis':'unmeasured; derive from HD geometry and physical fit'},
              'runtime': {'magRelease':'state only; source release track is static',
                          'magazine':'detach mesh and follow tracked hand; do not play reference translation',
                          'seat':'source frames 68-74 end 0.56 UU off PSK clip bind; resolve pose handoff deliberately',
                          'root':'static in all exported actions'}}
    (output/'animation_socket_manifest.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    return report


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    for key in ('psk','svwa','output'):
        ap.add_argument('--'+key,required=True,type=Path)
    args = ap.parse_args()
    print(json.dumps(build(args.psk,args.svwa,args.output),indent=2))
