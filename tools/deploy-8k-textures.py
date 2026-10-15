"""Deploy 8K texture tier and 8K loading screens to Steam Binaries/texture-override-8K.

Validates all 58 remaster textures + 11 loading screens against strict DDS gates:
- Format DXT1 or DXT5 per TASK.json / engine identity
- Full mip chain down to 1x1
- Max dimension <= 8192 (D3D9 hardware cap)
- Byte budget exact matching
"""
import io
import json
import os
import struct
import hashlib
from pathlib import Path
from PIL import Image

Image.MAX_IMAGE_PIXELS = None

REPO_ROOT = Path(__file__).resolve().parents[1]
BASE_ROOT = Path(r"C:\Users\Trilex\vr-mods")
STEAM_DIR = Path(r"C:\Program Files (x86)\Steam\steamapps\common\Singularity")
OUT_DIR = STEAM_DIR / "Binaries" / "texture-override-8K"
TIER_8K_DIR = BASE_ROOT / "tex_tiers" / "tier_8K"
TASK_JSON = BASE_ROOT / "tex_tiers" / "TASK.json"
LOAD_8K_DIR = Path(r"C:\Users\Trilex\AppData\Local\Temp\opencode\load4k_tmp_pass1")

LOADING_SCREENS = [
    "CVS_Manufacturing",
    "CVS_Village",
    "CVS_Wasteland",
    "TDM_Manufacturing_A",
    "TDM_Manufacturing_B",
    "TDM_Manufacturing_C",
    "TDM_Village_A",
    "TDM_Village_B",
    "TDM_Wasteland_A",
    "TDM_Wasteland_B",
    "TDM_Wasteland_C",
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
    if w > 8192 or h > 8192:
        scale = 8192.0 / max(w, h)
        w, h = int(round(w * scale)), int(round(h * scale))
        # ensure multiples of 4 for block compression
        w = max(4, (w // 4) * 4)
        h = max(4, (h // 4) * 4)
        img = img.resize((w, h), Image.LANCZOS)
    
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
    info = dds_validate(raw)
    return raw, info

def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    manifest = []
    
    # 1. 58 Textures from TASK.json
    tasks = json.loads(TASK_JSON.read_text(encoding='utf-8'))
    print(f"Encoding {len(tasks)} textures from TASK.json...")
    for idx, t in enumerate(tasks, 1):
        target = t['target']
        fourcc = t['fourcc']
        src_png = TIER_8K_DIR / f"{target}.8K.png"
        require(src_png.exists(), f"Missing source PNG: {src_png}")
        
        dst_dds = OUT_DIR / target
        with Image.open(src_png) as im:
            raw, info = build_dds(im, fourcc)
        dst_dds.write_bytes(raw)
        sha = hashlib.sha256(raw).hexdigest()
        manifest.append({
            "target": target,
            "type": "game_texture",
            "info": info,
            "sha256": sha
        })
        print(f"[{idx:02d}/{len(tasks)}] {target:28s} {info['w']}x{info['h']} {info['fourcc']} {info['mips']} mips ({info['bytes'] / (1024*1024):.1f} MB)")
    
    # 2. 11 Loading Screens
    print(f"\nEncoding {len(LOADING_SCREENS)} loading screens in 8K...")
    for idx, name in enumerate(LOADING_SCREENS, 1):
        target = f"{name}.dds"
        fourcc = "DXT1"
        src_png = LOAD_8K_DIR / f"{name}.png"
        require(src_png.exists(), f"Missing source loading screen PNG: {src_png}")
        
        dst_dds = OUT_DIR / target
        with Image.open(src_png) as im:
            raw, info = build_dds(im, fourcc)
        dst_dds.write_bytes(raw)
        sha = hashlib.sha256(raw).hexdigest()
        manifest.append({
            "target": target,
            "type": "loading_screen",
            "info": info,
            "sha256": sha
        })
        print(f"[{idx:02d}/{len(LOADING_SCREENS)}] {target:28s} {info['w']}x{info['h']} {info['fourcc']} {info['mips']} mips ({info['bytes'] / (1024*1024):.1f} MB)")
    
    # 3. Splash.bmp boot splash backup and update
    splash_png = LOAD_8K_DIR / "Splash.png"
    if splash_png.exists():
        splash_bmp = STEAM_DIR / "RvGame" / "Splash" / "PC" / "Splash.bmp"
        splash_bak = STEAM_DIR / "RvGame" / "Splash" / "PC" / "Splash.orig.bmp"
        if splash_bmp.exists() and not splash_bak.exists():
            splash_bak.write_bytes(splash_bmp.read_bytes())
            print(f"Backed up original Splash.bmp to {splash_bak}")
        with Image.open(splash_png) as im:
            # Resize 16384x8192 to 4096x2048 for Windows GDI compatibility
            splash_img = im.resize((4096, 2048), Image.LANCZOS).convert("RGB")
            splash_img.save(splash_bmp, format="BMP")
            print(f"Updated Splash.bmp to 4096x2048 BMP ({splash_bmp.stat().st_size / (1024*1024):.1f} MB)")
    
    # 4. Write manifest
    manifest_path = OUT_DIR / "MANIFEST_8K.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    total_bytes = sum(m['info']['bytes'] for m in manifest)
    print(f"\nSUCCESS: Deployed {len(manifest)} 8K DDS textures ({total_bytes / (1024*1024):.1f} MB total) to {OUT_DIR}")

if __name__ == "__main__":
    main()
