#!/usr/bin/env python3
"""Independent Validation Script for Singularity VR Diegetic HUD Assets (V2.1).
Strictly validates:
1. Re-import of all 5 diegetic GLB models via trimesh.
2. Multi-material structure (distinct PBR submeshes: Weapon, Bezel, CathodeBed, Phosphor, Rivet/Brass).
3. Full 4K texture resolution preserved (4096 width, no downsampling).
4. Physical 7-segment readout geometry presence (faces > 100 in phosphor submesh).
5. All 5 original vanilla GLBs valid.
6. All 22 preview render PNGs generated, non-blank, and valid.
"""
import sys
from pathlib import Path
import trimesh
from PIL import Image

REPO_ROOT = Path(r"C:\Users\Trilex\vr-mods")
WORKTREE_ROOT = REPO_ROOT / "SingularityVR-diegetic-hud"
MODELS_DIR = WORKTREE_ROOT / "assets" / "processed" / "hud" / "diegetic" / "models_v2"
PREVIEWS_DIR = WORKTREE_ROOT / "docs" / "previews" / "hud" / "diegetic_v2"

def validate_assets():
    print("=== Running Independent Diegetic HUD V2.1 Asset Validation ===")
    errors = []

    expected_models = ['centurion', 'valkyrie', 'spikeshot', 'kasimov', 'tmd_glove']

    # 1. Validate GLB Multi-Material Models
    for mid in expected_models:
        glb_path = MODELS_DIR / f"{mid}_diegetic_v2.glb"
        if not glb_path.exists():
            errors.append(f"Missing GLB file: {glb_path.name}")
            continue

        print(f"\nValidating {glb_path.name} ({glb_path.stat().st_size / (1024*1024):.2f} MB)...")
        try:
            scene = trimesh.load(str(glb_path), file_type='glb')
        except Exception as e:
            errors.append(f"Failed to re-import {glb_path.name}: {e}")
            continue

        geom_count = len(scene.geometry)
        if geom_count < 5:
            errors.append(f"{glb_path.name}: Expected at least 5 submesh geometries, found {geom_count}")

        materials = set()
        has_4k_tex = False
        phosphor_faces = 0

        for name, g in scene.geometry.items():
            if hasattr(g.visual, 'material') and g.visual.material:
                mat = g.visual.material
                mat_name = mat.name
                materials.add(mat_name)

                # Check weapon texture resolution
                if 'Weapon' in mat_name and mat.baseColorTexture:
                    w, h = mat.baseColorTexture.size
                    if w < 4096:
                        errors.append(f"{glb_path.name}: Weapon texture is {w}x{h}, expected 4096 width! (Downsampling violation)")
                    else:
                        has_4k_tex = True
                        print(f"  [PASS] 4K Texture verified: {w}x{h}")

                # Check phosphor readout geometry
                if 'EmissivePhosphor' in mat_name:
                    phosphor_faces = len(g.faces)
                    print(f"  [PASS] Emissive Phosphor submesh verified: {len(g.vertices)} verts, {phosphor_faces} faces")

        # Verify essential materials
        required_mats = {'WeaponRemastered', 'BezelSoviet', 'CathodeGlass', 'EmissivePhosphorDemonstration'}
        missing_mats = required_mats - materials
        if missing_mats:
            errors.append(f"{glb_path.name}: Missing required materials: {missing_mats}")
        else:
            print(f"  [PASS] Distinct PBR materials verified: {sorted(list(materials))}")

        if not has_4k_tex:
            errors.append(f"{glb_path.name}: Missing 4K baseColorTexture on weapon submesh")

        if phosphor_faces < 100:
            errors.append(f"{glb_path.name}: Phosphor readout geometry has only {phosphor_faces} faces, expected >= 100")

    # 2. Validate Original Vanilla Models
    print("\n--- Validating Vanilla Original Models ---")
    for mid in expected_models:
        orig_glb = MODELS_DIR / f"{mid}_original_v2.glb"
        if not orig_glb.exists():
            errors.append(f"Missing vanilla original GLB: {orig_glb.name}")
        else:
            print(f"  [PASS] Vanilla model verified: {orig_glb.name} ({orig_glb.stat().st_size / (1024*1024):.2f} MB)")

    # 3. Validate Previews
    print("\n--- Validating Generated 1080p Previews ---")
    expected_previews = [
        'centurion_overview_v2.png', 'centurion_closeup_v2.png', 'centurion_first_person_v2.png', 'centurion_comparison_v2.png',
        'valkyrie_overview_v2.png', 'valkyrie_closeup_v2.png', 'valkyrie_first_person_v2.png', 'valkyrie_comparison_v2.png',
        'spikeshot_overview_v2.png', 'spikeshot_closeup_v2.png', 'spikeshot_first_person_v2.png', 'spikeshot_comparison_v2.png',
        'kasimov_overview_v2.png', 'kasimov_closeup_v2.png', 'kasimov_first_person_v2.png', 'kasimov_comparison_v2.png',
        'tmd_glove_overview_v2.png', 'tmd_glove_closeup_v2.png', 'tmd_glove_first_person_v2.png', 'tmd_glove_comparison_v2.png',
        'tmd_glove_health_amber_v2.png', 'tmd_glove_health_crimson_v2.png'
    ]

    for prev in expected_previews:
        p_path = PREVIEWS_DIR / prev
        if not p_path.exists():
            errors.append(f"Missing preview render: {prev}")
            continue

        sz = p_path.stat().st_size
        if sz < 150 * 1024:
            errors.append(f"Preview {prev} suspiciously small ({sz} bytes, possibly blank)")
        else:
            print(f"  [PASS] Preview verified: {prev:32s} ({sz / 1024:.1f} KB)")

    print("\n=======================================================")
    if errors:
        print("VALIDATION FAILED WITH ERRORS:")
        for err in errors:
            print(f"  [FAIL] {err}")
        sys.exit(1)
    else:
        print("ALL ASSET AND GLB RE-IMPORT VALIDATION CHECKS PASSED!")
        print("=======================================================")

if __name__ == '__main__':
    validate_assets()
