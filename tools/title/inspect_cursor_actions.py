"""Read-only bounded AVM1 inspection of a local GFxMovieInfo export."""
import argparse
import struct
from pathlib import Path
from ue3_layout import inspect
from swf_title_patch import split_tags
from swf_ui_polish import action_records

def walk(data, pool=(), depth=0):
    if depth > 12:
        raise ValueError('function nesting cap')
    for r in action_records(data):
        p = data[r['payload']:r['metadata_end']]
        code = r['code']
        detail = p.hex()
        if code == 0x88:
            count = struct.unpack_from('<H', p)[0]
            pool = p[2:].rstrip(b'\0').decode('latin1').split('\0')
            assert len(pool) == count
            detail = repr(list(enumerate(pool)))
        elif code == 0x96:
            values = []; i = 0
            while i < len(p):
                kind = p[i]; i += 1
                if kind == 0:
                    end = p.index(0, i); v = p[i:end].decode('latin1'); i = end+1
                elif kind in (4, 5, 8):
                    v = p[i]; i += 1
                    v = pool[v] if kind == 8 else ('register' if kind == 4 else 'bool', v)
                elif kind == 9:
                    v = pool[struct.unpack_from('<H', p, i)[0]]; i += 2
                elif kind in (1, 7):
                    v = struct.unpack_from('<f' if kind == 1 else '<i', p, i)[0]; i += 4
                elif kind == 6:
                    v = struct.unpack('<d', p[i+4:i+8]+p[i:i+4])[0]; i += 8
                elif kind in (2, 3):
                    v = 'null' if kind == 2 else 'undefined'
                else:
                    raise ValueError(f'unknown Push type {kind}')
                values.append(v)
            detail = repr(values)
        print('  '*depth+f'{r["start"]:04x} op={code:02x} {r.get("name", "")} {detail}')
        if 'body_start' in r:
            walk(data[r['body_start']:r['body_end']], pool, depth+1)

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('package'); ap.add_argument('--movie',default='Dialogs')
    ap.add_argument('--contains',default='_pointer')
    a=ap.parse_args(); b=Path(a.package).read_bytes()
    d=inspect('memory',b,include_all=True)
    e=next(e for e in d['exports'] if e['name']==a.movie and e['class_name']=='GFxMovieInfo')
    p=next(p for p in e['properties'] if p['name']=='RawData')
    start=p['tag_offset']+28; m=b[start:start+p['value']['bytes']-4]
    _,tags=split_tags(m)
    for t in tags:
        if t['code'] not in (12,59): continue
        payload=m[t['pay_off']:t['pay_off']+t['pay_len']]
        if a.contains.encode('ascii') not in payload: continue
        print(f'TAG {t["code"]} at {t["pay_off"]}')
        walk(payload[2:] if t['code']==59 else payload)

if __name__=='__main__':main()
