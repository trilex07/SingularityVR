#!/usr/bin/env python3
"""
Singularity VR - offline material remaster, batch 3 (normal-only assets).

Batch 2 exhausted the true-mip0 diffuse+normal pairs in the local inventory.
What remains with proven mip0 and local sources are three normal maps whose
diffuses only exist as lower mips:

  weapon_sniper_normal : ar_sniper_rifle_n (2048x1024, AR_SniperRifle)
  weapon_railgun_normal: elite_beam_n      (2048x2048, AR_Railgun_Inview)
  device_tmd_normal    : TMD_new_n         (2048x1024, tmd_base_optimized)

Because no true-mip0 diffuse exists to derive relief from, micro relief here is
procedural-only (fbm perturbation + renormalisation) via
remaster_batch2.enhance_normal_procedural - weaker provenance than
diffuse-coupled relief, labelled as such in every manifest. Diffuse maps are
NOT processed and NOT substituted; the manifests record why.

Output layout (OUTSIDE the repository):
  <assets-local>/material-remaster-batch3/<key>/{*.dds, comparison/*.png, manifest.json}
Committed to this branch (safe metadata only, no pixels):
  material-remaster-batch3/manifests/<key>.json + manifest_summary.json

Status of everything produced here: OFFLINE ONLY - NOT VR TESTED.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

LOCAL_ROOT = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local")
ORIG = LOCAL_ROOT / "textures" / "original"
OUT = LOCAL_ROOT / "material-remaster-batch3"
REPO_ROOT = Path(__file__).resolve().parents[2]
SAFE_OUT = REPO_ROOT / "material-remaster-batch3" / "manifests"

sys.path.insert(0, str(Path(__file__).resolve().parent))
import remaster_batch2 as B2  # noqa: E402 (shared orchestration + procedural relief)


def _p(*parts: str) -> Path:
    return ORIG.joinpath(*parts)


BATCH = {
    "weapon_sniper_normal": {
        "asset": "ar_sniper_rifle_n", "material": "AR_SniperRifle",
        "kind": "weapon", "package": "RvGameMPLoader_SF",
        "detail": "sniper rifle normal relief only (diffuse is a lower mip)",
        "relief": "procedural",
        "maps": {
            "normal": _p("RvGameMPLoader_SF", "Texture2D", "ar_sniper_rifle_n.dds"),
        },
        "expected": {"normal": (2048, 1024)},
        "substitute": ["normal"],
        "unprocessed": {
            "ar_sniper_rifle_d": "diffuse exported at 1024x512 (object 2048x1024, mip1) - not processed, not substituted"
        },
    },
    "weapon_railgun_normal": {
        "asset": "elite_beam_n", "material": "AR_Railgun_Inview",
        "kind": "weapon", "package": "RvGameMPLoader_SF",
        "detail": "railgun normal relief only (diffuse is a lower mip)",
        "relief": "procedural",
        "maps": {
            "normal": _p("RvGameMPLoader_SF", "Texture2D", "elite_beam_n.dds"),
        },
        "expected": {"normal": (2048, 2048)},
        "substitute": ["normal"],
        "unprocessed": {
            "elite_beam_d": "diffuse exported at 1024x1024 (object 2048x2048, mip1) - not processed, not substituted"
        },
    },
    "device_tmd_normal": {
        "asset": "TMD_new_n", "material": "tmd_base_optimized",
        "kind": "device", "package": "RvGame",
        "detail": "TMD wrist-device normal relief only (no diffuse at mip0)",
        "relief": "procedural",
        "maps": {
            "normal": _p("RvGame", "Texture2D", "TMD_new_n.dds"),
        },
        "expected": {"normal": (2048, 1024)},
        "substitute": ["normal"],
        "unprocessed": {
            "TMD_new2_d": "diffuse exported at 1024x512 (object 2048x1024, mip1), no local export - not processed, not substituted"
        },
    },
}

DEFERRED_ADDENDUM = {
    "ar_sniper_rifle_d / elite_beam_d / TMD diffuses": "covered in batch 3 as unprocessed lower mips; normals done procedurally",
    "remaining inventory": "masks, emissives, FX sprites, unprovenanced captures - no further true-mip0 colour data in the local inventory",
}


def main() -> None:
    print("Singularity VR material remaster - batch 3 normal-only (OFFLINE ONLY)", flush=True)
    B2.OUT = OUT  # redirect shared orchestration to the batch-3 local dir
    summary = {"batch": 3, "status": "OFFLINE ONLY - NOT VR TESTED", "materials": [],
               "deferred": DEFERRED_ADDENDUM, "total_source_mip0_bytes": 0,
               "total_replacement_bytes": 0}
    for key, spec in BATCH.items():
        result = B2.process_asset(key, spec)
        result["batch"] = 3
        result["unprocessed"] = spec.get("unprocessed", {})
        result["comparisons"] = B2.make_comparisons(key, spec, result)
        (OUT / key / "manifest.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
        safe = B2.safe_manifest(result)
        SAFE_OUT.mkdir(parents=True, exist_ok=True)
        (SAFE_OUT / f"{key}.json").write_text(json.dumps(safe, indent=2), encoding="utf-8")
        summary["materials"].append({
            "key": key, "asset": spec["asset"],
            "manifest": f"material-remaster-batch3/manifests/{key}.json",
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
