"""Offline weapon catalog index hints from Default__RvGameSystem (decompressed RvGame).

Does not launch the game. Catalog *runtime* layout is GGameSystem+0x160 (stride 0x50);
this tool recovers path order and filter hints from the default-object property blob.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "title"))
from ue3_layout import inspect

DEFAULT_EXPORT_NAME = "Default__RvGameSystem"
PATH_RE = re.compile(rb"wp_SP_[A-Za-z0-9_]+")
ORDINARY = (
    "wp_SP_ARPistol",
    "wp_SP_ARMachineGun",
    "wp_SP_ARShotgun",
    "wp_SP_ARSniperRifle",
)


def load_default_blob(rvgame_decompressed: Path) -> tuple[bytes, dict]:
    data = rvgame_decompressed.read_bytes()
    pkg = inspect("memory", data, include_all=True)
    idx = next(i for i, r in enumerate(pkg["records"]) if r["name"] == DEFAULT_EXPORT_NAME)
    rec = pkg["records"][idx]
    blob = data[rec["offset"] : rec["offset"] + rec["size"]]
    return blob, {"export_index": idx, "size": rec["size"], "offset": rec["offset"]}


def ordered_paths(blob: bytes) -> list[dict]:
    hits = [(m.start(), m.group().decode()) for m in PATH_RE.finditer(blob)]
    hits.sort(key=lambda x: x[0])
    out = []
    for catalog_index, (off, path) in enumerate(hits):
        out.append(
            {
                "catalog_index": catalog_index,
                "path": path,
                "blob_offset": hex(off),
                "native_mask": hex(1 << catalog_index),
            }
        )
    return out


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--rvgame",
        type=Path,
        default=Path(r"C:\Users\Trilex\vr-mods\research\native-hands-20261007\RvGame.decompressed.xxx"),
    )
    args = ap.parse_args()
    blob, meta = load_default_blob(args.rvgame)
    rows = ordered_paths(blob)
    ordinary = {r["path"]: r for r in rows if r["path"] in ORDINARY}
    report = {
        "schema": "singularityvr-gamesystem-catalog-offline-v1",
        "source": {
            "package": str(args.rvgame),
            "sha256": hashlib.sha256(args.rvgame.read_bytes()).hexdigest(),
            "default_object": DEFAULT_EXPORT_NAME,
            **meta,
        },
        "population_note": (
            "Runtime TArray at GGameSystem+0x160/+0x164 (stride 0x50) is hydrated from "
            "Default__RvGameSystem via UE3 default-property serialization (native init); "
            "not written by debug config import at 0xBF9860."
        ),
        "catalog_rows_by_blob_path_order": rows,
        "ordinary_weapons_inferred": ordinary,
        "order_evidence": (
            "Ascending wp_SP_* offsets in Default__RvGameSystem blob; matches FD5AC0 "
            "index i -> (1<<i) if blob order equals runtime TArray order (not independently "
            "bound to a named export in script package)."
        ),
        "vr_wheel_translation_example": {
            "note": "Do not assume bits 1/2/4/8; map from native_mask per weapon.",
            "ARPistol_native": ordinary.get("wp_SP_ARPistol"),
            "ARMachineGun_native": ordinary.get("wp_SP_ARMachineGun"),
            "ARShotgun_native": ordinary.get("wp_SP_ARShotgun"),
            "ARSniperRifle_native": ordinary.get("wp_SP_ARSniperRifle"),
        },
    }
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
