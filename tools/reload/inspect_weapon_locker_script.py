"""WeaponLocker / BarisovStation script + export inventory witness (offline)."""
from __future__ import annotations

import argparse
import json
from pathlib import Path

from dump_ue3_script_bytecode import dump_package
from inspect_use_station_methods import inspect as inspect_natives


def weapon_info_exports(export_text: str) -> dict:
    lines = export_text.splitlines()
    fields = []
    for line in lines:
        if "WeaponInfo" not in line and "outer WeaponInfo" not in line:
            continue
        if "ScriptStruct WeaponInfo" in line:
            fields.append({"kind": "struct", "line": line.strip()})
        if "Property" in line and any(k in line for k in ("mbIsHeavyWeapon", "mName", "mPath", "mType", "Mid")):
            fields.append({"kind": "field", "line": line.strip()})
    return {
        "struct_size_export": "0x190",
        "field_exports": fields,
        "offset_note": (
            "UProperty Offset dwords are 0 in package serial; do not infer layout from declaration order alone."
        ),
    }


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--export-inventory",
        type=Path,
        default=Path(r"C:\Users\Trilex\vr-mods\research\etw\controller-features-20261006\rvgame-assets.txt"),
    )
    ap.add_argument(
        "--exe",
        type=Path,
        default=Path(r"C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\Singularity.exe"),
    )
    ap.add_argument(
        "--text",
        type=Path,
        default=Path(r"C:\Users\Trilex\vr-mods\research\etw\capture-20261006-205508\Singularity.text-memory.bin"),
    )
    ap.add_argument(
        "--rvgame",
        type=Path,
        default=Path(r"C:\Users\Trilex\vr-mods\research\native-hands-20261007\RvGame.decompressed.xxx"),
    )
    args = ap.parse_args()
    export_text = args.export_inventory.read_text(encoding="utf-8", errors="replace")
    natives = inspect_natives(args.exe.read_bytes(), args.text.read_bytes())
    script = dump_package(
        args.rvgame,
        [
            ("RvUIGFxMovie_BarisovStation", "GetItemAt"),
            ("RvUIGFxMovie_BarisovStation", "TransferData"),
            ("RvUIGFxMovie_WeaponLocker", "Initialize"),
            ("RvUIGFxMovie_WeaponLocker", "LoadEquippedWeapons"),
            ("RvUIGFxMovie_WeaponLocker", "equip"),
        ],
    )
    out = {
        "schema": "singularityvr-weapon-locker-script-v1",
        "weapon_info": weapon_info_exports(export_text),
        "native_witness": natives,
        "script_bytecode": script,
        "class_export_matrix": {
            "RvUIGFxMovie_BarisovStation": ["GetItemAt", "TransferData", "Initialize", "equip"],
            "RvUIGFxMovie_WeaponLocker": [
                "Start",
                "Initialize",
                "PopMenu",
                "LoadUpgrades",
                "LoadEquippedWeapons",
                "equip",
            ],
            "RvUIGFxMovie_UseStation": ["PopMenu"],
        },
    }
    print(json.dumps(out, indent=2))


if __name__ == "__main__":
    main()
