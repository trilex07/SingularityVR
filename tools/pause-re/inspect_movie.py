"""Bounded offline SWF/GFx recovery; outputs assets only to explicit external directory."""
import argparse
import hashlib
import json
import re
import struct
import zlib
from pathlib import Path


def movie_info(raw):
    if len(raw) < 12 or raw[:3] not in (b'FWS', b'GFX'):
        raise ValueError('not an uncompressed movie')
    bits = ''.join(f'{b:08b}' for b in raw[8:32])
    width = int(bits[:5], 2)
    if not 1 <= width <= 31:
        raise ValueError('RECT bit width')
    coords = []
    for j in range(4):
        n = int(bits[5+j*width:5+(j+1)*width], 2)
        coords.append(n - (1 << width) if n & (1 << (width-1)) else n)
    p = 8 + (5+width*4+7)//8
    fps, frames = struct.unpack_from('<HH', raw, p)
    p += 4
    tags = {}
    while p+2 <= len(raw):
        head = struct.unpack_from('<H', raw, p)[0]; p += 2
        tag, size = head >> 6, head & 63
        if size == 63:
            size = struct.unpack_from('<I', raw, p)[0]; p += 4
        if p+size > len(raw):
            raise ValueError('tag exceeds declared file')
        tags[tag] = tags.get(tag, 0)+1
        p += size
        if tag == 0:
            break
    else:
        raise ValueError('missing End tag')
    strings = re.findall(rb'[ -~]{4,}', raw)
    hits = [s.decode('ascii')[:160] for s in strings if re.search(
        rb'pause|objective|selectedIndex|scaleMode|Stage|setFocus|GetSelected|tab', s, re.I)]
    return dict(stage_pixels=[(coords[1]-coords[0])/20, (coords[3]-coords[2])/20],
                fps=fps/256, frames=frames, tags=tags, parsed_end=p,
                identifiers=hits[:60])


def main():
    ap=argparse.ArgumentParser(); ap.add_argument('package', type=Path)
    ap.add_argument('--extract-to', type=Path)
    args=ap.parse_args(); data=args.package.read_bytes()
    found=[]
    for match in re.finditer(rb'FWS|CWS|GFX|CFX', data):
        p=match.start()
        if p+8>len(data): continue
        n=struct.unpack_from('<I', data, p+4)[0]
        if not 12<=n<=32*1024*1024 or not 1<=data[p+3]<=40: continue
        try:
            sig=data[p:p+3]
            if sig in (b'CWS',b'CFX'):
                dec=zlib.decompressobj(); body=dec.decompress(data[p+8:],n-8+1)
                if not dec.eof or len(body)!=n-8: continue
                raw=(b'FWS' if sig==b'CWS' else b'GFX')+data[p+3:p+8]+body
            else: raw=data[p:p+n]
            if len(raw)!=n: continue
            info=movie_info(raw)
        except (ValueError,struct.error,zlib.error): continue
        info.update(offset=hex(p), signature=sig.decode(), bytes=n,
                    sha256=hashlib.sha256(raw).hexdigest())
        if args.extract_to:
            args.extract_to.mkdir(parents=True,exist_ok=True)
            out=args.extract_to/f'movie-{p:08x}.swf'; out.write_bytes(raw)
            info['local_asset']=str(out)
        found.append(info)
    print(json.dumps(dict(package_sha256=hashlib.sha256(data).hexdigest(), movies=found),indent=2))

if __name__=='__main__': main()
