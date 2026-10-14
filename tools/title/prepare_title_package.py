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
from native_story_package import pack
from patch_opening_dialogue import require
from ue3_layout import inspect
from swf_title_patch import patch_first,split_tags

UI_SHA='5b2ea469a3ee06ff154ac78bd58604e8034970d4837f289e5c22503c6a673cde'
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

def build(original,logical):
    require(sha(original)==UI_SHA,'accepted UI hash mismatch')
    require(sha(logical)=='c5e7db296339ec0e5daa962e3683f01c854cb7fa09afc7c257d020ce34007bed','accepted logical UI hash mismatch')
    data=inspect('memory',logical,include_all=True)
    require(all('parse_error' not in e for e in data['exports']),'property parse')
    exports=data['exports'];by={e['name']:e for e in exports}
    require('RTT_VRPause' in by and len(exports)==45,'accepted RTT layout changed')
    logo=by['iis'];require(logo['class_name']=='GFxMovieInfo','logo class')
    raw=next(p for p in logo['properties'] if p['name']=='RawData')
    require(raw['type']=='ArrayProperty','RawData type')
    value=raw['tag_offset']+24;size=raw['value']['bytes']
    require(struct.unpack_from('<I',logical,value)[0]==size-4,'RawData count')
    movie=logical[value+4:value+size]
    _,tags=split_tags(movie)
    hits=[t for t in tags if t['code']==37 and b'SINGULARITY' in movie[t['pay_off']:t['pay_off']+t['pay_len']]]
    require(len(hits)==1 and struct.unpack_from('<H',movie,hits[0]['pay_off'])[0]==10,'exact logo text character not proved')
    patched,report=patch_first(movie,b'SINGULARITY',b'SINGULARITY VR')
    require(report['delta']==3,'unexpected title delta')
    blob=bytearray(logical[logo['offset']:value+4]+patched+logical[value+size:logo['offset']+logo['size']])
    struct.pack_into('<I',blob,raw['tag_offset']-logo['offset']+16,size+3)
    struct.pack_into('<I',blob,value-logo['offset'],size-4+3)
    ordered=sorted(exports,key=lambda e:e['offset']);cursor=ordered[0]['offset']
    out=bytearray(logical[:cursor]);relocations=[]
    for e in ordered:
        require(e['offset']==cursor,'unknown native gap');cursor+=e['size']
        new=len(out);part=bytearray(blob if e['index']==logo['index'] else logical[e['offset']:cursor])
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
        expected=bytearray(blob if a['index']==logo['index'] else logical[a['offset']:a['offset']+a['size']])
        for rel in relocations:
            if rel['export']==a['name']:
                old=struct.unpack_from('<I',expected,rel['local'])[0];struct.pack_into('<I',expected,rel['local'],old+rel['delta'])
        require(out[b['offset']:b['offset']+b['size']]==expected,'unrelated export change')
        if a['class_name'] in ('Texture2D','TextureRenderTarget2D'):offsets(out,b)
    return bytes(out),dict(movie='iis',character=10,title='SINGULARITY VR',font_bounds_and_actions='byte-exact',patch=report,relocations=relocations)

def main():
    ap=argparse.ArgumentParser()
    for x in ('original','logical','decoder','out'):ap.add_argument('--'+x,type=Path,required=True)
    a=ap.parse_args();require(not a.out.exists(),'refuse existing output')
    original=a.original.read_bytes();logical=a.logical.read_bytes();changed,report=build(original,logical)
    a.out.mkdir(parents=True)
    for name,expected in [('control',logical),('candidate',changed)]:
        physical=pack(expected,original);dest=a.out/(name+'.xxx');dest.write_bytes(physical)
        decoded=a.out/(name+'.u');subprocess.run([str(a.decoder),str(dest),str(decoded)],capture_output=True,check=True)
        require(decoded.read_bytes()==expected,'independent decoder roundtrip')
    report.update(source_sha=sha(original),logical_sha=sha(logical),candidate_sha=sha((a.out/'candidate.xxx').read_bytes()),status='OFFLINE_PASS_PHYSICAL_PENDING')
    (a.out/'title.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
