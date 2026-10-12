"""Read-only, bounded UE3 v584 name/import/export and tagged-property inspector.
Accepts observed 68-byte exports plus bounded generation counts; refuses invalid ranges.
"""
import argparse
import json
import struct
from pathlib import Path

def inspect(path, data=None):
    b=Path(path).read_bytes() if data is None else data
    assert struct.unpack_from('<I',b)[0]==0x9E2A83C1
    folder=struct.unpack_from('<i',b,12)[0]
    p=16+abs(folder)*(2 if folder<0 else 1)+4
    nc,no,ec,eo,ic,io=struct.unpack_from('<6I',b,p)
    names=[]; p=no
    for _ in range(nc):
        n=struct.unpack_from('<i',b,p)[0]; p+=4
        assert 0<abs(n)<4096
        end=p+abs(n)*(2 if n<0 else 1)
        names.append(b[p:end].decode('utf-16le' if n<0 else 'latin1').rstrip('\0')); p=end+8
    def name(n):
        assert 0<=n<len(names)
        return names[n]
    imports=[struct.unpack_from('<7i',b,io+j*28) for j in range(ic)]
    exports=[]
    p=eo
    for j in range(ec):
        record_offset=p
        e=struct.unpack_from('<6iQ4i',b,p)
        # Six 32-bit fields, 64-bit object flags, size/offset/flags/netCount,
        # Generation list precedes GUID and package flags.
        assert 0<=e[10]<=32, 'net-count unsupported'
        p+=68+e[10]*4
        assert e[7]>=0 and 0<=e[8]<=len(b) and e[8]+e[7]<=len(b)
        exports.append(dict(index=j+1,name=name(e[3]),cls=e[0],outer=e[2],size=e[7],offset=e[8],record_offset=record_offset))
    def ref(i):
        return name(imports[-i-1][5]) if i<0 else exports[i-1]['name'] if i else 'None'
    result=[]
    for e in exports:
        if not (ref(e['cls']).startswith('RvUIGFxMovie') or ref(e['cls']) in ('GFxMovieInfo','TextureRenderTarget2D')): continue
        p=e['offset']+4; end=e['offset']+e['size']; props=[]
        try:
            for _ in range(128):
                tag_start=p
                ni,number=struct.unpack_from('<ii',b,p); p+=8
                nm=name(ni)
                if nm=='None': break
                ti,tn,size,idx=struct.unpack_from('<4i',b,p);p+=16; typ=name(ti)
                extra=None
                if typ=='BoolProperty': extra=struct.unpack_from('<I',b,p)[0];p+=4
                elif typ=='StructProperty':
                    extra=name(struct.unpack_from('<i',b,p)[0]);p+=8
                assert size>=0 and p+size<=end
                val=b[p:p+size];p+=size
                if typ=='ObjectProperty' and size==4: val=ref(struct.unpack('<i',val)[0])
                elif typ=='IntProperty' and size==4: val=struct.unpack('<i',val)[0]
                elif typ=='BoolProperty':val=bool(extra)
                elif typ=='ByteProperty' and size==8:val=name(struct.unpack_from('<i',val)[0])
                elif typ=='ByteProperty' and size==1:val=val[0]
                else:val={'bytes':size}
                props.append(dict(name=nm,type=typ,value=val,tag_offset=tag_start,tag_bytes=p-tag_start))
            result.append(dict(**e,class_name=ref(e['cls']),properties=props))
        except (AssertionError,IndexError,struct.error) as ex:
            result.append(dict(**e,class_name=ref(e['cls']),parse_error=str(ex),properties=props))
    return dict(counts=dict(names=nc,exports=ec,imports=ic),exports=result,records=exports)

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('package');args=ap.parse_args()
    print(json.dumps(inspect(args.package),indent=2))
