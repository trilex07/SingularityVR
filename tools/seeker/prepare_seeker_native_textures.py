"""Prepare private UE3-compatible Seeker DDS maps from authored HD source."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'textures'))
from dds_io import load_dds, save_dds


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build(package):
    source = package / 'source'
    maps = package / 'textures'
    output = package / 'native_textures'
    output.mkdir(exist_ok=True)
    original = Image.open(source / 'ar_steerable_d.png').convert('RGBA')
    hd = Image.open(maps / 'seeker_albedo_4k.png').convert('RGBA')
    if original.size != (2048, 1024) or hd.size != (4096, 2048):
        raise ValueError('unexpected original/HD size')
    # Keep source alpha semantics: this is native material data, not glTF opacity.
    hd.putalpha(original.getchannel('A').resize(hd.size, Image.Resampling.BILINEAR))
    normal = Image.open(maps / 'seeker_normal_dx_4k.png').convert('RGBA')
    normal.putalpha(Image.open(source / 'ar_steerable_n.png').getchannel('A').resize(
        normal.size, Image.Resampling.BILINEAR))
    result = {}
    for label, image, format in (('AR_Steerable_d.dds', hd, 'DXT5'),
                                 ('AR_Steerable_n.dds', normal, 'DXT1')):
        path = output / label
        header = save_dds(path, image, format)
        decoded, verified = load_dds(path)
        if verified['width'] != 4096 or verified['height'] != 2048 or decoded.size != image.size:
            raise ValueError('DDS roundtrip failed')
        if np.asarray(decoded).shape != (2048, 4096, 4):
            raise ValueError('DDS decode shape')
        result[label] = {'sha256': sha(path), 'header': header}
    report = {'source_rgba_sha256': {p.name: sha(p) for p in source.glob('ar_steerable_*.png')},
              'native_alpha': 'resampled source alpha preserved',
              'normal': 'DirectX tangent Y; no GLB green-channel inversion',
              'material': 'offline DDS candidates only; UE3 binding and stereo appearance unverified',
              'files': result}
    (output / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--package', type=Path, required=True)
    print(json.dumps(build(p.parse_args().package), indent=2))
