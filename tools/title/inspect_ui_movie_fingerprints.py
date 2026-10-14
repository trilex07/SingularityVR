"""Offline source witnesses for the exact native MovieInfo.RawData array."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from ue3_layout import inspect

def fnv(data):
    h=14695981039346656037
    for b in data:h=((h^b)*1099511628211)&((1<<64)-1)
    return f'{h:016X}'

def main():
    ap=argparse.ArgumentParser();ap.add_argument('packages',nargs='+',type=Path)
    ap.add_argument('--report',type=Path,required=True);a=ap.parse_args()
    if a.report.exists():raise ValueError('Refuse existing report')
    rows=[]
    for path in a.packages:
        b=path.read_bytes()
        if len(b)>8*1024*1024:raise ValueError('Package cap')
        d=inspect('memory',b,include_all=True)
        for e in d['exports']:
            if e['class_name']!='GFxMovieInfo':continue
            raw=next(p for p in e['properties'] if p['name']=='RawData')
            at=raw['tag_offset']+24;count=struct.unpack_from('<i',b,at)[0]
            if count!=raw['value']['bytes']-4 or not 8<=count<=2*1024*1024:raise ValueError('RawData array contract')
            movie=b[at+4:at+4+count]
            rows.append(dict(package=str(path),movie=e['name'],bytes=count,fnv64=fnv(movie),
                sha256=hashlib.sha256(movie).hexdigest(),magic=movie[:4].hex(),
                status='SOURCE_ONLY_NOT_CONSUMER_OR_PIXEL_PROOF'))
    a.report.parent.mkdir(parents=True,exist_ok=True)
    a.report.write_text(json.dumps(rows,indent=2),encoding='utf-8')
    print(json.dumps(rows,indent=2))
if __name__=='__main__':main()
