"""Gate and hash private Kasimov outputs; no native runtime modifications."""
import json,struct,hashlib,sys
from pathlib import Path
import numpy as np
root=Path(sys.argv[1]);source=json.loads((root/'source_data.json').read_text())
report=json.loads((root/'blender_validation.json').read_text());rt=json.loads((root/'roundtrip_validation.json').read_text())
assert report['max_cpu_blender_error_m']<.0001 and rt['max_roundtrip_error_m']<.0001
def read(path):
 d=path.read_bytes();n=struct.unpack_from('<I',d,12)[0];return json.loads(d[20:20+n]),d[28+n:]
j,blob=read(root/'kasimov_hd_animated.glb');assert len(j['skins'])==1 and len(j['skins'][0]['joints'])==80
def arr(i):
 a=j['accessors'][i];v=j['bufferViews'][a['bufferView']];width={'SCALAR':1,'VEC3':3,'VEC4':4,'MAT4':16}[a['type']]
 return np.frombuffer(blob,{5126:'<f4',5123:'<u2',5121:'u1',5125:'<u4'}[a['componentType']],a['count']*width,v.get('byteOffset',0)+a.get('byteOffset',0)).reshape(-1,width)
assert len(arr(j['skins'][0]['inverseBindMatrices']))==80
weighted=0
for mesh in j['meshes']:
 for p in mesh['primitives']:
  a=p['attributes']
  if 'JOINTS_0' in a:
   w=arr(a['WEIGHTS_0']);assert np.max(abs(w.sum(1)-1))<1e-5;weighted+=len(w)
  if 'TANGENT' in a:assert np.isin(arr(a['TANGENT'])[:,3],[-1,1]).all()
mag,_=read(root/'kasimov_hd_magazine.glb');assert not mag.get('skins') and not mag.get('animations')
skin={'source_hashes':source['source_hashes'],'bone_names':source['names'],'parents':source['parents'],
 'original_bind_row_matrices_uu':source['bind'],'dual_weighted_points':source['dual_points'],
 'weights':source['weights'],'wedge_to_point':[w[0] for w in source['wedges']],
 'coordinate_units_per_metre':52.5,'blender_bone_axes':'Blender rest axes are adapted; source deformation and inverse bind contract verified numerically'}
(root/'skin_manifest.json').write_text(json.dumps(skin,indent=2))
(root/'animation_socket_manifest.json').write_text(json.dumps({'source_frames':239,'fps':60,'wire_hierarchy':['wire_1','wire_2','wire_3'],'root':'cog_sniper static','magazine_socket':'original clip bind; no added offset','actions':report['actions'],'manual_bolt_required':False,'reference_slices':'original source keys; runtime hand ownership overrides magazine reference playback'},indent=2))
manifest={'status':'OFFLINE_BLENDER_PASS','cpu_blender_max_error_m':report['max_cpu_blender_error_m'],
 'roundtrip_max_error_m':rt['max_roundtrip_error_m'],'source_dual_points':143,'export_weighted_vertices':weighted,
 'source_faces':13110,'scope_lens_faces':168,'bone_count':80,
 'native_material_limit':'Offline PBR lens reconstruction; not a proven match to the native UE3 shader. PBR maps require MAIN integration.',
 'hashes':{p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in root.rglob('*') if p.is_file() and p.suffix not in ('.blend1',) and p.name!='sha256_manifest.json'}}
(root/'sha256_manifest.json').write_text(json.dumps(manifest,indent=2));print(manifest['status'])
