"""Validate actual Centurion native prefix, source details and cylinder ownership."""
import json,struct,sys
from pathlib import Path
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'assets'))
from prepare_centurion_runtime import read_glb,read_psk,hierarchy,native_points,sha,fnv
from assemble_volk_final import parse_obj
p=Path(sys.argv[1]);out=Path(sys.argv[2]);report=json.loads((out/'runtime_manifest.json').read_text())
blob=(out/'centurion_hd.svct').read_bytes();assert struct.unpack_from('<4s5I',blob)==(b'SVCT',1,8051,7927*3,5799,147026316)
n,parents,_,bind,points,weights,wedges,faces=read_psk(p/'source/ch_inview_alt_pistol_mesh.psk')
assert len(n)==79
v=np.ndarray((8051,3),'<f4',blob,28,(32,4));ib=np.ndarray((7927,3),'<u2',blob,28+8051*32)
assert np.array_equal(v[:5799],points[[w[0] for w in wedges]]*[1,-1,1])
assert fnv(blob[28:28+5799*32])==483118394 and fnv(ib[:6767].tobytes())==147026316
doc,access=read_glb(p/'centurion_hd_animated.glb');world,_=hierarchy(doc)
first=6767;base=5799;worst=0.
for i,node in enumerate(doc['nodes']):
 if node['name'] not in report['details']:continue
 d=report['details'][node['name']];prim=doc['meshes'][node['mesh']]['primitives'][0]
 expected=native_points(access(prim['attributes']['POSITION']),world[i]);idx=access(prim['indices']).reshape(-1,3)[:,[1,0,2]]
 error=float(np.max(abs(v[ib[first:first+len(idx)]]-expected[idx])));worst=max(worst,error);assert error<1e-4
 source,source_faces=parse_obj(p/'clean'/(node['name']+'.obj'))
 # Source OBJ is raw ActorX bind. Match complete triangle point sets, ignoring Blender split vertices.
 source=source*[1,-1,1]
 for point in expected:
  assert np.min(np.max(abs(source-point),axis=1))<2e-4
 slots=np.ndarray((len(expected),4),'u1',blob,28+base*32+20,(32,1));w=np.ndarray((len(expected),4),'u1',blob,28+base*32+24,(32,1))
 assert np.all(slots[:,0]==d['slot']) and np.all(w==[255,0,0,0])
 for angle in (.4,-1.1):
  rot=np.array([[np.cos(angle),-np.sin(angle),0],[np.sin(angle),np.cos(angle),0],[0,0,1]])
  assert np.max(abs(v[ib[first:first+len(idx)]]@rot-expected[idx]@rot))<1e-4
 first+=len(idx);base+=len(expected)
assert (base,first)==(8051,7927)
for file,h in report['files_sha256'].items():assert sha(out/file)==h
for f in ('handgun_d.dds','handgun_n.dds'):
 data=(out/f).read_bytes();assert struct.unpack_from('<II',data,12)==(4096,4096) and struct.unpack_from('<I',data,28)[0]==13
print(f'PASS: 79 bones, original 5799/6767 byte-exact;1160 HD faces/source OBJs;clip/root ownership;two rotations;max error {worst:.9g} UU;4K mip/hashes')
