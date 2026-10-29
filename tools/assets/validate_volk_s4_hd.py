"""Offline structural and texture validation of the private Volk S4 HD handoff."""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image
from finish_volk_s4_hd import load_obj


def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser();p.add_argument("--package",type=Path,required=True)
    root=p.parse_args().package
    rig=json.loads((root/"clean"/"rig_manifest.json").read_text())
    hd=json.loads((root/"clean"/"hd_detail_manifest.json").read_text())
    assert digest(root/"source"/"ch_inview_shotgun_mesh.psk")==rig["source_psk_sha256"]==hd["source_psk_sha256"]
    assert sum(x["triangles"] for x in rig["objects"].values())==10835
    for name,entry in rig["objects"].items():
        v,uv,f=load_obj(root/"clean"/f"{name}.obj")
        assert len(v)==entry["vertices"] and len(f)==entry["triangles"] and len(v)==len(uv)
        assert np.isfinite(v).all() and np.isfinite(uv).all() and f.min()>=0 and f.max()<len(v)
    added=0
    for name,entry in hd["new_geometry"].items():
        filename=f"{name}_machined_fasteners.obj" if "instances" in entry else f"{name}.obj"
        v,uv,f=load_obj(root/"clean"/filename)
        assert entry["source_bone"] in {b["name"] for b in rig["bones"]}
        assert len(v)==entry["added_vertices"] and len(f)==entry["added_triangles"]
        assert np.isfinite(v).all() and f.min()>=0 and f.max()<len(v)
        added+=len(f)
    maps=["albedo","normal","roughness","metallic","height"]
    for name in maps:
        im=Image.open(root/"textures"/f"volk_s4_hd_{name}_4k.png")
        assert im.size==(4096,4096)
    n=np.asarray(Image.open(root/"textures"/"volk_s4_hd_normal_4k.png"),dtype=np.float32)/127.5-1
    norm=np.linalg.norm(n,axis=2)
    assert np.isfinite(n).all() and np.abs(norm-1).max()<.018
    previews=["original_vs_hd_side.png","original_vs_hd_reload_close.png",
              "material_detail_original_vs_hd.png"]
    assert all((root/"previews"/x).is_file() for x in previews)
    report={"status":"OFFLINE_STRUCTURAL_PASS","source_psk_sha256":rig["source_psk_sha256"],
            "source_faces":10835,"added_faces":added,"normal_length_max_error":float(np.abs(norm-1).max()),
            "comparison_previews":previews,"runtime_or_visual_acceptance":"NOT_RUN"}
    (root/"hd_validation.json").write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(report,indent=2))


if __name__=="__main__":main()
