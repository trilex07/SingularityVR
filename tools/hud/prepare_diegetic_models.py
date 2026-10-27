"""Constructs the unified 3D models with seamlessly integrated diegetic HUD fixtures:
- glove_left_diegetic.glb / .obj
- centurion_diegetic.glb / .obj
- valkyrie_diegetic.glb / .obj
- spikeshot_diegetic.glb / .obj
- kasimov_diegetic.glb / .obj
"""
import math
import os
import struct
import json

def parse_psk(data):
    p = 0
    chunks = {}
    while p < len(data):
        raw, _, size, count = struct.unpack_from('<20siii', data, p)
        name = raw.split(b'\0')[0].decode('ascii')
        p += 32
        chunks[name] = (size, count, data[p:p+size*count])
        p += size * count
    
    pts_size, pts_count, pts_data = chunks['PNTS0000']
    points = [struct.unpack_from('<3f', pts_data, i*12) for i in range(pts_count)]
    
    faces_size, faces_count, faces_data = chunks['FACE0000']
    faces = [struct.unpack_from('<3HBBI', faces_data, i*12)[:3] for i in range(faces_count)]
    
    wedges_size, wedges_count, wedges_data = chunks['VTXW0000']
    wedges = [struct.unpack_from('<IffBBH', wedges_data, i*16) for i in range(wedges_count)]
    
    return points, faces, wedges

def create_box(center, size, rot_pitch_deg=0.0):
    # center = (cx, cy, cz)
    # size = (sx, sy, sz)
    cx, cy, cz = center
    hx, hy, hz = size[0]*0.5, size[1]*0.5, size[2]*0.5
    
    # 8 local corners
    raw_corners = [
        (-hx, -hy, -hz), (hx, -hy, -hz), (hx, hy, -hz), (-hx, hy, -hz), # back (-z)
        (-hx, -hy, hz), (hx, -hy, hz), (hx, hy, hz), (-hx, hy, hz)     # front (+z)
    ]
    
    rad = math.radians(rot_pitch_deg)
    cos_p = math.cos(rad)
    sin_p = math.sin(rad)
    
    rotated = []
    for x, y, z in raw_corners:
        # pitch rotation around X axis (Y and Z rotate)
        ry = y * cos_p - z * sin_p
        rz = y * sin_p + z * cos_p
        rotated.append((cx + x, cy + ry, cz + rz))
        
    # 6 faces (12 triangles)
    # Indices: 0-3 (back), 4-7 (front)
    # 0:(-,-,-), 1:(+,-,-), 2:(+,+,-), 3:(-,+,-)
    # 4:(-,-,+), 5:(+,-,+), 6:(+,+,+), 7:(-,+,+)
    faces = [
        # Top (+Y in unrotated space: 3, 2, 6, 7)
        (3, 2, 6), (3, 6, 7),
        # Bottom (-Y: 0, 4, 5, 1)
        (0, 4, 5), (0, 5, 1),
        # Front (+Z: 4, 7, 6, 5)
        (4, 7, 6), (4, 6, 5),
        # Back (-Z: 1, 2, 3, 0)
        (1, 2, 3), (1, 3, 0),
        # Left (-X: 0, 3, 7, 4)
        (0, 3, 7), (0, 7, 4),
        # Right (+X: 1, 5, 6, 2)
        (1, 5, 6), (1, 6, 2)
    ]
    return rotated, faces

def export_obj(path, vertices, faces, object_name="Model"):
    with open(path, "w", encoding="utf-8") as f:
        f.write(f"# Wavefront OBJ: {object_name} with integrated Diegetic HUD\n")
        f.write(f"o {object_name}\n")
        for v in vertices:
            f.write(f"v {v[0]:.4f} {v[1]:.4f} {v[2]:.4f}\n")
        for tri in faces:
            # 1-based indexing
            f.write(f"f {tri[0]+1} {tri[1]+1} {tri[2]+1}\n")

def process_mesh_with_hud(psk_path, output_obj_path, fixture_center, fixture_size, fixture_rot_pitch, obj_name):
    with open(psk_path, "rb") as f:
        data = f.read()
    points, faces, wedges = parse_psk(data)
    
    # In PSK, faces index into wedges, and wedges index into points
    # Reconstruct triangle vertex indices
    tri_points = []
    base_vertices = points[:]
    base_faces = []
    for tri in faces:
        p0 = wedges[tri[0]][0]
        p1 = wedges[tri[1]][0]
        p2 = wedges[tri[2]][0]
        base_faces.append((p0, p1, p2))
        
    # Create the 3D HUD housing/fixture box
    hud_verts, hud_faces = create_box(fixture_center, fixture_size, fixture_rot_pitch)
    
    # Merge HUD geometry into model
    offset = len(base_vertices)
    combined_verts = base_vertices + hud_verts
    combined_faces = base_faces + [(f[0] + offset, f[1] + offset, f[2] + offset) for f in hud_faces]
    
    export_obj(output_obj_path, combined_verts, combined_faces, obj_name)
    print(f"Exported {output_obj_path}: {len(combined_verts)} verts, {len(combined_faces)} tris")

def main():
    out_dir = r"C:\Users\Trilex\vr-mods\SingularityVR-settings-rifle-reload\assets\processed\hud\diegetic\models"
    os.makedirs(out_dir, exist_ok=True)
    
    # 1. Centurion Revolver with integrated top-strap HUD housing
    # Center: X=0, Y=115.3, Z=43.5, Size: (2.2, 0.5, 3.2), Pitch: -15 deg (angled back towards shooter)
    process_mesh_with_hud(
        r"C:\Users\Trilex\vr-mods\research\native-hands-20261007\assets\RvGameMPLoader_SF\SkeletalMesh3\ch_inview_pistol_mesh.psk",
        os.path.join(out_dir, "centurion_diegetic.obj"),
        fixture_center=(0.0, 115.3, 43.5),
        fixture_size=(2.2, 0.4, 3.2),
        fixture_rot_pitch=-15.0,
        obj_name="Centurion_Revolver_Diegetic"
    )
    
    # 2. Valkyrie AR9 with integrated receiver HUD housing
    # Center: X=0, Y=117.2, Z=35.0, Size: (2.6, 0.5, 3.4), Pitch: -20 deg
    process_mesh_with_hud(
        r"C:\Users\Trilex\vr-mods\research\native-hands-20261007\assets\RvGame\SkeletalMesh3\ch_inview_machinegun_mesh.psk",
        os.path.join(out_dir, "valkyrie_diegetic.obj"),
        fixture_center=(0.0, 117.2, 35.0),
        fixture_size=(2.6, 0.4, 3.4),
        fixture_rot_pitch=-20.0,
        obj_name="Valkyrie_AR9_Diegetic"
    )
    
    # 3. Spikeshot Shotgun with integrated receiver rail HUD
    # Center: X=0, Y=116.8, Z=42.0, Size: (2.6, 0.5, 3.4), Pitch: -15 deg
    process_mesh_with_hud(
        r"C:\Users\Trilex\vr-mods\research\native-hands-20261007\assets\RvGameMPLoader_SF\SkeletalMesh3\ch_inview_shotgun_mesh.psk",
        os.path.join(out_dir, "spikeshot_diegetic.obj"),
        fixture_center=(0.0, 116.8, 42.0),
        fixture_size=(2.6, 0.4, 3.4),
        fixture_rot_pitch=-15.0,
        obj_name="Spikeshot_Diegetic"
    )
    
    # 4. Kasimov Sniper Rifle with integrated receiver HUD
    # Center: X=0, Y=116.5, Z=40.0, Size: (2.4, 0.5, 3.2), Pitch: -15 deg
    process_mesh_with_hud(
        r"C:\Users\Trilex\vr-mods\research\native-hands-20261007\assets\RvGameMPLoader_SF\SkeletalMesh3\ch_inview_sniper_mesh.psk",
        os.path.join(out_dir, "kasimov_diegetic.obj"),
        fixture_center=(0.0, 116.5, 40.0),
        fixture_size=(2.4, 0.4, 3.2),
        fixture_rot_pitch=-15.0,
        obj_name="Kasimov_Diegetic"
    )
    
    # 5. Left Glove / TMD with integrated wrist bio-gauge
    # Left wrist in ch_inview_tmd_mesh is on negative X side or positive X side (positive in raw PSK: X~36.3, Y~95.8, Z~0.0)
    process_mesh_with_hud(
        r"C:\Users\Trilex\vr-mods\research\native-hands-20261007\assets\RvGame\SkeletalMesh3\ch_inview_tmd_mesh.psk",
        os.path.join(out_dir, "tmd_glove_diegetic.obj"),
        fixture_center=(36.3, 95.8, 1.2),
        fixture_size=(4.2, 1.4, 0.5),
        fixture_rot_pitch=0.0,
        obj_name="TMD_Glove_Diegetic"
    )

if __name__ == "__main__":
    main()
