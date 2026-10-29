"""Build a private, bind-pose-preserving Volk S4 visual handoff from owned PSK/DDS.

The source skeleton, UVs, bind pose, pivots and coordinates are never modified.
No runtime files are written. Output contains game-derived assets; keep private.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import sys
from collections import Counter, defaultdict
from pathlib import Path

import numpy as np
from PIL import Image, ImageEnhance, ImageFilter

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "gloves"))
from prepare_gloves import read_psk


PARTS = {"cog_shotgun": "weapon_body", "clip": "magazine",
         "Slide": "moving_reload_part", "Trigger": "trigger", "release": "release"}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def write_obj(path, name, triangles, points, wedges, material):
    rows = {}
    vertices = []
    uvs = []
    faces = []
    for tri in triangles:
        face = []
        for wi in tri[:3]:
            p, u, v = wedges[wi]
            key = (p, u, v)
            if key not in rows:
                rows[key] = len(vertices) + 1
                vertices.append(points[p])
                uvs.append((u, 1.0-v))
            face.append(rows[key])
        faces.append(face)
    with path.open("w", encoding="utf-8", newline="\n") as out:
        out.write(f"# Original PSK bind-pose coordinates, Unreal units\nmtllib {material}.mtl\no {name}\n")
        for x, y, z in vertices:
            out.write(f"v {x:.9g} {y:.9g} {z:.9g}\n")
        for u, v in uvs:
            out.write(f"vt {u:.9g} {v:.9g}\n")
        out.write(f"usemtl {material}\n")
        for a, b, c in faces:
            out.write(f"f {a}/{a} {b}/{b} {c}/{c}\n")
    return {"vertices": len(vertices), "triangles": len(faces), "source_bone": name,
            "bounds_uu": [np.min(vertices, axis=0).tolist(), np.max(vertices, axis=0).tolist()]}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--psk", type=Path, required=True)
    ap.add_argument("--diffuse", type=Path, required=True)
    ap.add_argument("--normal", type=Path, required=True)
    ap.add_argument("--output", type=Path, required=True)
    a = ap.parse_args()
    root = a.output.resolve()
    for folder in ("source", "clean", "textures", "previews"):
        (root / folder).mkdir(parents=True, exist_ok=True)
    psk = read_psk(a.psk)
    names, parents, _, bind, points, weights, wedges, faces = psk
    if a.psk.stem != "ch_inview_shotgun_mesh" or len(wedges) != 8297:
        raise ValueError("wrong Volk source mesh")
    if any(b not in names for b in PARTS):
        raise ValueError("missing expected mechanical bones")
    shutil.copy2(a.psk, root / "source" / a.psk.name)
    shutil.copy2(a.diffuse, root / "source" / "AR_shotgun_d_source.dds")
    shutil.copy2(a.normal, root / "source" / "AR_shotgun_n_source.dds")

    # Faithful material polish only: restrained tonal lift and edge clarity.
    # UVs and normals are retained. PBR scalar maps are authored approximations.
    diffuse = Image.open(a.diffuse).convert("RGBA")
    rgb = diffuse.convert("RGB")
    rgb = ImageEnhance.Contrast(rgb).enhance(1.075)
    rgb = ImageEnhance.Brightness(rgb).enhance(1.045)
    rgb = rgb.filter(ImageFilter.UnsharpMask(radius=1.0, percent=70, threshold=5))
    rgb.save(root / "textures" / "volk_s4_basecolor.png")
    Image.open(a.normal).convert("RGB").save(root / "textures" / "volk_s4_normal_source_encoding.png")
    arr = np.asarray(rgb, dtype=np.uint8)
    lum = arr.mean(axis=2)
    saturation = arr.max(axis=2).astype(float) - arr.min(axis=2).astype(float)
    # Conservative authored preview estimates; source normal uses UE3 packing.
    metal = np.where((lum > 52) & (saturation < 34), 155, 45).astype("uint8")
    rough = np.clip(174 - 0.18 * lum + 0.25 * saturation, 90, 211).astype("uint8")
    Image.fromarray(metal, "L").save(root / "textures" / "volk_s4_metallic_estimate.png")
    Image.fromarray(rough, "L").save(root / "textures" / "volk_s4_roughness_estimate.png")
    (root / "clean" / "volk_s4.mtl").write_text(
        "newmtl volk_s4\nKd 1 1 1\nmap_Kd ../textures/volk_s4_basecolor.png\n",
        encoding="utf-8")

    grouped = defaultdict(list)
    mixed = Counter()
    for face in faces:
        bone_ids = []
        for wi in face[:3]:
            point = wedges[wi][0]
            row = weights[point]
            bone_ids.append(max(row)[1])
        if len(set(bone_ids)) == 1 and names[bone_ids[0]] in PARTS:
            grouped[names[bone_ids[0]]].append(face)
        else:
            mixed.update([names[b] for b in bone_ids])
    if mixed:
        raise ValueError(f"mixed/unassigned faces: {mixed}")
    objects = {}
    for bone, label in PARTS.items():
        objects[label] = write_obj(root / "clean" / f"{label}.obj", bone,
                                   grouped[bone], points, wedges, "volk_s4")

    reflect = np.diag([1., -1., 1., 1.])
    native_bind = reflect @ bind @ reflect
    bones = [{"index": i, "name": name, "parent": parents[i],
              "bind_matrix_row_major_uu": native_bind[i].tolist(),
              "pivot_uu": native_bind[i][3, :3].tolist()}
             for i, name in enumerate(names)]
    manifest = {
        "identity": "Volk S4 / ch_inview_shotgun_mesh", "source_psk_sha256": sha(a.psk),
        "source_diffuse_sha256": sha(a.diffuse), "source_normal_sha256": sha(a.normal),
        "coordinate_contract": "OBJ vertices are original PSK PNTS0000 XYZ in Unreal units; no recenter, reflection, rotation or scale. UV V is flipped only for OBJ convention.",
        "rig_contract": "Use original PSK/PSA skeleton and native sockets. These OBJ files are bind-pose visual partitions, not a replacement animation rig.",
        "objects": objects, "bones": bones,
        "markers": {"muzzle": "muzzle_flash bone pivot", "magazine_socket": "clip bind pivot", "grip": "body surface; no new pivot authored"},
        "texture_status": "basecolor polished; metallic/roughness are estimates; normal is original UE3 encoding, conversion required for glTF/Blender tangent space",
    }
    (root / "clean" / "rig_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    (root / "volk_s4_notes.md").write_text(
        "# Volk S4 visual handoff\n\nOriginal PSK skeleton, bind pose, coordinates, UVs, mechanical pivots and reload attachment points are preserved. Source PSK/DDS files are private game-derived inputs. The five clean OBJ partitions retain original bind-pose vertices and separate magazine, slide, trigger and release. The body remains the recognizable original Volk silhouette.\n\nThe base color has restrained contrast, brightness and edge cleanup for close viewing. Metallic and roughness maps are authored estimates that need art review. The normal map is preserved in its original UE3 encoding and is **not** a validated Blender/glTF tangent-space normal map. Geometry density and silhouette are unchanged; a high-poly bevel or bake has not been performed.\n\nReload motion stays on the original PSK/PSA. Import OBJ with no transform and bind to the corresponding original bones using `clean/rig_manifest.json`. `muzzle_flash` and `clip` pivots define the original muzzle and magazine socket references. The `grip` is a named body region, not a new socket. Review UV/material parity and source texture licensing before integration. No Steam, runtime or PSVR2 validation was done.\n",
        encoding="utf-8")
    print(json.dumps({"output": str(root), "parts": {k: v["triangles"] for k, v in objects.items()}, "source_sha256": sha(a.psk)}, indent=2))


if __name__ == "__main__":
    main()
