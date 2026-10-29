"""Create lossless glTF sidecars sharing identical embedded weapon images.

Original private GLBs remain untouched. All geometry, material, node and
animation arrays are preserved; only embedded image storage is externalized.
"""
import argparse
import copy
import hashlib
import json
import struct
from pathlib import Path
from audit_hd_weapon_packages import glb

PAIRS={
    'volk_s4':('volk_s4_hd_animated.glb','volk_s4_hd_magazine.glb'),
    'valkyrie_ar9':('valkyrie_ar9_hd_animated.glb','valkyrie_ar9_hd_magazine.glb'),
    'kasimov_snv_e99':('kasimov_hd_animated.glb','kasimov_hd_magazine.glb'),
    'centurion':('centurion_hd_animated.glb','centurion_hd_cylinder.glb')}

def sha(data):return hashlib.sha256(data).hexdigest()

def externalize(src,out,shared):
    original,bin_data=glb(src);doc=copy.deepcopy(original)
    if len(doc.get('buffers',[]))!=1:raise ValueError('expected one buffer')
    views=doc.get('bufferViews',[]);image_views={}
    for im in doc.get('images',[]):
        if 'bufferView' not in im:continue
        i=im['bufferView'];v=views[i];start=v.get('byteOffset',0)
        payload=bin_data[start:start+v['byteLength']]
        if len(payload)!=v['byteLength']:raise ValueError('truncated image')
        kind=im.get('mimeType')
        ext={ 'image/png':'.png','image/jpeg':'.jpg','image/ktx2':'.ktx2'}.get(kind)
        if ext is None:raise ValueError('unsupported image '+str(kind))
        name=sha(payload)+ext;target=shared/name
        if target.exists() and target.read_bytes()!=payload:raise ValueError('hash collision')
        if not target.exists():target.write_bytes(payload)
        image_views[i]=name;im.pop('bufferView');im.pop('mimeType',None)
        im['uri']='shared_textures/'+name
    # Image-only views are omitted. Preserve every other bufferView exactly,
    # apart from its offset in the compacted binary.
    used_by_accessor=set()
    for a in doc.get('accessors',[]):
        if 'bufferView' in a:used_by_accessor.add(a['bufferView'])
        for row in a.get('sparse',{}).values():
            if isinstance(row,dict) and 'bufferView' in row:used_by_accessor.add(row['bufferView'])
    if set(image_views)&used_by_accessor:raise ValueError('image view is used by accessor')
    compact=bytearray();mapping={};new_views=[]
    for i,v in enumerate(views):
        if i in image_views:continue
        compact.extend(b'\0'*((-len(compact))%4));nv=copy.deepcopy(v)
        start=v.get('byteOffset',0);end=start+v['byteLength']
        if end>len(bin_data):raise ValueError('truncated bufferView')
        nv['byteOffset']=len(compact);compact.extend(bin_data[start:end])
        mapping[i]=len(new_views);new_views.append(nv)
    doc['bufferViews']=new_views
    def remap(value):
        if isinstance(value,list):
            for item in value:remap(item)
        elif isinstance(value,dict):
            for k,v in value.items():
                if k=='bufferView':value[k]=mapping[v]
                else:remap(v)
    remap(doc)
    bin_name=src.stem+'.bin';doc['buffers'][0]['uri']=bin_name
    doc['buffers'][0]['byteLength']=len(compact)
    out.mkdir(parents=True,exist_ok=True)
    (out/bin_name).write_bytes(compact)
    (out/(src.stem+'.gltf')).write_text(json.dumps(doc,separators=(',',':'))+'\n',encoding='utf-8')
    return {'source':src.name,'source_bytes':src.stat().st_size,'gltf':src.stem+'.gltf',
            'binary_bytes':len(compact),'images':[{'sha256':name.split('.')[0],'file':name}
            for name in image_views.values()],
            'source_counts':{k:len(original.get(k,[])) for k in ('meshes','nodes','materials','animations','accessors')},
            'candidate_counts':{k:len(doc.get(k,[])) for k in ('meshes','nodes','materials','animations','accessors')}}

def main():
    p=argparse.ArgumentParser();p.add_argument('--private-root',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    report={}
    for name,files in PAIRS.items():
        src=a.private_root/name/'final_handoff';dst=a.output/name
        shared=dst/'shared_textures';shared.mkdir(parents=True,exist_ok=True)
        if not all((src/f).exists() for f in files):raise FileNotFoundError(name)
        entries=[externalize(src/f,dst,shared) for f in files]
        original=sum(e['source_bytes'] for e in entries)
        candidate=sum(p.stat().st_size for p in dst.rglob('*') if p.is_file()
                      and p.suffix.lower() in ('.gltf','.bin','.png','.jpg','.ktx2'))
        report[name]={'entries':entries,'source_pair_bytes':original,
                      'candidate_bytes':candidate,'saved_bytes':original-candidate,
                      'shared_images':len(list(shared.iterdir()))}
    (a.output/'shared_texture_report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:{'saved_bytes':v['saved_bytes'],'shared_images':v['shared_images']}
                      for k,v in report.items()},indent=2))
if __name__=='__main__':main()
