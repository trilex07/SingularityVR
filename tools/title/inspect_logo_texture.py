"""Decode the stock logo's observed inline mip for inspection; never edit it."""
import argparse
import hashlib
import io
import json
import struct
from pathlib import Path
from PIL import Image
from ue3_layout import inspect

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--input',type=Path,required=True)
    ap.add_argument('--out',type=Path,required=True)
    a=ap.parse_args()
    if a.out.exists():raise ValueError('Refuse existing output')
    data=a.input.read_bytes();d=inspect('memory',data,include_all=True)
    hits=[e for e in d['exports'] if e['name']=='logo' and e['class_name']=='Texture2D']
    if len(hits)!=1:raise ValueError('Exact logo texture identity required')
    e=hits[0];props={p['name']:p['value'] for p in e['properties']};p=e['property_end']
    def bulk():
        nonlocal p
        flags,count,size,off=struct.unpack_from('<4I',data,p)
        if flags or count!=size or off!=p+16 or off+size>e['offset']+e['size']:raise ValueError('Unknown/non-inline texture bulk')
        raw=data[off:off+size];p=off+size;return raw
    bulk();n=struct.unpack_from('<I',data,p)[0];p+=4
    if n!=1 or props.get('Format')!='PF_DXT5':raise ValueError('Unknown mip profile')
    raw=bulk();w,h=struct.unpack_from('<2I',data,p);p+=8
    if (w,h)!=(props['SizeX'],props['SizeY']) or len(raw)!=((w+3)//4)*((h+3)//4)*16:raise ValueError('Native DXT5 dimensions/byte count mismatch')
    header=[124,0x81007,h,w,len(raw),0,0]+[0]*11+[32,4,struct.unpack('<I',b'DXT5')[0],0,0,0,0,0]+[0x1000,0,0,0,0]
    if len(header)!=31:raise ValueError('DDS header size')
    dds=b'DDS '+struct.pack('<31I',*header)+raw
    image=Image.open(io.BytesIO(dds));image.load()
    if image.size!=(w,h):raise ValueError('Independent DDS decode dimensions')
    a.out.mkdir(parents=True)
    (a.out/'stock-logo.dds').write_bytes(dds)
    image.save(a.out/'stock-logo.png')
    result=dict(status='OFFLINE_DECODE_NOT_RENDER_PROOF',package_sha256=hashlib.sha256(data).hexdigest(),
        texture='ui_logos.logo',dimensions=[w,h],format='PF_DXT5',mip_sha256=hashlib.sha256(raw).hexdigest(),
        native_payload_unchanged=True,tail_bytes=e['offset']+e['size']-p)
    (a.out/'logo.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps(result))
if __name__=='__main__':main()
