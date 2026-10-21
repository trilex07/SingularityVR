"""Generate and validate 2K remastered DDS files for all SingularityVR weapon, muzzle, spark and impact VFX."""
import io
import struct
import os
from pathlib import Path
from PIL import Image, ImageFilter

REPO_ROOT = Path(__file__).resolve().parents[1]
BASE_ROOT = Path(r"C:\Users\Trilex\vr-mods")
TIER_8K_DIR = BASE_ROOT / "tex_tiers" / "tier_8K"
ASSETS_LOCAL = BASE_ROOT / "SingularityVR-assets-local"
DEST_DIRS = [
    REPO_ROOT / "deployment" / "texture-override",
    REPO_ROOT / "deployment" / "texture-cache" / "artifact" / "texture-override"
]

def require(ok, why):
    if not ok:
        raise ValueError(why)

def dds_validate(b):
    require(len(b) >= 128 and b[:4] == b'DDS ' and struct.unpack_from('<I', b, 4)[0] == 124, 'DDS header')
    h, w = struct.unpack_from('<2I', b, 12)
    m = struct.unpack_from('<I', b, 28)[0]
    fc = b[84:88].decode('ascii')
    require(0 < w <= 8192 and 0 < h <= 8192 and fc in ('DXT1', 'DXT5'), 'DDS format/dimensions')
    require(m == max(w, h).bit_length(), 'complete mip chain required')
    block = 8 if fc == 'DXT1' else 16
    expected = 128 + sum(max(1, ((w >> i) + 3) // 4) * max(1, ((h >> i) + 3) // 4) * block for i in range(m))
    require(len(b) == expected, f'DDS mip byte budget mismatch: got {len(b)} expected {expected}')
    return dict(w=w, h=h, mips=m, fourcc=fc, bytes=len(b))

def build_dds(img, fmt):
    w, h = img.size
    mips = [img]
    cur_w, cur_h = w, h
    while cur_w > 1 or cur_h > 1:
        cur_w, cur_h = max(1, cur_w // 2), max(1, cur_h // 2)
        mips.append(mips[-1].resize((cur_w, cur_h), Image.LANCZOS))
    m = len(mips)
    fourcc = fmt.encode('ascii')
    block = 8 if fmt == 'DXT1' else 16
    hdr = bytearray(128)
    hdr[0:4] = b'DDS '
    struct.pack_into('<I', hdr, 4, 124)
    flags = 0x1 | 0x2 | 0x4 | 0x8 | 0x1000 | 0x20000
    struct.pack_into('<I', hdr, 8, flags)
    struct.pack_into('<I', hdr, 12, h)
    struct.pack_into('<I', hdr, 16, w)
    pitch = max(1, (w + 3) // 4) * block
    struct.pack_into('<I', hdr, 20, pitch)
    struct.pack_into('<I', hdr, 28, m)
    struct.pack_into('<I', hdr, 76, 32)
    struct.pack_into('<I', hdr, 80, 0x4)
    hdr[84:88] = fourcc
    struct.pack_into('<I', hdr, 108, 0x1000 | 0x400008)
    out = bytearray(hdr)
    for level_img in mips:
        buf = io.BytesIO()
        level_img.convert('RGBA').save(buf, format='DDS', pixel_format=fmt)
        out.extend(buf.getvalue()[128:])
    raw = bytes(out)
    dds_validate(raw)
    return raw

def main():
    for d in DEST_DIRS:
        d.mkdir(parents=True, exist_ok=True)

    # 1. Effects from tier_8K downscaled/optimized to 2K sweetspot
    items = [
        ('muzzleflashCenter.dds.8K.png', 'muzzleflashCenter.dds', (2048, 2048), 'DXT1'),
        ('muzzleFlashDir_01.dds.8K.png', 'muzzleFlashDir_01.dds', (1024, 2048), 'DXT1'),
        ('brightFlash_01.dds.8K.png', 'brightFlash_01.dds', (2048, 2048), 'DXT1'),
        ('spark_hit01.dds.8K.png', 'spark_hit01.dds', (2048, 2048), 'DXT1'),
        ('sparks1_subuv.dds.8K.png', 'sparks1_subuv.dds', (256, 512), 'DXT1'),
        ('explosion.dds.8K.png', 'explosion.dds', (512, 512), 'DXT5'),
        ('fireball_base_intense_02.dds.8K.png', 'fireball_base_intense_02.dds', (2048, 2048), 'DXT5'),
        ('smoketrail_01.dds.8K.png', 'smoketrail_01.dds', (512, 256), 'DXT1'),
        ('tracer2.dds.8K.png', 'tracer2.dds', (128, 512), 'DXT1'),
        ('bulletImpactFlesh_d.dds.8K.png', 'bulletImpactFlesh_d.dds', (512, 512), 'DXT1'),
    ]

    for src_name, out_name, target_size, fmt in items:
        p = TIER_8K_DIR / src_name
        if not p.exists():
            print(f'Warning: {p} not found, skipping')
            continue
        im = Image.open(p).convert('RGBA')
        if im.size != target_size:
            im = im.resize(target_size, Image.LANCZOS)
        raw = build_dds(im, fmt)
        for dest in DEST_DIRS:
            (dest / out_name).write_bytes(raw)
        print(f'Generated {out_name:25} -> {target_size} {fmt} ({len(raw)} bytes)')

    # 2. t_decal_metal_d_v4 from original 128x128 -> 2048x2048 (2K) DXT5
    metal_src = ASSETS_LOCAL / "textures" / "original" / "RvGame" / "Texture2D" / "t_decal_metal_d_v4.dds"
    if metal_src.exists():
        im_metal = Image.open(metal_src).convert('RGBA')
        im_metal = im_metal.resize((2048, 2048), Image.LANCZOS).filter(ImageFilter.UnsharpMask(radius=2, percent=150, threshold=3))
        raw_metal = build_dds(im_metal, 'DXT5')
        for dest in DEST_DIRS:
            (dest / 't_decal_metal_d_v4.dds').write_bytes(raw_metal)
        print(f'Generated t_decal_metal_d_v4.dds    -> (2048, 2048) DXT5 ({len(raw_metal)} bytes)')

if __name__ == '__main__':
    main()
