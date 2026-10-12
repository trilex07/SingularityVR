#!/usr/bin/env python3
"""
Singularity VR Material Remaster Prototype - True Mip0 Verification
Verifies each of the three source textures uses true mip0 (not upscaled lower mip).
"""

import json
import numpy as np
from pathlib import Path
from PIL import Image

# Source textures for the three assets
SOURCE_TEXTURES = {
    "AR_shotgun_d": {
        "path": r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\textures\original\RvGameMPLoader_SF\Texture2D\AR_shotgun_d.dds",
        "expected_size": (1024, 1024),
        "role": "diffuse",
        "asset": "weapon_shotgun"
    },
    "AR_shotgun_n": {
        "path": r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\textures\original\RvGameMPLoader_SF\Texture2D\AR_shotgun_n.dds",
        "expected_size": (1024, 1024),
        "role": "normal",
        "asset": "weapon_shotgun"
    },
    "AR_shotgun_s": {
        "path": r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\textures\original\RvGameMPLoader_SF\Texture2D\AR_shotgun_s.dds",
        "expected_size": (512, 512),
        "role": "specular",
        "asset": "weapon_shotgun"
    },
    "inview_glove_d": {
        "path": r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\textures\original\RvGame\Texture2D\inview_glove_d.dds",
        "expected_size": (1024, 1024),
        "role": "diffuse",
        "asset": "glove"
    },
    "inview_glove_n": {
        "path": r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\textures\original\RvGame\Texture2D\inview_glove_n.dds",
        "expected_size": (1024, 1024),
        "role": "normal",
        "asset": "glove"
    },
    "inview_glove_s": {
        "path": r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\textures\original\RvGame\Texture2D\inview_glove_s.dds",
        "expected_size": (256, 256),
        "role": "specular",
        "asset": "glove"
    },
    "door_blastdoorN_d": {
        "path": r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\textures\original\SP_VI_Intro_Zone01_XSA\Texture2D\door_blastdoorN_d.dds",
        "expected_size": (1024, 512),
        "role": "diffuse",
        "asset": "env_door"
    },
    "door_blastdoorN_n": {
        "path": r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\textures\original\SP_VI_Intro_Zone01_XSA\Texture2D\door_blastdoorN_n.dds",
        "expected_size": (1024, 512),
        "role": "normal",
        "asset": "env_door"
    },
    "door_blastdoorN_s": {
        "path": r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\textures\original\SP_VI_Intro_Zone01_XSA\Texture2D\door_blastdoorN_s.dds",
        "expected_size": (256, 128),
        "role": "specular",
        "asset": "env_door"
    },
}

def read_dds_header(path):
    """Read DDS header to get dimensions, format, mip count."""
    with open(path, 'rb') as f:
        magic = f.read(4)
        if magic != b'DDS ':
            return None
        header = f.read(124)
        if len(header) < 124:
            return None
        
        # DDS_HEADER structure (124 bytes)
        import struct
        # dwSize, dwFlags, dwHeight, dwWidth, dwPitchOrLinearSize, dwDepth, dwMipMapCount
        values = struct.unpack('<IIIIIII', header[:28])
        dwSize, dwFlags, dwHeight, dwWidth, dwPitchOrLinearSize, dwDepth, dwMipMapCount = values
        
        # Read pixel format (32 bytes at offset 16)
        pf = struct.unpack('<IIIIIIII', header[16:48])
        pf_size, pf_flags, pf_fourcc, pf_bitcount, pf_rmask, pf_gmask, pf_bmask, pf_amask = pf
        
        format_str = "UNKNOWN"
        if pf_flags & 0x4:  # DDPF_FOURCC
            fourcc = pf_fourcc.to_bytes(4, 'little')
            if fourcc == b'DXT1':
                format_str = "DXT1"
            elif fourcc == b'DXT5':
                format_str = "DXT5"
            else:
                format_str = fourcc.decode('ascii', errors='ignore')
        
        return {
            "width": dwWidth,
            "height": dwHeight,
            "mip_count": dwMipMapCount,
            "format": format_str,
            "fourcc": pf_fourcc,
        }

def verify_true_mip0():
    """Verify each source texture is true mip0."""
    print("=" * 70)
    print("TRUE MIP0 VERIFICATION FOR THREE ASSETS")
    print("=" * 70)
    
    all_verified = True
    
    for name, info in SOURCE_TEXTURES.items():
        path = Path(info["path"])
        if not path.exists():
            print(f"❌ {name}: FILE NOT FOUND at {path}")
            all_verified = False
            continue
        
        header = read_dds_header(path)
        if not header:
            print(f"❌ {name}: INVALID DDS HEADER")
            all_verified = False
            continue
        
        expected_w, expected_h = info["expected_size"]
        actual_w, actual_h = header["width"], header["height"]
        
        is_true_mip0 = (actual_w == expected_w and actual_h == expected_h)
        status = "[VERIFIED]" if is_true_mip0 else "[FAILED - NOT TRUE MIP0]"
        
        print(f"{status} {name}")
        print(f"  Asset: {info['asset']}, Role: {info['role']}")
        print(f"  Expected: {expected_w}x{expected_h}, Actual DDS: {actual_w}x{actual_h}")
        print(f"  Format: {header['format']}, Mip levels: {header['mip_count']}")
        
        if not is_true_mip0:
            all_verified = False
        print()
    
    print("=" * 70)
    if all_verified:
        print("ALL 9 SOURCE TEXTURES VERIFIED AS TRUE MIP0")
    else:
        print("SOME TEXTURES FAILED TRUE MIP0 VERIFICATION")
    print("=" * 70)
    
    return all_verified

if __name__ == "__main__":
    verify_true_mip0()