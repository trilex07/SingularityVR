"""Lossless source/candidate accessor and texture verification."""
import argparse,hashlib,json
from pathlib import Path
from audit_hd_weapon_packages import glb
from build_shared_texture_gltf import PAIRS

def sha(b):return hashlib.sha256(b).hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument('--private-root',type=Path,required=True)
    p.add_argument('--candidate',type=Path,required=True);a=p.parse_args();rows=[]
    for weapon,files in PAIRS.items():
        for filename in files:
            original,ob=glb(a.private_root/weapon/'final_handoff'/filename)
            path=a.candidate/weapon/(Path(filename).stem+'.gltf')
            candidate=json.loads(path.read_text(encoding='utf-8'))
            cb=(path.parent/candidate['buffers'][0]['uri']).read_bytes()
            for key in ('meshes','nodes','materials','textures','animations','scenes','scene','skins'):
                if original.get(key)!=candidate.get(key):raise ValueError(f'{weapon}/{filename}: {key} changed')
            if len(original['images'])!=len(candidate['images']):raise ValueError('image count')
            for i,im in enumerate(original['images']):
                ov=original['bufferViews'][im['bufferView']]
                payload=ob[ov.get('byteOffset',0):ov.get('byteOffset',0)+ov['byteLength']]
                if payload!=(path.parent/candidate['images'][i]['uri']).read_bytes():
                    raise ValueError(f'image mismatch {weapon}/{filename}/{i}')
            original_image_views={im['bufferView'] for im in original['images'] if 'bufferView' in im}
            remap={old:new for new,old in enumerate(i for i in range(len(original['bufferViews']))
                                              if i not in original_image_views)}
            accessors=json.loads(json.dumps(original.get('accessors',[])))
            for acc in accessors:
                if 'bufferView' in acc:acc['bufferView']=remap[acc['bufferView']]
                for row in acc.get('sparse',{}).values():
                    if isinstance(row,dict) and 'bufferView' in row:row['bufferView']=remap[row['bufferView']]
            if accessors!=candidate.get('accessors',[]):raise ValueError(f'{weapon}/{filename}: accessors changed')
            ov=[v for i,v in enumerate(original['bufferViews']) if i not in original_image_views]
            cv=candidate['bufferViews']
            if len(ov)!=len(cv):raise ValueError('bufferView count')
            for i,(v,w) in enumerate(zip(ov,cv)):
                before=ob[v.get('byteOffset',0):v.get('byteOffset',0)+v['byteLength']]
                after=cb[w.get('byteOffset',0):w.get('byteOffset',0)+w['byteLength']]
                if before!=after:raise ValueError(f'bufferView mismatch {weapon}/{filename}/{i}')
            rows.append({'weapon':weapon,'source':filename,'accessors':len(original.get('accessors',[])),
                         'images':len(original['images']),'mesh_count':len(original['meshes']),
                         'animation_count':len(original.get('animations',[])),
                         'all_non_image_buffer_views_byte_identical':True,'all_images_byte_identical':True})
    out=a.candidate/'lossless_validation.json';out.write_text(json.dumps({'status':'PASS','files':rows},indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'status':'PASS','files':len(rows),'output':str(out)}))
if __name__=='__main__':main()
