"""Read-only verification of owned Engine script + archived interpreter dispatch.

No process access, Steam writes or proprietary function-body output.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'title'))
from ue3_layout import inspect


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--engine', type=Path, required=True)
    ap.add_argument('--text', type=Path, required=True)
    ap.add_argument('--text-base', type=lambda s: int(s, 0), required=True)
    ap.add_argument('--interpreter', type=lambda s: int(s, 0), required=True)
    a = ap.parse_args()
    engine = a.engine.read_bytes()
    p = inspect('memory', engine, include_all=True)
    functions = []
    for f in p['exports']:
        if f['name'] != 'ThrowActiveWeapon' or f['outer'] <= 0:
            continue
        owner = p['records'][f['outer']-1]['name']
        if owner not in ('Pawn', 'Vehicle'):
            continue
        raw = engine[f['offset']:f['offset']+f['size']]
        n = struct.unpack_from('<I', raw, 40)[0]
        if not 0 < n <= len(raw)-44:
            raise ValueError('script length invalid')
        functions.append(dict(owner=owner, export=f['index'], script_bytes=n))
    if not any(f['owner'] == 'Pawn' for f in functions):
        raise ValueError('Engine.Pawn.ThrowActiveWeapon script missing')
    text = a.text.read_bytes()
    off = a.interpreter-a.text_base
    if not 0 <= off <= len(text)-0x1a0:
        raise ValueError('interpreter outside archived text')
    body = text[off+5:off+0x1a0]  # allow existing entry detour
    sites = [(i, struct.unpack_from('<I', body, i+3)[0])
             for i in range(len(body)-7) if body[i:i+3] == b'\x8b\x14\x95']
    if len(sites) != 2 or sites[0][1] != sites[1][1]:
        raise ValueError('ambiguous dispatch table')
    if b'\x8b\x46\x10' not in body or b'\x8b\x46\x18\x0f\xb6\x10\x8b\x4e\x14' not in body:
        raise ValueError('frame Node/Code/Object layout not proven')
    print(json.dumps(dict(engine_sha256=hashlib.sha256(engine).hexdigest(),
        text_sha256=hashlib.sha256(text).hexdigest(), functions=functions,
        dispatch_sites=2, table=hex(sites[0][1]), node=16, code=24, object=20,
        evidence='offline structural identity, NOT live suppression or L4'), indent=2))


if __name__ == '__main__':
    main()
