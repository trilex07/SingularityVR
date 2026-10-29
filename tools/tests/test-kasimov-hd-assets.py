"""Real Kasimov weighted source, lens exclusion and GLB detail geometry tests."""
import json,struct,sys
from pathlib import Path
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'assets'))
from prepare_kasimov_runtime import read_psk,read_glb,hierarchy,native_points,sha,fnv,verify
package=Path(sys.argv[1]);runtime=Path(sys.argv[2]);verify(package)
report=json.loads((runtime/'runtime_manifest.json').read_text());blob=(runtime/'kasimov_hd.svks').read_bytes()
assert struct.unpack_from('<4s6I',blob)[:6]==(b'SVKS',1,12637,13722*3,11389,855862860)
assert fnv(blob[28:28+11389*32])==2571367080
assert fnv(blob[28+12637*32:28+12637*32+12942*6])==855862860
names,parents,_,bind,points,weights,wedges,faces=read_psk(package/'ch_inview_sniper_mesh.psk')
assert len(names)==80 and sum(len(w)==2 for w in weights.values())==143
assert names[parents[names.index('wire_3')]]=='wire_2' and names[parents[names.index('wire_2')]]=='wire_1'
vertices=np.ndarray((12637,3),'<f4',blob,28,(32,4));indices=np.ndarray((13722,3),'<u2',blob,28+12637*32)
assert np.array_equal(vertices[:11389],points[[w[0] for w in wedges]]*[1,-1,1])
assert indices[:12942].max()<11292 and indices[12942:].min()>=11389
# Original lens97 vertices remain in the source prefix but are never submitted
# through the replacement body IB; original168-face draw stays native.
assert not np.any((indices>=11292)&(indices<11389))
doc,access=read_glb(package/'kasimov_hd_animated.glb');world,ancestors=hierarchy(doc)
base=11389;first=12942;worst=0
for i,node in enumerate(doc['nodes']):
 if not node.get('name','').startswith('HD_'):continue
 detail=report['details'][node['name']];p=doc['meshes'][node['mesh']]['primitives'][0]
 expected=native_points(access(p['attributes']['POSITION']),world[i]);fs=access(p['indices']).reshape(-1,3)[:,[1,0,2]]
 error=float(np.max(np.abs(vertices[indices[first:first+len(fs)]]-expected[fs])));worst=max(worst,error);assert error<1e-4
 slots=np.ndarray((len(expected),4),'u1',blob,28+base*32+20,(32,1));blends=np.ndarray((len(expected),4),'u1',blob,28+base*32+24,(32,1))
 assert np.all(slots[:,0]==detail['slot']) and np.all(blends==[255,0,0,0])
 base+=len(expected);first+=len(fs)
assert (base,first)==(12637,13722)
for file,h in report['files_sha256'].items():assert sha(runtime/file)==h
for file in ('ar_sniper_rifle_d.dds','ar_sniper_rifle_n.dds'):
 data=(runtime/file).read_bytes();assert struct.unpack_from('<II',data,12)==(2048,4096);assert struct.unpack_from('<I',data,28)[0]==13
print(f'PASS: 780 source-derived detail faces,143 dual-weighted points byte-exact,80 bones/wire hierarchy,168-face native lens excluded; max GLB/native error {worst:.9g} UU;4K mip chains/hashes')
