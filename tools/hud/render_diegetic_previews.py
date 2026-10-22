"""High-Performance 3D Software Studio Renderer for Singularity VR Diegetic HUD.
Renders high-resolution (1920x1080) previews for:
1. Centurion Revolver
2. Valkyrie AR9
3. Spikeshot Shotgun
4. Kasimov Sniper Rifle
5. Left Glove / TMD Gauntlet (with health states: Cyan 100%, Amber 50%, Crimson 20%)

For each asset, produces:
- Overview (full 3/4 studio view)
- Close-up (macro shot of HUD module)
- First-Person VR (ADS sightline or wrist elevation)
- Side-by-side Before/After comparison
"""
import math
import os
import shutil
import numpy as np
from PIL import Image, ImageDraw, ImageFilter
import numba

# -----------------------------------------------------------------------------
# Fast Numba JIT Z-Buffer Rasterizer
# -----------------------------------------------------------------------------
@numba.njit(parallel=True, fastmath=True)
def rasterize_triangles(
    screen_pts,      # (N, 3, 3): for each tri, 3 verts (x_pixel, y_pixel, 1/z_cam)
    tri_colors,      # (N, 3): base RGB color (0.0 to 1.0)
    tri_emissive,    # (N, 3): emissive RGB (0.0 to 1.0)
    width, height,
    out_color,       # (H, W, 3) float32
    out_emissive,    # (H, W, 3) float32
    out_depth        # (H, W) float32
):
    n_tri = screen_pts.shape[0]
    for i in numba.prange(n_tri):
        x0 = screen_pts[i, 0, 0]
        y0 = screen_pts[i, 0, 1]
        z0 = screen_pts[i, 0, 2]

        x1 = screen_pts[i, 1, 0]
        y1 = screen_pts[i, 1, 1]
        z1 = screen_pts[i, 1, 2]

        x2 = screen_pts[i, 2, 0]
        y2 = screen_pts[i, 2, 1]
        z2 = screen_pts[i, 2, 2]

        area = (x2 - x0) * (y1 - y0) - (y2 - y0) * (x1 - x0)
        if area <= 0.00001:
            continue
        inv_area = 1.0 / area

        min_x = max(0, int(min(x0, x1, x2)))
        max_x = min(width - 1, int(max(x0, x1, x2)) + 1)
        min_y = max(0, int(min(y0, y1, y2)))
        max_y = min(height - 1, int(max(y0, y1, y2)) + 1)

        r = tri_colors[i, 0]
        g = tri_colors[i, 1]
        b = tri_colors[i, 2]

        er = tri_emissive[i, 0]
        eg = tri_emissive[i, 1]
        eb = tri_emissive[i, 2]

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
                        out_color[py, px, 0] = r
                        out_color[py, px, 1] = g
                        out_color[py, px, 2] = b
                        out_emissive[py, px, 0] = er
                        out_emissive[py, px, 1] = eg
                        out_emissive[py, px, 2] = eb

# -----------------------------------------------------------------------------
# Multi-Light Studio Shading
# -----------------------------------------------------------------------------
def compute_shading(verts, faces, face_types):
    v0 = verts[faces[:, 0]]
    v1 = verts[faces[:, 1]]
    v2 = verts[faces[:, 2]]
    normals = np.cross(v1 - v0, v2 - v0)
    lens = np.linalg.norm(normals, axis=1, keepdims=True)
    lens[lens < 1e-6] = 1.0
    normals = normals / lens

    l1_dir = np.array([0.45, 0.75, -0.48], dtype=np.float32)
    l1_dir /= np.linalg.norm(l1_dir)
    l1_col = np.array([0.90, 0.95, 1.0], dtype=np.float32)

    l2_dir = np.array([-0.60, -0.30, 0.74], dtype=np.float32)
    l2_dir /= np.linalg.norm(l2_dir)
    l2_col = np.array([0.45, 0.42, 0.40], dtype=np.float32)

    l3_dir = np.array([0.0, -0.65, 0.76], dtype=np.float32)
    l3_dir /= np.linalg.norm(l3_dir)
    l3_col = np.array([0.70, 0.80, 0.95], dtype=np.float32)

    amb = np.array([0.18, 0.20, 0.24], dtype=np.float32)

    dot1 = np.clip(np.dot(normals, l1_dir), 0.0, 1.0)
    dot2 = np.clip(np.dot(normals, l2_dir), 0.0, 1.0)
    dot3 = np.clip(np.dot(normals, l3_dir), 0.0, 1.0)

    colors = np.zeros((len(faces), 3), dtype=np.float32)
    emissive = np.zeros((len(faces), 3), dtype=np.float32)

    for i in range(len(faces)):
        ft = face_types[i]
        d1 = float(dot1[i])
        d2 = float(dot2[i])
        d3 = float(dot3[i])
        
        if ft == -1: # Weapon Gunmetal
            diffuse = np.array([0.28, 0.31, 0.36], dtype=np.float32)
            spec1 = (d1 ** 24) * 0.45
            spec3 = (d3 ** 16) * 0.35
            col = amb * diffuse + d1 * l1_col * diffuse + d2 * l2_col * diffuse + (spec1 + spec3)
            colors[i] = np.clip(col, 0.0, 1.0)
        elif ft == 0: # Bezel Metal
            diffuse = np.array([0.22, 0.25, 0.30], dtype=np.float32)
            spec1 = (d1 ** 32) * 0.60
            spec3 = (d3 ** 20) * 0.45
            col = amb * diffuse + d1 * l1_col * diffuse + d2 * l2_col * diffuse + (spec1 + spec3)
            colors[i] = np.clip(col, 0.0, 1.0)
        elif ft == 1: # Screen Glass
            col = np.array([0.03, 0.06, 0.09], dtype=np.float32) + (d1 ** 40) * 0.25
            colors[i] = np.clip(col, 0.0, 1.0)
        elif ft == 2: # Rivets
            diffuse = np.array([0.55, 0.60, 0.68], dtype=np.float32)
            spec1 = (d1 ** 48) * 0.80
            col = amb * diffuse + d1 * l1_col * diffuse + spec1
            colors[i] = np.clip(col, 0.0, 1.0)
        elif ft >= 10: # Emissive Phosphor Readouts
            if ft == 10: # Cyan (Normal)
                c_glow = np.array([0.0, 0.90, 1.0], dtype=np.float32)
            elif ft == 11: # Amber (Warning)
                c_glow = np.array([1.0, 0.57, 0.0], dtype=np.float32)
            elif ft == 12: # Crimson (Critical)
                c_glow = np.array([1.0, 0.09, 0.27], dtype=np.float32)
            elif ft == 13: # Dim unlit segment
                c_glow = np.array([0.06, 0.12, 0.16], dtype=np.float32)
            else:
                c_glow = np.array([0.0, 0.90, 1.0], dtype=np.float32)

            colors[i] = c_glow
            if ft != 13:
                emissive[i] = c_glow

    return colors, emissive

def render_scene(verts, faces, face_types, cam_pos, target, up, fov_deg=42.0, width=1920, height=1080):
    colors, emissive = compute_shading(verts, faces, face_types)

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

    tri_verts = np.zeros((len(faces), 3, 3), dtype=np.float32)
    tri_verts[:, 0, 0] = x_proj[faces[:, 0]]
    tri_verts[:, 0, 1] = y_proj[faces[:, 0]]
    tri_verts[:, 0, 2] = inv_z[faces[:, 0]]

    tri_verts[:, 1, 0] = x_proj[faces[:, 1]]
    tri_verts[:, 1, 1] = y_proj[faces[:, 1]]
    tri_verts[:, 1, 2] = inv_z[faces[:, 1]]

    tri_verts[:, 2, 0] = x_proj[faces[:, 2]]
    tri_verts[:, 2, 1] = y_proj[faces[:, 2]]
    tri_verts[:, 2, 2] = inv_z[faces[:, 2]]

    bg_color = np.zeros((height, width, 3), dtype=np.float32)
    y_coords, x_coords = np.ogrid[:height, :width]
    dist_sq = ((x_coords - width*0.5)/(width*0.5))**2 + ((y_coords - height*0.4)/(height*0.5))**2
    vignette = np.clip(1.0 - dist_sq * 0.45, 0.0, 1.0)[:, :, None]
    bg_base = np.array([14.0/255.0, 17.0/255.0, 22.0/255.0], dtype=np.float32)
    bg_light = np.array([24.0/255.0, 30.0/255.0, 40.0/255.0], dtype=np.float32)
    bg_color[:, :] = bg_base + (bg_light - bg_base) * vignette

    out_color = bg_color.copy()
    out_emissive = np.zeros((height, width, 3), dtype=np.float32)
    out_depth = np.full((height, width), 1e9, dtype=np.float32)

    rasterize_triangles(
        tri_verts, colors, emissive,
        width, height,
        out_color, out_emissive, out_depth
    )

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
# 3D Diegetic Display Quads Generator (Precision Layout)
# -----------------------------------------------------------------------------
def build_ammo_display_quads(center, size, pitch_deg, clip_val, reserve_val, color_mode="cyan"):
    cx, cy, cz = center
    sx, sy, sz = size
    rad_p = math.radians(pitch_deg)
    cos_p, sin_p = math.cos(rad_p), math.sin(rad_p)

    right = np.array([1.0, 0.0, 0.0], dtype=np.float32)
    fwd = np.array([0.0, sin_p, cos_p], dtype=np.float32)
    fwd /= np.linalg.norm(fwd)
    norm = np.array([0.0, cos_p, -sin_p], dtype=np.float32)
    norm /= np.linalg.norm(norm)

    disp_c = np.array([cx, cy, cz], dtype=np.float32) + norm * (sy * 0.70)

    verts = []
    faces = []
    face_types = []

    def add_quad(c, w, h, f_type):
        idx = len(verts)
        p0 = c - right*(w*0.5) + fwd*(h*0.5)
        p1 = c + right*(w*0.5) + fwd*(h*0.5)
        p2 = c + right*(w*0.5) - fwd*(h*0.5)
        p3 = c - right*(w*0.5) - fwd*(h*0.5)
        verts.extend([p0, p1, p2, p3])
        faces.append([idx, idx+1, idx+2])
        faces.append([idx, idx+2, idx+3])
        face_types.extend([f_type, f_type])

    f_type_active = 10 if color_mode == "cyan" else (11 if color_mode == "amber" else 12)
    f_type_dim = 13

    # Clean 7-segment builder with zero overlap
    def add_digit(c, w, h, digit_char, active_type):
        t = h * 0.16 # segment thickness
        digit_map = {
            '0': 'abcdef', '1': 'bc', '2': 'abged', '3': 'abgcd',
            '4': 'fgbc', '5': 'afgcd', '6': 'afedcg', '7': 'abc',
            '8': 'abcdefg', '9': 'abcdfg'
        }
        active_segs = digit_map.get(str(digit_char), '')
        
        bar_w = w - t * 1.8
        bar_h = (h * 0.5) - t * 0.9

        segs = {
            'a': (c + fwd*(h*0.5 - t*0.5), bar_w, t),
            'b': (c + right*(w*0.5 - t*0.5) + fwd*(h*0.25), t, bar_h),
            'c': (c + right*(w*0.5 - t*0.5) - fwd*(h*0.25), t, bar_h),
            'd': (c - fwd*(h*0.5 - t*0.5), bar_w, t),
            'e': (c - right*(w*0.5 - t*0.5) - fwd*(h*0.25), t, bar_h),
            'f': (c - right*(w*0.5 - t*0.5) + fwd*(h*0.25), t, bar_h),
            'g': (c, bar_w, t)
        }
        for s_name, (sc, sw, sh) in segs.items():
            stype = active_type if s_name in active_segs else f_type_dim
            add_quad(sc, sw, sh, stype)

    sw = sx * 0.74 # active screen width
    sh = sz * 0.70 # active screen height

    dw = sw * 0.20
    dh = sh * 0.75

    rw = sw * 0.14
    rh = sh * 0.54

    # 1. Cartridge Icon
    icon_c = disp_c - right * (sw * 0.40)
    add_quad(icon_c, dw*0.35, dh*0.60, f_type_active)

    # 2. Clip Digits
    c_tens = clip_val // 10
    c_ones = clip_val % 10
    c_tens_pos = disp_c - right * (sw * 0.22)
    c_ones_pos = disp_c - right * (sw * 0.07)
    add_digit(c_tens_pos, dw, dh, c_tens, f_type_active)
    add_digit(c_ones_pos, dw, dh, c_ones, f_type_active)

    # 3. Slash '/' Separator
    slash_c = disp_c + right * (sw * 0.05)
    add_quad(slash_c, dw*0.14, dh*0.85, 10)

    # 4. Reserve Digits
    r_val = reserve_val
    if r_val >= 100:
        r_hund = (r_val // 100) % 10
        r_tens = (r_val // 10) % 10
        r_ones = r_val % 10
        add_digit(disp_c + right * (sw * 0.16), rw, rh, r_hund, 10)
        add_digit(disp_c + right * (sw * 0.27), rw, rh, r_tens, 10)
        add_digit(disp_c + right * (sw * 0.38), rw, rh, r_ones, 10)
    else:
        r_tens = (r_val // 10) % 10
        r_ones = r_val % 10
        add_digit(disp_c + right * (sw * 0.18), rw, rh, r_tens, 10)
        add_digit(disp_c + right * (sw * 0.32), rw, rh, r_ones, 10)

    return np.array(verts, dtype=np.float32), np.array(faces, dtype=np.int32), np.array(face_types, dtype=np.int32)

def build_health_display_quads(center, size, roll_deg=0.0, hp_fraction=1.0):
    cx, cy, cz = center
    sx, sy, sz = size

    fwd = np.array([0.35, 0.90, -0.22], dtype=np.float32)
    fwd /= np.linalg.norm(fwd)
    norm = np.array([-0.35, 0.35, 0.87], dtype=np.float32)
    norm /= np.linalg.norm(norm)
    right = np.cross(norm, fwd)
    right /= np.linalg.norm(right)

    disp_c = np.array([cx, cy, cz], dtype=np.float32) + norm * (sy * 0.70)

    verts = []
    faces = []
    face_types = []

    def add_quad(c, w, h, f_type):
        idx = len(verts)
        p0 = c - right*(w*0.5) + fwd*(h*0.5)
        p1 = c + right*(w*0.5) + fwd*(h*0.5)
        p2 = c + right*(w*0.5) - fwd*(h*0.5)
        p3 = c - right*(w*0.5) - fwd*(h*0.5)
        verts.extend([p0, p1, p2, p3])
        faces.append([idx, idx+1, idx+2])
        faces.append([idx, idx+2, idx+3])
        face_types.extend([f_type, f_type])

    if hp_fraction >= 0.70:
        active_f_type = 10
    elif hp_fraction >= 0.30:
        active_f_type = 11
    else:
        active_f_type = 12

    # 1. Medical Cross Icon at proximal end
    icon_c = disp_c - fwd * (sz * 0.38)
    cross_size = sx * 0.55
    add_quad(icon_c, cross_size, cross_size * 0.32, active_f_type)
    add_quad(icon_c, cross_size * 0.32, cross_size, active_f_type)

    # 2. 10 Discrete Vacuum-Tube Health Cells
    total_segs = 10
    active_count = int(round(hp_fraction * total_segs))
    seg_start = -sz * 0.22
    seg_end   =  sz * 0.42
    seg_step  = (seg_end - seg_start) / float(total_segs)
    seg_h     = seg_step * 0.75
    seg_w     = sx * 0.65

    for i in range(total_segs):
        seg_dist = seg_start + (i + 0.5) * seg_step
        seg_pos = disp_c + fwd * seg_dist
        stype = active_f_type if i < active_count else 13
        add_quad(seg_pos, seg_w, seg_h, stype)

    return np.array(verts, dtype=np.float32), np.array(faces, dtype=np.int32), np.array(face_types, dtype=np.int32)

# -----------------------------------------------------------------------------
# Main Rendering Workflow
# -----------------------------------------------------------------------------
def load_obj_mesh(obj_path):
    verts = []
    faces = []
    with open(obj_path, 'r', encoding='utf-8') as f:
        for line in f:
            if line.startswith('v '):
                parts = line.strip().split()
                verts.append([float(parts[1]), float(parts[2]), float(parts[3])])
            elif line.startswith('f '):
                parts = line.strip().split()
                v_indices = [int(p.split('/')[0]) - 1 for p in parts[1:4]]
                faces.append(v_indices)
    return np.array(verts, dtype=np.float32), np.array(faces, dtype=np.int32)

def add_header_and_badge(img, title, subtitle, badge_text="DIEGETIC HUD"):
    draw = ImageDraw.Draw(img)
    w, h = img.size

    draw.rectangle([40, 35, 480, 105], fill=(10, 13, 18, 220), outline=(40, 52, 68, 255), width=2)
    draw.rectangle([40, 35, 46, 105], fill=(0, 229, 255, 255))
    
    draw.text((60, 42), title.upper(), fill=(255, 255, 255, 255))
    draw.text((60, 68), subtitle, fill=(160, 185, 205, 255))
    draw.text((60, 85), "E99 RETROFUTURISM // KATORGA-12", fill=(0, 229, 255, 255))

    draw.rectangle([w - 320, 35, w - 40, 85], fill=(12, 16, 22, 220), outline=(0, 229, 255, 180), width=2)
    draw.text((w - 300, 50), f"[ {badge_text} ]", fill=(0, 229, 255, 255))

    draw.text((40, h - 35), "SINGULARITY VR // PHYSICAL WEAPON & GLOVE HUD INTEGRATION // ZERO FLOATING UI", fill=(90, 105, 120, 255))
    return img

def create_side_by_side_comparison(img_orig, img_mod, title, subtitle, callouts):
    w, h = img_orig.size
    comp = Image.new('RGB', (w, h), (12, 14, 18))
    
    half_w = w // 2
    left = img_orig.crop((w//4, 0, w//4 + half_w, h))
    right = img_mod.crop((w//4, 0, w//4 + half_w, h))

    comp.paste(left, (0, 0))
    comp.paste(right, (half_w, 0))

    draw = ImageDraw.Draw(comp)
    draw.line([half_w, 0, half_w, h], fill=(0, 229, 255, 200), width=3)

    draw.rectangle([40, 40, 360, 95], fill=(10, 12, 16, 230), outline=(70, 80, 95, 255), width=2)
    draw.text((60, 52), "ORIGINAL VANILLA MESH", fill=(180, 190, 205, 255))
    draw.text((60, 72), "Stock Model // No Diegetic Counter", fill=(120, 130, 145, 255))

    draw.rectangle([half_w + 40, 40, half_w + 420, 95], fill=(10, 16, 22, 230), outline=(0, 229, 255, 255), width=2)
    draw.text((half_w + 60, 52), "MODIFIED DIEGETIC HUD", fill=(255, 255, 255, 255))
    draw.text((half_w + 60, 72), "Milled Bezel // E99 Phosphor Readout", fill=(0, 229, 255, 255))

    draw.rectangle([half_w + 40, h - 140, w - 40, h - 35], fill=(8, 14, 20, 240), outline=(0, 229, 255, 200), width=2)
    draw.text((half_w + 60, h - 128), "TECHNICAL SPECIFICATIONS:", fill=(0, 229, 255, 255))
    for idx, c in enumerate(callouts):
        draw.text((half_w + 60, h - 105 + idx * 18), f"* {c}", fill=(200, 220, 235, 255))

    draw.text((half_w - 200, 20), f"=== {title.upper()} COMPARISON ===", fill=(255, 255, 255, 255))
    return comp

def run_all_previews():
    repo_root = r"c:\Users\Trilex\vr-mods"
    models_dir = os.path.join(repo_root, "SingularityVR-diegetic-hud", "assets", "processed", "hud", "diegetic", "models")
    previews_dir = os.path.join(repo_root, "SingularityVR-diegetic-hud", "docs", "previews", "hud", "diegetic")
    os.makedirs(previews_dir, exist_ok=True)
    
    artifact_dir = r"C:\Users\Trilex\.gemini\antigravity\brain\37e373c5-dc1a-4785-b9b3-94b614b04e5d"

    print("=== Starting Singularity VR Diegetic HUD 3D Render Pipeline ===")

    assets_config = [
        # 1. Centurion Revolver
        {
            'id': 'centurion',
            'name': 'Centurion Revolver',
            'ammo': (6, 24),
            'fixture': {'center': (0.0, 115.3, 43.5), 'size': (2.6, 0.45, 1.8), 'pitch': 15.0},
            'cams': {
                'overview': {'cam': np.array([24.0, 125.0, 28.0]), 'target': np.array([0.0, 107.0, 49.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 42.0},
                'closeup': {'cam': np.array([2.5, 123.0, 36.5]), 'target': np.array([0.0, 115.3, 43.5]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 22.0},
                'vr_ads': {'cam': np.array([0.0, 117.8, 30.0]), 'target': np.array([0.0, 115.5, 62.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 45.0}
            },
            'callouts': [
                "Top-strap milled fixture integrated flush above cylinder",
                "15° rearward tilt matches natural VR hand hold and ADS sightline",
                "Zero obstruction: Front blade remains perfectly centered in rear notch"
            ]
        },
        # 2. Valkyrie AR9
        {
            'id': 'valkyrie',
            'name': 'Valkyrie AR9 Rifle',
            'ammo': (30, 120),
            'fixture': {'center': (0.0, 117.2, 35.0), 'size': (2.8, 0.45, 1.9), 'pitch': 20.0},
            'cams': {
                'overview': {'cam': np.array([42.0, 130.0, 24.0]), 'target': np.array([0.0, 106.0, 52.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 46.0},
                'closeup': {'cam': np.array([3.5, 125.5, 28.0]), 'target': np.array([0.0, 117.2, 35.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 22.0},
                'vr_ads': {'cam': np.array([0.0, 119.8, 14.0]), 'target': np.array([0.0, 117.5, 85.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 48.0}
            },
            'callouts': [
                "Receiver top-rail housing forward of rear sight aperture",
                "20° rearward tilt provides clear sightline without raising gun",
                "Compact Soviet nixie numerals display 30 rounds clip / 120 reserve"
            ]
        },
        # 3. Spikeshot Shotgun
        {
            'id': 'spikeshot',
            'name': 'Spikeshot Shotgun',
            'ammo': (8, 32),
            'fixture': {'center': (0.0, 116.8, 42.0), 'size': (2.8, 0.45, 1.9), 'pitch': 15.0},
            'cams': {
                'overview': {'cam': np.array([42.0, 130.0, 28.0]), 'target': np.array([0.0, 110.0, 50.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 48.0},
                'closeup': {'cam': np.array([3.5, 125.0, 34.0]), 'target': np.array([0.0, 116.8, 42.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 22.0},
                'vr_ads': {'cam': np.array([0.0, 119.6, 22.0]), 'target': np.array([0.0, 117.0, 105.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 48.0}
            },
            'callouts': [
                "Receiver top rail mount positioned forward of heavy breech",
                "Recessed display protects phosphor glass from heavy recoil",
                "Instant glance readability during frantic close-quarters VR combat"
            ]
        },
        # 4. Kasimov Sniper Rifle
        {
            'id': 'kasimov',
            'name': 'Kasimov SNV-E99 Sniper Rifle',
            'ammo': (5, 20),
            'fixture': {'center': (0.0, 116.5, 40.0), 'size': (2.7, 0.45, 1.9), 'pitch': 15.0},
            'cams': {
                'overview': {'cam': np.array([52.0, 134.0, 24.0]), 'target': np.array([0.0, 112.0, 48.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 50.0},
                'closeup': {'cam': np.array([3.5, 124.5, 32.5]), 'target': np.array([0.0, 116.5, 40.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 22.0},
                'vr_ads': {'cam': np.array([0.0, 120.2, 16.0]), 'target': np.array([0.0, 118.0, 112.0]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 48.0}
            },
            'callouts': [
                "Receiver chassis mount beneath optical scope brackets",
                "Visible below scope eye relief without obstruction",
                "Displays 5 heavy sniper cartridges and 20 reserve rounds"
            ]
        },
        # 5. Left Glove / TMD Gauntlet
        {
            'id': 'tmd_glove',
            'name': 'Left Glove / TMD Bio-Monitor',
            'is_wrist': True,
            'fixture': {'center': (39.5, 99.5, -3.2), 'size': (1.8, 0.45, 4.8)},
            'cams': {
                'overview': {'cam': np.array([58.0, 126.0, 22.0]), 'target': np.array([39.5, 99.5, -3.2]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 46.0},
                'closeup': {'cam': np.array([32.0, 108.0, 5.0]), 'target': np.array([39.5, 99.5, -3.2]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 20.0},
                'vr_ads': {'cam': np.array([28.0, 120.0, 12.0]), 'target': np.array([39.5, 99.5, -3.2]), 'up': np.array([0.0, 1.0, 0.0]), 'fov': 35.0}
            },
            'callouts': [
                "Curved dorsal cuff mounted on TMD gauntlet wrist casing",
                "Outward-facing angle aligns directly with VR headset when arm is raised",
                "10-segment E99 vacuum-tube gauge: Cyan (Normal) -> Amber (50%) -> Crimson (20%)"
            ]
        }
    ]

    generated_paths = []

    for cfg in assets_config:
        model_id = cfg['id']
        name = cfg['name']
        print(f"--- Rendering {name} ---")

        orig_obj = os.path.join(models_dir, f"{model_id}_original.obj")
        mod_obj = os.path.join(models_dir, f"{model_id}_diegetic.obj")

        v_orig, f_orig = load_obj_mesh(orig_obj)
        v_mod, f_mod = load_obj_mesh(mod_obj)

        ft_orig = np.full(len(f_orig), -1, dtype=np.int32)
        ft_mod = np.full(len(f_mod), 0, dtype=np.int32)
        ft_mod[:len(f_orig)] = -1
        ft_mod[-18:-16] = 1 # screen
        ft_mod[-16:] = 2 # rivets

        if not cfg.get('is_wrist', False):
            d_verts, d_faces, d_types = build_ammo_display_quads(
                cfg['fixture']['center'], cfg['fixture']['size'], cfg['fixture'].get('pitch', 0.0),
                cfg['ammo'][0], cfg['ammo'][1], color_mode="cyan"
            )
        else:
            d_verts, d_faces, d_types = build_health_display_quads(
                cfg['fixture']['center'], cfg['fixture']['size'],
                hp_fraction=1.0
            )

        offset = len(v_mod)
        v_full = np.vstack([v_mod, d_verts])
        f_full = np.vstack([f_mod, d_faces + offset])
        ft_full = np.concatenate([ft_mod, d_types])

        cams = cfg['cams']

        # A. Overview Render
        img_over = render_scene(v_full, f_full, ft_full, cams['overview']['cam'], cams['overview']['target'], cams['overview']['up'], cams['overview']['fov'])
        add_header_and_badge(img_over, name, "FULL OVERVIEW // DIEGETIC INTEGRATION", "ACCEPTED PROFILE")
        path_over = os.path.join(previews_dir, f"{model_id}_overview.png")
        img_over.save(path_over)
        generated_paths.append(path_over)

        # B. Close-Up Macro Render
        img_close = render_scene(v_full, f_full, ft_full, cams['closeup']['cam'], cams['closeup']['target'], cams['closeup']['up'], cams['closeup']['fov'])
        add_header_and_badge(img_close, name, "MACRO CLOSE-UP // MILLED BEZEL & PHOSPHOR DISPLAY", "1024 ATLAS")
        path_close = os.path.join(previews_dir, f"{model_id}_closeup.png")
        img_close.save(path_close)
        generated_paths.append(path_close)

        # C. First-Person VR View
        img_vr = render_scene(v_full, f_full, ft_full, cams['vr_ads']['cam'], cams['vr_ads']['target'], cams['vr_ads']['up'], cams['vr_ads']['fov'])
        view_subtitle = "FIRST-PERSON VR // ADS SIGHTLINE CLEARANCE" if not cfg.get('is_wrist') else "FIRST-PERSON VR // RAISED WRIST ELEVATION"
        add_header_and_badge(img_vr, name, view_subtitle, "100% CLEAR")
        path_vr = os.path.join(previews_dir, f"{model_id}_first_person.png")
        img_vr.save(path_vr)
        generated_paths.append(path_vr)

        # D. Side-by-Side Comparison
        img_orig_over = render_scene(v_orig, f_orig, ft_orig, cams['overview']['cam'], cams['overview']['target'], cams['overview']['up'], cams['overview']['fov'])
        img_comp = create_side_by_side_comparison(img_orig_over, img_over, name, "PHYSICAL DIEGETIC INTEGRATION", cfg['callouts'])
        path_comp = os.path.join(previews_dir, f"{model_id}_comparison.png")
        img_comp.save(path_comp)
        generated_paths.append(path_comp)

        # E. For TMD Glove: Health State Variants
        if cfg.get('is_wrist'):
            # Amber 50%
            d_verts_amb, d_faces_amb, d_types_amb = build_health_display_quads(
                cfg['fixture']['center'], cfg['fixture']['size'], hp_fraction=0.5
            )
            v_amb = np.vstack([v_mod, d_verts_amb])
            f_amb = np.vstack([f_mod, d_faces_amb + offset])
            ft_amb = np.concatenate([ft_mod, d_types_amb])
            img_amb = render_scene(v_amb, f_amb, ft_amb, cams['closeup']['cam'], cams['closeup']['target'], cams['closeup']['up'], cams['closeup']['fov'])
            add_header_and_badge(img_amb, name, "HEALTH STATE // 50% WARNING (AMBER)", "STATE: WARN")
            path_amb = os.path.join(previews_dir, f"{model_id}_health_amber.png")
            img_amb.save(path_amb)
            generated_paths.append(path_amb)

            # Crimson 20%
            d_verts_crm, d_faces_crm, d_types_crm = build_health_display_quads(
                cfg['fixture']['center'], cfg['fixture']['size'], hp_fraction=0.2
            )
            v_crm = np.vstack([v_mod, d_verts_crm])
            f_crm = np.vstack([f_mod, d_faces_crm + offset])
            ft_crm = np.concatenate([ft_mod, d_types_crm])
            img_crm = render_scene(v_crm, f_crm, ft_crm, cams['closeup']['cam'], cams['closeup']['target'], cams['closeup']['up'], cams['closeup']['fov'])
            add_header_and_badge(img_crm, name, "HEALTH STATE // 20% CRITICAL ALARM (CRIMSON)", "STATE: ALARM")
            path_crm = os.path.join(previews_dir, f"{model_id}_health_crimson.png")
            img_crm.save(path_crm)
            generated_paths.append(path_crm)

    for p in generated_paths:
        fname = os.path.basename(p)
        art_dest = os.path.join(artifact_dir, fname)
        shutil.copy2(p, art_dest)

    print(f"\nAll {len(generated_paths)} preview images rendered and saved to:")
    print(f"  Repo: {previews_dir}")
    print(f"  Artifact: {artifact_dir}")

if __name__ == "__main__":
    run_all_previews()
