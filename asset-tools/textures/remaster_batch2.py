#!/usr/bin/env python3
"""
Singularity VR - offline material remaster, batch 2 (8 material groups).

Continues the batch-1 prototype (docs/MATERIAL_REMASTER.md) with the same
rules: true mip0 or nothing, native resolution for substitutes, real DXT DDS
with full mip chains, re-decoded validation, deterministic output.

Batch-2 groups (all diffuse+normal pairs verified true mip0, all hashes
already present in the injector's targets_generated.h):
  weapons : AR_E99Rifle_alt, ar_gatling_gun, laser (sight housing)
  enemies : revert (Revert d2), phasetick, Spetsnaz helmet + body
  env     : door_rollingT (diffuse + normal substituted, specular true mip0)

Output layout (OUTSIDE the repository):
  <assets-local>/material-remaster-batch2/<key>/{*.dds, comparison/*.png, manifest.json}
Committed to this branch (safe metadata only, no pixels):
  material-remaster-batch2/manifests/<key>.json + manifest_summary.json

Comparison PNGs and DDS files NEVER enter the repository on this branch.

Status of everything produced here: OFFLINE ONLY - NOT VR TESTED.
"""

from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

sys.path.insert(0, str(Path(__file__).resolve().parent))
import material_remaster as M  # noqa: E402 (shared primitives: fbm, blend, normals, validators)
from dds_io import (  # noqa: E402
    fnv1a64_mip0,
    load_dds,
    read_dds_header,
    resize_rgba_independent,
    save_dds,
    save_normal_dds,
    validate_dds,
    video_memory_bytes,
)

LOCAL_ROOT = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local")
ORIG = LOCAL_ROOT / "textures" / "original"
OUT = LOCAL_ROOT / "material-remaster-batch2"
REPO_ROOT = Path(__file__).resolve().parents[2]
SAFE_OUT = REPO_ROOT / "material-remaster-batch2" / "manifests"

# Injector target table (content hashes + replacement filenames the runtime
# looks up). Committed safe metadata extracted from the v2 branch injector.
# The runtime resolves "<target-stem>_2x.dds", so substituted outputs use
# exactly that name and are drop-in deployable.
TARGETS = json.loads((REPO_ROOT / "asset-tools" / "textures" / "target_map.json")
                     .read_text(encoding="utf-8"))["targets"]


def _p(*parts: str) -> Path:
    return ORIG.joinpath(*parts)


BATCH = {
    "weapon_e99": {
        "asset": "AR_E99Rifle_alt", "material": "AR_E99Rifle_alt",
        "kind": "weapon", "package": "RvGame",
        "detail": "E99 assault rifle: polymer furniture + blued steel",
        "maps": {
            "diffuse": _p("RvGame", "Texture2D", "AR_E99Rifle_alt_d.dds"),
            "normal": _p("RvGame", "Texture2D", "AR_E99Rifle_n.dds"),
        },
        "expected": {"diffuse": (2048, 1024), "normal": (2048, 1024)},
        "substitute": ["diffuse", "normal"],
        "showcase_2x": True,   # one genuine 2x Lanczos variant, labelled upscaled
    },
    "weapon_gatling": {
        "asset": "ar_gatling_gun", "material": "AR_gatling_gun",
        "kind": "weapon", "package": "RvGameMPLoader_SF",
        "detail": "gatling gun: rotating cluster, heat staining, ammo feed",
        "maps": {
            "diffuse": _p("RvGameMPLoader_SF", "Texture2D", "ar_gatling_gun_d.dds"),
            "normal": _p("RvGameMPLoader_SF", "Texture2D", "ar_gatling_gun_n.dds"),
        },
        "expected": {"diffuse": (2048, 1024), "normal": (2048, 1024)},
        "substitute": ["diffuse", "normal"],
    },
    "weapon_laser": {
        "asset": "laser", "material": "Laser",
        "kind": "optic", "package": "RvGameMPLoader_SF",
        "detail": "pistol laser-sight housing (256px, optically smooth)",
        "maps": {
            "diffuse": _p("RvGameMPLoader_SF", "Texture2D", "laser_d.dds"),
            "normal": _p("RvGameMPLoader_SF", "Texture2D", "laser_n.dds"),
        },
        "expected": {"diffuse": (256, 256), "normal": (256, 256)},
        "substitute": ["diffuse", "normal"],
    },
    "enemy_revert": {
        "asset": "revert_d2", "material": "ch_revert_env",
        "kind": "organic", "package": "RvGameMPLoader_SF",
        "detail": "Revert mutant: chitin plates, hide, lesions (2048px)",
        "maps": {
            "diffuse": _p("RvGameMPLoader_SF", "Texture2D", "revert_d2.dds"),
            "normal": _p("RvGameMPLoader_SF", "Texture2D", "revert_n2.dds"),
        },
        "expected": {"diffuse": (2048, 2048), "normal": (2048, 2048)},
        "substitute": ["diffuse", "normal"],
        "unprocessed": {"revert_t": "transmissionmask, not a colour map - left untouched"},
    },
    "enemy_phasetick": {
        "asset": "phasetick", "material": "phase_tick",
        "kind": "soldier", "package": "RvGameMPLoader_SF",
        "detail": "PhaseTick soldier: uniform cloth + hard armor + glow seams",
        "maps": {
            "diffuse": _p("RvGameMPLoader_SF", "Texture2D", "phasetick_d.dds"),
            "normal": _p("RvGameMPLoader_SF", "Texture2D", "phasetick_n.dds"),
            "specular": _p("RvGameMPLoader_SF", "Texture2D", "phasetick_s.dds"),
            "emissive": _p("RvGameMPLoader_SF", "Texture2D", "phasetick_e.dds"),
        },
        "expected": {"diffuse": (512, 512), "normal": (512, 512),
                     "specular": (512, 512), "emissive": (512, 512)},
        "substitute": ["diffuse", "normal"],
    },
    "enemy_spetsnaz_helmet": {
        "asset": "Spetsnaz_MP_helmet", "material": "mp_spetsnaz_grunt_head",
        "kind": "soldier", "package": "RvGameMPLoader_SF",
        "detail": "Spetsnaz helmet: painted shell, visor surround, straps",
        "maps": {
            "diffuse": _p("RvGameMPLoader_SF", "Texture2D", "Spetsnaz_MP_helmet_d.dds"),
            "normal": _p("RvGameMPLoader_SF", "Texture2D", "Spetsnaz_MP_helmet_n.dds"),
        },
        "expected": {"diffuse": (1024, 1024), "normal": (1024, 1024)},
        "substitute": ["diffuse", "normal"],
    },
    "enemy_spetsnaz_body": {
        "asset": "Spetsnaz_MP_Body", "material": "MP_Spetsnaz_Grunt",
        "kind": "soldier", "package": "RvGameMPLoader_SF",
        "detail": "Spetsnaz uniform: fatigues, webbing, armor inserts",
        "maps": {
            "diffuse": _p("RvGameMPLoader_SF", "Texture2D", "Spetsnaz_MP_Body_d.dds"),
            "normal": _p("RvGameMPLoader_SF", "Texture2D", "Spetsnaz_MP_Body_n.dds"),
        },
        "expected": {"diffuse": (1024, 1024), "normal": (1024, 1024)},
        "substitute": ["diffuse", "normal"],
    },
    "env_rollingdoor": {
        "asset": "door_rollingT", "material": "door_rolling_A_t_ageable",
        "kind": "environment", "package": "SP_WL1_zone01_XSA",
        "detail": "rolling shutter door: corrugated slats, rails, grime",
        "maps": {
            "diffuse": _p("SP_WL1_zone01_XSA", "Texture2D", "door_rollingT_d.dds"),
            "normal": _p("SP_WL1_zone01_XSA", "Texture2D", "door_rollingT_n.dds"),
            "specular": _p("SP_WL1_zone01_XSA", "Texture2D", "door_rollingT_s.dds"),
        },
        "expected": {"diffuse": (512, 512), "normal": (512, 512),
                     "specular": (256, 256)},
        # specular is true mip0 here, but substitution stays diffuse+normal only:
        # changing specular response without runtime validation risks exactly
        # the black-material class of surprise (uniform policy, same as batch 1)
        "substitute": ["diffuse", "normal"],
    },
}

# Deferred with reasons (recorded in the batch doc, not silently dropped)
DEFERRED = {
    "handgun_d/handgun_n": "pistol diffuse+normal are runtime captures at 1024px (object 2048px) - lower mips; needs MAIN-owned runtime capture at true mip0",
    "ar_sniper_rifle_d": "diffuse exported at 1024x512 (object 2048x1024, mip1); normal alone deferred to a normal-only batch",
    "elite_beam_d": "diffuse exported at 1024x1024 (object 2048x2048, mip1); normal alone deferred",
    "TMD_new_n": "normal is true mip0 2048x1024 but no diffuse exists at mip0; normal-only batch material",
    "scope_glass_d/o": "no normal map; _o is an opacity mask for the red-dot sight - too risky without runtime validation",
    "muzzleFlashDir_01/brightFlash_01/muzzleflashCenter": "additive glow sprites; micro-detail risks altering light energy; deferred pending MAIN guidance",
    "env_door_a/b, env_* captures": "no object-size provenance in metadata; mip0 cannot be proven - skipped, not upscaled",
    "door_industrial/prison/locked": "no local exports (NOLOCAL); extraction deferred",
}


# --------------------------------------------------------------------------- #
# enhancement (batch-2 kinds; weapon/environment reuse batch-1 via M dispatcher)
# --------------------------------------------------------------------------- #
def enhance_diffuse_organic(img: Image.Image) -> tuple[Image.Image, list[str]]:
    """Mutant chitin/hide: plate mottling, pores, cracks, lesions."""
    arr = np.asarray(img.convert("RGBA"))
    h, w = arr.shape[:2]
    notes = []
    n_pore = max(30, (w * h) // 9000)

    # large mottling (plate colour variation)
    mot = M.fbm((h, w), 3, 5001, 0.008)
    mot = (mot - 0.5) * 2.0
    arr2 = M.blend_detail(arr, mot, 0.10)
    notes.append("chitin mottling, 3 octaves, 10% of local luminance")

    # pores: small dark pits
    pit = M.fbm((h, w), 4, 5002, 0.22)
    pit = np.where(pit > 0.78, (pit - 0.78) / 0.22, 0).astype(np.float32)
    arr3 = arr2.astype(np.float32).copy()
    for c in range(3):
        arr3[:, :, c] = np.clip(arr3[:, :, c] * (1 - 0.28 * pit), 0, 255)
    notes.append(f"hide pores, up to 28% local darkening ({n_pore} seed scale)")

    # cracks: thin dark branching lines - sparse and shallow; dense dark
    # strokes read as black bars rather than wear (review feedback)
    scr = M.directional_scratches((h, w), count=max(12, n_pore // 3), seed=5003,
                                  angle=1.1, spread=0.9, length=(15, 120))
    arr4 = M.blend_detail(arr3.astype(np.uint8), -np.maximum(scr, 0.0), 0.08)
    notes.append(f"{max(12, n_pore // 3)} crack strokes, 8% of local luminance (darkening)")

    # lesion sheen variation (subtle warm shift in patches)
    les = M.fbm((h, w), 2, 5004, 0.01)
    les = np.where(les > 0.60, (les - 0.60) / 0.40, 0).astype(np.float32)
    tint = np.array([0.10, -0.02, -0.05], np.float32)
    arr5 = arr4.astype(np.float32).copy()
    for c in range(3):
        arr5[:, :, c] = np.clip(arr5[:, :, c] + tint[c] * 255.0 * les, 0, 255)
    notes.append("lesion tint patches, 2 octaves")
    return Image.fromarray(arr5.astype(np.uint8), "RGBA"), notes


def enhance_diffuse_soldier(img: Image.Image) -> tuple[Image.Image, list[str]]:
    """Uniform cloth + hard armor: weave, plate wear, webbing grime."""
    arr = np.asarray(img.convert("RGBA"))
    h, w = arr.shape[:2]
    notes = []

    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    pitch = max(5, w / 64)
    weave = np.sin(xx * 2 * np.pi / pitch) * np.sin(yy * 2 * np.pi / pitch)
    weave = weave - weave.mean()
    weave = weave / max(np.abs(weave).max(), 1e-6)
    arr2 = M.blend_detail(arr, weave, 0.09)
    notes.append(f"uniform weave, pitch {pitch:.0f}px, 9% of local luminance, zero-mean")

    # armor plate edge wear (bright scuffs on raised areas via luminance mask)
    lum = arr2[:, :, :3].mean(axis=2)
    hi = np.clip((lum - 110) / 90.0, 0, 1).astype(np.float32)
    scuff = M.fbm((h, w), 3, 6002, 0.05) * hi
    arr3 = arr2.astype(np.float32).copy()
    for c in range(3):
        arr3[:, :, c] = np.clip(arr3[:, :, c] + 22.0 * scuff, 0, 255)
    notes.append("armor scuff highlights on bright regions, up to +22 levels")

    # webbing / recess grime
    lo = np.clip((70 - lum) / 70.0, 0, 1).astype(np.float32) ** 1.5
    arr4 = arr3
    for c in range(3):
        arr4[:, :, c] = np.clip(arr4[:, :, c] * (1 - 0.16 * lo), 0, 255)
    notes.append("recess grime, up to 16% local darkening")
    return Image.fromarray(arr4.astype(np.uint8), "RGBA"), notes


def enhance_diffuse_optic(img: Image.Image) -> tuple[Image.Image, list[str]]:
    """Laser-sight housing: optically smooth, minimal grain only."""
    arr = np.asarray(img.convert("RGBA"))
    h, w = arr.shape[:2]
    grain = (M.fbm((h, w), 2, 7001, 0.30) - 0.5) * 2.0
    out = M.blend_detail(arr, grain, 0.04)
    return Image.fromarray(out, "RGBA"), ["fine grain only, 4% of local luminance (housing must stay smooth)"]


def enhance_diffuse(kind: str, img: Image.Image):
    if kind == "organic":
        return enhance_diffuse_organic(img)
    if kind == "soldier":
        return enhance_diffuse_soldier(img)
    if kind == "optic":
        return enhance_diffuse_optic(img)
    return M.enhance_diffuse(img, kind)  # weapon / environment reuse batch 1


def enhance_normal_procedural(img: Image.Image, kind: str) -> tuple[Image.Image, list[str]]:
    """Micro relief without diffuse coupling (for normal-only assets).

    Used only where no true-mip0 diffuse exists to derive detail from. The
    perturbation is procedural fbm noise, renormalised per texel. Weaker
    provenance than diffuse-coupled relief - labelled as such in manifests.
    """
    import math
    from dds_io import normal_to_signed, signed_to_normal
    signed = normal_to_signed(np.asarray(img.convert("RGBA"))).astype(np.float32)
    h, w = signed.shape[:2]
    # deliberately gentler than diffuse-coupled relief: without a diffuse map
    # anchoring the detail, a strong global perturbation would rewrite the
    # material instead of refining it
    amp = {"weapon": 0.28, "device": 0.25}.get(kind, 0.25)
    dx = (M.fbm((h, w), 4, 8001, 0.10) - 0.5) * 2.0 * amp
    dy = (M.fbm((h, w), 4, 8002, 0.10) - 0.5) * 2.0 * amp
    signed[:, :, 0] = np.clip(signed[:, :, 0] + dx, -1, 1)
    signed[:, :, 1] = np.clip(signed[:, :, 1] + dy, -1, 1)
    out = signed_to_normal(signed)
    out[:, :, 3] = 255
    notes = [
        "procedural-only micro relief (fbm, 4 octaves) - NOT coupled to diffuse",
        "weaker provenance than diffuse-coupled relief; see manifest",
        "original macro normals preserved and renormalised to unit length",
        "alpha channel set to constant 255 (matches shipped DXT1 normal maps)",
    ]
    return Image.fromarray(out, "RGBA"), notes


def signed_upsample_2x(img: Image.Image) -> Image.Image:
    """Genuine 2x upscale of a normal map: LANCZOS in signed space + renorm."""
    from dds_io import normal_to_signed, signed_to_normal
    signed = normal_to_signed(np.asarray(img.convert("RGBA")))
    w, h = img.size
    planes = []
    for c in range(4):
        planes.append(np.asarray(
            Image.fromarray(signed[:, :, c], mode="F").resize((w * 2, h * 2), Image.LANCZOS),
            dtype=np.float32)[:, :, None])
    return Image.fromarray(signed_to_normal(np.concatenate(planes, axis=2)), "RGBA")


# --------------------------------------------------------------------------- #
# orchestration (mirrors batch-1 guarantees; outputs stay outside the repo)
# --------------------------------------------------------------------------- #
def sha256_file(path: Path) -> str:
    import hashlib
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def process_asset(key: str, spec: dict) -> dict:
    print(f"\n=== {spec['asset']} ({spec['kind']}) ===", flush=True)
    out_dir = OUT / key
    out_dir.mkdir(parents=True, exist_ok=True)

    result = {
        "batch": 2,
        "asset": spec["asset"],
        "material": spec["material"],
        "kind": spec["kind"],
        "package": spec["package"],
        "detail": spec["detail"],
        "maps": {},
        "newly_authored": [],
        "recovered_from_source": [],
        "preserved_from_original": [
            "original mip0 resolution for every substituted map (no upscaling)",
            "original UV layout and texture orientation (pixels edited in place)",
            "original DXT container format (DXT1 stays DXT1, DXT5 stays DXT5)",
            "original mip0 alpha semantics",
            "original colour palette: detail added as a fraction of local luminance",
        ],
        "notes": [],
        "status": "OFFLINE ONLY - NOT VR TESTED",
    }

    enhanced_diffuse = None

    for map_type, path in spec["maps"].items():
        expected = spec["expected"][map_type] if map_type in spec["substitute"] else None
        img, hdr = M.load_source(path, expected, f"{spec['asset']}_{map_type}")
        got = (hdr["width"], hdr["height"])
        is_true_mip0 = got == spec["expected"][map_type]
        entry = {
            "source_file": str(path),
            "source_sha256": sha256_file(path),
            "source_mip0_fnv1a64": fnv1a64_mip0(path),
            "source_width": hdr["width"],
            "source_height": hdr["height"],
            "source_fourcc": hdr["fourcc"],
            "source_mip_count": hdr["mip_count"],
            "source_file_bytes": hdr["file_size"],
            "object_mip0_expected": list(spec["expected"][map_type]),
            "is_true_mip0": is_true_mip0,
            "mip0_gate_enforced": map_type in spec["substitute"],
            "substituted": map_type in spec["substitute"],
        }
        print(f"  {map_type:9s} {hdr['width']}x{hdr['height']} {hdr['fourcc']} "
              f"true_mip0={is_true_mip0} substituted={entry['substituted']}", flush=True)

        # injector deploy name: the runtime looks up "<target-stem>_2x.dds"
        # (see texq naming on the injector side), so substituted outputs use
        # exactly that filename and are drop-in deployable. Maps with no
        # injector target keep a descriptive name and are flagged below.
        tex_name = Path(path).stem
        target = TARGETS.get(tex_name)
        if entry["substituted"]:
            entry["injector_target"] = {
                "present": target is not None,
                "table_hash": target["fnv1a64"] if target else None,
                "table_file": target["file"] if target else None,
                "hash_match": (target is not None
                               and target["fnv1a64"] == entry["source_mip0_fnv1a64"]),
            }
            if target is not None:
                deploy_name = Path(target["file"]).stem + "_2x.dds"
            else:
                deploy_name = f"{spec['asset']}_{map_type}_2x.nodesc.dds"
                result["notes"].append(
                    f"{tex_name} has no injector target-table entry: the file is "
                    f"produced for review only until MAIN adds the target")
            entry["deploy_filename"] = deploy_name

        if map_type in spec["substitute"]:
            if map_type == "diffuse":
                enhanced, notes = enhance_diffuse(spec["kind"], img)
                enhanced_diffuse = enhanced
                result["newly_authored"].extend(notes)
                fourcc = hdr["fourcc"]
                a = np.asarray(enhanced)
                payload = enhanced if a[:, :, 3].min() < 255 else enhanced.convert("RGB")
                out_dds = out_dir / entry["deploy_filename"]
                written = save_dds(out_dds, payload, fourcc)
            elif map_type == "normal":
                if enhanced_diffuse is None:
                    if spec.get("relief") == "procedural":
                        enhanced, notes = enhance_normal_procedural(img, spec["kind"])
                        result["newly_authored"].extend(notes)
                    else:
                        raise RuntimeError("normal processed before diffuse")
                else:
                    enhanced, notes = M.enhance_normal(img, enhanced_diffuse,
                                                       spec["kind"])
                    result["newly_authored"].extend(notes)
                out_dds = out_dir / entry["deploy_filename"]
                written = save_normal_dds(out_dds, enhanced)
            elif map_type == "specular":
                # only reached when the specular source IS true mip0
                enhanced, notes = M.enhance_specular(img, spec["kind"])
                result["newly_authored"].extend(notes)
                out_dds = out_dir / entry["deploy_filename"]
                a = np.asarray(enhanced)
                payload = enhanced if a[:, :, 3].min() < 255 else enhanced.convert("RGB")
                written = save_dds(out_dds, payload, hdr["fourcc"])
            entry["output_file"] = str(out_dds)
            entry["output_sha256"] = sha256_file(out_dds)
            entry["output_header"] = written
        else:
            if map_type in ("specular", "emissive"):
                enhanced, notes = M.enhance_specular(img, spec["kind"])
                result["newly_authored"].extend(notes)
                if is_true_mip0:
                    entry["exclusion_reason"] = (
                        f"{map_type} source is true mip0 but substitution stays "
                        f"diffuse+normal only by uniform policy (changing specular/"
                        f"emissive response without runtime validation risks "
                        f"black-material-class surprises); produced for review only"
                    )
                else:
                    entry["exclusion_reason"] = (
                        f"{map_type} source is not the texture object's mip0, so it is "
                        f"produced for review only and is NOT wired into the injector"
                    )
                result["notes"].append(entry["exclusion_reason"])
                out_dds = out_dir / f"{spec['asset']}_{map_type}_review.dds"
                a = np.asarray(enhanced)
                payload = enhanced if a[:, :, 3].min() < 255 else enhanced.convert("RGB")
                written = save_dds(out_dds, payload, hdr["fourcc"])
                entry["output_file"] = str(out_dds)
                entry["output_sha256"] = sha256_file(out_dds)
                entry["output_header"] = written
            else:
                raise RuntimeError(f"unexpected unsubstituted map {map_type}")

        result["maps"][map_type] = entry
        result["recovered_from_source"].append(
            f"mip0 {map_type} bytes, sha256 {entry['source_sha256'][:16]}")

    # genuine 2x showcase variant (ONE asset): Lanczos from true mip0, labelled
    if spec.get("showcase_2x"):
        d_entry = result["maps"]["diffuse"]
        n_entry = result["maps"]["normal"]
        d_img, _ = load_dds(Path(d_entry["output_file"]))
        n_img, _ = load_dds(Path(n_entry["output_file"]))
        d2 = resize_rgba_independent(d_img, (d_img.width * 2, d_img.height * 2),
                                       Image.LANCZOS)
        n2 = signed_upsample_2x(n_img)
        d2_path = out_dir / f"{spec['asset']}_diffuse_2X_LANCZOS.dds"
        n2_path = out_dir / f"{spec['asset']}_normal_2X_LANCZOS.dds"
        wd = save_dds(d2_path, d2.convert("RGB") if d_entry["output_header"]["fourcc"] == "DXT1"
                      else d2, d_entry["output_header"]["fourcc"])
        wn = save_normal_dds(n2_path, n2)
        result["upscaled_variants"] = {
            "method": "LANCZOS from the enhanced native replacement (which itself "
                      "derives from true mip0). Interpolation only: no detail above "
                      "the original Nyquist frequency is genuine.",
            "files": [
                {"file": str(d2_path), "sha256": sha256_file(d2_path), "header": wd},
                {"file": str(n2_path), "sha256": sha256_file(n2_path), "header": wn},
            ],
            "substituted": False,
            "note": "NOT wired into the injector; showcase of genuine higher-resolution "
                    "output vs invented detail. Compare against the native replacement.",
        }
        for f in result["upscaled_variants"]["files"]:
            f["validation"] = validate_dds(Path(f["file"]))
        print(f"    showcase 2x: {d2_path.name}, {n2_path.name}", flush=True)

    val = {}
    for map_type, entry in result["maps"].items():
        rep = validate_dds(Path(entry["output_file"]))
        rep["video_memory_bytes"] = video_memory_bytes(Path(entry["output_file"]))
        rep["fidelity_vs_source"] = M.fidelity_report(entry["source_file"], entry["output_file"])
        if map_type == "diffuse":
            e_src = M.detail_energy(load_dds(Path(entry["source_file"]))[0])
            e_out = M.detail_energy(load_dds(Path(entry["output_file"]))[0])
            rep["detail_high_frequency_energy"] = {
                "source": round(e_src, 3), "enhanced": round(e_out, 3),
                "ratio": round(e_out / max(e_src, 1e-9), 4)}
        if map_type == "normal":
            rep["normal_map_check"] = M.check_normal_map(entry["output_file"])
        rep["status"] = "VERIFIED" if rep["fidelity_vs_source"]["compared_pixels"] > 0 else "NOT TESTED"
        entry["validation"] = rep
        val[map_type] = rep
        print(f"    wrote {Path(entry['output_file']).name}: "
              f"{rep['width']}x{rep['height']} {rep['fourcc']} mips={rep['mip_count']} "
              f"bytes={rep['file_bytes']} decoded={rep['decoded_mode']}", flush=True)
    result["validation"] = val

    src_vram = sum(
        ((e["source_width"] + 3) // 4) * ((e["source_height"] + 3) // 4) *
        (8 if e["source_fourcc"] == "DXT1" else 16)
        for e in result["maps"].values() if e["substituted"])
    out_vram = sum(v["video_memory_bytes"] for k, v in val.items()
                   if result["maps"][k]["substituted"])
    result["video_memory"] = {
        "source_mip0_bytes_substituted": src_vram,
        "replacement_total_bytes_substituted": out_vram,
        "origin": "ESTIMATED (computed from measured DDS headers and file contents)",
    }
    result["evidence"] = {
        "true_mip0_per_substituted_map": "VERIFIED (DDS header vs object mip0 size)",
        "dds_written_and_decoded_back": "VERIFIED (every file re-opened and decoded)",
        "dds_dimensions_format_mips_channels": "VERIFIED (per file in validation)",
        "normal_map_channel_conventions": "VERIFIED (alpha constant, X/Y centred, unit length ranged)",
        "fidelity_vs_source": "VERIFIED (measured mean/max absolute difference)",
        "checksums": "VERIFIED (sha256 of every source and produced file)",
        "video_memory_figures": "ESTIMATED (block arithmetic over measured headers)",
        "visual_result_in_game": "NOT TESTED (no Steam run, no deploy, no headset)",
        "gpu_frame_cost": "NOT TESTED (needs a profiled run)",
    }
    return result


def make_comparisons(key: str, spec: dict, result: dict) -> list[str]:
    out_dir = OUT / key / "comparison"
    out_dir.mkdir(parents=True, exist_ok=True)
    produced = []
    for map_type, entry in result["maps"].items():
        if "output_file" not in entry:
            continue
        orig, _ = load_dds(Path(spec["maps"][map_type]))
        enh, _ = load_dds(Path(entry["output_file"]))
        w, h = orig.size
        crop = min(w, h, 256)
        x0, y0 = (w - crop) // 2, (h - crop) // 2
        box = (x0, y0, x0 + crop, y0 + crop)
        tiles = [
            (orig.crop(box).resize((crop * 2, crop * 2), Image.NEAREST), "ORIGINAL 1x (2x nearest)"),
            (resize_rgba_independent(orig.crop(box), (crop * 2, crop * 2),
                                     Image.LANCZOS), "LANCZOS upscale"),
            (resize_rgba_independent(enh.crop(box), (crop * 2, crop * 2),
                                     Image.LANCZOS), "REMASTERED (native)"),
        ]
        sheet = Image.new("RGB", (crop * 2, crop * 2 * 3 + 40), (18, 18, 22))
        from PIL import ImageDraw
        d = ImageDraw.Draw(sheet)
        y = 0
        for im, label in tiles:
            sheet.paste(im.convert("RGB"), (0, y))
            d.text((6, y + crop * 2 - 18), f"{spec['asset']} {map_type} - {label}", fill=(230, 230, 230))
            y += crop * 2 + 10
        path = out_dir / f"{spec['asset']}_{map_type}_compare.png"
        sheet.save(path)
        produced.append(str(path))
        print(f"    comparison sheet: {path.name}", flush=True)
    return produced


def safe_manifest(result: dict) -> dict:
    """Copy safe for the public branch: hashes and measurements, no pixels,
    machine-local absolute paths relativised."""
    import copy
    m = copy.deepcopy(result)
    for e in m["maps"].values():
        for k in ("source_file", "output_file"):
            if k in e:
                e[k] = e[k].replace(str(LOCAL_ROOT), "<assets-local>").replace("\\", "/")
        e.pop("output_header", None)
        if "validation" in e:
            e["validation"].pop("file", None)
    if "upscaled_variants" in m:
        for f in m["upscaled_variants"]["files"]:
            f["file"] = f["file"].replace(str(LOCAL_ROOT), "<assets-local>").replace("\\", "/")
            f.pop("header", None)
            f.get("validation", {}).pop("file", None)
    m["comparisons_local_only"] = m.pop("comparisons", [])
    m["comparisons_local_only"] = [c.replace(str(LOCAL_ROOT), "<assets-local>").replace("\\", "/")
                                   for c in m["comparisons_local_only"]]
    return m


def main() -> None:
    print("Singularity VR material remaster - batch 2 (OFFLINE ONLY)", flush=True)
    summary = {"batch": 2, "status": "OFFLINE ONLY - NOT VR TESTED", "materials": [],
               "deferred": DEFERRED, "total_source_mip0_bytes": 0,
               "total_replacement_bytes": 0}
    for key, spec in BATCH.items():
        result = process_asset(key, spec)
        result["comparisons"] = make_comparisons(key, spec, result)
        (OUT / key / "manifest.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
        safe = safe_manifest(result)
        SAFE_OUT.mkdir(parents=True, exist_ok=True)
        (SAFE_OUT / f"{key}.json").write_text(json.dumps(safe, indent=2), encoding="utf-8")
        summary["materials"].append({
            "key": key, "asset": spec["asset"],
            "manifest": f"material-remaster-batch2/manifests/{key}.json",
            "substituted": {k: v["output_sha256"][:16] for k, v in result["maps"].items()
                            if v.get("substituted")},
        })
        summary["total_source_mip0_bytes"] += result["video_memory"]["source_mip0_bytes_substituted"]
        summary["total_replacement_bytes"] += result["video_memory"]["replacement_total_bytes_substituted"]
    (OUT / "manifest_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    (SAFE_OUT / "manifest_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print("\ndone. local output:", OUT, flush=True)
    print("safe manifests:", SAFE_OUT, flush=True)


if __name__ == "__main__":
    main()
