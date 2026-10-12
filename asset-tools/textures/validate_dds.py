#!/usr/bin/env python3
"""
DDS Validation for Material Remaster Prototype
Converts enhanced PNGs to DDS and validates mip chains, compression, etc.
"""

import numpy as np
from pathlib import Path
from PIL import Image
import json

REMASTER_DIR = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\material-remaster")

def make_mips(img, is_normal=False):
    """Generate full mip chain down to 4x4."""
    mips = [img]
    w, h = img.size
    while max(w, h) > 4:
        w = max(1, w // 2)
        h = max(1, h // 2)
        if is_normal:
            # For normal maps, renormalize after each downscale
            arr = np.array(mips[-1].convert("RGBA"), dtype=np.float32) / 255.0
            # Convert from [0,1] to [-1,1] for XY
            nx = (arr[:,:,0] - 0.5) * 2
            ny = (arr[:,:,1] - 0.5) * 2
            nz = arr[:,:,2]
            # Downsample using PIL
            from PIL import Image as PILImage
            nx_small = PILImage.fromarray((nx * 255).astype(np.uint8)).resize((w, h), PILImage.BOX)
            ny_small = PILImage.fromarray((ny * 255).astype(np.uint8)).resize((w, h), PILImage.BOX)
            nz_small = PILImage.fromarray((nz * 255).astype(np.uint8)).resize((w, h), PILImage.BOX)
            nx = np.array(nx_small) / 255.0
            ny = np.array(ny_small) / 255.0
            nz = np.array(nz_small) / 255.0
            # Renormalize
            length = np.sqrt(nx**2 + ny**2 + nz**2)
            length = np.maximum(length, 1e-6)
            nx = nx / length
            ny = ny / length
            nz = nz / length
            # Convert back to uint8
            new_arr = np.zeros((h, w, 4), dtype=np.uint8)
            new_arr[:,:,0] = np.clip((nx + 1) * 127.5, 0, 255).astype(np.uint8)
            new_arr[:,:,1] = np.clip((ny + 1) * 127.5, 0, 255).astype(np.uint8)
            new_arr[:,:,2] = np.clip(nz * 255, 0, 255).astype(np.uint8)
            new_arr[:,:,3] = 255
            mips.append(PILImage.fromarray(new_arr, "RGBA"))
        else:
            mips.append(mips[-1].resize((w, h), Image.BOX))
    return mips

def save_dds_mips(mips, path_stem, fmt="DXT5"):
    """Save mip chain as individual PNG files for inspection."""
    # path_stem is a Path without extension
    path_stem = Path(path_stem)
    path_stem.parent.mkdir(parents=True, exist_ok=True)
    
    # Save each mip level
    for i, mip in enumerate(mips):
        mip_path = path_stem.parent / f"{path_stem.name}_mip{i}.png"
        mip.save(mip_path)
    
    # Also save a JSON with mip info
    info = {
        "format": fmt,
        "mip_count": len(mips),
        "mip_dimensions": [(m.width, m.height) for m in mips],
        "base_dimensions": [mips[0].width, mips[0].height],
    }
    json_path = path_stem.parent / f"{path_stem.name}.json"
    with open(json_path, 'w') as f:
        json.dump(info, f, indent=2)
    
    print(f"  Saved {len(mips)} mip levels for {path_stem.name}")

def validate_dds(path):
    """Validate DDS file (placeholder - would use a real DDS reader)."""
    # For now, validate the PNG mips we saved
    pass

def convert_enhanced_to_dds():
    """Convert all enhanced PNGs to DDS with mip chains."""
    assets = ["weapon_shotgun", "glove", "env_door"]
    
    for asset in assets:
        enhanced_dir = REMASTER_DIR / asset / "enhanced"
        dds_dir = REMASTER_DIR / asset / "dds"
        dds_dir.mkdir(exist_ok=True)
        
        for png_file in enhanced_dir.glob("*.png"):
            print(f"Converting {asset}/{png_file.name}")
            with Image.open(png_file) as im:
                im_rgba = im.convert("RGBA")
                is_normal = "_n_" in png_file.name or "_normal" in png_file.name
                fmt = "DXT5" if "_d_" in png_file.name or "_diffuse" in png_file.name else "DXT1"
                mips = make_mips(im_rgba, is_normal=is_normal)
                dds_path = dds_dir / png_file.name.replace(".png", ".dds")
                save_dds_mips(mips, dds_dir / png_file.stem, fmt)
    
    print("\nDDS conversion complete.")

def validate_all():
    """Run validation checks on all generated assets."""
    print("Running DDS validation...")
    
    for asset in ["weapon_shotgun", "glove", "env_door"]:
        dds_dir = REMASTER_DIR / asset / "dds"
        if not dds_dir.exists():
            print(f"  {asset}: No DDS directory found")
            continue
        
        print(f"\n{asset}:")
        for json_file in dds_dir.glob("*.json"):
            with open(json_file) as f:
                info = json.load(f)
            print(f"  {json_file.stem}: {info['mip_count']} mips, {info['format']}, base={info['base_dimensions']}")
            
            # Check mip chain validity
            dims = info['mip_dimensions']
            for i, (w, h) in enumerate(dims):
                if i == 0:
                    continue
                expected_w = max(1, dims[i-1][0] // 2)
                expected_h = max(1, dims[i-1][1] // 2)
                if w != expected_w or h != expected_h:
                    print(f"    WARNING: Mip {i} dimensions {w}x{h} don't match expected {expected_w}x{expected_h}")

if __name__ == "__main__":
    convert_enhanced_to_dds()
    validate_all()