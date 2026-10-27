#!/usr/bin/env python3
"""Build assets/hud/katorga/3d GLB pack from game PSK + procedural HUD meshes."""
from __future__ import annotations

import json
import math
import shutil
import struct
from pathlib import Path

import numpy as np
import trimesh
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
PACK = ROOT / "assets" / "hud" / "katorga" / "3d"
PSK_GLOVE = Path(r"C:\Users\Trilex\vr-mods\research\native-hands-20261007\assets\RvGame\SkeletalMesh3\ch_inview_noTMD.psk")
PSK_GUN = Path(
    r"C:\Users\Trilex\vr-mods\research\native-hands-20261007\assets\RvGameMPLoader_SF\SkeletalMesh3\ch_inview_pistol_mesh.psk"
)
TEX_GLOVE = Path(
    r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\textures\original-png\RvGame\Texture2D\inview_glove_d.png"
)
TEX_GUN = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\textures\upscaled\handgun_d__A_lanczos.png")
PREVIEW_SRC = Path(r"C:\Users\Trilex\.cursor\projects\c-Users-Trilex-vr-mods\assets")

GUN_BONES = frozenset(
    {"Slide", "clip", "hammer", "Trigger", "slide_lock", "cog_pistol", "muzzle_flash"}
)


def load_psk(path: Path):
    data = path.read_bytes()
    pos = 0
    chunks = {}
    while pos < len(data):
        name, _kind, size, count = struct.unpack_from("<20siii", data, pos)
        pos += 32
        name = name.split(b"\0")[0].decode()
        chunks[name] = (size, count, data[pos : pos + size * count])
        pos += size * count
    size, count, raw = chunks["REFSKELT"]
    bones = []
    bone_parent = []
    bone_pos = []
    bone_rot = []
    for i in range(count):
        rec = raw[i * size : (i + 1) * size]
        bones.append(rec[:64].split(b"\0")[0].decode())
        bone_parent.append(struct.unpack_from("<i", rec, 72)[0])
        qx, qy, qz, qw = struct.unpack_from("<4f", rec, 76)
        px, py, pz = struct.unpack_from("<3f", rec, 92)
        bone_rot.append(np.array([qx, qy, qz, qw], dtype=np.float64))
        bone_pos.append(np.array([px, py, pz], dtype=np.float64))
    size, count, raw = chunks["PNTS0000"]
    pts = np.array(struct.unpack_from(f"<{count * 3}f", raw), dtype=np.float64).reshape(-1, 3)
    size, count, raw = chunks["RAWWEIGHTS"]
    weights: dict[int, list[tuple[float, int]]] = {}
    for i in range(count):
        w, p, b = struct.unpack_from("<fii", raw, i * size)
        if w > 0:
            weights.setdefault(p, []).append((w, b))
    size, count, raw = chunks["VTXW0000"]
    wedges = []
    for i in range(count):
        point, u, v = struct.unpack_from("<Iff", raw, i * size)
        wedges.append((point, u, 1.0 - v))
    size, count, raw = chunks["FACE0000"]
    faces = [struct.unpack_from("<3HBBI", raw, i * size) for i in range(count)]
    return bones, bone_parent, bone_pos, bone_rot, pts, weights, wedges, faces


def qmul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return np.array(
        [
            aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz,
        ],
        dtype=np.float64,
    )


def qrot(q, v):
    x, y, z, w = q
    u = np.array([x, y, z], dtype=np.float64)
    return 2.0 * np.dot(u, v) * u + (w * w - np.dot(u, u)) * v + 2.0 * w * np.cross(u, v)


def bone_abs(bone_pos, bone_rot, bone_parent):
    n = len(bone_pos)
    pos = [None] * n
    rot = [None] * n

    def rec(i, depth=0):
        if pos[i] is not None:
            return
        if bone_parent[i] < 0 or bone_parent[i] == i or depth > 64:
            pos[i] = bone_pos[i].copy()
            rot[i] = bone_rot[i].copy()
            return
        rec(bone_parent[i], depth + 1)
        rot[i] = qmul(rot[bone_parent[i]], bone_rot[i])
        pos[i] = pos[bone_parent[i]] + qrot(rot[bone_parent[i]], bone_pos[i])

    for i in range(n):
        rec(i)
    return {i: pos[i] for i in range(n)}, {i: rot[i] for i in range(n)}


def is_hand(name: str) -> bool:
    return any(k in name for k in ("_wrist", "_thumb_", "_index_", "_middle_", "_ring_", "_pinky"))


def side_of(name: str) -> str | None:
    if name.startswith("l_"):
        return "l"
    if name.startswith("r_"):
        return "r"
    return None


def dominant(weights):
    return {p: max(wl)[1] for p, wl in weights.items()}


def glove_tris_bind(bones, pts, weights, wedges, faces):
    dom = dominant(weights)
    hand_pts = {p for p, b in dom.items() if is_hand(bones[b])}
    out = {"l": [], "r": []}
    for a, b, c, mat, _aux, _sm in faces:
        if mat != 0:
            continue
        ids = [wedges[a][0], wedges[b][0], wedges[c][0]]
        uvs = np.array([wedges[a][1:], wedges[b][1:], wedges[c][1:]], dtype=np.float64)
        hand_n = sum(1 for i in ids if i in hand_pts)
        if hand_n == 0:
            continue
        xyz = np.stack([pts[i] for i in ids])
        if hand_n < 3:
            keep = xyz[[k for k, i in enumerate(ids) if i in hand_pts]]
            anchor = keep.mean(axis=0)
            for k, i in enumerate(ids):
                if i not in hand_pts:
                    xyz[k] = anchor
        sides = [side_of(bones[dom[i]]) for i in ids if i in hand_pts]
        side = max(set(sides), key=sides.count)
        if side in out:
            out[side].append((xyz, uvs))
    return out


def gun_tris_bind(bones, pts, weights, wedges, faces):
    dom = dominant(weights)
    body = []
    laser = []
    for a, b, c, mat, _aux, _sm in faces:
        ids = [wedges[a][0], wedges[b][0], wedges[c][0]]
        bone = bones[dom[ids[0]]]
        if bone not in GUN_BONES:
            continue
        xyz = np.stack([pts[i] for i in ids])
        uvs = np.array([wedges[a][1:], wedges[b][1:], wedges[c][1:]], dtype=np.float64)
        (laser if mat == 1 else body).append((xyz, uvs))
    return body, laser


def tris_to_trimesh(tri_list, texture: Path | None):
    if not tri_list:
        raise ValueError("empty triangle list")
    verts = []
    faces = []
    uvs = []
    vi = 0
    for xyz, uv in tri_list:
        for i in range(3):
            verts.append(xyz[i])
            uvs.append(uv[i])
        faces.append([vi, vi + 1, vi + 2])
        vi += 3
    mesh = trimesh.Trimesh(vertices=np.array(verts), faces=np.array(faces), process=True)
    if texture and texture.is_file():
        img = np.asarray(Image.open(texture).convert("RGB"))
        mesh.visual = trimesh.visual.TextureVisuals(uv=np.array(uvs), image=img)
    return mesh


def plate_mesh(width=6.4, height=3.8, thick=0.12):
    sx, sy, sz = width / 2, height / 2, thick / 2
    corners = np.array(
        [
            [-sx, -sy, -sz],
            [sx, -sy, -sz],
            [sx, sy, -sz],
            [-sx, sy, -sz],
            [-sx, -sy, sz],
            [sx, -sy, sz],
            [sx, sy, sz],
            [-sx, sy, sz],
        ],
        dtype=np.float64,
    )
    faces = np.array(
        [
            [4, 5, 6],
            [4, 6, 7],
            [0, 2, 1],
            [0, 3, 2],
            [0, 1, 5],
            [0, 5, 4],
            [2, 7, 6],
            [2, 3, 7],
            [0, 4, 7],
            [0, 7, 3],
            [1, 2, 6],
            [1, 6, 5],
        ]
    )
    uvs = np.zeros((8, 2))
    uvs[[4, 5, 6, 7]] = [[0, 0], [1, 0], [1, 1], [0, 1]]
    mesh = trimesh.Trimesh(vertices=corners, faces=faces, process=False)
    mesh.visual = trimesh.visual.TextureVisuals(uv=uvs)
    return mesh


def ring_mesh(r0=20.16, r1=24.84, segments=72):
    tris = []
    uvs = []
    for i in range(segments):
        t0 = (i / segments) * math.tau
        t1 = ((i + 1) / segments) * math.tau
        p0 = np.array([math.cos(t0) * r0, math.sin(t0) * r0, 0.0])
        p1 = np.array([math.cos(t0) * r1, math.sin(t0) * r1, 0.0])
        q0 = np.array([math.cos(t1) * r0, math.sin(t1) * r0, 0.0])
        q1 = np.array([math.cos(t1) * r1, math.sin(t1) * r1, 0.0])
        u0, u1 = i / segments, (i + 1) / segments
        tris.append(np.stack([p0, p1, q1]))
        uvs.append([[u0, 0.0], [u0, 1.0], [u1, 1.0]])
        tris.append(np.stack([p0, q1, q0]))
        uvs.append([[u0, 0.0], [u1, 1.0], [u1, 0.0]])
    return tris_to_trimesh(list(zip(tris, uvs)), None)


def export_glb(mesh: trimesh.Trimesh, path: Path, extras: dict | None = None):
    path.parent.mkdir(parents=True, exist_ok=True)
    scene = trimesh.Scene(mesh)
    if extras:
        scene.graph.nodes_geometry  # touch
        scene.metadata = extras
    scene.export(path, file_type="glb")


def export_gun_scene(body_mesh, grip: np.ndarray, muzzle: np.ndarray, path: Path):
    body_mesh.vertices -= grip
    scene = trimesh.Scene()
    scene.add_geometry(body_mesh, geom_name="centurion_body")
    marker = trimesh.creation.icosphere(radius=0.35, subdivisions=1)
    marker.vertices += muzzle - grip
    marker.visual.vertex_colors = [255, 80, 60, 255]
    scene.add_geometry(marker, geom_name="muzzle_forward")
    scene.metadata = {
        "attach": {
            "grip_origin_bind_cm": grip.tolist(),
            "muzzle_forward_bind_cm": muzzle.tolist(),
        }
    }
    path.parent.mkdir(parents=True, exist_ok=True)
    scene.export(path, file_type="glb")


def vec3(v):
    return [round(float(x), 4) for x in v]


def quat_identity():
    return [0.0, 0.0, 0.0, 1.0]


def write_attach_schema(bones, abs_pos, wrist_l, wrist_r, cog_i):
    cog = abs_pos[cog_i]
    wl = abs_pos[wrist_l]
    wr = abs_pos[wrist_r]
    # Art offsets: plate above wrist cuff, readable in PSVR2 seated (see ATTACHMENTS.md).
    health_off = np.array([0.0, 6.5, 4.0])
    ammo_off = np.array([-5.0, 2.0, 5.0])
    wheel_off = np.array([0.0, 0.0, 0.0])
    schema = {
        "schema": "singularityvr-hud-attach-v1",
        "coordinate": "UE3_cm_right_handed_Z_up",
        "aim_hand": "right",
        "body_frame": {
            "parent": "body_yaw_hip",
            "axes": "+X forward, +Y right, +Z up",
            "units": "UU",
            "holster_slots_file": "holster_slots.json",
        },
        "bind_reference": {
            "mesh_gloves": "ch_inview_noTMD.psk",
            "mesh_pistol": "ch_inview_pistol_mesh.psk",
            "bone_wrist_left": "l_arm_wrist",
            "bone_wrist_right": "r_arm_wrist",
            "bone_pistol_cog": "cog_pistol",
            "wrist_left_bind_cm": vec3(wl),
            "wrist_right_bind_cm": vec3(wr),
            "cog_pistol_bind_cm": vec3(cog),
        },
        "nodes": {
            "grip": {
                "parent": "openxr_grip_pose",
                "offset_pos": [0, 0, 0],
                "offset_rot_quat": quat_identity(),
                "source": "bind",
            },
            "plate_health": {
                "parent": "l_arm_wrist",
                "offset_pos": vec3(health_off),
                "offset_rot_quat": quat_identity(),
                "face_camera": True,
                "source": "art_psvr2",
            },
            "plate_ammo": {
                "parent": "r_arm_wrist",
                "offset_pos": vec3(ammo_off),
                "offset_rot_quat": quat_identity(),
                "face_camera": True,
                "source": "art_psvr2",
            },
            "weapon_wheel": {
                "parent": "r_arm_wrist",
                "offset_pos": vec3(wheel_off),
                "offset_rot_quat": quat_identity(),
                "radius_cm": 18.0,
                "segments": 8,
                "catalog_file": "weapon_wheel_catalog.json",
                "face_camera": True,
                "source": "art_psvr2",
            },
            "holster_weapon": {
                "parent": "body_yaw_hip",
                "offset_pos_uu": [8.0, 12.0, -30.0],
                "offset_rot_quat": quat_identity(),
                "mesh": "hud/holster_weapon_right.glb",
                "activation_radius_uu": 12.0,
                "slot_id": "weapon_holster_right",
                "source": "runtime_holster_sync",
            },
            "holster_medkit": {
                "parent": "body_yaw_hip",
                "offset_pos_uu": [8.0, -12.0, -30.0],
                "offset_rot_quat": [0.0, 0.0, 1.0, 0.0],
                "mesh": "hud/holster_medkit_left.glb",
                "activation_radius_uu": 10.0,
                "slot_id": "medkit_pouch_left",
                "atlas_icon": "sense-circle",
                "source": "art_psvr2",
            },
        },
        "runtime_paths": {
            "asset_root_ini_key": "KatorgaHudAssetRoot",
            "default_relative": "vr-assets/hud/katorga/3d",
        },
    }
    out = PACK / "attachments" / "attach_schema.json"
    out.write_text(json.dumps(schema, indent=2) + "\n", encoding="utf-8")
    return schema


def copy_previews():
    mapping = {
        "katorga-target-spokoj.jpg": "spokoj.png",
        "katorga-target-walka.jpg": "walka.png",
        "katorga-target-zmiana.jpg": "zmiana_broni.png",
    }
    prev = PACK / "previews"
    prev.mkdir(parents=True, exist_ok=True)
    for src_name, dst_name in mapping.items():
        src = PREVIEW_SRC / src_name
        dst = prev / dst_name
        if not src.is_file():
            raise FileNotFoundError(src)
        im = Image.open(src).convert("RGB")
        w, h = im.size
        if w < 1920:
            im = im.resize((1920, int(h * 1920 / w)), Image.Resampling.LANCZOS)
        im.save(dst, quality=92)
        print("preview", dst, im.size)


def main():
    PACK.mkdir(parents=True, exist_ok=True)
    tex_dir = PACK / "textures"
    tex_dir.mkdir(exist_ok=True)
    shutil.copyfile(TEX_GLOVE, tex_dir / "inview_glove_d.png")
    shutil.copyfile(TEX_GUN, tex_dir / "handgun_d.png")

    bones, parents, bpos, brot, pts, weights, wedges, faces = load_psk(PSK_GLOVE)
    abs_pos, _ = bone_abs(bpos, brot, parents)
    idx = {n: i for i, n in enumerate(bones)}
    gloves = glove_tris_bind(bones, pts, weights, wedges, faces)
    export_glb(tris_to_trimesh(gloves["l"], TEX_GLOVE), PACK / "gloves" / "glove_left.glb")
    export_glb(tris_to_trimesh(gloves["r"], TEX_GLOVE), PACK / "gloves" / "glove_right.glb")

    gbones, gparents, gbpos, gbrot, gpts, gweights, gwedges, gfaces = load_psk(PSK_GUN)
    gabs, _ = bone_abs(gbpos, gbrot, gparents)
    gidx = {n: i for i, n in enumerate(gbones)}
    body, _laser = gun_tris_bind(gbones, gpts, gweights, gwedges, gfaces)
    grip = gpts[
        [p for p, wl in gweights.items() if gbones[max(wl)[1]] == "clip"]
    ].mean(axis=0)
    muzzle = gabs[gidx["muzzle_flash"]]
    export_gun_scene(tris_to_trimesh(body, TEX_GUN), grip, muzzle, PACK / "weapons" / "centurion_inview.glb")

    export_glb(plate_mesh(), PACK / "hud" / "plate_health.glb")
    export_glb(plate_mesh(), PACK / "hud" / "plate_ammo.glb")
    export_glb(ring_mesh(), PACK / "hud" / "ring_weapon_wheel.glb")

    import importlib.util

    holster_py = Path(__file__).resolve().parent / "export_holster_hip_meshes.py"
    spec = importlib.util.spec_from_file_location("export_holster_hip_meshes", holster_py)
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader
    spec.loader.exec_module(mod)
    mod.main()

    write_attach_schema(bones, abs_pos, idx["l_arm_wrist"], idx["r_arm_wrist"], idx["cog_pistol"])
    copy_previews()
    print("done", PACK)


if __name__ == "__main__":
    main()
