"""Offline pinned native SoundNodeWave replacement, no installed-game writes.

Retains event scheduling, properties and native Vorbis loading. Relocates only
declared inline bulk offsets of the final INTRO exports. No assets in Git.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from ue3_sound_package import inspect

SOURCE_SHA = '890c61720abc0bb5afad86f16433a811a6f21ba6fc6284563bc5875e52642e85'
LOGICAL_SHA = 'b943231a13f335db0664b53b97033d031a15ae81263079dee0da9476b05410dd'
TAG = 0x9E2A83C1
TARGETS = {'INTRO_' + n for n in ('1005_NIM','1006_CP','1007_GST','1008_GST',
           '1009_NIM','1010_CP','1011_GST','1012_NIM','1014_GST','1016_CP')}


def require(ok, message):
    if not ok:
        raise ValueError(message)


def bulks(data, e):
    p = e['property_end']
    end = e['offset'] + e['size']
    result = []
    for i in range(4):
        require(p+16 <= end, 'bulk header outside export')
        flags, count, size, off = struct.unpack_from('<4I', data, p)
        require(flags == 0 and count == size and off == p+16, 'unsupported bulk layout')
        require(off+size <= end, 'bulk payload outside export')
        require((i == 1 and size > 0) or (i != 1 and size == 0), 'unexpected platform payload')
        result.append((p, data[off:off+size]))
        p = off+size
    require(p == end, 'unrecognized native wave footer')
    require(result[1][1].startswith(b'OggS'), 'PC audio not Vorbis')
    return result


def literals(data):
    require(len(data) >= 19, 'literal block too small')
    remaining = len(data)-18
    prefix = bytearray([0])
    while remaining > 255:
        prefix.append(0)
        remaining -= 255
    prefix.append(remaining)
    return bytes(prefix)+data+b'\x11\0\0'


def pack(logical, original):
    # This exact source summary is verified, not transferred from another
    # game/version. Chunk array at 0x61 and logical names at 0x71.
    require(struct.unpack_from('<2I', original, 0x61) == (2, 2), 'source summary changed')
    require(struct.unpack_from('<I', original, 0x69)[0] == 0x71, 'logical header changed')
    body = logical[0x71:]
    raw = [body[p:p+0x20000] for p in range(0, len(body), 0x20000)]
    chunks = [literals(part) for part in raw]
    container = struct.pack('<4I', TAG, 0x20000, sum(map(len,chunks)), len(body))
    container += b''.join(struct.pack('<2I', len(c),len(u)) for c,u in zip(chunks,raw))
    container += b''.join(chunks)
    header = bytearray(logical[:0x71])
    struct.pack_into('<2I', header, 0x61, 2, 1)
    record = struct.pack('<4I', 0x71, len(body), 0x81, len(container))
    physical = header[:0x69]+record+header[0x69:]+container
    return physical+b'\0'*((-len(physical))%65536)


def build(original, logical, replacements):
    require(hashlib.sha256(original).hexdigest() == SOURCE_SHA, 'unrecognized original package')
    require(hashlib.sha256(logical).hexdigest() == LOGICAL_SHA, 'logical input not pinned original')
    require(set(replacements) == TARGETS or not replacements, 'exact opening dialogue set required')
    package = inspect(logical)
    exports = package['exports']
    require(len(exports) == 143, 'export count changed')
    waves = [e for e in exports if e['class_name'] == 'SoundNodeWave']
    require(len(waves) == 138, 'wave count changed')
    native = {e['index']: bulks(logical, e) for e in waves}
    require({e['name'] for e in exports if e['name'] in TARGETS} == TARGETS, 'missing target')
    first = min(e['offset'] for e in exports)
    out = bytearray(logical[:first])
    changed = []
    for e in sorted(exports, key=lambda e:e['offset']):
        old_offset = e['offset']
        new_offset = len(out)
        blob = logical[old_offset:old_offset+e['size']]
        if e['index'] in native:
            prefix = logical[old_offset:e['property_end']]
            chunks = native[e['index']]
            pieces = bytearray(prefix)
            for i, (old_header, payload) in enumerate(chunks):
                if i == 1 and e['name'] in replacements:
                    payload = replacements[e['name']]
                    require(payload.startswith(b'OggS'), 'replacement is not Ogg')
                    # Vorbis identification packet: mono, 48kHz like source.
                    at = payload.find(b'\x01vorbis')
                    require(at >= 0 and payload[at+11] == 1 and
                            struct.unpack_from('<I', payload, at+12)[0] == 48000,
                            'replacement format mismatch')
                pieces += struct.pack('<4I', 0, len(payload), len(payload), new_offset+len(pieces)+16)
                pieces += payload
            blob = bytes(pieces)
        elif new_offset != old_offset:
            raise ValueError('unsupported relocation of non-wave export')
        if e['name'] in replacements:
            changed.append(e['name'])
        if new_offset == old_offset and e['name'] not in replacements:
            require(blob == logical[old_offset:old_offset+e['size']], 'nontarget changed')
        out += blob
        struct.pack_into('<2i', out, e['record_offset']+32, len(blob), new_offset)
    # Compare parsed properties and every platform payload, independent of relocation.
    after = inspect(bytes(out))
    for before, e in zip(exports, after['exports']):
        require(before['name'] == e['name'] and before['cls'] == e['cls'] and
                before['outer'] == e['outer'], 'identity changed')
        if before['index'] in native:
            a = logical[before['offset']:before['property_end']]
            b = out[e['offset']:e['property_end']]
            require(a == b, 'wave properties/event metadata changed')
            for i, ((hp, old), (np, new)) in enumerate(zip(native[before['index']], bulks(out,e))):
                expected = replacements.get(e['name'], old) if i == 1 else old
                require(new == expected, 'unexpected audio change')
    require(set(changed) == set(replacements), 'replacement coverage changed')
    return bytes(out), pack(out, original)


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('--original', type=Path, required=True)
    ap.add_argument('--logical', type=Path, required=True)
    ap.add_argument('--replacements', type=Path)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    replacements = {p.stem:p.read_bytes() for p in args.replacements.glob('INTRO_*.ogg')} if args.replacements else {}
    require(not args.out.exists() and not args.out.with_suffix('.u').exists(), 'refuse overwrite')
    logical, physical = build(args.original.read_bytes(), args.logical.read_bytes(), replacements)
    args.out.with_suffix('.u').write_bytes(logical)
    args.out.write_bytes(physical)
    print(json.dumps(dict(changed=sorted(replacements), sha256=hashlib.sha256(physical).hexdigest(),
                          logical_sha256=hashlib.sha256(logical).hexdigest(), size=len(physical))))
