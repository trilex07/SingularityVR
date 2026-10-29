"""Export the original Volk clip as a separate, untextured GLB.

The source PSK and SVWA remain authoritative. This is an inspection / authoring
asset, not a replacement for UE3's mesh or animation contract.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

import numpy as np
import trimesh

from prepare_centurion import extract, preview, read_psk


def export(psk_path: Path, output: Path):
    source = psk_path.read_bytes()
    psk = read_psk(psk_path)
    names, parents = psk[:2]
    if len(psk[6]) != 8297 or names[53:59] != [
        'cog_shotgun', 'release', 'clip', 'Slide', 'Trigger', 'muzzle_flash'
    ] or parents[55] != 53:
        raise ValueError('not the reviewed Volk S4 PSK hierarchy')
    cache, report = extract(psk)
    magic, version, count, index_count = struct.unpack_from('<4sIII', cache)
    if (magic, version, count, index_count) != (b'SVRK', 2, 242, 594):
        raise ValueError('unexpected magazine extraction')
    rows = np.array([struct.unpack_from('<3fI2f', cache, 16 + i * 24)
                     for i in range(count)], dtype=np.float64)
    faces = np.frombuffer(cache, '<u2', index_count, 16 + count * 24).reshape(-1, 3)
    if not np.isfinite(rows).all() or faces.max() >= count:
        raise ValueError('invalid exported geometry')
    mesh = trimesh.Trimesh(vertices=rows[:, :3], faces=faces, process=False)
    mesh.visual = trimesh.visual.TextureVisuals(uv=rows[:, 4:6])
    scene = trimesh.Scene()
    scene.add_geometry(mesh, node_name='volk_magazine', geom_name='volk_magazine')
    output.mkdir(parents=True, exist_ok=True)
    (output / 'volk_magazine.glb').write_bytes(scene.export(file_type='glb'))
    preview(cache, output / 'volk_magazine_preview.png', 'Volk S4 original clip')
    manifest = {
        'source_psk_sha256': hashlib.sha256(source).hexdigest(),
        'magazine_cache_sha256': hashlib.sha256(cache).hexdigest(),
        'glb_sha256': hashlib.sha256((output / 'volk_magazine.glb').read_bytes()).hexdigest(),
        'coordinate_space': 'original clip bone local; metres at 52.5 UU/m',
        'source_bones': {name: {'index': i, 'parent_index': parents[i]}
                         for i, name in enumerate(names) if i in (0, 52, 53, 54, 55, 56, 57, 58)},
        'extraction': report,
        'limitation': 'GLB contains magazine geometry only; original PSK/PSA skeletal contract stays authoritative',
    }
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    return manifest


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--psk', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    result = export(args.psk, args.output)
    print(json.dumps({key: result[key] for key in ('source_psk_sha256', 'glb_sha256')}, indent=2))
