"""Pinned accepted UI -> logo's own GFx text; OFFLINE, no runtime hook.

Reuse SIDE title patcher. Keep fonts, bounds, matrices, formatting, ActionScript
and accepted pause RTT byte-identical. Unknown native export layouts refuse.
"""
import argparse
import hashlib
import json
import struct
import subprocess
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parents[1]/'audio'))
from patch_opening_dialogue import literals, require
from ue3_layout import inspect
from swf_title_patch import fit_title_field,split_tags
from swf_ui_polish import polish_movie

UI_SHA='5b2ea469a3ee06ff154ac78bd58604e8034970d4837f289e5c22503c6a673cde'
TAG=0x9E2A83C1

def pack_summary(logical):
    """Write the pinned UI summary container as LZO literal blocks.

    The accepted package is one chunk at logical 0x71 and physical 0x81.
    Raw uncompressed blocks made the game run its decompressor on plain
    bytes and abort with the virtual-memory error before the main menu.
    """
    require(len(logical)>0x71 and struct.unpack_from('<I',logical)[0]==TAG,'logical package')
    require(logical[0x61:0x69]==b'\x00'*8,'decoded summary slot is not empty')
    body=logical[0x71:]
    block=0x20000
    parts=[body[i:i+block] for i in range(0,len(body),block)]
    encoded=[literals(part) for part in parts]
    container=struct.pack('<4I',TAG,block,sum(map(len,encoded)),len(body))
    container+=b''.join(struct.pack('<2I',len(comp),len(raw)) for comp,raw in zip(encoded,parts))
    container+=b''.join(encoded)
    physical=bytearray(logical[:0x61])
    physical+=struct.pack('<2I',2,1)
    physical+=struct.pack('<4I',0x71,len(body),0x81,len(container))
    physical+=logical[0x69:0x71]
    require(len(physical)==0x81,'summary header drifted')
    physical+=container
    physical+=b'\x00'*((-len(physical))%65536)
    return bytes(physical)

def sha(b):return hashlib.sha256(b).hexdigest()

def offsets(b,e):
    p=e['property_end'];end=e['offset']+e['size'];fields=[]
    def bulk(p):
        flags,count,size,off=struct.unpack_from('<4I',b,p)
        require(flags==0 and count==size and off==p+16 and off+size<=end,'unsupported inline texture')
        fields.append(p+12);return off+size
    p=bulk(p)
    if e['class_name']=='Texture2D':
        n=struct.unpack_from('<I',b,p)[0];p+=4;require(0<n<=16,'mip count')
        for _ in range(n):
            p=bulk(p);w,h=struct.unpack_from('<2I',b,p);p+=8
            require(0<w<=8192 and 0<h<=8192,'mip dimensions')
        require(p+16==end,'unknown texture tail')
    else:require(p==end,'unknown RTT tail')
    return fields

def build(original,logical,polish_ui=False):
    require(sha(original)==UI_SHA,'accepted UI hash mismatch')
    require(sha(logical)=='c5e7db296339ec0e5daa962e3683f01c854cb7fa09afc7c257d020ce34007bed','accepted logical UI hash mismatch')
    data=inspect('memory',logical,include_all=True)
    require(all('parse_error' not in e for e in data['exports']),'property parse')
    exports=data['exports'];by={e['name']:e for e in exports}
    require('RTT_VRPause' in by and len(exports)==45,'accepted RTT layout changed')
    def splice(export,movie_bytes):
        raw=next(p for p in export['properties'] if p['name']=='RawData')
        require(raw['type']=='ArrayProperty','RawData type')
        value=raw['tag_offset']+24;size=raw['value']['bytes']
        require(struct.unpack_from('<I',logical,value)[0]==size-4,'RawData count')
        delta=len(movie_bytes)-(size-4)
        blob=bytearray(logical[export['offset']:value+4]+movie_bytes+logical[value+size:export['offset']+export['size']])
        struct.pack_into('<I',blob,raw['tag_offset']-export['offset']+16,size+delta)
        struct.pack_into('<I',blob,value-export['offset'],size-4+delta)
        return blob
    logo=by['iis'];require(logo['class_name']=='GFxMovieInfo','logo class')
    logo_raw=next(p for p in logo['properties'] if p['name']=='RawData')
    logo_at=logo_raw['tag_offset']+24;logo_n=logo_raw['value']['bytes']-4
    movie=logical[logo_at+4:logo_at+4+logo_n]
    _,tags=split_tags(movie)
    hits=[t for t in tags if t['code']==37 and b'SINGULARITY' in movie[t['pay_off']:t['pay_off']+t['pay_len']]]
    require(len(hits)==1 and struct.unpack_from('<H',movie,hits[0]['pay_off'])[0]==10,'exact logo text character not proved')
    menus=by['SPMenus'];require(menus['class_name']=='GFxMovieInfo','menus class')
    menu_raw=next(p for p in menus['properties'] if p['name']=='RawData')
    menu_at=menu_raw['tag_offset']+24;menu_n=menu_raw['value']['bytes']-4
    menu_movie=logical[menu_at+4:menu_at+4+menu_n]
    if polish_ui:
        patched,report=polish_movie(movie,True)
        changed,main_report=polish_movie(menu_movie,False)
    else:
        patched,report=fit_title_field(movie,b'SINGULARITY',b'SINGULARITY VR')
        require(report['character']==10 and report['align']==1 and report['new_bounds'][1]==report['old_bounds'][1],'boot title must stay right-anchored')
        changed,main_report=fit_title_field(menu_movie,b'SINGULARITY',b'SINGULARITY VR')
        require(main_report['character']==166 and main_report['align']==2,'menu title must stay centred')
    blobs={logo['index']:splice(logo,patched),menus['index']:splice(menus,changed)}
    report={'logo':report,'menus':main_report,'cursor':'unchanged' if not polish_ui else 'native Mouse leaf','dll':'not part of this package'}
    ordered=sorted(exports,key=lambda e:e['offset']);cursor=ordered[0]['offset']
    out=bytearray(logical[:cursor]);relocations=[]
    for e in ordered:
        require(e['offset']==cursor,'unknown native gap');cursor+=e['size']
        new=len(out);part=bytearray(blobs.get(e['index'],logical[e['offset']:cursor]))
        cls=e['class_name']
        if cls in ('Texture2D','TextureRenderTarget2D'):
            for field in offsets(logical,e):
                local=field-e['offset'];old=struct.unpack_from('<I',part,local)[0]
                struct.pack_into('<I',part,local,old+new-e['offset'])
                relocations.append(dict(export=e['name'],local=local,delta=new-e['offset']))
        elif cls=='ShaderCache':
            # Empty bodies confirmed in this pinned package; all bytes preserved.
            p=e['property_end'];end=cursor;require(logical[p]==0,'shader platform');p+=1
            for section in range(2):
                n=struct.unpack_from('<I',logical,p)[0];p+=4;require(n<=1024,'shader CRC count')
                for _ in range(n):
                    ni,num,_crc=struct.unpack_from('<3I',logical,p);p+=12
                    require(ni<len(data['names']) and num==0,'shader CRC name')
                if section==0:require(struct.unpack_from('<I',logical,p)[0]==0,'nonempty shader body');p+=4
            require(logical[p:end]==b'\0'*4,'nonempty material cache')
        else:require(cls in ('GFxMovieInfo','ObjectRedirector','ObjectReferencer','Package') or cls.startswith('RvUIGFxMovie'),'unknown class')
        out+=part;struct.pack_into('<2I',out,e['record_offset']+32,len(part),new)
    require(cursor==len(logical),'unknown native tail')
    verify=inspect('result',out,include_all=True)
    for a,b in zip(exports,verify['exports']):
        require((a['name'],a['cls'],a['outer'])==(b['name'],b['cls'],b['outer']),'export identity')
        expected=bytearray(blobs.get(a['index'],logical[a['offset']:a['offset']+a['size']]))
        for rel in relocations:
            if rel['export']==a['name']:
                old=struct.unpack_from('<I',expected,rel['local'])[0];struct.pack_into('<I',expected,rel['local'],old+rel['delta'])
        require(out[b['offset']:b['offset']+b['size']]==expected,'unrelated export change')
        if a['class_name'] in ('Texture2D','TextureRenderTarget2D'):offsets(out,b)
    return bytes(out),dict(movie='iis+SPMenus',title='SINGULARITY VR',
        actions='byte-exact' if not polish_ui else 'scoped Initialize title conditional',
        patch=report,relocations=relocations)

def main():
    ap=argparse.ArgumentParser()
    for x in ('original','logical','decoder','out'):ap.add_argument('--'+x,type=Path,required=True)
    ap.add_argument('--polish-ui',action='store_true',help='native Mouse concentric dot + runtime main-title assignment')
    a=ap.parse_args();require(not a.out.exists(),'refuse existing output')
    original=a.original.read_bytes();logical=a.logical.read_bytes();changed,report=build(original,logical,a.polish_ui)
    a.out.mkdir(parents=True)
    for name,expected in [('control',logical),('candidate',changed)]:
        physical=pack_summary(expected);dest=a.out/(name+'.xxx');dest.write_bytes(physical)
        decoded=a.out/(name+'.u');subprocess.run([str(a.decoder),str(dest),str(decoded)],capture_output=True,check=True)
        require(decoded.read_bytes()==expected,'independent decoder roundtrip')
    report.update(source_sha=sha(original),logical_sha=sha(logical),candidate_sha=sha((a.out/'candidate.xxx').read_bytes()),status='OFFLINE_PASS_PHYSICAL_PENDING')
    (a.out/'title.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
