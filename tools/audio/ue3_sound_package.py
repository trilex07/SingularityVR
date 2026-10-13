"""Bounded offline UE3 v584 SoundNodeWave inspector. No Steam writes.

Package layout independently checked against uepkg; assets stay outside Git.
"""
import argparse
import json
import struct
from pathlib import Path


def inspect(data):
    def ints(p, n):
        assert 0 <= p <= len(data) - 4*n
        return struct.unpack_from('<' + 'i'*n, data, p)

    def string(p, end):
        n = ints(p, 1)[0]
        assert 0 < abs(n) < 100000
        stop = p + 4 + abs(n)*(2 if n < 0 else 1)
        assert stop <= end
        raw = data[p+4:stop]
        assert raw.endswith(b'\0\0' if n < 0 else b'\0')
        return raw.decode('utf-16le' if n < 0 else 'latin1').rstrip('\0'), stop

    assert struct.unpack_from('<I', data)[0] == 0x9E2A83C1
    assert struct.unpack_from('<H', data, 4)[0] == 584
    folder, flags_at = string(12, len(data))
    nc, no, ec, eo, ic, io = ints(flags_at+4, 6)
    assert 0 < nc < 100000 and 0 < ec < 100000 and 0 < ic < 100000
    names = []
    p = no
    for _ in range(nc):
        name, p = string(p, len(data))
        names.append(name)
        p += 8
    assert p == io

    def name(i):
        assert 0 <= i < len(names)
        return names[i]

    imports = [ints(io+j*28, 7) for j in range(ic)]
    exports = []
    p = eo
    for j in range(ec):
        e = struct.unpack_from('<6iQ4i', data, p)
        assert 0 <= e[10] <= 32
        assert 0 <= e[8] <= len(data)-e[7] and e[7] >= 0
        exports.append(dict(index=j+1, name=name(e[3]), cls=e[0], outer=e[2],
                            size=e[7], offset=e[8], record_offset=p))
        p += 68 + e[10]*4

    def ref(i):
        assert -ic <= i <= ec
        return name(imports[-i-1][5]) if i < 0 else exports[i-1]['name'] if i else 'None'

    for e in exports:
        e['class_name'] = ref(e['cls'])
        if e['class_name'] != 'SoundNodeWave':
            continue
        props = []
        p = e['offset'] + 4
        end = e['offset'] + e['size']
        for _ in range(256):
            tag = p
            ni, number = ints(p, 2)
            p += 8
            nm = name(ni)
            assert number == 0
            if nm == 'None':
                break
            ti, tn, size, idx = ints(p, 4)
            p += 16
            typ = name(ti)
            assert tn == 0 and 0 <= size <= end-p
            if typ == 'BoolProperty':
                p += 4
            elif typ == 'StructProperty':
                p += 8
            assert p+size <= end
            value = dict(bytes=size)
            if typ == 'StrProperty':
                value, stop = string(p, p+size)
                assert stop == p+size
            elif typ == 'FloatProperty' and size == 4:
                value = struct.unpack_from('<f', data, p)[0]
            elif typ == 'IntProperty' and size == 4:
                value = ints(p, 1)[0]
            elif typ == 'ArrayProperty' and nm == 'Subtitles':
                # Array of FSubtitleCue: FString text followed by float time.
                cues = []
                count = ints(p, 1)[0]
                assert 0 <= count <= 128
                q = p+4
                for _ in range(count):
                    text, q = string(q, p+size)
                    assert q+4 <= p+size
                    time = struct.unpack_from('<f', data, q)[0]
                    q += 4
                    cues.append(dict(text=text, time=time))
                assert q == p+size
                value = cues
            props.append(dict(name=nm, type=typ, value=value, tag_offset=tag,
                              value_offset=p, size=size))
            p += size
        else:
            raise ValueError('unterminated properties')
        e['properties'] = props
        e['property_end'] = p
    return dict(names=names, exports=exports, flags_at=flags_at)


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('package', type=Path)
    ap.add_argument('--prefix', default='INTRO_')
    args = ap.parse_args()
    data = args.package.read_bytes()
    package = inspect(data)
    for e in package['exports']:
        if e['name'].startswith(args.prefix):
            print(json.dumps(e, ensure_ascii=False))
            print('footer', data[e['property_end']:e['property_end']+112].hex(' '))
