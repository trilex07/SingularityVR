"""Add measured rigid surface details and authored PBR maps to the Volk handoff.

Consumes the existing private extraction. It never changes the source PSK,
original OBJ partitions, bone hierarchy, pivots, or animation data.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import shutil
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter


def load_obj(path):
    vertices, uvs, faces = [], [], []
    for line in path.read_text(encoding="utf-8").splitlines():
        row = line.split()
        if not row:
            continue
        if row[0] == "v": vertices.append([float(x) for x in row[1:4]])
        elif row[0] == "vt": uvs.append([float(x) for x in row[1:3]])
        elif row[0] == "f": faces.append([int(x.split("/")[0])-1 for x in row[1:4]])
    return np.array(vertices, float), np.array(uvs, float), np.array(faces, int)


def surface_x(vertices, faces, side, y, z):
    """Exact barycentric side surface lookup; no guessed offset."""
    best = None
    for tri in faces:
        p = vertices[tri]
        yz = p[:, 1:]
        mat = np.array([[yz[0,0]-yz[2,0], yz[1,0]-yz[2,0]],
                        [yz[0,1]-yz[2,1], yz[1,1]-yz[2,1]]])
        if abs(np.linalg.det(mat)) < 1e-7: continue
        a,b = np.linalg.solve(mat, np.array([y,z])-yz[2])
        c = 1-a-b
        if min(a,b,c) < .035: continue
        n = np.cross(p[1]-p[0], p[2]-p[0])
        if np.linalg.norm(n) < 1e-8 or abs(n[0])/np.linalg.norm(n) < .35: continue
        x = a*p[0,0]+b*p[1,0]+c*p[2,0]
        if side*x < .15: continue
        if best is None or side*x > side*best: best=x
    return best


def cylinder_disk(center, normal, radius, depth, sides=16):
    n = np.array(normal, float); n /= np.linalg.norm(n)
    a = np.array([0.,1.,0.]) if abs(n[1]) < .9 else np.array([0.,0.,1.])
    u = np.cross(n,a); u /= np.linalg.norm(u)
    v = np.cross(n,u)
    rings=[]
    for h,r in ((0.,radius*.88),(depth*.22,radius),(depth*.78,radius),(depth,radius*.76)):
        rings.append([center+n*h+r*(u*math.cos(2*math.pi*i/sides)+v*math.sin(2*math.pi*i/sides)) for i in range(sides)])
    points=np.array([p for ring in rings for p in ring])
    tris=[]
    for k in range(3):
        for i in range(sides):
            j=(i+1)%sides
            tris += [(k*sides+i,k*sides+j,(k+1)*sides+j),
                     (k*sides+i,(k+1)*sides+j,(k+1)*sides+i)]
    for i in range(1,sides-1): tris.append((3*sides,3*sides+i,3*sides+i+1))
    return points, tris


def write_detail(path, points, faces, material="machined_dark_metal"):
    with path.open("w", encoding="utf-8", newline="\n") as f:
        f.write(f"# Original PSK coordinate space, Unreal units\nmtllib volk_s4_hd.mtl\no {path.stem}\n")
        for p in points: f.write("v %.8f %.8f %.8f\n"%tuple(p))
        f.write(f"usemtl {material}\n")
        for tri in faces: f.write("f %d %d %d\n"%tuple(i+1 for i in tri))


def chamfered_panel(side, x, y0, y1, z0, z1, thickness, chamfer):
    """Raised eight-corner plate; all dimensions are original Unreal units."""
    outline=[(y0+chamfer,z0),(y1-chamfer,z0),(y1,z0+chamfer),
             (y1,z1-chamfer),(y1-chamfer,z1),(y0+chamfer,z1),
             (y0,z1-chamfer),(y0,z0+chamfer)]
    rear=[(side*x,y,z) for y,z in outline]
    front=[(side*(x+thickness),y,z) for y,z in outline]
    vertices=rear+front
    faces=[]
    for i in range(1,7): faces.append((8,8+i,8+i+1))
    for i in range(8):
        j=(i+1)%8
        faces.extend(((i,j,8+j),(i,8+j,8+i)))
    return vertices,faces


def make_details(root, manifest):
    # Placement is limited to independently identified original rigid surfaces.
    # Each candidate must resolve to an outward side triangle on its own part.
    specs={
        "magazine": [(y,z,.33) for y,z in ((103.4,55.0),(106.1,56.0),(108.5,57.2))],
        "moving_reload_part": [(y,z,.28) for y,z in ((114.0,61.0),(114.0,67.0),(114.0,72.0))],
        "weapon_body": [(y,z,.31) for y,z in ((115.0,47.0),(115.0,51.0),(116.0,83.0),(116.0,89.0))],
    }
    report={}
    for part, placements in specs.items():
        vertices,_,faces=load_obj(root/"clean"/f"{part}.obj")
        all_points=[]; all_faces=[]; accepted=[]
        for y,z,r in placements:
            for side in (-1,1):
                x=surface_x(vertices,faces,side,y,z)
                if x is None: continue
                center=np.array([x+side*.012,y,z])
                p,t=cylinder_disk(center,[side,0,0],r,.095)
                offset=len(all_points)
                all_points.extend(p); all_faces.extend([(a+offset,b+offset,c+offset) for a,b,c in t])
                accepted.append({"center_uu":center.tolist(),"radius_uu":r,"source_part":part})
        if not accepted: raise ValueError(f"no surface-confirmed details on {part}")
        write_detail(root/"clean"/f"{part}_machined_fasteners.obj",all_points,all_faces)
        report[part]={"source_bone":manifest["objects"][part]["source_bone"],"instances":accepted,
                      "added_vertices":len(all_points),"added_triangles":len(all_faces)}
    # Two purpose-built reinforcement pieces follow measured planar faces.
    # Their rear lies on or barely above the source surface; no socket moves.
    plates={
        "magazine_rib_lower":("magazine",105.1,57.2,104.97,105.23,55.8,58.8,.07,.08),
        "magazine_rib_middle":("magazine",106.6,57.0,106.47,106.73,55.8,58.8,.07,.08),
        "magazine_rib_upper":("magazine",108.1,57.2,107.97,108.23,55.8,58.8,.07,.08),
    }
    for name,(part,cy,cz,y0,y1,z0,z1,thick,chamfer) in plates.items():
        v,_,f=load_obj(root/"clean"/f"{part}.obj")
        all_p=[];all_f=[];anchors=[]
        for side in (-1,1):
            x=surface_x(v,f,side,cy,cz)
            if x is None: raise ValueError(f"no source surface for {name} side {side}")
            p,t=chamfered_panel(side,abs(x)+.015,y0,y1,z0,z1,thick,chamfer)
            offset=len(all_p);all_p.extend(p)
            all_f.extend([(a+offset,b+offset,c+offset) for a,b,c in t])
            anchors.append({"side":side,"surface_x_uu":float(x),"center_yz_uu":[cy,cz]})
        write_detail(root/"clean"/f"{name}.obj",all_p,all_f)
        report[name]={"source_bone":manifest["objects"][part]["source_bone"],
                      "source_part":part,"anchors":anchors,"added_vertices":len(all_p),
                      "added_triangles":len(all_f)}
    return report


def make_maps(root, ai_tile, ai_atlas):
    size=4096
    source=np.asarray(Image.open(root/"source"/"AR_shotgun_d_source.dds").convert("RGB").resize((size,size),Image.Resampling.LANCZOS),dtype=np.float32)
    old_normal=np.asarray(Image.open(root/"source"/"AR_shotgun_n_source.dds").convert("RGB").resize((size,size),Image.Resampling.BILINEAR),dtype=np.float32)/127.5-1
    tile_img=Image.open(ai_tile).convert("L")
    # Remove broad panel lines from the generated tile; retain real new machining microdetail.
    low=np.asarray(tile_img.filter(ImageFilter.GaussianBlur(8)),dtype=np.float32)
    high=np.asarray(tile_img,dtype=np.float32)-low
    high=np.asarray(Image.fromarray(np.uint8(np.clip(high+128,0,255))).resize((2048,2048),Image.Resampling.BICUBIC),dtype=np.float32)-128
    # The generated reference was not pixel-exact seamless. Feather opposite
    # edges together before tiling to avoid a normal-map seam.
    for i in range(48):
        t=.5*(1-i/48)
        left=high[:,i].copy();right=high[:,-i-1].copy()
        high[:,i]=(1-t)*left+t*right
        high[:,-i-1]=(1-t)*right+t*left
    for i in range(48):
        t=.5*(1-i/48)
        top=high[i,:].copy();bottom=high[-i-1,:].copy()
        high[i,:]=(1-t)*top+t*bottom
        high[-i-1,:]=(1-t)*bottom+t*top
    detail=np.tile(high,(2,2))/128.0
    authored=Image.open(ai_atlas).convert("L").resize((size,size),Image.Resampling.LANCZOS)
    authored_high=np.asarray(authored,dtype=np.float32)-np.asarray(authored.filter(ImageFilter.GaussianBlur(6)),dtype=np.float32)
    # Material confidence follows the source pixels, never coarse atlas rectangles.
    # Ambiguous dark surfaces remain dielectric pending artist review.
    lum=source.mean(axis=2)
    sat=source.max(axis=2)-source.min(axis=2)
    neutral=np.clip((lum-48)/47,0,1)*np.clip((80-sat)/45,0,1)
    bronze=np.clip((source[...,0]-source[...,2]-20)/55,0,1)*np.clip((lum-55)/45,0,1)
    metal_mask=np.clip(np.maximum(neutral,bronze),0,1)
    metal_mask=np.asarray(Image.fromarray(np.uint8(metal_mask*255),"L").filter(ImageFilter.GaussianBlur(2)),dtype=np.float32)/255
    # Transfer only AI-authored fine detail. The large generated shapes are rejected
    # because they do not share exact original UV boundaries.
    edge_y,edge_x=np.gradient(lum)
    interior=np.clip((23-np.maximum(np.abs(edge_x),np.abs(edge_y)))/17,0,1)
    interior*=np.clip((lum-18)/42,0,1)
    authored_detail=authored_high*interior
    # Newly authored machining height is subtle in albedo and strongest in normal/roughness.
    albedo=np.clip(source + ((detail*13*metal_mask)+(authored_detail*.86))[...,None],0,255).astype(np.uint8)
    Image.fromarray(albedo,"RGB").save(root/"textures"/"volk_s4_hd_albedo_4k.png")
    height=detail*metal_mask+authored_detail/55
    dy,dx=np.gradient(height)
    n=old_normal.copy()
    n[...,0]-=dx*1.45; n[...,1]-=dy*1.45
    n/=np.maximum(np.linalg.norm(n,axis=2,keepdims=True),1e-6)
    enc=np.uint8(np.clip((n*.5+.5)*255,0,255))
    Image.fromarray(enc,"RGB").save(root/"textures"/"volk_s4_hd_normal_4k.png")
    # Metal values stay high only in curated metal islands; polymer remains dielectric.
    metallic=np.uint8(np.clip(8+metal_mask*235,0,255))
    roughness=np.uint8(np.clip(182-metal_mask*63+np.abs(detail)*20,70,220))
    Image.fromarray(metallic,"L").save(root/"textures"/"volk_s4_hd_metallic_4k.png")
    Image.fromarray(roughness,"L").save(root/"textures"/"volk_s4_hd_roughness_4k.png")
    Image.fromarray(np.uint8(np.clip(height*55+128,0,255)),"L").save(root/"textures"/"volk_s4_hd_height_4k.png")
    return {"resolution":[size,size],"normal_length_min":float(np.linalg.norm(n,axis=2).min()),
            "normal_length_max":float(np.linalg.norm(n,axis=2).max()),
            "ai_tile_sha256":hashlib.sha256(Path(ai_tile).read_bytes()).hexdigest(),
            "ai_atlas_sha256":hashlib.sha256(Path(ai_atlas).read_bytes()).hexdigest(),
            "albedo_mean_absolute_delta":float(np.abs(albedo.astype(float)-source).mean())}


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--package",type=Path,required=True)
    ap.add_argument("--ai-tile",type=Path,required=True)
    ap.add_argument("--ai-atlas",type=Path,required=True)
    a=ap.parse_args(); root=a.package.resolve()
    manifest=json.loads((root/"clean"/"rig_manifest.json").read_text())
    if manifest["source_psk_sha256"]!="941c871b695480e608ac8eb0ca25b99731344eca57383c9c5927cf71e7baf1ab":
        raise ValueError("unexpected PSK source")
    for incoming, saved in ((a.ai_tile,"ai_machining_tile.png"),(a.ai_atlas,"ai_atlas_detail_reference.png")):
        destination=root/"source"/saved
        if incoming.resolve()!=destination.resolve(): shutil.copy2(incoming,destination)
    detail=make_details(root,manifest)
    maps=make_maps(root,a.ai_tile,a.ai_atlas)
    (root/"clean"/"volk_s4.mtl").write_text(
        "newmtl volk_s4\nKd 1 1 1\nmap_Kd ../textures/volk_s4_hd_albedo_4k.png\n"
        "map_Pr ../textures/volk_s4_hd_roughness_4k.png\n"
        "map_Pm ../textures/volk_s4_hd_metallic_4k.png\n"
        "norm ../textures/volk_s4_hd_normal_4k.png\n",encoding="utf-8")
    (root/"clean"/"volk_s4_hd.mtl").write_text(
        "newmtl machined_dark_metal\nKd 0.14 0.15 0.15\nKs 0.55 0.55 0.55\nNs 150\n",encoding="utf-8")
    report={"source_psk_sha256":manifest["source_psk_sha256"],"rig_unchanged":True,
            "source_vertices_unchanged":True,"new_geometry":detail,"new_maps":maps,
            "binding":"Parent each detail OBJ to its named source_bone in rig_manifest.json; do not add bones or change sockets."}
    (root/"clean"/"hd_detail_manifest.json").write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(report,indent=2))


if __name__=="__main__": main()
