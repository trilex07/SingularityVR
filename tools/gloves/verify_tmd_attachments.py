"""Verify private UModel static exports against cooked package and runtime identities."""
import argparse
import re
import struct
import zlib
from pathlib import Path

p=argparse.ArgumentParser()
p.add_argument('--exports',type=Path,required=True)
p.add_argument('--cooked',type=Path,required=True)
a=p.parse_args()
header=Path(__file__).resolve().parents[2]/'spikes/view_matrix/tmd_attachment_identity.h'
expected={(int(v),int(f),int(c,16)) for v,f,c in re.findall(r'\{(\d+),(\d+),(0x[0-9a-f]+)\}',header.read_text())}
package=a.cooked.read_bytes()
seen=set()
for path in a.exports.glob('*.pskx'):
    data=path.read_bytes();offset=0;vertices=0
    while offset<len(data):
        name,_,size,count=struct.unpack_from('<20siii',data,offset);offset+=32
        assert size>=0 and count>=0 and offset+size*count<=len(data)
        if name.startswith(b'VTXW0000'):vertices=count
        if name.startswith(b'FACE0000'):
            faces=[struct.unpack_from('<3H',data,offset+i*size) for i in range(count)]
            raw=b''.join(struct.pack('<3H',y,x,z) for x,y,z in faces)
            identity=(vertices,count,zlib.crc32(raw))
            assert identity in expected,(path,identity)
            assert package.count(raw)==1,path
            seen.add(identity)
        offset+=size*count
assert seen==expected,(seen,expected)
print('PASS: 11 original socket meshes uniquely match cooked index streams and runtime CRCs')
