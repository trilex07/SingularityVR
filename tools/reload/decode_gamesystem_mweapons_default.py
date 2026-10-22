"""Decode Default__RvGameSystem.mWeapons default-property array (offline row order).

Runtime hydration: GGameSystem+0x160/+0x164, stride 0x50 in memory (see FD5AC0).
Serialized default rows are ~420 bytes each — do NOT apply runtime +0x24/+0x4C offsets
to serialized bytes without a proven mapper.
"""
from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "title"))
from ue3_layout import inspect  # noqa: E402

ORDINARY = (
    "wp_SP_ARPistol",
    "wp_SP_ARMachineGun",
    "wp_SP_ARShotgun",
    "wp_SP_ARSniperRifle",
)


def load_mweapons_array(rvgame: Path) -> tuple[bytes, dict]:
    data = rvgame.read_bytes()
    pkg = inspect("memory", data, include_all=True)
    names = pkg["names"]
    rec = next(r for r in pkg["records"] if r["name"] == "Default__RvGameSystem")

    def name(n: int) -> str:
        return names[n]

    p = rec["offset"] + 4
    arr = b""
    while True:
        ni, _ = struct.unpack_from("<ii", data, p)
        p += 8
        nm = name(ni)
        if nm == "None":
            break
        ti, _tn, size, _idx = struct.unpack_from("<4i", data, p)
        p += 16
        typ = name(ti)
        if typ == "BoolProperty":
            p += 4
        elif typ == "StructProperty":
            p += 8
        val = data[p : p + size]
        p += size
        if nm == "mWeapons":
            arr = val
            break
    meta = {"export": rec["name"], "offset": rec["offset"], "size": rec["size"]}
    return arr, meta


def decode_rows(arr: bytes) -> list[dict]:
    if len(arr) < 4:
        return []
    count = struct.unpack_from("<i", arr, 0)[0]
    body = arr[4:]
    path_hits = [(m.start(), m.group().decode()) for m in re.finditer(rb"wp_SP_[A-Za-z0-9_]+", body)]
    path_by_index: dict[int, str] = {}
    if path_hits:
        # Primary wp_SP_* strings appear in catalog index order at +0x6A within each serialized row.
        primary_offset_in_row = 0x6A
        row0 = path_hits[0][0] - primary_offset_in_row
        if len(path_hits) >= 2:
            ser_stride = round(
                sum(path_hits[i + 1][0] - path_hits[i][0] for i in range(len(path_hits) - 1))
                / (len(path_hits) - 1)
            )
        else:
            ser_stride = 420
        for off, path in path_hits:
            idx = (off - row0) // ser_stride if ser_stride else 0
            if 0 <= idx < count:
                path_by_index.setdefault(idx, path)
    rows: list[dict] = []
    for i in range(count):
        rows.append(
            {
                "runtime_catalog_index": i,
                "native_mask": hex(1 << i),
                "path": path_by_index.get(i),
                "filter24": None,
                "filter4c": None,
                "note": "filter24/4C only defined on hydrated runtime 0x50 rows (FD5AC0), not serialized blob.",
            }
        )
    return rows


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--rvgame",
        type=Path,
        default=Path(r"C:\Users\Trilex\vr-mods\research\native-hands-20261007\RvGame.decompressed.xxx"),
    )
    args = ap.parse_args()
    arr, meta = load_mweapons_array(args.rvgame)
    rows = decode_rows(arr)
    ordinary = {r["path"]: r for r in rows if r["path"] in ORDINARY}
    report = {
        "schema": "singularityvr-mweapons-default-v1",
        "source": {"package": str(args.rvgame), **meta},
        "default_array_count": struct.unpack_from("<i", arr, 0)[0] if len(arr) >= 4 else 0,
        "psvr2_live_count_observed": 17,
        "binding_hypothesis": (
            "Default__RvGameSystem.mWeapons count/order matches live GGameSystem+0x164 "
            "when hydrated at boot; index i -> native bit (1<<i) per FD5AC0."
        ),
        "rows": rows,
        "ordinary_weapons": ordinary,
        "ordinary_index_check": {
            "wp_SP_ARPistol": {"expected_index": 0, "offline": ordinary.get("wp_SP_ARPistol")},
            "wp_SP_ARMachineGun": {"expected_index": 1, "offline": ordinary.get("wp_SP_ARMachineGun")},
            "wp_SP_ARShotgun": {"expected_index": 2, "offline": ordinary.get("wp_SP_ARShotgun")},
            "wp_SP_ARSniperRifle": {"expected_index": 5, "offline": ordinary.get("wp_SP_ARSniperRifle")},
        },
    }
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
