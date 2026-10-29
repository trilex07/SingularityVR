"""Read-only GLB and texture inventory for private HD weapon handoffs.

No game assets are copied into Git. Reports contain counts, sizes and hashes only.
"""
import argparse
import hashlib
import json
import struct
from io import BytesIO
from collections import defaultdict
from pathlib import Path
from PIL import Image

WEAPONS = ('volk_s4', 'valkyrie_ar9', 'kasimov_snv_e99', 'centurion')

def digest(data): return hashlib.sha256(data).hexdigest()

def glb(path):
    raw = path.read_bytes()
    if len(raw) < 20 or raw[:4] != b'glTF' or struct.unpack_from('<I',raw,4)[0] != 2:
        raise ValueError(f'not glTF 2 GLB: {path}')
    if struct.unpack_from('<I',raw,8)[0] != len(raw): raise ValueError(f'length mismatch: {path}')
    at=12; chunks=[]
    while at < len(raw):
        length,kind=struct.unpack_from('<I4s',raw,at);at+=8
        chunks.append((kind,raw[at:at+length]));at+=length
    doc=json.loads(next(data for kind,data in chunks if kind==b'JSON'))
    binary=next((data for kind,data in chunks if kind==b'BIN\0'),b'')
    return doc,binary

def accessor_triangles(doc,primitive):
    if primitive.get('mode',4)!=4:return 0
    acc=doc['accessors'][primitive['indices']] if 'indices' in primitive else doc['accessors'][primitive['attributes']['POSITION']]
    return acc['count']//3

def inspect_glb(path):
    doc,binary=glb(path); views=doc.get('bufferViews',[]);images=[]
    for i,entry in enumerate(doc.get('images',[])):
        if 'bufferView' in entry:
            view=views[entry['bufferView']];offset=view.get('byteOffset',0)
            payload=binary[offset:offset+view['byteLength']]
            with Image.open(BytesIO(payload)) as im:width,height=im.size
            images.append({'index':i,'bytes':len(payload),'sha256':digest(payload),
                           'mime':entry.get('mimeType'),'width':width,'height':height,
                           'rgba8_mip_chain_estimate':int(width*height*4*4/3)})
        else:images.append({'index':i,'uri':entry.get('uri')})
    meshes=doc.get('meshes',[]);primitives=[p for m in meshes for p in m.get('primitives',[])]
    animated_nodes={c['target']['node'] for a in doc.get('animations',[]) for c in a['channels'] if 'node' in c.get('target',{})}
    return {'file':path.name,'bytes':path.stat().st_size,'sha256':digest(path.read_bytes()),
            'meshes':len(meshes),'primitives':len(primitives),
            'triangles':sum(accessor_triangles(doc,p) for p in primitives),
            'nodes':len(doc.get('nodes',[])),'animations':len(doc.get('animations',[])),
            'animated_nodes':len(animated_nodes),'materials':len(doc.get('materials',[])),
            'textures':len(doc.get('textures',[])),'images':images,
            'embedded_image_bytes':sum(x.get('bytes',0) for x in images),
            'unused_materials':sorted(set(range(len(doc.get('materials',[]))))-
                                      {p['material'] for p in primitives if 'material' in p}),
            'uncompressed_geometry_bytes':sum(v['byteLength'] for v in views if v.get('target') in (34962,34963))}

def inspect_package(root):
    out={'root':str(root),'exists':root.is_dir()}
    if not root.is_dir():return out
    glbs=[inspect_glb(p) for p in sorted(root.glob('*.glb'))]
    maps=[]
    for p in sorted(root.rglob('*')):
        if p.is_file() and p.suffix.lower() in ('.png','.dds','.ktx2','.jpg','.jpeg'):
            if any(part.lower() in ('maps','textures') for part in p.parts):
                maps.append({'path':str(p.relative_to(root)).replace('\\','/'),'bytes':p.stat().st_size,
                             'sha256':digest(p.read_bytes())})
    images=defaultdict(list)
    for g in glbs:
        for im in g['images']:
            if 'sha256' in im:images[im['sha256']].append(g['file'])
    out.update({'glbs':glbs,'maps':maps,'duplicate_embedded_images':
                {h:files for h,files in images.items() if len(files)>1},
                'glb_bytes':sum(g['bytes'] for g in glbs),
                'map_bytes':sum(m['bytes'] for m in maps)})
    return out

def main():
    p=argparse.ArgumentParser();p.add_argument('--private-root',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    report={'schema':1,'packages':{name:inspect_package(a.private_root/name/'final_handoff') for name in WEAPONS}}
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'output':str(a.output),'packages':{k:{'glb_bytes':v.get('glb_bytes'),
        'map_bytes':v.get('map_bytes'),'glbs':len(v.get('glbs',[]))} for k,v in report['packages'].items()}},indent=2))
if __name__=='__main__':main()
