"""Prove the first PRESENT cooked mip, never relabel stripped mip0 as recovered.

Inputs are private logical (already decompressed) UE3 packages and UModel DDS.
The existing offline LZO decoder verifies DDS bytes against the exact TFC range.
No installed game, GPU, source asset upload or Steam is involved.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import subprocess
import sys
import tempfile
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'audio'))
from ue3_sound_package import inspect
from dds_io import read_dds_header, fnv1a64_mip0
from prepare_hires_first_person import safe_private

def sha(path: Path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda:f.read(4*1024*1024),b''): h.update(b)
    return h.hexdigest()

def properties(data: bytes, names: list, start: int, end: int):
    q=start; result={}
    for _ in range(256):
        if q+8>end: raise ValueError('truncated property name')
        ni,number=struct.unpack_from('<ii',data,q); q+=8
        if number or not 0<=ni<len(names): raise ValueError('invalid property name')
        name=names[ni]
        if name=='None': return result,q
        if q+16>end: raise ValueError('truncated property header')
        ti,tn,size,index=struct.unpack_from('<iiii',data,q);q+=16
        if tn or not 0<=ti<len(names) or not 0<=size<=end-q: raise ValueError('invalid property')
        typ=names[ti]
        if typ=='BoolProperty':q+=4
        elif typ=='StructProperty':q+=8
        if q+size>end: raise ValueError('property beyond export')
        value=data[q:q+size];q+=size
        if typ=='IntProperty' and size==4:value=struct.unpack('<i',value)[0]
        elif typ in ('ByteProperty','NameProperty') and size==8:
            v,vn=struct.unpack('<ii',value)
            if vn or not 0<=v<len(names):raise ValueError('invalid FName value')
            value=names[v]
        result[name]=value
    raise ValueError('unbounded property chain')

def mips(data: bytes, start: int, end: int, fmt: str):
    if start+20>end or data[start:start+12]!=bytes(12):raise ValueError('unknown texture footer')
    count=struct.unpack_from('<I',data,start+16)[0];q=start+20
    if not 1<=count<=16:raise ValueError('invalid mip count')
    result=[]
    for index in range(count):
        if q+16>end:raise ValueError('truncated bulk descriptor')
        descriptor=q
        flags,raw,size,offset=struct.unpack_from('<IIii',data,q);q+=16
        if flags==0:
            if size!=raw or q+raw>end:raise ValueError('invalid inline bulk')
            q+=raw
        elif flags==0x11:
            if not raw or size<24 or offset<0:raise ValueError('invalid external LZO bulk')
        elif flags==0x21:
            if raw or size!=-1 or offset!=-1:raise ValueError('invalid unused bulk')
        else:raise ValueError('unsupported bulk flags '+hex(flags))
        if q+8>end:raise ValueError('truncated mip dimensions')
        w,h=struct.unpack_from('<II',data,q);q+=8
        if not 1<=w<=16384 or not 1<=h<=16384:raise ValueError('invalid mip dimensions')
        expected=((w+3)//4)*((h+3)//4)*(8 if fmt=='PF_DXT1' else 16)
        if flags!=0x21 and raw!=expected:raise ValueError('mip byte layout mismatch')
        result.append(dict(index=index,descriptor_offset=descriptor,flags=flags,raw_size=raw,
                           stored_size=size,bulk_offset=offset,width=w,height=h))
    return result

def audit(package: Path, name: str, source: Path, tfc: Path, decoder: Path, output: Path):
    safe_private(output)
    if output.exists():raise FileExistsError(output)
    data=package.read_bytes();pkg=inspect(data)
    found=[e for e in pkg['exports'] if e['class_name']=='Texture2D' and e['name']==name]
    if len(found)!=1:raise ValueError('texture identity not unique')
    e=found[0];p,q=properties(data,pkg['names'],e['offset']+4,e['offset']+e['size'])
    fmt=p.get('Format')
    if fmt not in ('PF_DXT1','PF_DXT5'):raise ValueError('unsupported source format')
    chain=mips(data,q,e['offset']+e['size'],fmt)
    first=next((m for m in chain if m['flags']!=0x21),None)
    if first is None or first['flags']!=0x11:raise ValueError('expected external first-present mip')
    header=read_dds_header(source)
    if (header['width'],header['height'],header['fourcc']) != (first['width'],first['height'],fmt[3:]):
        raise ValueError('DDS is not first-present cooked mip')
    output.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(dir=output.parent) as tmp:
        raw=Path(tmp)/'raw.bin'
        subprocess.run([str(decoder),str(tfc),str(first['bulk_offset']),str(first['stored_size']),
                        str(first['raw_size']),str(raw)],check=True)
        blocks=raw.read_bytes();dds=source.read_bytes()
        if dds[128:128+len(blocks)]!=blocks:raise ValueError('DDS bytes differ from exact cooked TFC bulk')
    report=dict(texture=name,export_index=e['index'],object_layout=[p['SizeX'],p['SizeY']],
        format=fmt,cache_name=p['TextureFileCacheName'],first_present_mip=first['index'],mips=chain,
        source_layout=[header['width'],header['height']],source_sha256=sha(source),
        source_fnv64=fnv1a64_mip0(source),package_sha256=sha(package),cache_sha256=sha(tfc),
        raw_sha256=hashlib.sha256(blocks).hexdigest(),exact_bulk_dds_match=True,
        physical_acceptance='PENDING',note='DDS mip0 is first PRESENT cooked mip; nominal stripped mips are never called recovered')
    output.write_text(json.dumps(report,indent=2));return report

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('package','source','tfc','decoder','output'):p.add_argument('--'+arg,type=Path,required=True)
    p.add_argument('--name',required=True);a=p.parse_args()
    print(json.dumps(audit(a.package,a.name,a.source,a.tfc,a.decoder,a.output),indent=2))
