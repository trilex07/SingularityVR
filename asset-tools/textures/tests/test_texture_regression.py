#!/usr/bin/env python3
"""Regression tests for the texture upscale pipeline (no game, no GPU).

Guards the fixed bugs:
  1. Fake-upscaled lower mips (door_blastdoorN_s incident): is_true_mip0 must
     reject anything whose exported file is not the object mip0.
  2. Stale deploy files: every injector target must have a matching candidate,
     so build.ps1 can only ship what targets_generated.h lists.
  3. Content-hash stability: fnv1a64_mip0 is deterministic and distinguishes
     different textures (the injector's whole matching depends on it).

Run:  python tests/test_texture_regression.py   (exit 0 = all pass)
"""
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))  # asset-tools/textures for texture_pipeline
REPO = HERE.parents[2]  # repo root

import texture_pipeline as tp

FAIL = []


def check(cond, msg):
    if not cond:
        FAIL.append(msg)
        print(f"FAIL: {msg}")


def test_mip0_gate():
    base = {"size_x": 1024, "size_y": 1024}
    check(tp.is_true_mip0({**base, "decoded_size": [1024, 1024]}), "exact decoded_size passes")
    check(tp.is_true_mip0({**base, "exported_dims": [1024, 1024]}), "exported_dims fallback passes")
    check(tp.is_true_mip0({**base, "decoded_size": [1024, 1024],
                           "exported_dims": [128, 256]}), "decoded_size wins over bad log parse")
    check(not tp.is_true_mip0({**base, "decoded_size": [512, 512]}), "lower mip rejected")
    check(not tp.is_true_mip0({**base, "exported_dims": None}), "missing export rejected")
    check(not tp.is_true_mip0({}), "empty metadata rejected")
    # the actual incident: blastdoor specular object 512x256, file 256x128
    check(not tp.is_true_mip0({"size_x": 512, "size_y": 256,
                               "decoded_size": [256, 128]}), "blastdoor_s case rejected")


def test_fnv():
    dds_dir = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\textures\original")
    a = next(iter((dds_dir / "RvGame" / "Texture2D").glob("inview_glove_d.dds")), None)
    b = next(iter((dds_dir / "RvGame" / "Texture2D").glob("inview_glove_n.dds")), None)
    if a is None or b is None:
        print("SKIP fnv: originals not extracted locally")
        return
    check(tp.fnv1a64_mip0(a) == tp.fnv1a64_mip0(a), "fnv deterministic")
    check(tp.fnv1a64_mip0(a) != tp.fnv1a64_mip0(b), "fnv distinguishes textures")


def test_targets_consistent():
    header = REPO / "asset-tools" / "texture-injector" / "targets_generated.h"
    up = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\textures\upscaled")
    if not header.exists():
        print("SKIP targets: header not generated")
        return
    names = sorted(set(re.findall(r'"([^"]+)\.dds"', header.read_text(encoding="utf-8"))))
    check(len(names) > 0, "targets header non-empty")
    missing = [n for n in names
               if not (up / f"{n}__B_ibp.dds").exists() and not (up / f"{n}__A_lanczos.dds").exists()]
    check(not missing, f"every target has a candidate (missing: {missing[:5]})")
    # every upscaled target must either be a true mip0 or a runtime capture
    meta = json.loads((REPO / "research" / "texture-upscale" / "manifests"
                       / "texture_metadata.json").read_text(encoding="utf-8"))["textures"]
    results = json.loads((REPO / "research" / "texture-upscale" / "manifests"
                          / "upscale_results.json").read_text(encoding="utf-8"))
    # runtime captures carry what the engine uploads (offline meta, if any,
    # does not describe them) - only offline-path entries must pass the gate
    bad = [n for n in names
           if n in results and "source_fnv" in results[n]
           and "captured/" not in results[n].get("source_dds", "")
           and n in meta and not tp.is_true_mip0(meta[n])]
    check(not bad, f"no fake-mip targets deployed (bad: {bad[:5]})")


if __name__ == "__main__":
    test_mip0_gate()
    test_fnv()
    test_targets_consistent()
    if FAIL:
        print(f"{len(FAIL)} regression check(s) FAILED")
        sys.exit(1)
    print("PASS: texture pipeline regression tests")
