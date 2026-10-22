#!/usr/bin/env python3
"""Singularity VR — Diegetic HUD 3D Model Generator & Multi-Material PBR Exporter (V2.1).
Generates production-grade bespoke diegetic HUD fixtures:
1. Centurion Revolver (Arched frame contour mount with knurled brass bolts, '06/24' 7-segment readout)
2. Valkyrie AR9 Rifle (Low-profile receiver plate with front chamfer ramp, '30/120' 7-segment readout)
3. Spikeshot Shotgun (Heavy ruggedized cast-iron housing with lateral shock ribs, '08/32' 7-segment readout)
4. Kasimov Sniper Rifle (Cantilever scope bracket mount with anti-glare visor hood, '05/20' 7-segment readout)
5. Left Glove / TMD Gauntlet (Curved ergonomic cuff with 10 vacuum-tube cell gauge & Soviet medical cross)

Key fixes in V2.1:
- Outward box face winding (strictly outward-pointing normals on all 6 faces, zero inverted lighting)
- True 7-segment VFD numerals (crisp, authentic Soviet typography, zero solid illuminated rectangles)
- Recessed display well layering (dark cathode bed behind readouts, zero opaque glass occlusion)
- Multi-material GLTF 2.0 Binary (.glb) export preserving separate submeshes and PBR materials
- Full 4K diffuse and normal textures preserved (zero 2K downsampling)
"""
import io
import math
import os
import shutil
import struct
import sys
from pathlib import Path
import numpy as np
from PIL import Image
import trimesh

REPO_ROOT = Path(r"C:\Users\Trilex\vr-mods")
WORKTREE_ROOT = REPO_ROOT / "SingularityVR-diegetic-hud"
OUT_MODELS_DIR = WORKTREE_ROOT / "assets" / "processed" / "hud" / "diegetic" / "models_v2"
TEX_DIR = WORKTREE_ROOT / "assets" / "processed" / "textures_remastered"

OUT_MODELS_DIR.mkdir(parents=True, exist_ok=True)

# -----------------------------------------------------------------------------
# PSK Parser with UV Extraction
# -----------------------------------------------------------------------------
def parse_psk_with_uvs(data):
    p = 0
    chunks = {}
    while p < len(data):
        raw, _, size, count = struct.unpack_from('<20siii', data, p)
        name = raw.split(b'\0')[0].decode('ascii')
        p += 32
        chunks[name] = (size, count, data[p:p+size*count])
        p += size * count
    
    pts_size, pts_count, pts_data = chunks['PNTS0000']
    points = np.frombuffer(pts_data, '<f4', pts_count * 3).reshape(-1, 3).astype(np.float32)
    
    wedges_size, wedges_count, wedges_data = chunks['VTXW0000']
    wedges = [struct.unpack_from('<IffBBH', wedges_data, i*16) for i in range(wedges_count)]
    
    faces_size, faces_count, faces_data = chunks['FACE0000']
    faces = [struct.unpack_from('<3HBBI', faces_data, i*12)[:3] for i in range(faces_count)]
    
    tri_pts = []
    tri_uvs = []
    for tri in faces:
        w0, w1, w2 = wedges[tri[0]], wedges[tri[1]], wedges[tri[2]]
        tri_pts.append((w0[0], w1[0], w2[0]))
        tri_uvs.append(((w0[1], w0[2]), (w1[1], w1[2]), (w2[1], w2[2])))
        
    return points, np.array(tri_pts, dtype=np.int32), np.array(tri_uvs, dtype=np.float32)

# -----------------------------------------------------------------------------
# Robust Geometry Builder with Strict Outward Normals
# -----------------------------------------------------------------------------
class MeshBuilder:
    def __init__(self):
        self.verts = []
        self.uvs = []
        self.faces = []
        self.face_types = [] # -1=Weapon, 0=Bezel, 1=CathodeBed, 2=Rivet/Bolt, 3=EmissiveDisplay, 4=AccentBrass

    def add_quad(self, p0, p1, p2, p3, uv0=(0,0), uv1=(1,0), uv2=(1,1), uv3=(0,1), face_type=0):
        """Adds a quad split into two counter-clockwise outward triangles."""
        idx = len(self.verts)
        self.verts.extend([p0, p1, p2, p3])
        self.uvs.extend([uv0, uv1, uv2, uv3])
        # Two triangles with outward winding: (p0, p1, p2) and (p0, p2, p3)
        self.faces.append([idx, idx+1, idx+2])
        self.faces.append([idx, idx+2, idx+3])
        self.face_types.extend([face_type, face_type])

    def add_box(self, center, size, right, fwd, up, face_type=0):
        """
        Adds a 3D box with guaranteed strictly outward-pointing normals on all 6 faces.
        right: local +X
        fwd: local +Z
        up: local +Y (display face surface normal)
        """
        c = np.array(center, dtype=np.float32)
        r = np.array(right, dtype=np.float32) * (size[0] * 0.5)
        u = np.array(up, dtype=np.float32) * (size[1] * 0.5)
        f = np.array(fwd, dtype=np.float32) * (size[2] * 0.5)

        # 8 corners
        p0 = c - r - f + u # top front-left
        p1 = c + r - f + u # top front-right
        p2 = c + r + f + u # top back-right
        p3 = c - r + f + u # top back-left
        p4 = c - r - f - u # bot front-left
        p5 = c + r - f - u # bot front-right
        p6 = c + r + f - u # bot back-right
        p7 = c - r + f - u # bot back-left

        # Top (+u outward)
        self.add_quad(p0, p3, p2, p1, face_type=face_type)
        # Bottom (-u outward)
        self.add_quad(p4, p5, p6, p7, face_type=face_type)
        # Front (-f outward)
        self.add_quad(p0, p1, p5, p4, face_type=face_type)
        # Back (+f outward)
        self.add_quad(p2, p3, p7, p6, face_type=face_type)
        # Left (-r outward)
        self.add_quad(p3, p0, p4, p7, face_type=face_type)
        # Right (+r outward)
        self.add_quad(p1, p2, p6, p5, face_type=face_type)

    def add_cylinder(self, center, radius, height, axis, segments=12, face_type=2):
        """Adds a cylinder aligned with `axis` with outward normals."""
        c = np.array(center, dtype=np.float32)
        a = np.array(axis, dtype=np.float32)
        a /= np.linalg.norm(a)

        ref = np.array([0, 1, 0], dtype=np.float32) if abs(a[1]) < 0.9 else np.array([1, 0, 0], dtype=np.float32)
        r = np.cross(a, ref)
        r /= np.linalg.norm(r)
        u = np.cross(r, a)

        half_h = height * 0.5
        top_pts = []
        bot_pts = []

        for i in range(segments):
            theta = 2.0 * math.pi * i / segments
            x = math.cos(theta) * radius
            y = math.sin(theta) * radius
            p_top = c + a * half_h + r * x + u * y
            p_bot = c - a * half_h + r * x + u * y
            top_pts.append(p_top)
            bot_pts.append(p_bot)

        for i in range(segments):
            next_i = (i + 1) % segments
            # Side quad with outward normal
            self.add_quad(top_pts[i], top_pts[next_i], bot_pts[next_i], bot_pts[i], face_type=face_type)

        top_center = c + a * half_h
        idx_c = len(self.verts)
        self.verts.append(top_center)
        self.uvs.append((0.5, 0.5))
        for i in range(segments):
            next_i = (i + 1) % segments
            idx1 = len(self.verts)
            self.verts.extend([top_pts[i], top_pts[next_i]])
            self.uvs.extend([(0,0), (1,0)])
            self.faces.append([idx_c, idx1, idx1+1])
            self.face_types.append(face_type)

# -----------------------------------------------------------------------------
# Authentic 7-Segment VFD Numeral Generator
# -----------------------------------------------------------------------------
def add_digit_7seg(b, center, char, digit_w, digit_h, right, fwd, up, seg_thick=0.03, seg_w=0.07, gap=0.015, face_type=3):
    """
    Renders an authentic Soviet 7-segment phosphor display numeral (0-9).
    center: center of the digit bounding box on display plane
    right: horizontal axis across display (+X)
    fwd: vertical axis up along display (+Z)
    up: display surface outward normal (+Y)
    """
    lookup = {
        '0': ('a', 'b', 'c', 'd', 'e', 'f'),
        '1': ('b', 'c'),
        '2': ('a', 'b', 'g', 'e', 'd'),
        '3': ('a', 'b', 'g', 'c', 'd'),
        '4': ('f', 'g', 'b', 'c'),
        '5': ('a', 'f', 'g', 'c', 'd'),
        '6': ('a', 'f', 'e', 'd', 'c', 'g'),
        '7': ('a', 'b', 'c'),
        '8': ('a', 'b', 'c', 'd', 'e', 'f', 'g'),
        '9': ('a', 'b', 'c', 'd', 'f', 'g'),
    }
    active_segs = set(lookup.get(str(char), ()))
    if not active_segs:
        return

    c = np.array(center, dtype=np.float32)
    hw = digit_w * 0.5
    hh = digit_h * 0.5

    h_len = max(0.01, digit_w - 2.0 * seg_w - 2.0 * gap)
    v_len = max(0.01, hh - seg_w - 1.5 * gap)

    # a (top horizontal)
    if 'a' in active_segs:
        pos = c + fwd * (hh - seg_w * 0.5)
        b.add_box(pos, (h_len, seg_thick, seg_w), right, fwd, up, face_type=face_type)

    # d (bottom horizontal)
    if 'd' in active_segs:
        pos = c - fwd * (hh - seg_w * 0.5)
        b.add_box(pos, (h_len, seg_thick, seg_w), right, fwd, up, face_type=face_type)

    # g (middle horizontal)
    if 'g' in active_segs:
        pos = c
        b.add_box(pos, (h_len, seg_thick, seg_w), right, fwd, up, face_type=face_type)

    # b (top-right vertical)
    if 'b' in active_segs:
        pos = c + right * (hw - seg_w * 0.5) + fwd * (hh * 0.5)
        b.add_box(pos, (seg_w, seg_thick, v_len), right, fwd, up, face_type=face_type)

    # c (bottom-right vertical)
    if 'c' in active_segs:
        pos = c + right * (hw - seg_w * 0.5) - fwd * (hh * 0.5)
        b.add_box(pos, (seg_w, seg_thick, v_len), right, fwd, up, face_type=face_type)

    # f (top-left vertical)
    if 'f' in active_segs:
        pos = c - right * (hw - seg_w * 0.5) + fwd * (hh * 0.5)
        b.add_box(pos, (seg_w, seg_thick, v_len), right, fwd, up, face_type=face_type)

    # e (bottom-left vertical)
    if 'e' in active_segs:
        pos = c - right * (hw - seg_w * 0.5) - fwd * (hh * 0.5)
        b.add_box(pos, (seg_w, seg_thick, v_len), right, fwd, up, face_type=face_type)

def add_slash(b, center, w, h, right, fwd, up, seg_thick=0.03, face_type=3):
    """Renders a crisp diagonal separator slash '/'."""
    c = np.array(center, dtype=np.float32)
    num_steps = 4
    for i in range(num_steps):
        t = (i - (num_steps - 1) * 0.5) / max(1, num_steps - 1)
        pos = c + right * (t * w * 0.75) + fwd * (t * h * 0.82)
        step_sz = (w * 0.35, seg_thick, h / num_steps * 1.25)
        b.add_box(pos, step_sz, right, fwd, up, face_type=face_type)

def add_ammo_counter(b, center, primary_str, reserve_str, right, fwd, up, total_width=1.6, total_height=0.8, face_type=3):
    """
    Renders a complete ammunition counter: primary clip ammo, slash '/', and reserve ammo.
    """
    c = np.array(center, dtype=np.float32)
    num_prim = len(primary_str)
    num_res = len(reserve_str)

    prim_h = total_height * 0.78
    prim_w = prim_h * 0.52

    res_h = total_height * 0.56
    res_w = res_h * 0.52

    slash_w = prim_w * 0.35
    slash_h = prim_h * 0.90

    gap_char = total_width * 0.035
    gap_slash = total_width * 0.055

    total_w = (num_prim * prim_w + (num_prim - 1) * gap_char +
               gap_slash + slash_w + gap_slash +
               num_res * res_w + (num_res - 1) * gap_char)

    start_x = -total_w * 0.5
    cur_x = start_x

    # Primary digits
    for ch in primary_str:
        digit_center = c + right * (cur_x + prim_w * 0.5)
        add_digit_7seg(b, digit_center, ch, prim_w, prim_h, right, fwd, up, seg_thick=0.03, seg_w=prim_w*0.22, gap=0.015, face_type=face_type)
        cur_x += prim_w + gap_char

    cur_x += gap_slash - gap_char
    # Slash
    slash_center = c + right * (cur_x + slash_w * 0.5)
    add_slash(b, slash_center, slash_w, slash_h, right, fwd, up, seg_thick=0.03, face_type=face_type)
    cur_x += slash_w + gap_slash

    # Reserve digits
    for ch in reserve_str:
        digit_center = c + right * (cur_x + res_w * 0.5) - fwd * ((prim_h - res_h) * 0.35)
        add_digit_7seg(b, digit_center, ch, res_w, res_h, right, fwd, up, seg_thick=0.03, seg_w=res_w*0.22, gap=0.012, face_type=face_type)
        cur_x += res_w + gap_char

# -----------------------------------------------------------------------------
# Bespoke Fixture Builders V2.1
# -----------------------------------------------------------------------------

# 1. Centurion Revolver (Top strap arched contour bracket + brass cap screws + 06/24)
def build_centurion_fixture_v2(center, size, pitch_deg=18.0):
    b = MeshBuilder()
    cx, cy, cz = center
    sx, sy, sz = size

    rad_p = math.radians(pitch_deg)
    cos_p, sin_p = math.cos(rad_p), math.sin(rad_p)

    fwd = np.array([0.0, sin_p, cos_p], dtype=np.float32)
    norm = np.array([0.0, cos_p, -sin_p], dtype=np.float32)
    right = np.array([1.0, 0.0, 0.0], dtype=np.float32)

    # 1. Main Bezel Body (Milled steel housing)
    b.add_box(center, size, right, fwd, norm, face_type=0)

    # 2. Recessed Dark Cathode Display Bed
    bed_center = np.array(center) + norm * (sy * 0.38)
    bed_size = (sx * 0.82, sy * 0.08, sz * 0.78)
    b.add_box(bed_center, bed_size, right, fwd, norm, face_type=1)

    # 3. Twin Knurled Brass Retaining Side Screws
    screw_offset = sx * 0.52
    screw_l = np.array(center) - right * screw_offset
    screw_r = np.array(center) + right * screw_offset
    b.add_cylinder(screw_l, radius=0.22, height=sy*0.8, axis=right, segments=10, face_type=2)
    b.add_cylinder(screw_r, radius=0.22, height=sy*0.8, axis=right, segments=10, face_type=2)

    # 4. True 7-Segment Readout Elements (Ammo: 06 / 24)
    disp_center = bed_center + norm * (bed_size[1] * 0.5 + 0.015)
    add_ammo_counter(b, disp_center, '06', '24', right, fwd, norm, total_width=sx*0.72, total_height=sz*0.65, face_type=3)

    return np.array(b.verts, dtype=np.float32), np.array(b.faces, dtype=np.int32), np.array(b.uvs, dtype=np.float32), np.array(b.face_types, dtype=np.int32)

# 2. Valkyrie AR9 Rifle (Low-profile receiver plate + front ramp + 30/120)
def build_valkyrie_fixture_v2(center, size, pitch_deg=20.0):
    b = MeshBuilder()
    cx, cy, cz = center
    sx, sy, sz = size

    rad_p = math.radians(pitch_deg)
    cos_p, sin_p = math.cos(rad_p), math.sin(rad_p)

    fwd = np.array([0.0, sin_p, cos_p], dtype=np.float32)
    norm = np.array([0.0, cos_p, -sin_p], dtype=np.float32)
    right = np.array([1.0, 0.0, 0.0], dtype=np.float32)

    # 1. Main Housing
    b.add_box(center, size, right, fwd, norm, face_type=0)

    # 2. Front Chamfer Ramp
    ramp_center = np.array(center) + fwd * (sz * 0.55) - norm * (sy * 0.2)
    b.add_box(ramp_center, (sx, sy * 0.6, sz * 0.3), right, fwd, norm, face_type=0)

    # 3. Recessed Dark Cathode Display Bed
    bed_center = np.array(center) + norm * (sy * 0.38)
    bed_size = (sx * 0.84, sy * 0.08, sz * 0.76)
    b.add_box(bed_center, bed_size, right, fwd, norm, face_type=1)

    # 4. Four Countersunk Hex Socket Screws
    rx = sx * 0.42
    rz = sz * 0.38
    for ox in (-rx, rx):
        for oz in (-rz, rz):
            b.add_cylinder(np.array(center) + right*ox + fwd*oz + norm*(sy*0.48), radius=0.14, height=sy*0.12, axis=norm, segments=8, face_type=2)

    # 5. True 7-Segment Ammo Readout Elements (Ammo: 30 / 120)
    disp_center = bed_center + norm * (bed_size[1] * 0.5 + 0.015)
    add_ammo_counter(b, disp_center, '30', '120', right, fwd, norm, total_width=sx*0.75, total_height=sz*0.65, face_type=3)

    return np.array(b.verts, dtype=np.float32), np.array(b.faces, dtype=np.int32), np.array(b.uvs, dtype=np.float32), np.array(b.face_types, dtype=np.int32)

# 3. Spikeshot Shotgun (Heavy cast-iron housing + lateral shock ribs + 08/32)
def build_spikeshot_fixture_v2(center, size, pitch_deg=15.0):
    b = MeshBuilder()
    cx, cy, cz = center
    sx, sy, sz = size

    rad_p = math.radians(pitch_deg)
    cos_p, sin_p = math.cos(rad_p), math.sin(rad_p)

    fwd = np.array([0.0, sin_p, cos_p], dtype=np.float32)
    norm = np.array([0.0, cos_p, -sin_p], dtype=np.float32)
    right = np.array([1.0, 0.0, 0.0], dtype=np.float32)

    # 1. Heavy Base Housing
    b.add_box(center, size, right, fwd, norm, face_type=0)

    # 2. Three Lateral Shock-Absorbing Ribs on Each Flank
    rib_offsets = [-sz*0.35, 0.0, sz*0.35]
    for ro in rib_offsets:
        rib_l = np.array(center) - right * (sx * 0.52) + fwd * ro
        rib_r = np.array(center) + right * (sx * 0.52) + fwd * ro
        b.add_box(rib_l, (sx * 0.10, sy * 0.70, sz * 0.16), right, fwd, norm, face_type=4)
        b.add_box(rib_r, (sx * 0.10, sy * 0.70, sz * 0.16), right, fwd, norm, face_type=4)

    # 3. Recessed Dark Cathode Display Bed
    bed_center = np.array(center) + norm * (sy * 0.38)
    bed_size = (sx * 0.80, sy * 0.08, sz * 0.76)
    b.add_box(bed_center, bed_size, right, fwd, norm, face_type=1)

    # 4. True 7-Segment Ammo Readout Elements (Ammo: 08 / 32)
    disp_center = bed_center + norm * (bed_size[1] * 0.5 + 0.015)
    add_ammo_counter(b, disp_center, '08', '32', right, fwd, norm, total_width=sx*0.72, total_height=sz*0.65, face_type=3)

    return np.array(b.verts, dtype=np.float32), np.array(b.faces, dtype=np.int32), np.array(b.uvs, dtype=np.float32), np.array(b.face_types, dtype=np.int32)

# 4. Kasimov Sniper Rifle (Canted cantilever scope bracket mount + anti-glare visor + 05/20)
def build_kasimov_fixture_v2(center, size, pitch_deg=15.0, cant_deg=32.0):
    b = MeshBuilder()
    cx, cy, cz = center
    sx, sy, sz = size

    rad_p = math.radians(pitch_deg)
    rad_c = math.radians(cant_deg)

    # Pitched base vectors along rifle axis
    fwd = np.array([0.0, math.sin(rad_p), math.cos(rad_p)], dtype=np.float32)
    norm_base = np.array([0.0, math.cos(rad_p), -math.sin(rad_p)], dtype=np.float32)
    right_base = np.array([1.0, 0.0, 0.0], dtype=np.float32)

    # Cant around fwd toward shooter eye (+X direction from left mount)
    norm = norm_base * math.cos(rad_c) + right_base * math.sin(rad_c)
    norm /= np.linalg.norm(norm)
    right = np.cross(norm, fwd)
    right /= np.linalg.norm(right)
    fwd = np.cross(right, norm)
    fwd /= np.linalg.norm(fwd)

    # 1. Main Scope Mount Chassis (Canted Left-Rail Housing)
    b.add_box(center, size, right, fwd, norm, face_type=0)

    # 2. Anti-glare Hood Visor forward of screen
    hood_c = np.array(center) + fwd * (sz * 0.52) + norm * (sy * 0.45)
    b.add_box(hood_c, (sx * 0.95, sy * 0.35, sz * 0.16), right, fwd, norm, face_type=0)

    # 3. Recessed Dark Cathode Display Bed
    bed_center = np.array(center) + norm * (sy * 0.38)
    bed_size = (sx * 0.78, sy * 0.08, sz * 0.74)
    b.add_box(bed_center, bed_size, right, fwd, norm, face_type=1)

    # 4. Cantilever Clamp Arm bridging from housing inner edge towards scope rail dovetail
    arm_c = np.array(center) + right * (sx * 0.5 + 0.12) - norm * (sy * 0.15)
    arm_sz = (0.28, sy * 0.70, sz * 0.60)
    b.add_box(arm_c, arm_sz, right, fwd, norm, face_type=0)

    # 5. Brass Knurled Scope Bracket Clamp Studs
    b.add_cylinder(np.array(center) - right*(sx*0.42) + fwd*(sz*0.35), radius=0.14, height=sy*0.8, axis=norm, segments=10, face_type=2)
    b.add_cylinder(np.array(center) - right*(sx*0.42) - fwd*(sz*0.35), radius=0.14, height=sy*0.8, axis=norm, segments=10, face_type=2)

    # 6. True 7-Segment Ammo Readout Elements (Ammo: 05 / 20)
    disp_center = bed_center + norm * (bed_size[1] * 0.5 + 0.015)
    add_ammo_counter(b, disp_center, '05', '20', right, fwd, norm, total_width=sx*0.70, total_height=sz*0.65, face_type=3)

    return np.array(b.verts, dtype=np.float32), np.array(b.faces, dtype=np.int32), np.array(b.uvs, dtype=np.float32), np.array(b.face_types, dtype=np.int32)

# 5. Left Glove / TMD Gauntlet (Curved dorsal crest housing + contoured saddle wings + 10-cell gauge)
def build_tmd_wrist_fixture_v2(center, size):
    b = MeshBuilder()
    cx, cy, cz = center
    sx, sy, sz = size

    # Strictly right-handed orthonormal basis matching dorsal wrist curvature
    norm = np.array([-0.2500689, 0.45012403, 0.8572362], dtype=np.float32)
    norm /= np.linalg.norm(norm)
    fwd = np.array([-0.1694995, 0.85134184, -0.49647456], dtype=np.float32)
    fwd /= np.linalg.norm(fwd)
    right = np.cross(norm, fwd)
    right /= np.linalg.norm(right)
    fwd = np.cross(right, norm)
    fwd /= np.linalg.norm(fwd)

    # 1. Main Ergonomic Dorsal Crest Housing
    b.add_box(center, size, right, fwd, norm, face_type=0)

    # 2. Contoured Ergonomic Saddle Clamping Wings
    wing_w = 0.22
    wing_drop = 0.10
    wing_l = np.array(center) - right * (sx * 0.5 + wing_w * 0.5) - norm * (wing_drop * 0.5)
    wing_r = np.array(center) + right * (sx * 0.5 + wing_w * 0.5) - norm * (wing_drop * 0.5)
    b.add_box(wing_l, (wing_w, sy + wing_drop, sz * 0.92), right, fwd, norm, face_type=0)
    b.add_box(wing_r, (wing_w, sy + wing_drop, sz * 0.92), right, fwd, norm, face_type=0)

    # 3. Recessed Dark Cathode Display Bed
    bed_c = np.array(center) + norm * (sy * 0.38)
    bed_sz = (sx * 0.82, sy * 0.08, sz * 0.84)
    b.add_box(bed_c, bed_sz, right, fwd, norm, face_type=1)

    # 4. Embossed Soviet Medical Cross Plate on Left
    cross_c = bed_c - right * (sx * 0.28) + norm * (bed_sz[1] * 0.5 + 0.015)
    b.add_box(cross_c, (sx*0.14, 0.02, sz*0.06), right, fwd, norm, face_type=4)
    b.add_box(cross_c, (sx*0.05, 0.02, sz*0.18), right, fwd, norm, face_type=4)

    # 5. 10 Segmented Vacuum-Tube Health Cells with Physical Divider Ribs
    gauge_c = bed_c + right * (sx * 0.10) + norm * (bed_sz[1] * 0.5 + 0.015)
    cell_w = sx * 0.48
    cell_h = sz * 0.70
    num_cells = 10
    step_z = cell_h / num_cells

    for i in range(num_cells):
        offset_z = -cell_h * 0.5 + (i + 0.5) * step_z
        cell_center = gauge_c + fwd * offset_z
        b.add_box(cell_center, (cell_w * 0.85, 0.02, step_z * 0.70), right, fwd, norm, face_type=3)
        if i < num_cells - 1:
            rib_center = gauge_c + fwd * (-cell_h * 0.5 + (i + 1) * step_z)
            b.add_box(rib_center, (cell_w * 0.95, 0.025, step_z * 0.20), right, fwd, norm, face_type=0)

    # 6. Corner Bezel Screws
    rx = sx * 0.42
    rz = sz * 0.44
    for ox in (-rx, rx):
        for oz in (-rz, rz):
            b.add_cylinder(np.array(center) + right*ox + fwd*oz + norm*(sy*0.48), radius=0.12, height=sy*0.14, axis=norm, segments=8, face_type=2)

    return np.array(b.verts, dtype=np.float32), np.array(b.faces, dtype=np.int32), np.array(b.uvs, dtype=np.float32), np.array(b.face_types, dtype=np.int32)

# -----------------------------------------------------------------------------
# Exporter with Multi-Material GLB & Co-Located Textures
# -----------------------------------------------------------------------------
def export_textured_model_v2(out_dir, model_name, pts, faces, uvs, face_types, tex_src_path, norm_src_path=None):
    """
    Exports:
    1. Standard .obj + .mtl with co-located local texture references.
    2. Complete multi-material GLTF 2.0 Binary (.glb) preserving separate submeshes,
       full 4K resolution textures (no downsampling!), and PBR metallic-roughness properties.
    """
    out_dir = Path(out_dir)
    obj_path = out_dir / f"{model_name}.obj"
    mtl_path = out_dir / f"{model_name}.mtl"
    glb_path = out_dir / f"{model_name}.glb"

    # Co-locate 4K diffuse texture
    tex_filename = tex_src_path.name
    tex_dst_path = out_dir / tex_filename
    if tex_src_path.resolve() != tex_dst_path.resolve():
        shutil.copy2(tex_src_path, tex_dst_path)

    # Write MTL
    mtl_name = f"{model_name}.mtl"
    with open(mtl_path, "w", encoding="utf-8") as f:
        f.write("# Singularity VR Diegetic HUD V2.1 Material Definition\n")
        f.write("newmtl WeaponRemastered\n")
        f.write("Ka 1.0 1.0 1.0\nKd 1.0 1.0 1.0\nKs 0.45 0.45 0.45\nNs 32.0\n")
        f.write(f"map_Kd {tex_filename}\n\n")

        f.write("newmtl BezelSoviet\n")
        f.write("Ka 0.20 0.24 0.28\nKd 0.22 0.26 0.32\nKs 0.85 0.85 0.85\nNs 45.0\n\n")

        f.write("newmtl CathodeGlass\n")
        f.write("Ka 0.03 0.05 0.08\nKd 0.04 0.06 0.09\nKs 0.30 0.35 0.40\nNs 90.0\n\n")

        f.write("newmtl RivetMetal\n")
        f.write("Ka 0.55 0.60 0.68\nKd 0.60 0.65 0.72\nKs 0.95 0.95 0.95\nNs 64.0\n\n")

        f.write("newmtl EmissivePhosphorDemonstration\n")
        f.write("Ka 0.0 0.92 1.0\nKd 0.0 0.92 1.0\nKe 0.0 0.95 1.0\nKs 0.1 0.1 0.1\nNs 128.0\n\n")

        f.write("newmtl AccentBrass\n")
        f.write("Ka 0.65 0.48 0.22\nKd 0.72 0.54 0.26\nKs 0.85 0.75 0.40\nNs 50.0\n\n")

    # Write OBJ with full vertex positions, vertex texture coords (vt), and face mappings
    with open(obj_path, "w", encoding="utf-8") as f:
        f.write(f"# Singularity VR Model — {model_name}\n")
        f.write(f"mtllib {mtl_name}\n")

        for v in pts:
            f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")

        # Write UVs per triangle vertex
        uv_indices = []
        cur_uv_idx = 1
        for tri_uv in uvs:
            for uv in tri_uv:
                f.write(f"vt {uv[0]:.6f} {1.0 - uv[1]:.6f}\n")
            uv_indices.append((cur_uv_idx, cur_uv_idx+1, cur_uv_idx+2))
            cur_uv_idx += 3

        current_mat = None
        for i, tri in enumerate(faces):
            ft = face_types[i]
            if ft == -1: mat = "WeaponRemastered"
            elif ft == 0: mat = "BezelSoviet"
            elif ft == 1: mat = "CathodeGlass"
            elif ft == 2: mat = "RivetMetal"
            elif ft == 3: mat = "EmissivePhosphorDemonstration"
            else: mat = "AccentBrass"

            if mat != current_mat:
                f.write(f"usemtl {mat}\n")
                current_mat = mat

            uvis = uv_indices[i]
            f.write(f"f {tri[0]+1}/{uvis[0]} {tri[1]+1}/{uvis[1]} {tri[2]+1}/{uvis[2]}\n")

    # Export Multi-Material GLTF 2.0 Binary (.glb)
    try:
        submeshes = []

        # 1. Weapon Submesh (Texture-Mapped PBR)
        weapon_mask = (face_types == -1)
        if np.any(weapon_mask):
            w_faces = faces[weapon_mask]
            w_uvs = uvs[weapon_mask]
            unrolled_v = pts[w_faces.flatten()]
            unrolled_uv = w_uvs.reshape(-1, 2).copy()
            unrolled_uv[:, 1] = 1.0 - unrolled_uv[:, 1] # Flip V for GLTF
            unrolled_f = np.arange(len(unrolled_v), dtype=np.int32).reshape(-1, 3)

            tex_img = Image.open(tex_src_path).convert('RGB')
            norm_img = Image.open(norm_src_path).convert('RGB') if (norm_src_path and Path(norm_src_path).exists()) else None

            pbr_weapon = trimesh.visual.material.PBRMaterial(
                name="WeaponRemastered",
                baseColorTexture=tex_img,
                normalTexture=norm_img,
                metallicFactor=0.6,
                roughnessFactor=0.4
            )
            visual_weapon = trimesh.visual.TextureVisuals(uv=unrolled_uv, material=pbr_weapon)
            m_weapon = trimesh.Trimesh(vertices=unrolled_v, faces=unrolled_f, visual=visual_weapon, process=False)
            submeshes.append(m_weapon)

        # 2. Bezel Submesh (Milled Soviet Dark Steel)
        bezel_mask = (face_types == 0)
        if np.any(bezel_mask):
            b_faces = faces[bezel_mask]
            b_v = pts[b_faces.flatten()]
            b_f = np.arange(len(b_v), dtype=np.int32).reshape(-1, 3)
            pbr_bezel = trimesh.visual.material.PBRMaterial(
                name="BezelSoviet",
                baseColorFactor=[0.20, 0.24, 0.28, 1.0],
                metallicFactor=0.85,
                roughnessFactor=0.35
            )
            m_bezel = trimesh.Trimesh(vertices=b_v, faces=b_f, process=False)
            m_bezel.visual.material = pbr_bezel
            submeshes.append(m_bezel)

        # 3. Cathode Glass Substrate Bed
        glass_mask = (face_types == 1)
        if np.any(glass_mask):
            g_faces = faces[glass_mask]
            g_v = pts[g_faces.flatten()]
            g_f = np.arange(len(g_v), dtype=np.int32).reshape(-1, 3)
            pbr_glass = trimesh.visual.material.PBRMaterial(
                name="CathodeGlass",
                baseColorFactor=[0.03, 0.05, 0.08, 1.0],
                metallicFactor=0.1,
                roughnessFactor=0.15
            )
            m_glass = trimesh.Trimesh(vertices=g_v, faces=g_f, process=False)
            m_glass.visual.material = pbr_glass
            submeshes.append(m_glass)

        # 4. Rivet / Screw Fasteners
        rivet_mask = (face_types == 2)
        if np.any(rivet_mask):
            r_faces = faces[rivet_mask]
            r_v = pts[r_faces.flatten()]
            r_f = np.arange(len(r_v), dtype=np.int32).reshape(-1, 3)
            pbr_rivet = trimesh.visual.material.PBRMaterial(
                name="RivetMetal",
                baseColorFactor=[0.60, 0.65, 0.72, 1.0],
                metallicFactor=0.95,
                roughnessFactor=0.20
            )
            m_rivet = trimesh.Trimesh(vertices=r_v, faces=r_f, process=False)
            m_rivet.visual.material = pbr_rivet
            submeshes.append(m_rivet)

        # 5. Emissive Phosphor Digits / Segments
        em_mask = (face_types == 3)
        if np.any(em_mask):
            em_faces = faces[em_mask]
            em_v = pts[em_faces.flatten()]
            em_f = np.arange(len(em_v), dtype=np.int32).reshape(-1, 3)
            pbr_em = trimesh.visual.material.PBRMaterial(
                name="EmissivePhosphorDemonstration",
                baseColorFactor=[0.0, 0.92, 1.0, 1.0],
                emissiveFactor=[0.0, 0.95, 1.0],
                metallicFactor=0.0,
                roughnessFactor=0.20
            )
            m_em = trimesh.Trimesh(vertices=em_v, faces=em_f, process=False)
            m_em.visual.material = pbr_em
            submeshes.append(m_em)

        # 6. Accent Brass / Badges
        brass_mask = (face_types == 4)
        if np.any(brass_mask):
            br_faces = faces[brass_mask]
            br_v = pts[br_faces.flatten()]
            br_f = np.arange(len(br_v), dtype=np.int32).reshape(-1, 3)
            pbr_brass = trimesh.visual.material.PBRMaterial(
                name="AccentBrass",
                baseColorFactor=[0.72, 0.54, 0.26, 1.0],
                metallicFactor=0.85,
                roughnessFactor=0.28
            )
            m_brass = trimesh.Trimesh(vertices=br_v, faces=br_f, process=False)
            m_brass.visual.material = pbr_brass
            submeshes.append(m_brass)

        scene = trimesh.Scene(submeshes)
        scene.export(str(glb_path), file_type='glb')
        print(f"Exported {model_name}: OBJ+MTL and Multi-Material GLB ({glb_path.stat().st_size / (1024*1024):.2f} MB, {len(submeshes)} submeshes)")
    except Exception as e:
        print(f"Warning: GLB export error for {model_name}: {e}")

# -----------------------------------------------------------------------------
# Main Generation Runner
# -----------------------------------------------------------------------------
def run_all_models_v2():
    print("=== Generating Singularity VR Diegetic HUD 3D Models V2.1 (PBR Multi-Material) ===")

    vanilla_tex_sources = {
        'centurion': REPO_ROOT / r"SingularityVR-production\deployment\install-backups\20261010-201052\texture-override\handgun_d.dds",
        'valkyrie': REPO_ROOT / r"SingularityVR-production\deployment\install-backups\20261010-201052\texture-override\AR_E99Rifle_alt_d.dds",
        'spikeshot': REPO_ROOT / r"SingularityVR-assets-local\textures\original-png\RvGameMPLoader_SF\Texture2D\AR_shotgun_d.png",
        'kasimov': REPO_ROOT / r"SingularityVR-assets-local\textures\original-png\RvGameMPLoader_SF\Texture2D\ar_sniper_rifle_d.png",
        'tmd_glove': REPO_ROOT / r"SingularityVR-assets-local\textures\original-png\RvGame\Texture2D\inview_glove_d.png",
    }

    configs = [
        # 1. Centurion Revolver
        {
            'id': 'centurion',
            'psk': REPO_ROOT / r"research\native-hands-20261007\assets\RvGameMPLoader_SF\SkeletalMesh3\ch_inview_pistol_mesh.psk",
            'params': {'center': (0.0, 115.3, 43.5), 'size': (2.6, 0.45, 1.8), 'pitch': 18.0},
            'builder': build_centurion_fixture_v2,
            'remaster_tex': TEX_DIR / "centurion_diffuse_4k.png",
            'normal_tex': TEX_DIR / "centurion_normal_4k.png"
        },
        # 2. Valkyrie AR9 Rifle
        {
            'id': 'valkyrie',
            'psk': REPO_ROOT / r"research\native-hands-20261007\assets\RvGame\SkeletalMesh3\ch_inview_machinegun_mesh.psk",
            'params': {'center': (0.0, 117.2, 35.0), 'size': (2.8, 0.45, 1.9), 'pitch': 20.0},
            'builder': build_valkyrie_fixture_v2,
            'remaster_tex': TEX_DIR / "valkyrie_diffuse_4k.png",
            'normal_tex': TEX_DIR / "valkyrie_normal_4k.png"
        },
        # 3. Spikeshot Shotgun
        {
            'id': 'spikeshot',
            'psk': REPO_ROOT / r"research\native-hands-20261007\assets\RvGameMPLoader_SF\SkeletalMesh3\ch_inview_shotgun_mesh.psk",
            'params': {'center': (0.0, 116.8, 42.0), 'size': (2.8, 0.45, 1.9), 'pitch': 15.0},
            'builder': build_spikeshot_fixture_v2,
            'remaster_tex': TEX_DIR / "spikeshot_diffuse_4k.png",
            'normal_tex': TEX_DIR / "spikeshot_normal_4k.png"
        },
        # 4. Kasimov Sniper Rifle
        {
            'id': 'kasimov',
            'psk': REPO_ROOT / r"research\native-hands-20261007\assets\RvGameMPLoader_SF\SkeletalMesh3\ch_inview_sniper_mesh.psk",
            'params': {'center': (-2.6, 117.0, 40.5), 'size': (2.2, 0.38, 1.55), 'pitch': 15.0, 'cant': 32.0},
            'builder': build_kasimov_fixture_v2,
            'remaster_tex': TEX_DIR / "kasimov_diffuse_4k.png",
            'normal_tex': TEX_DIR / "kasimov_normal_4k.png"
        },
        # 5. Left Glove / TMD Gauntlet
        {
            'id': 'tmd_glove',
            'psk': REPO_ROOT / r"research\native-hands-20261007\assets\RvGame\SkeletalMesh3\ch_inview_tmd_mesh.psk",
            'params': {'center': (41.66, 101.75, -0.88), 'size': (1.8, 0.45, 4.8)},
            'builder': lambda c, s: build_tmd_wrist_fixture_v2(c, s),
            'remaster_tex': TEX_DIR / "tmd_glove_diffuse_4k.png",
            'normal_tex': TEX_DIR / "tmd_glove_normal_4k.png"
        }
    ]

    for cfg in configs:
        model_id = cfg['id']
        print(f"\n--- Processing {model_id} ---")
        with open(cfg['psk'], "rb") as f:
            psk_data = f.read()
        pts, faces, tri_uvs = parse_psk_with_uvs(psk_data)

        # 1. Export Original Vanilla Model with UVs & Vanilla Texture
        vanilla_src_tex = vanilla_tex_sources[model_id]
        vanilla_png_path = OUT_MODELS_DIR / f"{model_id}_vanilla.png"
        if not vanilla_png_path.exists():
            Image.open(vanilla_src_tex).convert('RGB').save(vanilla_png_path, "PNG")

        orig_face_types = np.full(len(faces), -1, dtype=np.int32)
        export_textured_model_v2(
            OUT_MODELS_DIR,
            f"{model_id}_original_v2",
            pts, faces, tri_uvs, orig_face_types,
            vanilla_png_path
        )

        # 2. Build Bespoke Fixture V2.2
        builder = cfg['builder']
        p = cfg['params']
        if 'pitch' in p and 'cant' in p:
            f_pts, f_faces, f_uvs, f_types = builder(p['center'], p['size'], p['pitch'], p['cant'])
        elif 'pitch' in p:
            f_pts, f_faces, f_uvs, f_types = builder(p['center'], p['size'], p['pitch'])
        else:
            f_pts, f_faces, f_uvs, f_types = builder(p['center'], p['size'])

        # Combine
        offset = len(pts)
        combined_pts = np.vstack([pts, f_pts])
        combined_faces = np.vstack([faces, f_faces + offset])
        combined_types = np.concatenate([orig_face_types, f_types])

        fixture_tri_uvs = []
        for tri in f_faces:
            fixture_tri_uvs.append((f_uvs[tri[0]], f_uvs[tri[1]], f_uvs[tri[2]]))
        combined_uvs = np.concatenate([tri_uvs, np.array(fixture_tri_uvs, dtype=np.float32)])

        # 3. Export Modified Diegetic Model V2.1 with Full 4K Diffuse and Normal Map
        export_textured_model_v2(
            OUT_MODELS_DIR,
            f"{model_id}_diegetic_v2",
            combined_pts, combined_faces, combined_uvs, combined_types,
            cfg['remaster_tex'],
            cfg.get('normal_tex')
        )

    print("\n=== All 10 Models (5 Original V2 + 5 Diegetic V2.1) Successfully Exported ===")

if __name__ == '__main__':
    run_all_models_v2()
