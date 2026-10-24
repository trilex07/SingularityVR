"""Check fixture-space claims against ALL original cooked vertex positions.

Explicit private inputs; emits hashes/metrics only, never extracted geometry.
Does not assume the sign of the ActorX-to-native conversion.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "title"))
from ue3_layout import inspect


def audit(psk_path, package_path, mesh, mount):
    psk = psk_path.read_bytes()
    chunks = {}
    offset = 0
    while offset < len(psk):
        name, _, size, count = struct.unpack_from('<20siii', psk, offset)
        offset += 32
        if size < 0 or count < 0 or offset + size * count > len(psk):
            raise ValueError('invalid chunk')
        chunks[name.rstrip(b'\0')] = (size, count, psk[offset:offset+size*count])
        offset += size * count
    size, count, raw = chunks[b'PNTS0000']
    if size != 12:
        raise ValueError('unexpected point format')
    points = np.frombuffer(raw, '<f4').reshape(count, 3)
    size, count, raw = chunks[b'VTXW0000']
    wedges = np.array([struct.unpack_from('<I', raw, i*size)[0] for i in range(count)])
    source = points[wedges].astype(float)
    package = package_path.read_bytes()
    exports = inspect(package_path, data=package, include_all=True)['exports']
    matches = [e for e in exports if e['name'] == mesh and e['class_name'] == 'SkeletalMesh']
    if len(matches) != 1:
        raise ValueError('not one named mesh')
    export = matches[0]
    start, end = export['property_end'], export['offset'] + export['size']
    proven = []
    for sign in (1, -1):
        expected = source * [1, sign, 1]
        needle = struct.pack('<3f', *expected[0])
        at = package.find(needle, start, end)
        while at != -1:
            if at + count*32 <= end:
                actual = np.ndarray((count, 3), '<f4', package, at, (32, 4))
                if np.allclose(actual, expected, atol=2e-5, rtol=0):
                    proven.append((sign, at, actual.copy()))
            at = package.find(needle, at+1, end)
    if len(proven) != 1:
        raise ValueError('not one complete matching native stream')
    sign, at, actual = proven[0]
    distances = {str(s): float(np.linalg.norm(actual - np.array(mount)*[1, s, 1], axis=1).min())
                 for s in (1, -1)}
    return dict(mesh=mesh, vertices=count, y_sign=sign, native_offset=at,
                bounds=[actual.min(0).tolist(), actual.max(0).tolist()],
                mount_source=mount, nearest_vertex_distance_uu=distances,
                psk_sha256=hashlib.sha256(psk).hexdigest(),
                package_sha256=hashlib.sha256(package).hexdigest(),
                evidence='all named-mesh GPU positions, 32-byte stride; offline, not live shader matrices')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--psk', type=Path, required=True)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--mesh', required=True)
    parser.add_argument('--mount', type=float, nargs=3, required=True)
    args = parser.parse_args()
    print(json.dumps(audit(args.psk, args.package, args.mesh, args.mount), indent=2))
