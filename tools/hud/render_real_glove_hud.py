#!/usr/bin/env python3
"""Glove-only preview from the real noTMD PSK and inview_glove_d.

Arms are dropped. Wrist and weapon-ring plates are new geometry in the
same bind space as the gloves. No game runtime.
"""
from __future__ import annotations

import math
import shutil
import struct
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

PSK = Path(r"C:\Users\Trilex\vr-mods\research\native-hands-20261007\assets\RvGame\SkeletalMesh3\ch_inview_noTMD.psk")
GUN = Path(r"C:\Users\Trilex\vr-mods\research\native-hands-20261007\assets\RvGameMPLoader_SF\SkeletalMesh3\ch_inview_pistol_mesh.psk")
TEX = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\textures\original-png\RvGame\Texture2D\inview_glove_d.png")
GUN_TEX = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\textures\upscaled\handgun_d__A_lanczos.png")
OUT = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-hud-design\assets\hud\katorga\real")
SHOW = Path(r"C:\Users\Trilex\.cursor\projects\c-Users-Trilex-vr-mods\assets")

W, H = 1400, 788


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
    for i in range(count):
        rec = raw[i * size : (i + 1) * size]
        bones.append(rec[:64].split(b"\0")[0].decode())
    size, count, raw = chunks["PNTS0000"]
    pts = np.array(struct.unpack_from(f"<{count * 3}f", raw), dtype=np.float64).reshape(-1, 3)
    size, count, raw = chunks["RAWWEIGHTS"]
    weights = {}
    for i in range(count):
        w, p, b = struct.unpack_from("<fii", raw, i * size)
        if w > 0:
            weights.setdefault(p, []).append((w, b))
    size, count, raw = chunks["VTXW0000"]
    wedges = []
    for i in range(count):
        point, u, v = struct.unpack_from("<Iff", raw, i * size)
        wedges.append((point, u, v))
    size, count, raw = chunks["FACE0000"]
    faces = [struct.unpack_from("<3HBBI", raw, i * size) for i in range(count)]
    return bones, pts, weights, wedges, faces


def is_hand(name: str) -> bool:
    keys = ("_wrist", "_thumb_", "_index_", "_middle_", "_ring_", "_pinky")
    return any(k in name for k in keys)


def side_of(name: str) -> str | None:
    if name.startswith("l_"):
        return "l"
    if name.startswith("r_"):
        return "r"
    return None


def glove_triangles(bones, pts, weights, wedges, faces):
    dominant = {}
    for p, wl in weights.items():
        dominant[p] = max(wl)[1]
    hand_pts = {p for p, b in dominant.items() if is_hand(bones[b])}
    def mean_bone(side, pred):
        ids = [i for i in hand_pts if side_of(bones[dominant[i]]) == side and pred(bones[dominant[i]])]
        return pts[ids].mean(0) if ids else None

    wrist = {side: mean_bone(side, lambda n: "_wrist" in n) for side in ("l", "r")}
    tips = {side: mean_bone(side, lambda n: n.endswith("_3")) for side in ("l", "r")}

    tris = {"l": [], "r": []}
    for a, b, c, mat, _aux, _sm in faces:
        if mat != 0:
            continue
        ids = [wedges[a][0], wedges[b][0], wedges[c][0]]
        uvs = [(wedges[a][1], 1.0 - wedges[a][2]), (wedges[b][1], 1.0 - wedges[b][2]), (wedges[c][1], 1.0 - wedges[c][2])]
        hand_n = sum(1 for i in ids if i in hand_pts)
        if hand_n == 0:
            continue
        xyz = [pts[i].copy() for i in ids]
        if hand_n < 3:
            keep = [xyz[k] for k, i in enumerate(ids) if i in hand_pts]
            anchor = np.mean(keep, axis=0)
            for k, i in enumerate(ids):
                if i not in hand_pts:
                    xyz[k] = anchor
        sides = [side_of(bones[dominant[i]]) for i in ids if i in hand_pts]
        side = max(set(sides), key=sides.count)
        if side not in tris:
            continue
        tris[side].append((np.stack(xyz), np.array(uvs, dtype=np.float64)))
    return tris, wrist, tips


def write_obj(path: Path, tris, texture_name: str | None):
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = ["# glove-only extract, arms removed", "o mesh"]
    if texture_name:
        lines.insert(1, f"mtllib {path.stem}.mtl")
        lines.append("usemtl diffuse")
    vi = 1
    for xyz, uv in tris:
        for p, t in zip(xyz, uv):
            lines.append(f"v {p[0]:.5f} {p[1]:.5f} {p[2]:.5f}")
            lines.append(f"vt {t[0]:.5f} {t[1]:.5f}")
        lines.append(f"f {vi}/{vi} {vi+1}/{vi+1} {vi+2}/{vi+2}")
        vi += 3
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    if texture_name:
        (path.with_suffix(".mtl")).write_text(
            f"newmtl diffuse\nKd 1 1 1\nmap_Kd {texture_name}\n", encoding="utf-8"
        )


def box(center, ax, ay, az, sx, sy, sz):
    c = np.asarray(center, dtype=np.float64)
    ax, ay, az = (np.asarray(v, dtype=np.float64) for v in (ax, ay, az))
    ax = ax / np.linalg.norm(ax)
    ay = ay / np.linalg.norm(ay)
    az = az / np.linalg.norm(az)
    corners = []
    for x in (-sx, sx):
        for y in (-sy, sy):
            for z in (-sz, sz):
                corners.append(c + ax * x + ay * y + az * z)
    corners = np.stack(corners)
    # Corner order is z + 2y + 4x. The textured face is +z, toward the camera.
    faces = [(0, 2, 3), (0, 3, 1), (4, 5, 7), (4, 7, 6), (0, 1, 5), (0, 5, 4), (2, 6, 7), (2, 7, 3), (0, 4, 6), (0, 6, 2), (1, 5, 7), (1, 7, 3)]
    uvs = {
        (1, 5, 7): [(0, 0), (1, 0), (1, 1)],
        (1, 7, 3): [(0, 0), (1, 1), (0, 1)],
    }
    out = []
    for f in faces:
        xyz = corners[list(f)]
        uv = np.array(uvs.get(f, [(0, 0), (1, 0), (0, 1)]), dtype=np.float64)
        out.append((xyz, uv))
    return out


def annulus(center, ax, ay, r0, r1, segs=80):
    c = np.asarray(center, dtype=np.float64)
    ax = np.asarray(ax, dtype=np.float64)
    ay = np.asarray(ay, dtype=np.float64)
    ax = ax / np.linalg.norm(ax)
    ay = ay / np.linalg.norm(ay)

    def uv_of(p):
        u = 0.5 + float(np.dot(p - c, ax)) / (2.0 * r1)
        v = 0.5 + float(np.dot(p - c, ay)) / (2.0 * r1)
        return (u, v)

    tris = []
    for i in range(segs):
        t0 = (i / segs) * math.tau
        t1 = ((i + 1) / segs) * math.tau
        p0 = c + ax * math.cos(t0) * r0 + ay * math.sin(t0) * r0
        p1 = c + ax * math.cos(t0) * r1 + ay * math.sin(t0) * r1
        q0 = c + ax * math.cos(t1) * r0 + ay * math.sin(t1) * r0
        q1 = c + ax * math.cos(t1) * r1 + ay * math.sin(t1) * r1
        tris.append((np.stack([p0, p1, q1]), np.array([uv_of(p0), uv_of(p1), uv_of(q1)])))
        tris.append((np.stack([p0, q1, q0]), np.array([uv_of(p0), uv_of(q1), uv_of(q0)])))
    return tris


def plate_texture(title, digits, ticks_full, warn=False):
    im = Image.new("RGBA", (512, 320), (16, 18, 20, 255))
    d = ImageDraw.Draw(im)
    edge = (196, 164, 106, 255) if warn else (90, 111, 141, 255)
    d.rounded_rectangle((8, 8, 503, 311), radius=18, outline=edge, width=8)
    paper = (230, 225, 214, 255)
    for i in range(10):
        x = 36 + i * 28
        if i < ticks_full:
            d.rectangle((x, 48, x + 18, 150), fill=paper)
        else:
            d.rectangle((x, 48, x + 18, 150), outline=edge, width=3)
    font = ImageFont.load_default(size=42)
    small = ImageFont.load_default(size=28)
    d.text((36, 190), digits, fill=paper, font=font)
    d.text((36, 250), title, fill=edge, font=small)
    return np.asarray(im)


def ring_texture():
    im = Image.new("RGBA", (1024, 1024), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse((70, 70, 954, 954), outline=(198, 226, 236, 210), width=16)
    d.ellipse((132, 132, 892, 892), outline=(198, 226, 236, 80), width=6)
    font = ImageFont.load_default(size=36)
    d.rounded_rectangle((360, 78, 664, 148), radius=8, fill=(186, 220, 232, 120), outline=(236, 246, 250, 230), width=3)
    d.text((392, 96), "CENTURION", fill=(246, 250, 252, 255), font=font)
    for i in range(8):
        ang = (i / 8) * math.tau + 0.4
        if i == 2:
            continue
        x = 512 + math.cos(ang) * 390
        y = 512 + math.sin(ang) * 390
        d.rounded_rectangle((x - 28, y - 16, x + 28, y + 16), radius=4, outline=(220, 236, 242, 180), width=3)
    return np.asarray(im)


def look_at(eye, target, up=(0, 0, 1)):
    eye = np.asarray(eye, dtype=np.float64)
    target = np.asarray(target, dtype=np.float64)
    f = target - eye
    f = f / np.linalg.norm(f)
    u = np.asarray(up, dtype=np.float64)
    s = np.cross(f, u)
    s = s / np.linalg.norm(s)
    u = np.cross(s, f)
    return eye, f, s, u


def render(layers, eye, target, path: Path, bg=(18, 20, 22)):
    eye, f, s, u = look_at(eye, target)
    color = np.zeros((H, W, 3), dtype=np.float64)
    color[:, :] = np.array(bg, dtype=np.float64)
    depth = np.full((H, W), np.inf)
    fov = 0.72
    for tris, tex, shade, alpha in layers:
        th, tw = tex.shape[0], tex.shape[1]
        for xyz, uv in tris:
            cam = np.stack([np.dot(p - eye, s) for p in xyz]), np.stack([np.dot(p - eye, u) for p in xyz]), np.stack([np.dot(p - eye, f) for p in xyz])
            z = cam[2]
            if np.any(z < 1.0):
                continue
            sx = W * 0.5 + (cam[0] / z) * (W * 0.5) / fov
            sy = H * 0.5 - (cam[1] / z) * (W * 0.5) / fov
            area = (sx[1] - sx[0]) * (sy[2] - sy[0]) - (sx[2] - sx[0]) * (sy[1] - sy[0])
            if abs(area) < 0.5:
                continue
            n = np.cross(xyz[1] - xyz[0], xyz[2] - xyz[0])
            ln = np.linalg.norm(n)
            if ln < 1e-8:
                continue
            n /= ln
            light = max(0.22, float(np.dot(n, f))) * shade
            minx, maxx = int(max(0, np.floor(sx.min()))), int(min(W - 1, np.ceil(sx.max())))
            miny, maxy = int(max(0, np.floor(sy.min()))), int(min(H - 1, np.ceil(sy.max())))
            if minx >= maxx or miny >= maxy:
                continue
            xs = np.arange(minx, maxx + 1)
            ys = np.arange(miny, maxy + 1)
            gx, gy = np.meshgrid(xs, ys)
            den = area
            w0 = ((sx[1] - gx) * (sy[2] - gy) - (sx[2] - gx) * (sy[1] - gy)) / den
            w1 = ((sx[2] - gx) * (sy[0] - gy) - (sx[0] - gx) * (sy[2] - gy)) / den
            w2 = 1.0 - w0 - w1
            mask = (w0 >= 0) & (w1 >= 0) & (w2 >= 0)
            if not np.any(mask):
                continue
            zz = w0 * z[0] + w1 * z[1] + w2 * z[2]
            mask &= zz < depth[miny : maxy + 1, minx : maxx + 1]
            if not np.any(mask):
                continue
            uu = w0 * uv[0, 0] + w1 * uv[1, 0] + w2 * uv[2, 0]
            vv = w0 * uv[0, 1] + w1 * uv[1, 1] + w2 * uv[2, 1]
            px = np.clip((uu % 1.0) * (tw - 1), 0, tw - 1).astype(np.int32)
            py = np.clip((1.0 - (vv % 1.0)) * (th - 1), 0, th - 1).astype(np.int32)
            sampled = tex[py, px].astype(np.float64)
            if sampled.shape[-1] == 4:
                a = (sampled[..., 3:4] / 255.0) * alpha
                rgb = sampled[..., :3] * light
                old = color[miny : maxy + 1, minx : maxx + 1]
                blended = old * (1 - a) + rgb * a
                color[miny : maxy + 1, minx : maxx + 1] = np.where(mask[..., None], blended, old)
                depth[miny : maxy + 1, minx : maxx + 1] = np.where(mask & (a[..., 0] > 0.85), zz, depth[miny : maxy + 1, minx : maxx + 1])
            else:
                rgb = np.clip(sampled * light, 0, 255)
                color[miny : maxy + 1, minx : maxx + 1] = np.where(mask[..., None], rgb, color[miny : maxy + 1, minx : maxx + 1])
                depth[miny : maxy + 1, minx : maxx + 1] = np.where(mask, zz, depth[miny : maxy + 1, minx : maxx + 1])
    img = Image.fromarray(np.clip(color, 0, 255).astype(np.uint8), "RGB")
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path, quality=92)
    filled = int(np.isfinite(depth).sum())
    print("wrote", path, "pixels", filled)


def dominant_bones(bones, weights):
    dominant = {}
    for point, weights_for_point in weights.items():
        dominant[point] = max(weights_for_point)[1]
    return dominant


def gun_geometry(bones, pts, weights, wedges, faces):
    dominant = dominant_bones(bones, weights)
    grouped = {}
    for point, bone_index in dominant.items():
        grouped.setdefault(bones[bone_index], []).append(pts[point])
    slide = np.mean(grouped["Slide"], axis=0)
    clip = np.mean(grouped["clip"], axis=0)
    hammer = np.mean(grouped["hammer"], axis=0)
    barrel = np.array([0.0, 0.0, 1.0])
    if hammer[2] > slide[2]:
        barrel = -barrel
    grip = clip - slide
    grip = grip - barrel * np.dot(grip, barrel)
    grip = grip / np.linalg.norm(grip)
    side = np.cross(barrel, grip)
    side = side / np.linalg.norm(side)
    grip = np.cross(side, barrel)
    by_mat = {0: [], 1: []}
    for a, b, c, mat, _aux, _sm in faces:
        if mat not in by_mat:
            continue
        ids = (wedges[a][0], wedges[b][0], wedges[c][0])
        uvs = [(wedges[i][1], 1.0 - wedges[i][2]) for i in (a, b, c)]
        by_mat[mat].append((pts[list(ids)], np.array(uvs, dtype=np.float64)))
    return by_mat, clip, barrel, grip, side


def map_points(xyz, origin, x_axis, y_axis, z_axis, dest, dest_x, dest_y, dest_z):
    delta = np.asarray(xyz, dtype=np.float64) - origin
    return (
        dest
        + np.outer(delta @ x_axis, dest_x)
        + np.outer(delta @ y_axis, dest_y)
        + np.outer(delta @ z_axis, dest_z)
    )


def fit_camera(points, view_dir):
    center = points.mean(axis=0)
    view_dir = view_dir / np.linalg.norm(view_dir)
    up = np.array([0.0, 0.0, 1.0])
    right = np.cross(view_dir, up)
    right = right / np.linalg.norm(right)
    cam_up = np.cross(right, view_dir)
    rel = points - center
    span_x = float(np.max(np.abs(rel @ right)))
    span_y = float(np.max(np.abs(rel @ cam_up)))
    dist = max(span_x / (0.72 * 0.78), span_y / (0.72 * 0.78 * H / W), 24.0)
    eye = center - view_dir * dist
    return eye, center, right, cam_up


def main():
    bones, pts, weights, wedges, faces = load_psk(PSK)
    tris, wrist, tips = glove_triangles(bones, pts, weights, wedges, faces)
    print("left", len(tris["l"]), "right", len(tris["r"]), "wrist", wrist, "tips", tips)
    OUT.mkdir(parents=True, exist_ok=True)
    tex_name = "inview_glove_d.png"
    shutil.copyfile(TEX, OUT / tex_name)
    write_obj(OUT / "glove_left.obj", tris["l"], tex_name)
    write_obj(OUT / "glove_right.obj", tris["r"], tex_name)

    glove = np.asarray(Image.open(TEX).convert("RGB"))
    # The source diffuse is very dark. Lift it only for the preview so the
    # real weave stays recognizable.
    glove_view = np.clip(glove.astype(np.float64) * 2.4 + 18, 0, 255).astype(np.uint8)

    def pose(hand_tris, anchor, src_wrist, src_tips):
        finger = src_tips - src_wrist
        palm = np.array([0.0, 0.0, 1.0])
        finger = finger - palm * np.dot(finger, palm)
        finger = finger / np.linalg.norm(finger)
        side_axis = np.cross(finger, palm)
        side_axis = side_axis / np.linalg.norm(side_axis)
        out = []
        for xyz, uv in hand_tris:
            local = np.stack([
                np.dot(xyz - src_wrist, side_axis),
                np.dot(xyz - src_wrist, finger),
                np.dot(xyz - src_wrist, palm),
            ], axis=1)
            out.append((local + anchor, uv))
        return out, anchor.copy()

    # Preview pose. Bind-space gloves are already written above.
    tris["l"], left_c = pose(tris["l"], np.array([-28.0, 0.0, 0.0]), wrist["l"], tips["l"])
    tris["r"], right_c = pose(tris["r"], np.array([16.0, 0.0, 0.0]), wrist["r"], tips["r"])
    finger = np.array([0.0, 1.0, 0.0])
    palm = np.array([0.0, 0.0, 1.0])
    side_axis = np.array([1.0, 0.0, 0.0])

    gun_bones, gun_pts, gun_weights, gun_wedges, gun_faces = load_psk(GUN)
    gun_by_mat, grip_origin, barrel, grip, gun_side = gun_geometry(
        gun_bones, gun_pts, gun_weights, gun_wedges, gun_faces
    )
    gun_dest = right_c + finger * 6.0 + palm * 0.6

    def place_gun(src):
        placed = []
        for xyz, uv in src:
            mapped = map_points(xyz, grip_origin, gun_side, barrel, grip, gun_dest, side_axis, finger, -palm)
            placed.append((mapped, uv))
        return placed

    gun_body = place_gun(gun_by_mat[0])
    gun_laser = place_gun(gun_by_mat[1])
    gun_tex_name = "handgun_d.png"
    shutil.copyfile(GUN_TEX, OUT / gun_tex_name)
    write_obj(OUT / "pistol.obj", gun_body, gun_tex_name)
    gun_view = np.asarray(Image.open(GUN_TEX).convert("RGB"))
    gun_view = np.clip(gun_view.astype(np.float64) * 1.8 + 12, 0, 255).astype(np.uint8)
    laser_view = np.zeros((8, 8, 3), dtype=np.uint8)
    laser_view[:, :] = (150, 42, 36)

    right_and_gun = np.concatenate(
        [t[0] for t in tris["r"]] + [t[0] for t in gun_body]
    ).reshape(-1, 3)
    hand_gun_center = right_and_gun.mean(0)
    view_dir = np.array([0.0, 0.28, -0.96])
    view_dir = view_dir / np.linalg.norm(view_dir)
    toward_camera = -view_dir
    _eye, _center, screen_right, screen_up = fit_camera(
        np.concatenate([
            np.concatenate([t[0] for t in tris["l"]]).reshape(-1, 3),
            right_and_gun,
        ]),
        view_dir,
    )
    radial = right_and_gun - hand_gun_center
    flat = np.sqrt((radial @ screen_right) ** 2 + (radial @ screen_up) ** 2)
    reach = float(np.max(flat))
    def in_front(cloud, anchor, gap):
        proj = cloud @ toward_camera
        return anchor + toward_camera * (float(proj.max()) - float(anchor @ toward_camera) + gap)

    left_cloud = np.concatenate([t[0] for t in tris["l"]]).reshape(-1, 3)
    ring = annulus(hand_gun_center, screen_right, screen_up, reach * 1.12, reach * 1.38)
    wrist_plate = box(in_front(left_cloud, left_c + finger * 0.6, 0.9), screen_right, screen_up, toward_camera, 4.8, 2.8, 0.1)
    ammo_plate = box(in_front(right_and_gun, hand_gun_center - finger * 2.0, 0.9) + screen_right * -5.5, screen_right, screen_up, toward_camera, 4.6, 2.7, 0.1)

    cloud = np.concatenate(
        [t[0].reshape(-1, 3) for t in tris["l"] + tris["r"] + gun_body + wrist_plate + ammo_plate + ring]
    )
    eye, center, _right, _up = fit_camera(cloud, view_dir)
    print("eye", eye, "center", center, "ring reach", reach)

    write_obj(OUT / "preview_glove_left.obj", tris["l"], tex_name)
    write_obj(OUT / "preview_glove_right.obj", tris["r"], tex_name)
    write_obj(OUT / "hud_wrist.obj", wrist_plate, None)
    write_obj(OUT / "hud_ammo.obj", ammo_plate, None)
    write_obj(OUT / "hud_ring.obj", ring, None)

    calm_tex = plate_texture("NADGARSTEK", "10/10", 10, False)
    ammo_tex = plate_texture("MAGAZYNEK", "12 / 48", 10, False)
    ring_tex = ring_texture()
    Image.fromarray(calm_tex).save(OUT / "hud_wrist.png")
    Image.fromarray(ammo_tex).save(OUT / "hud_ammo.png")
    Image.fromarray(ring_tex).save(OUT / "hud_ring.png")

    hands = (tris["l"] + tris["r"], glove_view, 1.15, 1.0)
    pistol = (gun_body, gun_view, 1.25, 1.0)
    laser = (gun_laser, laser_view, 1.0, 1.0)
    calm = [
        hands, pistol,
        (wrist_plate, calm_tex, 1.0, 1.0),
        (ammo_plate, ammo_tex, 1.0, 1.0),
    ]
    switch = [
        hands, pistol,
        (wrist_plate, calm_tex, 1.0, 1.0),
        (ring, ring_tex, 1.0, 1.0),
    ]
    jobs = [
        (calm, OUT / "02-spokoj.jpg", SHOW / "katorga-real-spokoj.jpg"),
        (switch, OUT / "03-zmiana-broni.jpg", SHOW / "katorga-real-zmiana.jpg"),
    ]
    for layers, dest, show in jobs:
        render(layers, eye, center, dest)
        shutil.copyfile(dest, show)


if __name__ == "__main__":
    main()
