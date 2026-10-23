"""Read-only private cooked/PSK influence audit; never emits proprietary geometry."""
import argparse
import ctypes
import hashlib
import json
import struct
from pathlib import Path
from prepare_weapon_animation import cooked_layout, SPECS, inspect, read_psk, fnv


def audit(package, psk, lzo):
    raw = package.read_bytes()
    assert struct.unpack_from('<I', raw)[0] == 0x9e2a83c1
    flag, count = struct.unpack_from('<2I', raw, 0x61)
    assert flag == 2 and 0 < count <= 64
    chunks = [struct.unpack_from('<4I', raw, 0x69+i*16) for i in range(count)]
    assert chunks[0][0] == 0x71
    lib = ctypes.CDLL(str(lzo.resolve()))
    lib.unpack.argtypes = [ctypes.c_void_p, ctypes.c_ulonglong, ctypes.c_void_p, ctypes.c_ulonglong]
    data = bytearray(raw[:0x69] + raw[0x69+count*16:0x71+count*16])
    struct.pack_into('<2I', data, 0x61, 0, 0)
    for logical, size, physical, physical_size in chunks:
        assert logical == len(data) and physical+physical_size <= len(raw)
        tag, block, compressed, total = struct.unpack_from('<4I', raw, physical)
        assert tag == 0x9e2a83c1 and 0 < block <= 0x200000 and total == size
        blocks = (total + block - 1) // block
        pos = physical + 16 + blocks*8
        for i in range(blocks):
            cs, us = struct.unpack_from('<II', raw, physical+16+i*8)
            assert 0 < us <= block and pos+cs <= physical+physical_size
            dest = ctypes.create_string_buffer(us)
            assert lib.unpack(raw[pos:pos+cs], cs, dest, us) == 0
            data.extend(dest.raw)
            pos += cs
        assert len(data) == logical+size and pos == physical+physical_size
    pkg = inspect(package, data=data, include_all=True)
    exports = [e for e in pkg['exports'] if e['name'] == SPECS['volk']['mesh'] and e['class_name'] == 'SkeletalMesh']
    assert len(exports) == 1
    e = exports[0]
    layout = cooked_layout(data, SPECS['volk'], e['offset'], e['offset']+e['size'])
    names, parents, local, bind, points, weights, wedges, faces = read_psk(psk)
    assert len(wedges) == 8297 and len(faces) == 10835
    assert layout['chunks'][0]['palette'] == [55, 56, 53, 54, 57]
    influences = [{b for w, b in weights[p] if w > 0} for p, u, v in wedges]
    assert set.union(*influences) == set(layout['chunks'][0]['palette'])
    referenced = {w for f in faces for w in f[:3]}
    psk_indices = [w for f in faces for w in (f[1], f[0], f[2])]
    psk_index_hash = fnv(struct.pack('<'+'H'*len(psk_indices), *psk_indices))
    assert psk_index_hash == layout['index_fnv'], 'PSK/cooked index identity differs'
    rows = []
    for slot, bone in enumerate(layout['chunks'][0]['palette']):
        rows.append(dict(slot=slot, register_if_base5=5+slot*3, bone=bone, name=names[bone],
                         wedges=sum(bone in x for x in influences),
                         referenced_wedges=sum(bone in influences[w] for w in referenced),
                         triangles=sum(any(bone in influences[w] for w in f[:3]) for f in faces)))
    return dict(package_sha256=hashlib.sha256(raw).hexdigest(),
                psk_sha256=hashlib.sha256(psk.read_bytes()).hexdigest(), layout=layout,
                psk_index_hash=psk_index_hash,
                mixed_wedges=sum(len(x)>1 for x in influences), influences=rows)


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--package', type=Path, required=True)
    p.add_argument('--psk', type=Path, required=True)
    p.add_argument('--lzo', type=Path, required=True)
    args = p.parse_args()
    print(json.dumps(audit(args.package, args.psk, args.lzo), indent=2))
