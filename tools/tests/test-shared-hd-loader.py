"""Exercise real SIDE sidecars and fail-closed URI/content checks; no game."""
import json,sys
from pathlib import Path
from unittest.mock import patch
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'assets'))
from prepare_volk_runtime import read_glb
from shared_hd_gltf import load_shared,resolve
root=Path(sys.argv[1]).resolve();shared=Path(sys.argv[2]).resolve()
pairs={'volk_s4':('volk_s4_hd_animated','volk_s4_hd_magazine'),
       'valkyrie_ar9':('valkyrie_ar9_hd_animated','valkyrie_ar9_hd_magazine'),
       'kasimov_snv_e99':('kasimov_hd_animated','kasimov_hd_magazine'),
       'centurion':('centurion_hd_animated','centurion_hd_cylinder')}
count=0
for weapon,files in pairs.items():
 for name in files:
  source=root/weapon/'final_handoff'/(name+'.glb')
  doc,a=read_glb(source);external,b=read_glb(source,shared)
  for mesh,other in zip(doc['meshes'],external['meshes']):
   for p,q in zip(mesh['primitives'],other['primitives']):
    assert np.array_equal(a(p['indices']),b(q['indices']))
    for attribute in ('POSITION','NORMAL','TEXCOORD_0'):
     if attribute in p['attributes']:assert np.array_equal(a(p['attributes'][attribute]),b(q['attributes'][attribute]))
  count+=1
source=root/'centurion/final_handoff/centurion_hd_animated.glb'
candidate=shared/'centurion/centurion_hd_animated.gltf';doc=json.loads(candidate.read_text())
read_text=Path.read_text;read_bytes=Path.read_bytes
for mutation in ('outside','missing','nodes','image','binary'):
 modified=json.loads(json.dumps(doc))
 if mutation=='outside':modified['buffers'][0]['uri']='../escape.bin'
 if mutation=='missing':modified['buffers'][0]['uri']='not-there.bin'
 if mutation=='nodes':modified['nodes'][0]['name']='wrong'
 bad=(candidate.parent/(doc['images'][0]['uri'] if mutation=='image' else doc['buffers'][0]['uri'])).resolve()
 def text(p,*args,**kwargs):return json.dumps(modified) if p==candidate else read_text(p,*args,**kwargs)
 def data(p):
  result=read_bytes(p)
  return bytes([result[0]^1])+result[1:] if mutation in ('image','binary') and p==bad else result
 with patch.object(Path,'read_text',text),patch.object(Path,'read_bytes',data):
  try:load_shared(source,shared)
  except (ValueError,FileNotFoundError):pass
  else:raise AssertionError('accepted '+mutation)
for uri in ('../escape','C:/escape','https://example','%2e%2e/escape','..\\escape'):
 try:resolve(candidate.parent,uri)
 except ValueError:pass
 else:raise AssertionError('accepted URI '+uri)
print(f'PASS: {count} actual animated/detached sidecars, byte-exact geometry/UV/animation/images; corrupt/missing/escaping resources fail closed')
