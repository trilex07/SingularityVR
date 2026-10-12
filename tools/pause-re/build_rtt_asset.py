"""Build a pinned, isolated UE3 native pause RTT asset; never installs it.
LZO1X literal-only encoding is original format code. No compression library is copied.
Only the SHA-pinned v584/126 UI package is supported. Unknown native layouts refuse.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from inspect_package import inspect

SOURCE_SHA = '90a5ca7efffcce7bea8a201e99b8313f4a276c270657e5b521dc9ede8f4101c6'
PKG_COMPRESSED = 0x02000000
TAG = 0x9E2A83C1

def require(ok, why):
    if not ok: raise ValueError(why)

def sha(b): return hashlib.sha256(b).hexdigest()

def lzo_literals(b):
    """One long literal run followed by the standard LZO1X EOF marker."""
    require(len(b)>=19,'literal block too short')
    remaining=len(b)-18; prefix=bytearray([0])
    while remaining>255:
        prefix.append(0);remaining-=255
    prefix.append(remaining)
    return bytes(prefix)+b+b'\x11\0\0'

def pack(logical):
    # Original summary has compression flags/count at 0x61/0x65; its logical
    # header is 0x71 bytes, with the physical chunk record inserted at 0x69.
    body=logical[0x71:]
    chunks=[lzo_literals(body[p:p+0x20000]) for p in range(0,len(body),0x20000)]
    sizes=[len(body[p:p+0x20000]) for p in range(0,len(body),0x20000)]
    container=struct.pack('<4I',TAG,0x20000,sum(map(len,chunks)),len(body))
    container+=b''.join(struct.pack('<2I',len(c),u) for c,u in zip(chunks,sizes))+b''.join(chunks)
    header=bytearray(logical[:0x71])
    struct.pack_into('<I',header,0x15,struct.unpack_from('<I',header,0x15)[0]|PKG_COMPRESSED)
    struct.pack_into('<2I',header,0x61,2,1)
    physical=bytes(header[:0x69])+struct.pack('<4I',0x71,len(body),0x81,len(container))+bytes(header[0x69:])+container
    return physical+b'\0'*((-len(physical))%65536)

def texture_offset_fields(b,e):
    """Known SourceArt + Mips + GUID layout, all inline and uncompressed.
    Returns ONLY declared absolute offsets; never scans integers for matches.
    """
    p=e['property_end'];end=e['offset']+e['size'];fields=[]
    def bulk(p):
        flags,count,size,off=struct.unpack_from('<4I',b,p)
        require(flags==0 and count==size,'unsupported bulk flags/size')
        require(off==p+16,'bulk pointer not inline')
        require(p+16+size<=end,'bulk range')
        fields.append(p+12)
        return p+16+size
    p=bulk(p)
    if e['class_name']=='Texture2D':
        n=struct.unpack_from('<I',b,p)[0];p+=4
        require(0<n<=16,'unexpected mip count')
        for _ in range(n):
            p=bulk(p)
            w,h=struct.unpack_from('<2I',b,p);p+=8
            require(0<w<=8192 and 0<h<=8192,'mip dimensions')
        require(p+16==end,'unexpected native texture footer') # TFC GUID
    else:
        require(p==end,'unexpected native RTT footer')
    return fields

def validate_shader_cache(b,e,names):
    # This particular cache has CRC maps but zero shader/material bodies.
    p=e['property_end'];end=e['offset']+e['size']
    require(b[p]==0,'shader platform changed');p+=1
    for section in range(2):
        n=struct.unpack_from('<I',b,p)[0];p+=4
        require(n<=1024,'CRC map size')
        for _ in range(n):
            name,number,crc=struct.unpack_from('<3I',b,p);p+=12
            require(name<len(names) and number==0,'CRC map FName')
        if section==0:
            require(struct.unpack_from('<I',b,p)[0]==0,'nonempty shaders; relocation unsafe');p+=4
    require(b[p:end]==b'\0'*4,'nonempty material shader cache; relocation unsafe')

def build(raw,mode):
    require(mode in ('control','dedicated'),'unknown build mode')
    require(sha(raw)==SOURCE_SHA,'source hash mismatch; refuses unknown package')
    r=inspect('memory',raw,include_all=True);names=r['names']
    require(all('parse_error' not in e for e in r['exports']),'property parse failed')
    es={e['name']:e for e in r['exports']};nc=len(names);ec=len(r['records'])
    no,io,eo=[r['table_offsets'][k] for k in ('names','imports','exports')]
    header_size=struct.unpack_from('<I',raw,8)[0]
    record_end=max(e['record_offset']+e['record_bytes'] for e in r['records'])
    require(raw[record_end:header_size]==b'\0'*(4*ec),'depends map is not empty')
    require(struct.unpack_from('<I',raw,0x31)[0]==record_end,'depends offset mismatch')
    for e in r['exports']:
        if e['class_name'] in ('Texture2D','TextureRenderTarget2D'):texture_offset_fields(raw,e)
        elif e['class_name']=='ShaderCache':validate_shader_cache(raw,e,names)
        else:require(e['class_name'] in ('GFxMovieInfo','ObjectRedirector','ObjectReferencer','Package') or
                     e['class_name'].startswith('RvUIGFxMovie'),'unknown export class')
    if mode=='control':
        out=bytearray(raw)
        struct.pack_into('<I',out,0x15,struct.unpack_from('<I',out,0x15)[0]&~PKG_COMPRESSED)
        return bytes(out),dict(mode=mode,relocations=0,new_target=None)
    new_name='RTT_VRPause';require(new_name not in names,'target name already exists')
    # Name flags are taken from an existing target name, never a guessed mask.
    p=no; name_entries=[]
    for _ in names:
        begin=p;n=struct.unpack_from('<i',raw,p)[0];p+=4+abs(n)*(2 if n<0 else 1)+8
        name_entries.append(raw[begin:p])
    require(p==io,'name table does not end at imports')
    name_entry=struct.pack('<i',len(new_name)+1)+new_name.encode()+b'\0'+name_entries[names.index('RTT_UI2')][-8:]
    added_header=len(name_entry)+68+4
    out=bytearray(raw[:no]+raw[no:io]+name_entry+raw[io:eo])
    new_eo=len(out)
    export_bytes=bytearray(raw[eo:record_end])
    template=es['RTT_UI2'];new_export=bytearray(raw[template['record_offset']:template['record_offset']+68])
    struct.pack_into('<i',new_export,12,nc)
    payloads=[];relocations=[];cursor=header_size+added_header
    donor=es['SPLoadGame'];pause=es['SPPause']
    donor_tags=[p for p in donor['properties'] if p['name'] in ('RenderTexture','RenderTextureMode')]
    addition=bytearray(b''.join(raw[p['tag_offset']:p['tag_offset']+p['tag_bytes']] for p in donor_tags))
    require(len(addition)==60 and donor_tags[0]['name']=='RenderTexture','donor tags changed')
    struct.pack_into('<i',addition,24,ec+1) # the new dedicated export, not shared RTT_UI2
    for e in sorted(r['exports'],key=lambda x:x['offset']):
        require(e['offset']+e['size']<=len(raw),'export outside file')
        blob=bytearray(raw[e['offset']:e['offset']+e['size']])
        if e['index']==pause['index']:
            local=pause['property_end']-pause['offset']-8
            blob=blob[:local]+addition+blob[local:]
        if e['class_name'] in ('Texture2D','TextureRenderTarget2D'):
            for field in texture_offset_fields(raw,e):
                old=struct.unpack_from('<I',raw,field)[0]
                struct.pack_into('<I',blob,field-e['offset'],old+cursor-e['offset'])
                relocations.append(dict(export=e['index'],local=field-e['offset'],old=old,new=old+cursor-e['offset']))
        record=e['record_offset']-eo
        struct.pack_into('<2I',export_bytes,record+32,len(blob),cursor)
        payloads.append(bytes(blob));cursor+=len(blob)
    target=bytearray(raw[template['offset']:template['offset']+template['size']])
    # UObject NetIndex INDEX_NONE avoids duplicating a network identity from the template.
    struct.pack_into('<i',target,0,-1)
    for field in texture_offset_fields(raw,template):
        local=field-template['offset'];struct.pack_into('<I',target,local,cursor+local+4)
    struct.pack_into('<2I',new_export,32,len(target),cursor)
    out+=export_bytes+new_export
    new_depends=len(out);out+=raw[record_end:header_size]+b'\0'*4
    require(len(out)==header_size+added_header,'header reconstruction size')
    out+=b''.join(payloads)+target
    struct.pack_into('<I',out,8,header_size+added_header)
    struct.pack_into('<I',out,0x15,struct.unpack_from('<I',out,0x15)[0]&~PKG_COMPRESSED)
    struct.pack_into('<6I',out,0x19,nc+1,no,ec+1,new_eo,r['counts']['imports'],io+len(name_entry))
    struct.pack_into('<I',out,0x31,new_depends)
    struct.pack_into('<2I',out,0x4d,ec+1,nc+1) # latest generation counts
    verify=inspect('output',out,include_all=True)
    require(all('parse_error' not in e for e in verify['exports']),'output property parse')
    ves={e['name']:e for e in verify['exports']}
    props=lambda e:{p['name']:p['value'] for p in e['properties']}
    require(props(ves['SPPause'])==props(pause)|{'RenderTexture':new_name,'RenderTextureMode':'RTM_AlphaComposite'},'pause properties changed')
    for e in verify['exports']:
        if e['class_name'] in ('Texture2D','TextureRenderTarget2D'):texture_offset_fields(out,e)
        elif e['class_name']=='ShaderCache':validate_shader_cache(out,e,names)
    require(out[ves['SPMenus']['offset']:ves['SPMenus']['offset']+ves['SPMenus']['size']]==raw[es['SPMenus']['offset']:es['SPMenus']['offset']+es['SPMenus']['size']],'SPMenus changed')
    for name,e in es.items():
        if name=='SPPause':continue
        v=ves[name];expected=bytearray(raw[e['offset']:e['offset']+e['size']])
        for rr in relocations:
            if rr['export']==e['index']:struct.pack_into('<I',expected,rr['local'],rr['new'])
        require(out[v['offset']:v['offset']+v['size']]==expected,'unrelated export mutated: '+name)
    return bytes(out),dict(mode=mode,new_target=new_name,new_export=ec+1,relocations=relocations,
                         original_movie_sha256=sha(raw[es['SPMenus']['offset']:es['SPMenus']['offset']+es['SPMenus']['size']]))

def main():
    ap=argparse.ArgumentParser();ap.add_argument('logical_package',type=Path)
    ap.add_argument('--mode',choices=['control','dedicated'],required=True)
    ap.add_argument('--output-dir',type=Path,required=True);a=ap.parse_args()
    dest=a.output_dir.resolve()
    require(not any(p.lower().startswith('steam') for p in dest.parts),'refuses Steam output')
    require(not any((p/'.git').exists() for p in (dest,*dest.parents)),'assets must stay outside Git')
    require(not dest.exists(),'output directory already exists; refuses overwrite')
    raw=a.logical_package.read_bytes();logical,meta=build(raw,a.mode);physical=pack(logical)
    meta.update(input_sha256=sha(raw),logical_sha256=sha(logical),cooked_sha256=sha(physical),
                logical_bytes=len(logical),cooked_bytes=len(physical),compression='LZO1X literal-only, 128KiB blocks',
                structural='PASS',runtime='UNVERIFIED')
    dest.mkdir(parents=True)
    (dest/'UI_SinglePlayer_SF.u').write_bytes(logical)
    (dest/'UI_SinglePlayer_SF.xxx').write_bytes(physical)
    (dest/'manifest.json').write_text(json.dumps(meta,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:v for k,v in meta.items() if k!='relocations'},indent=2))

if __name__=='__main__':main()
