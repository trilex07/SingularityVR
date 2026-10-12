#!/usr/bin/env python3
"""
Singularity VR Material Remaster Prototype
Analyzes and enhances three representative assets:
1. First-person weapon (AR_shotgun)
2. First-person glove (inview_glove)
3. Environment material (door_blastdoorN)
"""

import json
import numpy as np
from pathlib import Path
from PIL import Image, ImageFilter, ImageEnhance, ImageOps
import shutil

# Paths
LOCAL_ROOT = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local")
ORIGINAL_DIR = LOCAL_ROOT / "textures" / "original"
REMASTER_DIR = LOCAL_ROOT / "material-remaster"
METADATA_PATH = Path(r"C:\Users\Trilex\vr-mods\SingularityVR\research\texture-upscale\manifests\texture_metadata.json")
UPSCALE_RESULTS = Path(r"C:\Users\Trilex\vr-mods\SingularityVR\research\texture-upscale\manifests\upscale_results.json")

# Three representative assets
ASSETS = {
    "weapon_shotgun": {
        "name": "AR_shotgun",
        "material": "AR_Shotgun",
        "maps": {
            "diffuse": "AR_shotgun_d",
            "normal": "AR_shotgun_n",
            "specular": "AR_shotgun_s",
            "emissive": "AR_shotgun_e",
            "rim_mask": "AR_shotgun_rim",
        },
        "source_package": "RvGameMPLoader_SF",
        "type": "weapon",
    },
    "glove": {
        "name": "inview_glove",
        "material": "inview_gloves_gauze",
        "maps": {
            "diffuse": "inview_glove_d",
            "normal": "inview_glove_n",
            "specular": "inview_glove_s",
            "detail_normal": "DETAILfinepitted02a_n",
        },
        "source_package": "RvGame",
        "type": "glove",
    },
    "env_door": {
        "name": "door_blastdoorN",
        "material": "door_blastdoorN",
        "maps": {
            "diffuse": "door_blastdoorN_d",
            "normal": "door_blastdoorN_n",
            "specular": "door_blastdoorN_s",
        },
        "source_package": "SP_VI_Intro_Zone01_XSA",
        "type": "environment",
    },
}

def load_metadata():
    with open(METADATA_PATH, 'r', encoding='utf-8') as f:
        return json.load(f)

def load_upscale_results():
    with open(UPSCALE_RESULTS, 'r', encoding='utf-8') as f:
        return json.load(f)

def find_source_dds(metadata, texture_name):
    """Find the source DDS path for a texture."""
    tex = metadata["textures"].get(texture_name)
    if tex and "source_dds" in tex:
        return LOCAL_ROOT / tex["source_dds"]
    return None

def load_image_as_rgba(path):
    """Load image and convert to RGBA numpy array."""
    if not path or not path.exists():
        return None
    with Image.open(path) as im:
        return np.array(im.convert("RGBA"))

def save_image(arr, path, fmt="PNG"):
    """Save numpy array as image."""
    im = Image.fromarray(arr.astype(np.uint8), "RGBA")
    path.parent.mkdir(parents=True, exist_ok=True)
    im.save(path, format=fmt)

def analyze_texture(arr, name):
    """Analyze texture statistics."""
    if arr is None:
        return {}
    h, w = arr.shape[:2]
    stats = {
        "dimensions": [int(w), int(h)],
        "channels": int(arr.shape[2] if len(arr.shape) > 2 else 1),
        "mean": [float(x) for x in arr.mean(axis=(0,1))],
        "std": [float(x) for x in arr.std(axis=(0,1))],
        "min": [int(x) for x in arr.min(axis=(0,1))],
        "max": [int(x) for x in arr.max(axis=(0,1))],
    }
    return stats

def analyze_normal_map(arr):
    """Analyze normal map properties."""
    if arr is None:
        return {}
    # Normal maps typically store XY in R and G, Z in B or computed
    r, g, b, a = arr[:,:,0], arr[:,:,1], arr[:,:,2], arr[:,:,3]
    # Check if it's a proper normal map (unit vectors)
    rx = (r / 255.0 - 0.5) * 2
    gy = (g / 255.0 - 0.5) * 2
    z_computed = np.sqrt(np.maximum(0, 1 - rx**2 - gy**2))
    return {
        "r_range": [int(r.min()), int(r.max())],
        "g_range": [int(g.min()), int(g.max())],
        "b_range": [int(b.min()), int(b.max())],
        "a_range": [int(a.min()), int(a.max())],
        "approx_unit_length": True,
    }

def generate_noise(shape, scale=1.0, seed=None):
    """Generate seeded noise texture."""
    if seed is not None:
        np.random.seed(seed)
    h, w = shape[:2]
    noise = np.random.rand(h, w).astype(np.float32)
    return noise

def generate_fractal_noise(shape, octaves=4, persistence=0.5, scale=1.0, seed=None):
    """Generate fractal (fBm) noise using pure numpy (fast)."""
    if seed is not None:
        np.random.seed(seed)
    h, w = shape[:2]
    noise = np.zeros((h, w), dtype=np.float32)
    amplitude = 1.0
    frequency = scale
    max_amplitude = 0
    
    for i in range(octaves):
        # Generate noise at this frequency using numpy
        nh = max(1, int(h * frequency))
        nw = max(1, int(w * frequency))
        n = np.random.rand(nh, nw).astype(np.float32)
        
        # Simple bilinear upsample using numpy
        # Repeat rows and cols
        y_indices = (np.arange(h) / frequency).clip(0, nh - 1 - 1e-6)
        x_indices = (np.arange(w) / frequency).clip(0, nw - 1 - 1e-6)
        
        y0 = y_indices.astype(int)
        y1 = np.minimum(y0 + 1, nh - 1)
        x0 = x_indices.astype(int)
        x1 = np.minimum(x0 + 1, nw - 1)
        
        wy = y_indices - y0
        wx = x_indices - x0
        
        # Bilinear interpolation
        n00 = n[y0[:,None], x0[None,:]]
        n01 = n[y0[:,None], x1[None,:]]
        n10 = n[y1[:,None], x0[None,:]]
        n11 = n[y1[:,None], x1[None,:]]
        
        n_interp = (n00 * (1-wx) + n01 * wx) * (1-wy[:,None]) + (n10 * (1-wx) + n11 * wx) * wy[:,None]
        
        noise += n_interp * amplitude
        max_amplitude += amplitude
        amplitude *= persistence
        frequency *= 2
    
    if max_amplitude > 0:
        noise = noise / max_amplitude
    return noise

def add_metal_scratches(base, intensity=0.15, seed=42):
    """Add procedural metal scratches to a diffuse texture."""
    h, w = base.shape[:2]
    np.random.seed(seed)
    
    # Generate scratch mask
    scratch_mask = np.zeros((h, w), dtype=np.float32)
    num_scratches = int((w * h) / 50000 * intensity * 100)
    
    for _ in range(num_scratches):
        x1 = np.random.randint(0, w)
        y1 = np.random.randint(0, h)
        length = np.random.randint(5, 40)
        angle = np.random.uniform(0, 2 * np.pi)
        width = np.random.randint(1, 3)
        
        x2 = int(x1 + length * np.cos(angle))
        y2 = int(y1 + length * np.sin(angle))
        
        # Draw line
        steps = max(abs(x2 - x1), abs(y2 - y1), 1)
        for i in range(steps):
            t = i / steps
            x = int(x1 + t * (x2 - x1))
            y = int(y1 + t * (y2 - y1))
            if 0 <= x < w and 0 <= y < h:
                for dx in range(-width, width + 1):
                    for dy in range(-width, width + 1):
                        nx, ny = x + dx, y + dy
                        if 0 <= nx < w and 0 <= ny < h:
                            dist = np.sqrt(dx*dx + dy*dy)
                            if dist <= width:
                                scratch_mask[ny, nx] = max(scratch_mask[ny, nx], 1.0 - dist / (width + 1))
    
    # Blur scratches slightly
    from PIL import Image, ImageFilter
    mask_img = Image.fromarray((scratch_mask * 255).astype(np.uint8))
    mask_img = mask_img.filter(ImageFilter.GaussianBlur(radius=0.5))
    scratch_mask = np.array(mask_img) / 255.0
    
    # Apply scratches - darken the base color
    enhanced = base.copy().astype(np.float32)
    for c in range(3):  # RGB only
        enhanced[:,:,c] = np.clip(enhanced[:,:,c] * (1 - scratch_mask * 0.5), 0, 255)
    
    return enhanced.astype(np.uint8)

def add_edge_wear(base, intensity=0.1, seed=43):
    """Add edge wear to metal surfaces."""
    h, w = base.shape[:2]
    np.random.seed(seed)
    
    # Create edge mask using gradient
    y_grad = np.linspace(0, 1, h).reshape(-1, 1)
    x_grad = np.linspace(0, 1, w).reshape(1, -1)
    
    # Distance from edges
    dist_top = y_grad
    dist_bottom = 1 - y_grad
    dist_left = x_grad
    dist_right = 1 - x_grad
    
    edge_dist = np.minimum(np.minimum(dist_top, dist_bottom), np.minimum(dist_left, dist_right))
    edge_mask = np.clip((0.05 - edge_dist) / 0.05, 0, 1) ** 0.5
    
    # Add noise to edge mask
    noise = generate_fractal_noise((h, w), octaves=3, scale=0.01, seed=seed)
    edge_mask = edge_mask * (0.5 + 0.5 * noise)
    
    # Apply edge wear - lighter color on edges
    enhanced = base.copy().astype(np.float32)
    wear_color = np.array([60, 55, 50], dtype=np.float32)  # lighter metal color
    for c in range(3):
        enhanced[:,:,c] = np.clip(enhanced[:,:,c] * (1 - edge_mask * 0.3) + wear_color[c] * edge_mask * 0.3, 0, 255)
    
    return enhanced.astype(np.uint8)

def add_fabric_weave(base, intensity=0.1, seed=44):
    """Add fabric weave pattern to glove texture."""
    h, w = base.shape[:2]
    np.random.seed(seed)
    
    # Generate weave pattern
    x = np.arange(w)
    y = np.arange(h)
    xx, yy = np.meshgrid(x, y)
    
    # Warp and weft threads
    warp_freq = w / 64
    weft_freq = h / 64
    
    warp = np.sin(2 * np.pi * xx / warp_freq) * 0.5 + 0.5
    weft = np.sin(2 * np.pi * yy / weft_freq) * 0.5 + 0.5
    
    # Combine
    weave = np.minimum(warp, weft)
    weave = (weave - 0.5) * 2 * intensity
    
    # Add some random variation
    noise = generate_fractal_noise((h, w), octaves=2, scale=0.02, seed=seed+1)
    weave = weave + noise * 0.02
    
    enhanced = base.copy().astype(np.float32)
    for c in range(3):
        enhanced[:,:,c] = np.clip(enhanced[:,:,c] * (1 + weave), 0, 255)
    
    return enhanced.astype(np.uint8)

def add_stitching(base, intensity=0.08, seed=45):
    """Add stitching lines to glove texture."""
    h, w = base.shape[:2]
    np.random.seed(seed)
    
    # Horizontal stitching lines
    stitch_mask = np.zeros((h, w), dtype=np.float32)
    stitch_spacing = h // 16
    stitch_width = 2
    
    for y in range(stitch_spacing, h, stitch_spacing):
        for dy in range(-stitch_width, stitch_width + 1):
            ny = y + dy
            if 0 <= ny < h:
                weight = 1.0 - abs(dy) / (stitch_width + 1)
                stitch_mask[ny, :] = np.maximum(stitch_mask[ny, :], weight * 0.5)
    
    # Apply stitching - slightly darker thread color
    enhanced = base.copy().astype(np.float32)
    thread_color = np.array([25, 25, 26], dtype=np.float32)
    for c in range(3):
        enhanced[:,:,c] = np.clip(enhanced[:,:,c] * (1 - stitch_mask * 0.4) + thread_color[c] * stitch_mask * 0.4, 0, 255)
    
    return enhanced.astype(np.uint8)

def add_metal_imperfections(base, intensity=0.12, seed=46):
    """Add metal surface imperfections (pits, scratches, discoloration)."""
    h, w = base.shape[:2]
    np.random.seed(seed)
    
    # Generate imperfection mask
    noise = generate_fractal_noise((h, w), octaves=5, persistence=0.5, scale=0.02, seed=seed)
    
    # Pits - dark spots
    pit_mask = np.clip((noise - 0.7) / 0.3, 0, 1) ** 2
    
    # Scratches - directional
    scratch_noise = generate_fractal_noise((h, w), octaves=3, scale=0.01, seed=seed+1)
    scratch_mask = np.clip((scratch_noise - 0.85) / 0.15, 0, 1) * 0.5
    
    # Discoloration patches
    discolor_noise = generate_fractal_noise((h, w), octaves=2, scale=0.005, seed=seed+2)
    discolor_mask = np.abs(discolor_noise - 0.5) * 2 * 0.3
    
    enhanced = base.copy().astype(np.float32)
    
    # Apply pits (darken)
    for c in range(3):
        enhanced[:,:,c] = np.clip(enhanced[:,:,c] * (1 - pit_mask * 0.4), 0, 255)
    
    # Apply scratches
    for c in range(3):
        enhanced[:,:,c] = np.clip(enhanced[:,:,c] * (1 - scratch_mask * 0.3), 0, 255)
    
    # Apply discoloration (subtle hue shift)
    rust_tint = np.array([10, -5, -10], dtype=np.float32)  # slight orange/brown
    for c in range(3):
        enhanced[:,:,c] = np.clip(enhanced[:,:,c] + rust_tint[c] * discolor_mask, 0, 255)
    
    return enhanced.astype(np.uint8)

def enhance_diffuse(original, asset_type):
    """Create fidelity-preserving enhanced diffuse map with microdetails."""
    enhanced = original.copy()
    
    if asset_type == "weapon":
        # Metal scratches, paint wear, edge wear
        enhanced = add_metal_scratches(enhanced, intensity=0.15, seed=1001)
        enhanced = add_edge_wear(enhanced, intensity=0.1, seed=1002)
    elif asset_type == "glove":
        # Fabric weave, stitching, wear at contact points
        enhanced = add_fabric_weave(enhanced, intensity=0.08, seed=2001)
        enhanced = add_stitching(enhanced, intensity=0.06, seed=2002)
        # Add subtle wear at fingertips/palm (lower portion)
        h = enhanced.shape[0]
        wear_mask = np.linspace(0, 0.15, h // 4).reshape(-1, 1)
        wear_mask = np.tile(wear_mask, (1, enhanced.shape[1]))
        wear_full = np.zeros_like(enhanced[:,:,0], dtype=np.float32)
        wear_full[-h//4:, :] = wear_mask
        for c in range(3):
            enhanced[-h//4:, :, c] = np.clip(enhanced[-h//4:, :, c] * (1 - wear_mask * 0.5), 0, 255)
    elif asset_type == "environment":
        # Metal surface imperfections, rust, grime
        enhanced = add_metal_imperfections(enhanced, intensity=0.12, seed=3001)
        # Add grime in crevices (darker areas)
        luminance = 0.299 * enhanced[:,:,0] + 0.587 * enhanced[:,:,1] + 0.114 * enhanced[:,:,2]
        crevice_mask = np.clip((80 - luminance) / 80, 0, 1) ** 2
        grime_color = np.array([20, 18, 15], dtype=np.float32)
        for c in range(3):
            enhanced[:,:,c] = np.clip(enhanced[:,:,c] * (1 - crevice_mask * 0.3) + grime_color[c] * crevice_mask * 0.3, 0, 255)
    
    return enhanced

def reconstruct_normal_from_height(height_map, strength=1.0):
    """Reconstruct normal map from height map using Sobel."""
    from scipy.ndimage import sobel
    hx = sobel(height_map, axis=1)
    hy = sobel(height_map, axis=0)
    
    # Normalize
    max_grad = max(np.max(np.abs(hx)), np.max(np.abs(hy)), 1e-6)
    hx = hx / max_grad * strength
    hy = hy / max_grad * strength
    
    # Normal map: R = X, G = Y, B = Z (computed)
    nx = hx
    ny = hy
    nz = np.sqrt(np.maximum(0, 1 - nx**2 - ny**2))
    
    # Convert to 0-255
    normal = np.zeros((*height_map.shape, 3), dtype=np.uint8)
    normal[:,:,0] = np.clip((nx + 1) * 127.5, 0, 255).astype(np.uint8)
    normal[:,:,1] = np.clip((ny + 1) * 127.5, 0, 255).astype(np.uint8)
    normal[:,:,2] = np.clip(nz * 255, 0, 255).astype(np.uint8)
    
    return normal

def enhance_normal(original, asset_type):
    """Improve/reconstruct normal map for believable material relief."""
    # Original is already a normal map, we'll enhance it by:
    # 1. Adding micro-detail normals
    # 2. Fixing compression artifacts
    # 3. Ensuring proper unit length
    
    h, w = original.shape[:2]
    
    # Extract XY from original normal map (assuming standard packing)
    orig_r = original[:,:,0].astype(np.float32) / 255.0
    orig_g = original[:,:,1].astype(np.float32) / 255.0
    orig_b = original[:,:,2].astype(np.float32) / 255.0
    
    # Convert from [0,1] to [-1,1]
    nx = (orig_r - 0.5) * 2
    ny = (orig_g - 0.5) * 2
    nz = orig_b  # Already in [0,1] for normal maps
    
    # Add micro-detail based on asset type
    if asset_type == "weapon":
        # Fine metal grain
        detail_scale = 0.005
        detail_strength = 0.08
    elif asset_type == "glove":
        # Fabric weave detail
        detail_scale = 0.01
        detail_strength = 0.1
    elif asset_type == "environment":
        # Metal surface detail
        detail_scale = 0.008
        detail_strength = 0.06
    else:
        detail_scale = 0.01
        detail_strength = 0.05
    
    # Generate detail normal map
    detail_x = generate_fractal_noise((h, w), octaves=3, scale=detail_scale, seed=5001) * 2 - 1
    detail_y = generate_fractal_noise((h, w), octaves=3, scale=detail_scale, seed=5002) * 2 - 1
    
    # Combine with original (perturb normal)
    nx = nx + detail_x * detail_strength
    ny = ny + detail_y * detail_strength
    
    # Renormalize
    length = np.sqrt(nx**2 + ny**2 + nz**2)
    length = np.maximum(length, 1e-6)
    nx = nx / length
    ny = ny / length
    nz = nz / length
    
    # Convert back to [0,255]
    enhanced = np.zeros((h, w, 4), dtype=np.uint8)
    enhanced[:,:,0] = np.clip((nx + 1) * 127.5, 0, 255).astype(np.uint8)
    enhanced[:,:,1] = np.clip((ny + 1) * 127.5, 0, 255).astype(np.uint8)
    enhanced[:,:,2] = np.clip(nz * 255, 0, 255).astype(np.uint8)
    enhanced[:,:,3] = 255
    
    return enhanced

def enhance_specular(original, asset_type):
    """Improve specular map preserving original material response."""
    enhanced = original.copy().astype(np.float32)
    
    if asset_type == "weapon":
        # Metals: increase contrast slightly, preserve dark/bright relationship
        # Specular should be brighter on edges and wear areas
        enhanced = enhanced * 1.1
        enhanced = np.clip(enhanced, 0, 255)
    elif asset_type == "glove":
        # Fabric/leather: very low specular, just subtle highlights
        enhanced = enhanced * 0.8
    elif asset_type == "environment":
        # Metal door: moderate specular with variation
        # Add subtle variation
        h, w = enhanced.shape[:2]
        var = generate_fractal_noise((h, w), octaves=2, scale=0.01, seed=6001) * 0.1 + 0.95
        for c in range(3):
            enhanced[:,:,c] = np.clip(enhanced[:,:,c] * var, 0, 255)
    
    return enhanced.astype(np.uint8)

def create_comparison_sheet(original, enhanced, name, asset_dir):
    """Create A/B comparison: original, upscale (lanczos), enhanced."""
    # Load upscaled version if available
    upscale_path = REMASTER_DIR / "upscaled" / f"{name}__A_lanczos.png"
    upscale = None
    if upscale_path.exists():
        with Image.open(upscale_path) as im:
            upscale = np.array(im.convert("RGBA"))
    
    # Create comparison
    h, w = original.shape[:2]
    
    # Resize all to same height for comparison
    target_h = 512
    scale = target_h / h
    target_w = int(w * scale)
    
    def resize(arr):
        if arr is None:
            return None
        im = Image.fromarray(arr.astype(np.uint8), "RGBA")
        return np.array(im.resize((target_w, target_h), Image.LANCZOS))
    
    orig_r = resize(original)
    enh_r = resize(enhanced)
    up_r = resize(upscale) if upscale is not None else None
    
    # Build comparison grid
    images = []
    labels = []
    
    if orig_r is not None:
        images.append(orig_r)
        labels.append("ORIGINAL")
    if up_r is not None:
        images.append(up_r)
        labels.append("LANCZOS 2x")
    if enh_r is not None:
        images.append(enh_r)
        labels.append("ENHANCED")
    
    if not images:
        return
    
    # Create horizontal comparison
    total_w = sum(im.shape[1] for im in images)
    comp = np.zeros((target_h + 40, total_w, 4), dtype=np.uint8)
    comp[:,:,:3] = 30  # dark bg
    comp[:,:,3] = 255
    
    x = 0
    for im, label in zip(images, labels):
        comp[:target_h, x:x+im.shape[1]] = im
        # Add label
        from PIL import ImageDraw, ImageFont
        # We'll add text labels after saving
        x += im.shape[1]
    
    # Save comparison
    comp_path = asset_dir / f"{name}__comparison.png"
    save_image(comp, comp_path)
    
    # Also create vertical strip with labels
    from PIL import ImageDraw, ImageFont
    comp_im = Image.fromarray(comp, "RGBA")
    draw = ImageDraw.Draw(comp_im)
    try:
        font = ImageFont.truetype("arial.ttf", 16)
    except:
        font = ImageFont.load_default()
    
    x = 0
    for im, label in zip(images, labels):
        draw.text((x + 10, target_h + 10), label, fill=(220, 220, 220), font=font)
        x += im.shape[1]
    
    comp_im.save(comp_path)
    print(f"  Comparison saved: {comp_path}")

def process_asset(asset_key, metadata, upscale_results):
    """Process a single asset through the remaster pipeline."""
    asset = ASSETS[asset_key]
    print(f"\n=== Processing {asset['name']} ({asset['type']}) ===")
    
    asset_dir = REMASTER_DIR / asset_key
    asset_dir.mkdir(parents=True, exist_ok=True)
    (asset_dir / "source").mkdir(exist_ok=True)
    (asset_dir / "enhanced").mkdir(exist_ok=True)
    (asset_dir / "comparison").mkdir(exist_ok=True)
    
    # Load all source maps
    maps = {}
    for map_type, tex_name in asset["maps"].items():
        dds_path = find_source_dds(metadata, tex_name)
        if dds_path and dds_path.exists():
            arr = load_image_as_rgba(dds_path)
            maps[map_type] = arr
            print(f"  Loaded {map_type} ({tex_name}): {arr.shape if arr is not None else 'FAILED'}")
            # Save source reference
            save_image(arr, asset_dir / "source" / f"{tex_name}.png")
        else:
            print(f"  MISSING {map_type} ({tex_name})")
            maps[map_type] = None
    
    # Analyze original maps
    analysis = {}
    for map_type, arr in maps.items():
        if arr is not None:
            analysis[map_type] = analyze_texture(arr, f"{asset['name']}_{map_type}")
            if map_type == "normal":
                analysis[map_type]["normal_analysis"] = analyze_normal_map(arr)
    
    # Create enhanced versions
    enhanced = {}
    if maps.get("diffuse") is not None:
        enhanced["diffuse"] = enhance_diffuse(maps["diffuse"], asset["type"])
        save_image(enhanced["diffuse"], asset_dir / "enhanced" / f"{asset['name']}_d_enhanced.png")
        print(f"  Enhanced diffuse saved")
    
    if maps.get("normal") is not None:
        enhanced["normal"] = enhance_normal(maps["normal"], asset["type"])
        save_image(enhanced["normal"], asset_dir / "enhanced" / f"{asset['name']}_n_enhanced.png")
        print(f"  Enhanced normal saved")
    
    if maps.get("specular") is not None:
        enhanced["specular"] = enhance_specular(maps["specular"], asset["type"])
        save_image(enhanced["specular"], asset_dir / "enhanced" / f"{asset['name']}_s_enhanced.png")
        print(f"  Enhanced specular saved")
    
    # Copy other maps as-is (emissive, rim, detail)
    for map_type in ["emissive", "rim_mask", "detail_normal"]:
        if maps.get(map_type) is not None:
            save_image(maps[map_type], asset_dir / "enhanced" / f"{asset['name']}_{map_type}.png")
    
    # Create comparison sheets
    if maps.get("diffuse") is not None and enhanced.get("diffuse") is not None:
        create_comparison_sheet(maps["diffuse"], enhanced["diffuse"], f"{asset['name']}_d", asset_dir / "comparison")
    if maps.get("normal") is not None and enhanced.get("normal") is not None:
        create_comparison_sheet(maps["normal"], enhanced["normal"], f"{asset['name']}_n", asset_dir / "comparison")
    if maps.get("specular") is not None and enhanced.get("specular") is not None:
        create_comparison_sheet(maps["specular"], enhanced["specular"], f"{asset['name']}_s", asset_dir / "comparison")
    
    # Estimate VRAM
    vram_estimate = estimate_vram(maps, enhanced, asset)
    
    # Build manifest
    manifest = {
        "asset": asset_key,
        "name": asset["name"],
        "material": asset["material"],
        "type": asset["type"],
        "original_maps": {},
        "enhanced_maps": {},
        "analysis": analysis,
        "vram_estimate_mb": vram_estimate,
        "notes": {
            "newly_authored": [],
            "recovered_from_source": [],
            "preserved_from_original": [],
        }
    }
    
    for map_type, arr in maps.items():
        if arr is not None:
            manifest["original_maps"][map_type] = {
                "texture_name": asset["maps"][map_type],
                "dimensions": [int(arr.shape[1]), int(arr.shape[0])],
                "format": "DXT5" if map_type in ["diffuse", "emissive"] else "DXT1",
                "has_alpha": bool(arr.shape[2] == 4 and arr[:,:,3].min() < 255),
            }
    
    for map_type, arr in enhanced.items():
        if arr is not None:
            manifest["enhanced_maps"][map_type] = {
                "dimensions": list(arr.shape[:2][::-1]),
                "format": "DXT5" if map_type in ["diffuse", "emissive"] else "DXT1",
            }
    
    # Save manifest
    with open(asset_dir / "manifest.json", 'w') as f:
        json.dump(manifest, f, indent=2)
    
    return manifest

def estimate_vram(original_maps, enhanced_maps, asset):
    """Estimate VRAM usage for original vs enhanced."""
    # DXT1 = 0.5 bytes/pixel (4bpp), DXT5 = 1 byte/pixel (8bpp)
    # Include full mip chain (~1.33x base)
    total_original = 0
    total_enhanced = 0
    
    for map_type, arr in original_maps.items():
        if arr is not None:
            w, h = arr.shape[1], arr.shape[0]
            bpp = 8 if map_type in ["diffuse", "emissive"] else 4
            mips = int(np.log2(max(w, h))) + 1
            # Sum of mip chain
            size = 0
            cw, ch = w, h
            for i in range(mips):
                size += max(1, cw) * max(1, ch) * bpp / 8
                cw //= 2
                ch //= 2
            total_original += size
    
    for map_type, arr in enhanced_maps.items():
        if arr is not None:
            w, h = arr.shape[1], arr.shape[0]
            bpp = 8 if map_type in ["diffuse", "emissive"] else 4
            mips = int(np.log2(max(w, h))) + 1
            size = 0
            cw, ch = w, h
            for i in range(mips):
                size += max(1, cw) * max(1, ch) * bpp / 8
                cw //= 2
                ch //= 2
            total_enhanced += size
    
    return {
        "original_mb": round(total_original / (1024*1024), 2),
        "enhanced_mb": round(total_enhanced / (1024*1024), 2),
        "delta_mb": round((total_enhanced - total_original) / (1024*1024), 2),
    }

def main():
    print("Singularity VR Material Remaster Prototype")
    print("=" * 50)
    
    metadata = load_metadata()
    upscale_results = load_upscale_results()
    
    all_manifests = {}
    
    for asset_key in ASSETS:
        manifest = process_asset(asset_key, metadata, upscale_results)
        all_manifests[asset_key] = manifest
    
    # Create master manifest
    master = {
        "project": "Singularity VR Material Remaster Prototype",
        "assets": all_manifests,
        "total_vram_original_mb": sum(m["vram_estimate_mb"]["original_mb"] for m in all_manifests.values()),
        "total_vram_enhanced_mb": sum(m["vram_estimate_mb"]["enhanced_mb"] for m in all_manifests.values()),
        "total_vram_delta_mb": sum(m["vram_estimate_mb"]["delta_mb"] for m in all_manifests.values()),
    }
    
    with open(REMASTER_DIR / "master_manifest.json", 'w') as f:
        json.dump(master, f, indent=2)
    
    print("\n" + "=" * 50)
    print("PROTOTYPE COMPLETE")
    print(f"Output directory: {REMASTER_DIR}")
    print(f"Total VRAM (original): {master['total_vram_original_mb']:.2f} MB")
    print(f"Total VRAM (enhanced): {master['total_vram_enhanced_mb']:.2f} MB")
    print(f"Delta: {master['total_vram_delta_mb']:.2f} MB")

if __name__ == "__main__":
    main()