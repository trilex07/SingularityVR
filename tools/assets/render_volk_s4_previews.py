"""Offline, texture-colored inspection views of the private Volk OBJ partitions."""
import argparse
from pathlib import Path
import numpy as np
from PIL import Image
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.collections import PolyCollection


def load_obj(path):
    vertices, uv, faces = [], [], []
    for line in path.read_text(encoding="utf-8").splitlines():
        row = line.split()
        if not row:
            continue
        if row[0] == "v":
            vertices.append([float(x) for x in row[1:4]])
        elif row[0] == "vt":
            uv.append([float(x) for x in row[1:3]])
        elif row[0] == "f":
            faces.append([int(x.split("/")[0])-1 for x in row[1:4]])
    return np.array(vertices), np.array(uv), np.array(faces)


def unit(x):
    return x / np.linalg.norm(x)


def render(parts, texture, path, eye, target, title, selected=None, span_override=None):
    forward = unit(np.array(target)-np.array(eye))
    up0 = np.array([0., 1., 0.])
    right = unit(np.cross(forward, up0))
    up = np.cross(right, forward)
    polys, colors, depths = [], [], []
    tex = np.asarray(texture)
    for name, (v, uv, faces) in parts.items():
        if selected is not None and name not in selected:
            continue
        rel = v - target
        xy = np.c_[rel @ right, rel @ up]
        depth = rel @ forward
        if len(uv):
            uv_mean = uv[faces].mean(axis=1)
            px = np.clip((uv_mean[:, 0] * (tex.shape[1]-1)).astype(int), 0, tex.shape[1]-1)
            py = np.clip(((1-uv_mean[:, 1]) * (tex.shape[0]-1)).astype(int), 0, tex.shape[0]-1)
        shade = np.clip(0.62 + 0.38*np.abs(np.cross(v[faces[:,1]]-v[faces[:,0]], v[faces[:,2]]-v[faces[:,0]]) @ forward) /
                        (np.linalg.norm(np.cross(v[faces[:,1]]-v[faces[:,0]], v[faces[:,2]]-v[faces[:,0]]), axis=1)+1e-9), .55, 1)
        # Display exposure compensates for the unusually dark source atlas;
        # it does not alter the packaged texture.
        base = tex[py, px, :3].astype(float) if len(uv) else np.full((len(faces),3),(53,58,57),float)
        # One fixed display exposure and geometric light for both comparison views.
        col = np.clip(np.sqrt(base/255.0)*255.0*shade[:, None]*1.05, 0, 255).astype(np.uint8)
        polys.extend(xy[faces]); colors.extend(col / 255); depths.extend(depth[faces].mean(axis=1))
    order = np.argsort(depths)[::-1]
    fig, ax = plt.subplots(figsize=(14, 8), dpi=150)
    fig.patch.set_facecolor("#181b1c")
    ax.set_facecolor("#181b1c")
    ax.add_collection(PolyCollection(np.array(polys)[order], facecolors=np.array(colors)[order], edgecolors="none"))
    all_xy = np.array(polys).reshape(-1, 2)
    lo, hi = all_xy.min(axis=0), all_xy.max(axis=0)
    mid = (lo+hi)/2
    span = span_override or max((hi-lo).max()*1.15, 1)
    ax.set_xlim(mid[0]-span/2, mid[0]+span/2)
    ax.set_ylim(mid[1]-span/2*8/14, mid[1]+span/2*8/14)
    ax.set_aspect("equal"); ax.axis("off")
    ax.set_title(title, color="#d4d5d0", fontsize=15, pad=16)
    fig.savefig(path, facecolor=fig.get_facecolor(), bbox_inches="tight", pad_inches=.2)
    plt.close(fig)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--package", type=Path, required=True)
    a = ap.parse_args()
    root = a.package
    names = ["weapon_body", "magazine", "moving_reload_part", "trigger", "release"]
    parts = {n: load_obj(root / "clean" / f"{n}.obj") for n in names}
    tex = Image.open(root / "textures" / "volk_s4_basecolor.png").convert("RGB")
    body = parts["weapon_body"][0]
    center = (body.min(axis=0)+body.max(axis=0))/2
    extent = body.max(axis=0)-body.min(axis=0)
    distance = max(extent)*2
    views = [
        ("left_side.png", center+[-distance,0,.05*distance], center, "Volk S4 — left side", None),
        ("right_side.png", center+[distance,0,.05*distance], center, "Volk S4 — right side", None),
        ("first_person_style.png", center+[-.6*distance,-.55*distance,-.2*distance], center, "Volk S4 — first-person style inspection", None),
        ("detached_magazine.png", parts["magazine"][0].mean(axis=0)+[distance*.4,-distance*.35,distance*.15], parts["magazine"][0].mean(axis=0), "Volk S4 — detached original magazine", {"magazine"}),
        ("reload_area.png", center+[-.5*distance,-.35*distance,.1*distance], center, "Volk S4 — reload area", {"weapon_body", "magazine", "moving_reload_part", "trigger", "release"}),
    ]
    for filename, eye, target, title, selected in views:
        render(parts, tex, root/"previews"/filename, eye, target, title, selected)
        print(filename)
    hd_map=root/"textures"/"volk_s4_hd_albedo_4k.png"
    if hd_map.exists():
        from PIL import ImageOps, ImageDraw
        original_tex=Image.open(root/"source"/"AR_shotgun_d_source.dds").convert("RGB")
        hd_parts=dict(parts)
        for n in ("weapon_body", "magazine", "moving_reload_part"):
            hd_parts[n+"_fasteners"]=load_obj(root/"clean"/f"{n}_machined_fasteners.obj")
        for n in ("magazine_rib_lower", "magazine_rib_middle", "magazine_rib_upper"):
            hd_parts[n]=load_obj(root/"clean"/f"{n}.obj")
        hd_tex=Image.open(hd_map).convert("RGB")
        for filename, eye, target, title, selected in views:
            hd_selected=None if selected is None else set(selected)|{
                n for n in hd_parts if (n.endswith("_fasteners") and n.removesuffix("_fasteners") in selected)
                or (n.startswith("magazine_rib_") and "magazine" in selected)}
            render(hd_parts,hd_tex,root/"previews"/("hd_"+filename),eye,target,
                   title.replace("Volk S4", "Volk S4 HD"),hd_selected)
        comparison_views=[
            ("side",center+[-distance,0,.05*distance],center,None),
            ("reload_close",center+[-.75*distance,-.25*distance,0],np.array([0.,108.,57.]),29),
        ]
        for label,eye,target,span in comparison_views:
            original=root/"previews"/f"compare_{label}_original.png"
            remaster=root/"previews"/f"compare_{label}_hd.png"
            render(parts,original_tex,original,eye,target,"Original",span_override=span)
            render(hd_parts,hd_tex,remaster,eye,target,"HD remaster",span_override=span)
            im1=Image.open(original).convert("RGB"); im2=Image.open(remaster).convert("RGB")
            w=max(im1.width,im2.width); h=max(im1.height,im2.height)
            pair=Image.new("RGB",(2*w,h),(24,27,28));pair.paste(im1,(0,0));pair.paste(im2,(w,0))
            pair.save(root/"previews"/f"original_vs_hd_{label}.png")


if __name__ == "__main__":
    main()
