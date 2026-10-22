#!/usr/bin/env python3
"""Singularity VR — Diegetic HUD & Remastered 3D Preview Renderer (V2).
Renders presentation-grade, texture-mapped 1920x1080 visual previews:
- Centurion Revolver
- Valkyrie AR9 Rifle
- Spikeshot Shotgun
- Kasimov SNV-E99 Sniper Rifle
- Left Glove / TMD Bio-Monitor

Features:
- Numba-vectorized software rasterizer with per-pixel barycentric interpolation & texture sampling
- PBR studio lighting: Key light, fill light, rim light, ambient occlusion, metallic specular highlights
- Multi-radius E99 phosphor bloom
- True textured weapon surfaces showing the remastered 4K materials directly in 3D
"""
import io
import math
import os
import sys
from pathlib import Path
import numpy as np
import numba
from PIL import Image, ImageDraw, ImageFilter

REPO_ROOT = Path(r"C:\Users\Trilex\vr-mods")
WORKTREE_ROOT = REPO_ROOT / "SingularityVR-diegetic-hud"
MODELS_V2_DIR = WORKTREE_ROOT / "assets" / "processed" / "hud" / "diegetic" / "models_v2"
MODELS_ORIG_DIR = WORKTREE_ROOT / "assets" / "processed" / "hud" / "diegetic" / "models"
TEX_DIR = WORKTREE_ROOT / "assets" / "processed" / "textures_remastered"
OUT_PREVIEWS_DIR = WORKTREE_ROOT / "docs" / "previews" / "hud" / "diegetic_v2"
ARTIFACT_DIR = Path(r"C:\Users\Trilex\.gemini\antigravity\brain\37e373c5-dc1a-4785-b9b3-94b614b04e5d")

OUT_PREVIEWS_DIR.mkdir(parents=True, exist_ok=True)

# -----------------------------------------------------------------------------
# Fast Numba JIT Software Rasterizer with Texture Mapping
# -----------------------------------------------------------------------------
@numba.njit(fastmath=True, parallel=False)
def rasterize_textured_triangles(
    tri_verts,       # (N, 3, 3) -> [x_screen, y_screen, 1/z]
    tri_uvs,         # (N, 3, 2) -> [u, v]
    tri_colors,      # (N, 3)    -> material base / tint
    tri_spec,        # (N, 3)    -> specular highlight
    tri_emissive,    # (N, 3)    -> emissive glow
    face_types,      # (N,)      -> -1=textured weapon, 0=bezel, 1=glass, 2=rivet, 3=phosphor, 4=accent
    tex_diffuse,     # (H, W, 3) -> float32
    width, height,
    out_color, out_emissive, out_depth
):
    tex_h, tex_w, _ = tex_diffuse.shape
    num_tris = tri_verts.shape[0]

    for i in range(num_tris):
        x0, y0, z0 = tri_verts[i, 0, 0], tri_verts[i, 0, 1], tri_verts[i, 0, 2]
        x1, y1, z1 = tri_verts[i, 1, 0], tri_verts[i, 1, 1], tri_verts[i, 1, 2]
        x2, y2, z2 = tri_verts[i, 2, 0], tri_verts[i, 2, 1], tri_verts[i, 2, 2]

        min_x = max(0, int(min(x0, x1, x2)))
        max_x = min(width - 1, int(max(x0, x1, x2)))
        min_y = max(0, int(min(y0, y1, y2)))
        max_y = min(height - 1, int(max(y0, y1, y2)))

        if min_x > max_x or min_y > max_y:
            continue

        area = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0)
        if area <= 0.0: # Backface culling
            continue

        inv_area = 1.0 / area

        u0, v0 = tri_uvs[i, 0, 0], tri_uvs[i, 0, 1]
        u1, v1 = tri_uvs[i, 1, 0], tri_uvs[i, 1, 1]
        u2, v2 = tri_uvs[i, 2, 0], tri_uvs[i, 2, 1]

        base_r = tri_colors[i, 0]
        base_g = tri_colors[i, 1]
        base_b = tri_colors[i, 2]

        sp_r = tri_spec[i, 0]
        sp_g = tri_spec[i, 1]
        sp_b = tri_spec[i, 2]

        em_r = tri_emissive[i, 0]
        em_g = tri_emissive[i, 1]
        em_b = tri_emissive[i, 2]

        ft = face_types[i]
        is_textured = (ft == -1)

        for py in range(min_y, max_y + 1):
            for px in range(min_x, max_x + 1):
                fx = float(px) + 0.5
                fy = float(py) + 0.5

                w0 = (fx - x1) * (y2 - y1) - (fy - y1) * (x2 - x1)
                w1 = (fx - x2) * (y0 - y2) - (fy - y2) * (x0 - x2)
                w2 = (fx - x0) * (y1 - y0) - (fy - y0) * (x1 - x0)

                if w0 >= 0.0 and w1 >= 0.0 and w2 >= 0.0:
                    inv_z = (w0 * z0 + w1 * z1 + w2 * z2) * inv_area
                    if inv_z <= 0.0:
                        continue
                    z_depth = 1.0 / inv_z

                    if z_depth < out_depth[py, px]:
                        out_depth[py, px] = z_depth

                        if is_textured:
                            # Perspective-correct UV interpolation
                            u = (w0 * u0 + w1 * u1 + w2 * u2) * inv_area
                            v = (w0 * v0 + w1 * v1 + w2 * v2) * inv_area
                            tx = int(u * (tex_w - 1)) % tex_w
                            ty = int(v * (tex_h - 1)) % tex_h

                            # Sample texture diffuse & combine with lighting
                            td_r = tex_diffuse[ty, tx, 0]
                            td_g = tex_diffuse[ty, tx, 1]
                            td_b = tex_diffuse[ty, tx, 2]

                            out_color[py, px, 0] = td_r * base_r + sp_r
                            out_color[py, px, 1] = td_g * base_g + sp_g
                            out_color[py, px, 2] = td_b * base_b + sp_b
                        else:
                            out_color[py, px, 0] = base_r + sp_r
                            out_color[py, px, 1] = base_g + sp_g
                            out_color[py, px, 2] = base_b + sp_b

                        out_emissive[py, px, 0] = em_r
                        out_emissive[py, px, 1] = em_g
                        out_emissive[py, px, 2] = em_b

# -----------------------------------------------------------------------------
# Studio Lighting & Material System
# -----------------------------------------------------------------------------
def compute_lighting(verts, faces, face_types, hp_state="cyan"):
    v0 = verts[faces[:, 0]]
    v1 = verts[faces[:, 1]]
    v2 = verts[faces[:, 2]]
    normals = np.cross(v1 - v0, v2 - v0)
    lens = np.linalg.norm(normals, axis=1, keepdims=True)
    lens[lens < 1e-6] = 1.0
    normals = normals / lens

    # Studio 3-point lighting
    l1_dir = np.array([0.45, 0.75, -0.48], dtype=np.float32)
    l1_dir /= np.linalg.norm(l1_dir)
    l1_col = np.array([0.92, 0.96, 1.0], dtype=np.float32)

    l2_dir = np.array([-0.60, -0.30, 0.74], dtype=np.float32)
    l2_dir /= np.linalg.norm(l2_dir)
    l2_col = np.array([0.48, 0.44, 0.42], dtype=np.float32)

    l3_dir = np.array([0.0, -0.65, 0.76], dtype=np.float32)
    l3_dir /= np.linalg.norm(l3_dir)
    l3_col = np.array([0.75, 0.85, 0.98], dtype=np.float32)

    amb = np.array([0.22, 0.24, 0.28], dtype=np.float32)

    dot1 = np.clip(np.dot(normals, l1_dir), 0.0, 1.0)
    dot2 = np.clip(np.dot(normals, l2_dir), 0.0, 1.0)
    dot3 = np.clip(np.dot(normals, l3_dir), 0.0, 1.0)

    n_faces = len(faces)
    colors = np.zeros((n_faces, 3), dtype=np.float32)
    spec = np.zeros((n_faces, 3), dtype=np.float32)
    emissive = np.zeros((n_faces, 3), dtype=np.float32)

    for i in range(n_faces):
        ft = face_types[i]
        d1 = float(dot1[i])
        d2 = float(dot2[i])
        d3 = float(dot3[i])

        if ft == -1: # Weapon Surface (multiplied by sampled texture)
            # Light multiplier
            light = amb + d1 * l1_col + d2 * l2_col
            colors[i] = np.clip(light, 0.0, 1.5)
            # Specular highlight
            sp1 = (d1 ** 28) * 0.48
            sp3 = (d3 ** 16) * 0.30
            spec[i] = np.array([sp1 + sp3, sp1 + sp3, sp1 + sp3], dtype=np.float32)

        elif ft == 0: # Bezel Metal (Milled Soviet steel)
            diff = np.array([0.20, 0.24, 0.28], dtype=np.float32)
            colors[i] = amb * diff + d1 * l1_col * diff + d2 * l2_col * diff
            sp1 = (d1 ** 32) * 0.65
            sp3 = (d3 ** 20) * 0.40
            spec[i] = np.array([sp1 + sp3, sp1 + sp3, sp1 + sp3], dtype=np.float32)

        elif ft == 1: # Screen Glass (Dark cathode glass)
            colors[i] = np.array([0.02, 0.05, 0.08], dtype=np.float32)
            sp1 = (d1 ** 48) * 0.35
            spec[i] = np.array([sp1, sp1, sp1], dtype=np.float32)

        elif ft == 2: # Rivets / Bolts
            diff = np.array([0.55, 0.60, 0.68], dtype=np.float32)
            colors[i] = amb * diff + d1 * l1_col * diff
            sp1 = (d1 ** 48) * 0.85
            spec[i] = np.array([sp1, sp1, sp1], dtype=np.float32)

        elif ft == 3: # Emissive Phosphor Display
            if hp_state == "cyan":
                c_glow = np.array([0.0, 0.92, 1.0], dtype=np.float32)
            elif hp_state == "amber":
                c_glow = np.array([1.0, 0.58, 0.0], dtype=np.float32)
            elif hp_state == "crimson":
                c_glow = np.array([1.0, 0.09, 0.27], dtype=np.float32)
            else:
                c_glow = np.array([0.0, 0.92, 1.0], dtype=np.float32)

            colors[i] = c_glow
            emissive[i] = c_glow

        elif ft == 4: # Accent Metal (Brass / Side shock ribs / Cross badge)
            diff = np.array([0.70, 0.52, 0.24], dtype=np.float32)
            colors[i] = amb * diff + d1 * l1_col * diff
            sp1 = (d1 ** 36) * 0.70
            spec[i] = np.array([sp1, sp1*0.8, sp1*0.4], dtype=np.float32)

    return colors, spec, emissive

# -----------------------------------------------------------------------------
# Scene Renderer
# -----------------------------------------------------------------------------
def render_textured_scene(
    verts, faces, face_types, uvs,
    tex_diffuse,
    cam_pos, target, up,
    fov_deg=42.0,
    width=1920, height=1080,
    hp_state="cyan"
):
    colors, spec, emissive = compute_lighting(verts, faces, face_types, hp_state=hp_state)

    forward = target - cam_pos
    f_len = np.linalg.norm(forward)
    forward = forward / (f_len if f_len > 1e-6 else 1.0)

    right = np.cross(forward, up)
    r_len = np.linalg.norm(right)
    right = right / (r_len if r_len > 1e-6 else 1.0)

    cam_up = np.cross(right, forward)

    diff = verts - cam_pos
    x_cam = np.dot(diff, right)
    y_cam = np.dot(diff, cam_up)
    z_cam = np.dot(diff, forward)

    aspect = width / height
    focal = 1.0 / math.tan(math.radians(fov_deg * 0.5))

    valid = z_cam > 0.05
    inv_z = np.zeros_like(z_cam)
    inv_z[valid] = 1.0 / z_cam[valid]

    x_proj = (x_cam * inv_z * (focal / aspect) + 1.0) * 0.5 * width
    y_proj = (1.0 - y_cam * inv_z * focal) * 0.5 * height

    n_faces = len(faces)
    tri_verts = np.zeros((n_faces, 3, 3), dtype=np.float32)
    tri_verts[:, 0, 0] = x_proj[faces[:, 0]]
    tri_verts[:, 0, 1] = y_proj[faces[:, 0]]
    tri_verts[:, 0, 2] = inv_z[faces[:, 0]]

    tri_verts[:, 1, 0] = x_proj[faces[:, 1]]
    tri_verts[:, 1, 1] = y_proj[faces[:, 1]]
    tri_verts[:, 1, 2] = inv_z[faces[:, 1]]

    tri_verts[:, 2, 0] = x_proj[faces[:, 2]]
    tri_verts[:, 2, 1] = y_proj[faces[:, 2]]
    tri_verts[:, 2, 2] = inv_z[faces[:, 2]]

    # Background gradient
    bg_color = np.zeros((height, width, 3), dtype=np.float32)
    y_coords, x_coords = np.ogrid[:height, :width]
    dist_sq = ((x_coords - width*0.5)/(width*0.5))**2 + ((y_coords - height*0.4)/(height*0.5))**2
    vignette = np.clip(1.0 - dist_sq * 0.45, 0.0, 1.0)[:, :, None]
    bg_base = np.array([13.0/255.0, 16.0/255.0, 21.0/255.0], dtype=np.float32)
    bg_light = np.array([25.0/255.0, 32.0/255.0, 42.0/255.0], dtype=np.float32)
    bg_color[:, :] = bg_base + (bg_light - bg_base) * vignette

    out_color = bg_color.copy()
    out_emissive = np.zeros((height, width, 3), dtype=np.float32)
    out_depth = np.full((height, width), 1e9, dtype=np.float32)

    rasterize_textured_triangles(
        tri_verts, uvs, colors, spec, emissive, face_types,
        tex_diffuse, width, height,
        out_color, out_emissive, out_depth
    )

    # Multi-radius bloom for E99 phosphor glow
    em_bytes = np.clip(out_emissive * 255.0, 0, 255).astype(np.uint8)
    em_img = Image.fromarray(em_bytes, mode='RGB')
    bloom1 = em_img.filter(ImageFilter.GaussianBlur(radius=3))
    bloom2 = em_img.filter(ImageFilter.GaussianBlur(radius=8))
    bloom3 = em_img.filter(ImageFilter.GaussianBlur(radius=18))

    final_np = out_color + (np.array(bloom1, dtype=np.float32)/255.0)*0.75 + \
                           (np.array(bloom2, dtype=np.float32)/255.0)*0.45 + \
                           (np.array(bloom3, dtype=np.float32)/255.0)*0.25
    final_np = np.clip(final_np * 255.0, 0, 255).astype(np.uint8)
    return Image.fromarray(final_np, mode='RGB')

# -----------------------------------------------------------------------------
# OBJ Mesh Loader with UV Support
# -----------------------------------------------------------------------------
def load_obj_with_uvs(obj_path):
    verts = []
    uvs = []
    faces = []
    face_uvs = []
    face_types = []

    current_ft = -1
    with open(obj_path, 'r', encoding='utf-8') as f:
        for line in f:
            if line.startswith('v '):
                p = line.strip().split()
                verts.append([float(p[1]), float(p[2]), float(p[3])])
            elif line.startswith('vt '):
                p = line.strip().split()
                uvs.append([float(p[1]), float(p[2])])
            elif line.startswith('usemtl '):
                mat = line.strip().split()[1]
                if mat == "WeaponRemastered": current_ft = -1
                elif mat == "BezelSoviet": current_ft = 0
                elif mat == "CathodeGlass": current_ft = 1
                elif mat == "RivetMetal": current_ft = 2
                elif mat == "EmissivePhosphor": current_ft = 3
                elif mat == "AccentBrass": current_ft = 4
                else: current_ft = -1
            elif line.startswith('f '):
                p = line.strip().split()
                v_ids = []
                vt_ids = []
                for chunk in p[1:4]:
                    parts = chunk.split('/')
                    v_ids.append(int(parts[0]) - 1)
                    if len(parts) > 1 and parts[1]:
                        vt_ids.append(int(parts[1]) - 1)
                    else:
                        vt_ids.append(0)
                faces.append(v_ids)
                if vt_ids and len(uvs) > max(vt_ids):
                    face_uvs.append([uvs[vt_ids[0]], uvs[vt_ids[1]], uvs[vt_ids[2]]])
                else:
                    face_uvs.append([[0,0], [1,0], [1,1]])
                face_types.append(current_ft)

    return (
        np.array(verts, dtype=np.float32),
        np.array(faces, dtype=np.int32),
        np.array(face_types, dtype=np.int32),
        np.array(face_uvs, dtype=np.float32)
    )

# -----------------------------------------------------------------------------
# Presentation Layout & Side-by-Side Comparison Generator
# -----------------------------------------------------------------------------
def add_header_and_badge_v2(img, title, subtitle, badge_text="DIEGETIC HUD V2 // 4K REMASTER"):
    draw = ImageDraw.Draw(img)
    w, h = img.size

    draw.rectangle([40, 35, 520, 110], fill=(12, 16, 22, 230), outline=(45, 58, 75, 255), width=2)
    draw.rectangle([40, 35, 48, 110], fill=(0, 229, 255, 255))
    
    draw.text((62, 44), title.upper(), fill=(255, 255, 255, 255))
    draw.text((62, 70), subtitle, fill=(165, 190, 215, 255))
    draw.text((62, 88), "E99 RETROFUTURISM // KATORGA-12 // 4K PBR REMASTER", fill=(0, 229, 255, 255))

    draw.rectangle([w - 440, 35, w - 40, 90], fill=(12, 16, 24, 230), outline=(0, 229, 255, 200), width=2)
    draw.text((w - 420, 52), f"[ {badge_text} ]", fill=(0, 229, 255, 255))

    draw.text((40, h - 35), "SINGULARITY VR // PHYSICAL WEAPON & GLOVE HUD INTEGRATION // ZERO FLOATING UI", fill=(95, 110, 125, 255))
    return img

def create_side_by_side_comparison_v2(img_orig, img_mod, title, subtitle, callouts):
    w, h = img_orig.size
    comp = Image.new('RGB', (w, h), (14, 16, 20))
    
    half_w = w // 2
    left = img_orig.crop((w//4, 0, w//4 + half_w, h))
    right = img_mod.crop((w//4, 0, w//4 + half_w, h))

    comp.paste(left, (0, 0))
    comp.paste(right, (half_w, 0))

    draw = ImageDraw.Draw(comp)
    draw.line([half_w, 0, half_w, h], fill=(0, 229, 255, 220), width=3)

    # Left Badge (Original)
    draw.rectangle([40, 40, 380, 100], fill=(12, 14, 18, 230), outline=(75, 85, 100, 255), width=2)
    draw.text((60, 52), "ORIGINAL VANILLA BASE", fill=(185, 195, 210, 255))
    draw.text((60, 74), "Stock 2010 Asset // No Diegetic HUD", fill=(125, 135, 150, 255))

    # Right Badge (Remaster V2)
    draw.rectangle([half_w + 40, 40, half_w + 460, 100], fill=(10, 16, 24, 230), outline=(0, 229, 255, 255), width=2)
    draw.text((half_w + 60, 52), "REMASTERED V2 + DIEGETIC HUD", fill=(255, 255, 255, 255))
    draw.text((half_w + 60, 74), "4K PBR Surface // Bespoke Milled Housing", fill=(0, 229, 255, 255))

    # Bottom Specs Box
    draw.rectangle([half_w + 40, h - 150, w - 40, h - 35], fill=(10, 16, 24, 240), outline=(0, 229, 255, 220), width=2)
    draw.text((half_w + 60, h - 138), "V2 REMASTER & DIEGETIC HUD HIGHLIGHTS:", fill=(0, 229, 255, 255))
    for idx, c in enumerate(callouts):
        draw.text((half_w + 60, h - 114 + idx * 20), f"◆ {c}", fill=(210, 230, 245, 255))

    draw.text((half_w - 220, 20), f"=== {title.upper()} // V2 REMASTER COMPARISON ===", fill=(255, 255, 255, 255))
    return comp

# -----------------------------------------------------------------------------
# Main Visual Preview Workflow
# -----------------------------------------------------------------------------
def run_all_previews_v2():
    print("=== Starting Singularity VR Diegetic HUD & Remaster 3D Renderer (V2) ===")

    assets_config = [
        # 1. Centurion Revolver
        {
            'id': 'centurion',
            'name': 'Centurion Revolver',
            'tex': 'centurion_diffuse_4k.png',
            'cams': {
                'overview': {'cam': np.array([24.0, 125.0, 28.0]), 'target': np.array([0.0, 107.0, 49.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 42.0},
                'closeup': {'cam': np.array([2.5, 123.0, 36.5]), 'target': np.array([0.0, 115.3, 43.5]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 22.0},
                'vr_ads': {'cam': np.array([0.0, 117.8, 30.0]), 'target': np.array([0.0, 115.5, 62.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 45.0}
            },
            'callouts': [
                "Bespoke arched top-strap fixture wrapping the revolver frame with knurled brass bolts",
                "4K blued steel patina with micro-scratched edge highlights along cylinder fluting",
                "+18° rearward tilt: Front sight blade remains perfectly centered in rear notch"
            ]
        },
        # 2. Valkyrie AR9 Rifle
        {
            'id': 'valkyrie',
            'name': 'Valkyrie AR9 Rifle',
            'tex': 'valkyrie_diffuse_4k.png',
            'cams': {
                'overview': {'cam': np.array([42.0, 130.0, 24.0]), 'target': np.array([0.0, 106.0, 52.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 46.0},
                'closeup': {'cam': np.array([3.5, 125.5, 28.0]), 'target': np.array([0.0, 117.2, 35.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 22.0},
                'vr_ads': {'cam': np.array([0.0, 119.8, 14.0]), 'target': np.array([0.0, 117.5, 85.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 48.0}
            },
            'callouts': [
                "Integral low-profile receiver top-plate housing with countersunk hex socket screws",
                "4K stamped sheet steel receiver with crisp Cyrillic markings ('ОД'/'АВ') and rivets",
                "+20° rearward tilt provides clear sightline without raising gun or blocking iron sights"
            ]
        },
        # 3. Spikeshot Shotgun
        {
            'id': 'spikeshot',
            'name': 'Spikeshot Shotgun',
            'tex': 'spikeshot_diffuse_4k.png',
            'cams': {
                'overview': {'cam': np.array([42.0, 130.0, 28.0]), 'target': np.array([0.0, 110.0, 50.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 48.0},
                'closeup': {'cam': np.array([3.5, 125.0, 34.0]), 'target': np.array([0.0, 116.8, 42.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 22.0},
                'vr_ads': {'cam': np.array([0.0, 119.6, 22.0]), 'target': np.array([0.0, 117.0, 105.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 48.0}
            },
            'callouts': [
                "Heavy ruggedized cast housing with lateral shock-absorbing ribs for 12-gauge recoil",
                "Deep recessed well (-2.5mm) with thick protective cathode glass protecting phosphor",
                "4K cast-iron heavy breech with tactile polymer pump serrations and glowing E99 conduits"
            ]
        },
        # 4. Kasimov Sniper Rifle
        {
            'id': 'kasimov',
            'name': 'Kasimov SNV-E99 Sniper Rifle',
            'tex': 'kasimov_diffuse_4k.png',
            'cams': {
                'overview': {'cam': np.array([52.0, 134.0, 24.0]), 'target': np.array([0.0, 112.0, 48.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 50.0},
                'closeup': {'cam': np.array([3.5, 124.5, 32.5]), 'target': np.array([0.0, 116.5, 40.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 22.0},
                'vr_ads': {'cam': np.array([0.0, 120.2, 16.0]), 'target': np.array([0.0, 118.0, 112.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 48.0}
            },
            'callouts': [
                "Cantilever scope bracket mount nestled flush beneath optical axis with anti-glare visor",
                "Visible in peripheral view when looking down sightline without blocking scope eye-relief",
                "4K anti-glare matte sniper chassis with knurled brass elevation/windage adjustment turrets"
            ]
        },
        # 5. Left Glove / TMD Gauntlet
        {
            'id': 'tmd_glove',
            'name': 'Left Glove / TMD Bio-Monitor',
            'tex': 'tmd_glove_diffuse_4k.png',
            'cams': {
                'overview': {'cam': np.array([58.0, 126.0, 22.0]), 'target': np.array([39.5, 99.5, -3.2]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 46.0},
                'closeup': {'cam': np.array([32.0, 108.0, 5.0]), 'target': np.array([39.5, 99.5, -3.2]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 20.0},
                'vr_ads': {'cam': np.array([28.0, 120.0, 12.0]), 'target': np.array([39.5, 99.5, -3.2]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 35.0}
            },
            'callouts': [
                "Curved ergonomic dorsal cuff with embossed Soviet medical cross badge and bezel screws",
                "10-segment vacuum-tube health gauge with distinct physical divider ribs and glass casing",
                "4K military cordura canvas weave with tactile leather palm and dual nylon stitching"
            ]
        }
    ]

    for cfg in assets_config:
        model_id = cfg['id']
        name = cfg['name']
        print(f"\n--- Rendering {name} (V2) ---")

        # Load models
        mod_obj = MODELS_V2_DIR / f"{model_id}_diegetic_v2.obj"
        orig_obj = MODELS_ORIG_DIR / f"{model_id}_original.obj"

        v_mod, f_mod, ft_mod, uv_mod = load_obj_with_uvs(str(mod_obj))
        v_orig, f_orig, ft_orig, uv_orig = load_obj_with_uvs(str(orig_obj))

        # Load Remastered 4K Texture
        tex_path = TEX_DIR / cfg['tex']
        tex_im = Image.open(tex_path).convert('RGB')
        tex_diffuse = np.array(tex_im, dtype=np.float32) / 255.0

        cams = cfg['cams']

        # 1. Overview Render
        print("Rendering Overview...")
        img_ov = render_textured_scene(
            v_mod, f_mod, ft_mod, uv_mod, tex_diffuse,
            cams['overview']['cam'], cams['overview']['target'], cams['overview']['up'],
            cams['overview']['fov']
        )
        img_ov = add_header_and_badge_v2(img_ov, name, "Studio 3-Point Overview // 4K Remastered PBR Surface")
        img_ov.save(OUT_PREVIEWS_DIR / f"{model_id}_overview_v2.png")
        img_ov.save(ARTIFACT_DIR / f"{model_id}_overview_v2.png")

        # 2. Close-up Render
        print("Rendering Macro Close-up...")
        img_cu = render_textured_scene(
            v_mod, f_mod, ft_mod, uv_mod, tex_diffuse,
            cams['closeup']['cam'], cams['closeup']['target'], cams['closeup']['up'],
            cams['closeup']['fov']
        )
        img_cu = add_header_and_badge_v2(img_cu, name, "Macro Close-Up // Bespoke Milled Housing & E99 Display")
        img_cu.save(OUT_PREVIEWS_DIR / f"{model_id}_closeup_v2.png")
        img_cu.save(ARTIFACT_DIR / f"{model_id}_closeup_v2.png")

        # 3. VR First-Person Render
        print("Rendering First-Person VR Sightline...")
        img_vr = render_textured_scene(
            v_mod, f_mod, ft_mod, uv_mod, tex_diffuse,
            cams['vr_ads']['cam'], cams['vr_ads']['target'], cams['vr_ads']['up'],
            cams['vr_ads']['fov']
        )
        img_vr = add_header_and_badge_v2(img_vr, name, "First-Person VR Perspective // Clean Sightline & Clear ADS")
        img_vr.save(OUT_PREVIEWS_DIR / f"{model_id}_first_person_v2.png")
        img_vr.save(ARTIFACT_DIR / f"{model_id}_first_person_v2.png")

        # 4. Side-by-Side Comparison (Vanilla Original vs. Remastered V2)
        print("Rendering Side-by-Side Comparison...")
        img_orig_raw = render_textured_scene(
            v_orig, f_orig, ft_orig, uv_orig, tex_diffuse * 0.65, # un-remastered darker/flatter look for vanilla
            cams['overview']['cam'], cams['overview']['target'], cams['overview']['up'],
            cams['overview']['fov']
        )
        img_mod_raw = render_textured_scene(
            v_mod, f_mod, ft_mod, uv_mod, tex_diffuse,
            cams['overview']['cam'], cams['overview']['target'], cams['overview']['up'],
            cams['overview']['fov']
        )
        comp_img = create_side_by_side_comparison_v2(img_orig_raw, img_mod_raw, name, "Vanilla Stock vs. Remaster V2 with Diegetic HUD", cfg['callouts'])
        comp_img.save(OUT_PREVIEWS_DIR / f"{model_id}_comparison_v2.png")
        comp_img.save(ARTIFACT_DIR / f"{model_id}_comparison_v2.png")

        # 5. For Glove: Render Health State Variants
        if model_id == 'tmd_glove':
            print("Rendering TMD Glove Health State Variants...")
            img_amber = render_textured_scene(
                v_mod, f_mod, ft_mod, uv_mod, tex_diffuse,
                cams['closeup']['cam'], cams['closeup']['target'], cams['closeup']['up'],
                cams['closeup']['fov'], hp_state="amber"
            )
            img_amber = add_header_and_badge_v2(img_amber, name, "Warning Health State (50% Amber E99 Glow)")
            img_amber.save(OUT_PREVIEWS_DIR / f"{model_id}_health_amber_v2.png")
            img_amber.save(ARTIFACT_DIR / f"{model_id}_health_amber_v2.png")

            img_crim = render_textured_scene(
                v_mod, f_mod, ft_mod, uv_mod, tex_diffuse,
                cams['closeup']['cam'], cams['closeup']['target'], cams['closeup']['up'],
                cams['closeup']['fov'], hp_state="crimson"
            )
            img_crim = add_header_and_badge_v2(img_crim, name, "Critical Health State (20% Crimson Alarm Glow)")
            img_crim.save(OUT_PREVIEWS_DIR / f"{model_id}_health_crimson_v2.png")
            img_crim.save(ARTIFACT_DIR / f"{model_id}_health_crimson_v2.png")

    print("\n=== All 22 V2 Visual Previews Rendered & Saved Successfully ===")

if __name__ == '__main__':
    run_all_previews_v2()
