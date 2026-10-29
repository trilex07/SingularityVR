"""Validate generated real geometry against all six hash-pinned source OBJs."""
import json, struct, sys
from pathlib import Path
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'assets'))
from prepare_volk_runtime import read_glb, sha, read_dds_header
from assemble_volk_final import parse_obj
root=Path(__file__).resolve().parents[2]
package=Path(sys.argv[1]); runtime=Path(sys.argv[2])
report=json.loads((runtime/'runtime_manifest.json').read_text())
assembly=json.loads((package/'assembly_manifest.json').read_text())
blob=(runtime/'volk_hd.svvh').read_bytes()
magic,version,vcount,icount,source,ihash,phash=struct.unpack_from('<4s6I',blob)
assert (magic,version,vcount,icount,source)==(b'SVVH',1,12085,38181,8297)
vertices=np.ndarray((vcount,3),'<f4',blob,28,(32,4))
indices=np.ndarray((icount//3,3),'<u2',blob,28+vcount*32)
doc,access=read_glb(package/'volk_s4_hd_animated.glb')
base=8297;first=10835;worst=0
for mesh in doc['meshes']:
    name=mesh['name']
    if name not in assembly['details']:continue
    obj=package.parent/'clean'/(name+'.obj')
    assert sha(obj)==assembly['details'][name]['source_sha256']
    pos,faces=parse_obj(obj)
    # Blender splits normals; compare each emitted face's actual world vertices,
    # preserving winding, instead of assuming a shared OBJ vertex order.
    n=assembly['details'][name]['triangles']
    drawn=vertices[indices[first:first+n]][:,[1,0,2]]
    expected=pos[faces]*[1,-1,1]
    # Exporter may cyclically rotate a face, but must not reverse it.
    error=min(float(np.max(np.abs(drawn-np.roll(expected,k,axis=1)))) for k in range(3))
    assert error<1e-4,(name,error)
    worst=max(worst,error)
    local=access(mesh['primitives'][0]['attributes']['POSITION'])
    weights=np.ndarray((len(local),4),'u1',blob,28+base*32+24,(32,1))
    assert np.all(weights==[255,0,0,0])
    base+=len(local);first+=n
assert (base,first)==(vcount,icount//3)
for file,hashvalue in report['files_sha256'].items():assert sha(runtime/file)==hashvalue
for file in ('AR_shotgun_d.dds','AR_shotgun_n.dds'):
    data=(runtime/file).read_bytes()
    assert struct.unpack_from('<II',data,12)==(4096,4096)
    assert struct.unpack_from('<I',data,28)[0]==13
print(f'PASS: all 1892 added triangles match six pinned source OBJs; max error {worst:.9g} UU; rigid weights, winding, 4K full mip DDS, hashes')
