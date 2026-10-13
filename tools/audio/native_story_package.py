"""Offline, hash-pinned PC Vorbis replacement in observed v584 localized voice banks.

No Steam writes. Non-wave exports MUST stay at their old offsets. Unknown gaps,
footers, layouts or relocations refuse; preserve every property/platform payload.
"""
import hashlib
import struct
from patch_opening_dialogue import bulks, literals, require, TAG
from ue3_sound_package import inspect

def pack(logical, original):
    require(len(original)>0x81 and struct.unpack_from('<I',original)[0]==TAG,'package tag')
    flag,count=struct.unpack_from('<2I',original,0x61)
    require(flag==2 and 1<=count<=32,'unobserved compression summary')
    require(struct.unpack_from('<I',original,0x69)[0]==0x71,'unobserved logical header')
    body=logical[0x71:]
    raw=[body[p:p+0x20000] for p in range(0,len(body),0x20000)]
    pieces=[literals(p) if len(p)>=19 else p for p in raw]
    container=struct.pack('<4I',TAG,0x20000,sum(map(len,pieces)),len(body))
    container+=b''.join(struct.pack('<2I',len(c),len(r)) for c,r in zip(pieces,raw))
    container+=b''.join(pieces)
    header=bytearray(logical[:0x71]);struct.pack_into('<2I',header,0x61,2,1)
    record=struct.pack('<4I',0x71,len(body),0x81,len(container))
    physical=header[:0x69]+record+header[0x69:]+container
    return bytes(physical)+b'\0'*((-len(physical))%65536)

def build(original, logical, replacements, source_sha, logical_sha):
    require(hashlib.sha256(original).hexdigest()==source_sha.lower(),'original hash mismatch')
    require(hashlib.sha256(logical).hexdigest()==logical_sha.lower(),'logical hash mismatch')
    require(len(logical)<128*1024*1024,'logical size cap')
    exports=inspect(logical)['exports'];ordered=sorted(exports,key=lambda e:e['offset'])
    require(len({e['name'] for e in exports})==len(exports),'ambiguous export names')
    waves={e['name']:e for e in exports if e['class_name']=='SoundNodeWave'}
    require(set(replacements)<=set(waves),'target is not a native wave')
    native={e['index']:bulks(logical,e) for e in waves.values()}
    cursor=min(e['offset'] for e in exports);out=bytearray(logical[:cursor])
    for e in ordered:
        require(e['offset']==cursor,'unknown export gap/order')
        new_offset=len(out);cursor+=e['size'];blob=logical[e['offset']:cursor]
        if e['index'] in native:
            pieces=bytearray(logical[e['offset']:e['property_end']])
            for i,(_,payload) in enumerate(native[e['index']]):
                if i==1 and e['name'] in replacements:
                    payload=replacements[e['name']]
                    at=payload.find(b'\x01vorbis');props={p['name']:p['value'] for p in e['properties']}
                    require(at>=0 and at+16<=len(payload) and payload.startswith(b'OggS'),'invalid Vorbis')
                    require(payload[at+11]==props['NumChannels'] and
                            struct.unpack_from('<I',payload,at+12)[0]==props['SampleRate'],'Vorbis format mismatch')
                pieces+=struct.pack('<4I',0,len(payload),len(payload),new_offset+len(pieces)+16)+payload
            blob=bytes(pieces)
        else:
            require(new_offset==e['offset'],'non-wave export would relocate')
        out+=blob;struct.pack_into('<2i',out,e['record_offset']+32,len(blob),new_offset)
    require(cursor==len(logical),'unknown package tail')
    parsed=inspect(out)['exports']
    for a,b in zip(exports,parsed):
        require((a['name'],a['cls'],a['outer'])==(b['name'],b['cls'],b['outer']),'export identity changed')
        if a['index'] in native:
            require(logical[a['offset']:a['property_end']]==out[b['offset']:b['property_end']],'wave properties changed')
            for i,((_,old),(_,new)) in enumerate(zip(native[a['index']],bulks(out,b))):
                require(new==(replacements.get(a['name'],old) if i==1 else old),'unexpected platform/audio change')
        else:
            require(logical[a['offset']:a['offset']+a['size']]==out[b['offset']:b['offset']+b['size']],'non-wave changed')
    if not replacements:require(bytes(out)==logical,'repack-only logical data changed')
    return bytes(out),pack(out,original)
