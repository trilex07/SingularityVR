"""Offline falsifier: cooked chunk order + ActorX weights vs authored policy.

All game-derived files and detailed results must stay outside Git. This tool
reads assets only; it never writes packages or touches the installed game.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cooked', type=Path, required=True)
    ap.add_argument('--psk', type=Path, required=True)
    ap.add_argument('--header', type=Path, required=True)
    args = ap.parse_args()
    data = args.cooked.read_bytes()
    pattern = struct.pack('<ihhiHhhiH', 2, 0, 0, 0, 4830, 1, 1, 14490, 674)
    at = data.find(pattern)
    assert at >= 0 and data.find(pattern, at+1) == -1
    p = at+len(pattern)

    def integer():
        nonlocal p
        result = struct.unpack_from('<i', data, p)[0]
        p += 4
        return result

    def array(fmt):
        nonlocal p
        count = integer()
        assert 0 <= count < 100000
        size = struct.calcsize(fmt)
        result = [struct.unpack_from(fmt, data, p+i*size)[0] for i in range(count)]
        p += count*size
        return result

    assert integer() == 2
    indices = array('<H')
    array('<H'); array('<H'); array('<B')
    count = integer()
    assert count == 2
    maps = []
    for _ in range(count):
        first = integer()
        assert integer() == integer() == 0
        palette = array('<H')
        rigid, soft, influences = integer(), integer(), integer()
        maps.append(dict(first=first, vertices=rigid+soft, palette=palette))
    assert maps[0]['first'] == 0 and maps[0]['vertices'] == 2924
    assert len(indices) == 16512
    source = args.header.read_text(encoding='utf-8')
    text = source.split('kNoTmdCookedPalette[] = {', 1)[1].split('};', 1)[0]
    candidate = [int(n) for n in re.findall(r'\d+', text)]
    assert candidate == maps[0]['palette'], 'candidate not actual GPU slot order'

    raw = args.psk.read_bytes()
    p = 0
    chunks = {}
    while p < len(raw):
        name, kind, size, count = struct.unpack_from('<20siii', raw, p)
        p += 32
        assert 0 <= size < 10000 and 0 <= count < 1000000 and p+size*count <= len(raw)
        chunks[name.split(b'\0')[0].decode()] = (size, count, raw[p:p+size*count])
        p += size*count
    assert p == len(raw)
    size, count, records = chunks['REFSKELT']
    names = [records[i*size:i*size+64].split(b'\0')[0].decode() for i in range(count)]
    size, count, records = chunks['RAWWEIGHTS']
    weights = {}
    for i in range(count):
        w, point, bone = struct.unpack_from('<fii', records, i*size)
        if w > 0:
            assert 0 <= bone < len(names)
            weights.setdefault(point, []).append((w, bone))
    size, count, records = chunks['VTXW0000']
    wedges = [struct.unpack_from('<I', records, i*size)[0] for i in range(count)]
    size, count, records = chunks['FACE0000']
    points = set()
    for i in range(count):
        a, b, c, material, aux, smoothing = struct.unpack_from('<3HBBI', records, i*size)
        if material == 0:
            points.update(wedges[w] for w in (a, b, c))
    contributors = {b for point in points for w, b in weights[point]}
    assert contributors == set(candidate), 'PSK and cooked chunk disagree'
    side = lambda b: 1 if names[b].startswith('r_') else 0 if names[b].startswith('l_') or names[b] == 'tmd_base' else -1
    finger = lambda b: '_wrist' in names[b] or any(t in names[b] for t in ('_thumb_', '_index_', '_middle_', '_ring_', '_pinky'))
    mixed = 0
    max_split_error = 0
    for point in points:
        ws = weights[point]
        assert abs(sum(w for w, b in ws)-1) < 0.001
        assert len({side(b) for w, b in ws}) == 1 and side(ws[0][1]) >= 0
        if any(finger(b) for w, b in ws) and any(not finger(b) for w, b in ws):
            mixed += 1
        # 40 UU controller translation, identity native palette: all weighted
        # contributors must translate by 40 UU. Old wrist-only policy fails.
        old = sum(w*40 for w, b in ws if finger(b))
        fixed = sum(w*40 for w, b in ws)
        assert abs(fixed-40) < 0.04
        max_split_error = max(max_split_error, abs(fixed-old))
    print(json.dumps(dict(status='L1_PASS', cooked_sha256=hashlib.sha256(data).hexdigest(),
                          psk_sha256=hashlib.sha256(raw).hexdigest(), slots=len(candidate),
                          weighted_points=len(points), mixed_points=mixed,
                          old_max_translation_error_UU=max_split_error), indent=2))


if __name__ == '__main__':
    main()
