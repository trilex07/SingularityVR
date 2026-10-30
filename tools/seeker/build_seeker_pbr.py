"""Author restrained 4K Seeker PBR maps on the verified original UV layout.

Adds independent micro-height and directional machining to the original
material, then composes tangent normals. This is a private derivative; output
maps must remain outside Git with the game source textures.
"""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

SOURCE = {'diffuse': 'ecb0b1f9245505115407503ea0cab7f2262e50325aaee73886a8e60196d26aeb'}
SIZE = (4096, 2048)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(array, path):
    Image.fromarray(np.clip(array, 0, 255).astype('uint8'), 'RGB' if array.ndim == 3 else 'L').save(path)
    return sha(path)


def build(diffuse, normal, specular, output):
    if sha(diffuse) != SOURCE['diffuse']:
        raise ValueError('diffuse does not match audited Seeker source')
    output.mkdir(parents=True, exist_ok=True)
    base = Image.open(diffuse).convert('RGB')
    norm = Image.open(normal).convert('RGB')
    spec = Image.open(specular).convert('RGB')
    if base.size != (2048, 1024) or norm.size != base.size or spec.size != (256, 128):
        raise ValueError('unreviewed source map dimensions')
    albedo = np.asarray(base.resize(SIZE, Image.Resampling.LANCZOS), dtype=np.float32)
    shine = np.asarray(spec.resize(SIZE, Image.Resampling.BICUBIC), dtype=np.float32).mean(2) / 255
    # Reconstruct narrow, restrained tooling marks at the *new* texel scale.
    # Pixelwise material masks gate detail away from black unused UV regions.
    luminance = np.mean(albedo, axis=2)
    valid = np.clip((luminance - 8) / 22, 0, 1)
    metal = np.clip((shine - .13) / .42, 0, 1) * valid
    rng = np.random.default_rng(0x5EEC0)
    height = Image.new('L', SIZE, 128)
    draw = ImageDraw.Draw(height)
    for _ in range(10500):
        x = int(rng.integers(0, SIZE[0])); y = int(rng.integers(0, SIZE[1]))
        length = int(rng.integers(5, 46))
        draw.line((x, y, min(x + length, SIZE[0]-1), y),
                  fill=int(rng.integers(105, 152)), width=1)
    height = np.asarray(height.filter(ImageFilter.GaussianBlur(.32)), dtype=np.float32)
    height = (height - 128) * metal
    noise = rng.normal(0, 1.0, size=height.shape).astype(np.float32) * valid
    # Small authored machining/wear response; broad original color variation
    # remains intact, while roughness and normals now carry 4K-only structure.
    detail = height * .16 + noise * .58
    albedo = np.clip(albedo + detail[:, :, None] * (.35 + .65 * metal[:, :, None]), 0, 255)
    base_hash = save(albedo, output / 'seeker_albedo_4k.png')
    original_normal = np.asarray(norm.resize(SIZE, Image.Resampling.BICUBIC), dtype=np.float32)
    normal_xy = (original_normal[:, :, :2] - 127.5) / 127.5
    dy, dx = np.gradient(height * .006 + noise * .0015)
    normal_xy[:, :, 0] -= dx
    normal_xy[:, :, 1] -= dy
    nz = np.sqrt(np.maximum(1 - np.minimum(np.sum(normal_xy**2, axis=2), .98), .02))
    vector = np.dstack((normal_xy, nz))
    vector /= np.linalg.norm(vector, axis=2, keepdims=True)
    normal_hash = save((vector * .5 + .5) * 255, output / 'seeker_normal_dx_4k.png')
    roughness = np.clip((.79 - .41 * shine) * 255 + noise * 4 - height * .25, 32, 235)
    rough_hash = save(roughness, output / 'seeker_roughness_4k.png')
    metallic = np.where(metal > .48, 228, np.where(metal > .18, 126, 12)).astype(np.uint8)
    metal_hash = save(metallic, output / 'seeker_metallic_4k.png')
    report = {'source': {p.name: sha(p) for p in (diffuse, normal, specular)},
              'size': SIZE, 'uv': 'original PSK upper-left V',
              'normal': 'DirectX tangent Y; source n map preserved and 4K micro-height composed',
              'authored_detail': {'seed': 0x5EEC0, 'machining_marks': 10500,
                                  'gated_to_visible_specular_regions': True},
              'maps': {'albedo': base_hash, 'normal_dx': normal_hash,
                       'roughness': rough_hash, 'metallic': metal_hash},
              'limits': 'material role inferred from source specular; verify metallic mask in Blender and native shader before deployment'}
    (output / 'pbr_manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for k in ('diffuse', 'normal', 'specular', 'output'):
        p.add_argument('--' + k, required=True, type=Path)
    a = p.parse_args()
    print(json.dumps(build(a.diffuse, a.normal, a.specular, a.output), indent=2))
