"""Dump UFunction script tails from decompressed RvGame (WeaponLocker / Barisov list API)."""
from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "title"))
from ue3_layout import inspect  # noqa: E402

# UE3 expression tags (partial, license-safe labels)
EXPR = {
    0x00: "EX_LocalVariable",
    0x01: "EX_InstanceVariable",
    0x04: "EX_DefaultVariable",
    0x05: "EX_Return",
    0x07: "EX_Jump",
    0x0B: "EX_Skip",
    0x16: "EX_PrimitiveCast",
    0x36: "EX_FinalFunction",
    0x60: "EX_Native",
    0x70: "EX_NativeAlt",
}


def ref_name(d: dict, idx: int) -> str:
    if idx == 0:
        return "None"
    if idx < 0:
        imp = d["imports"][-idx - 1]
        return d["names"][imp[5]]
    return d["records"][idx - 1]["name"]


def ref_class(d: dict, idx: int) -> str:
    if idx == 0:
        return "None"
    if idx < 0:
        imp = d["imports"][-idx - 1]
        return d["names"][imp[2]]
    rec = d["records"][idx - 1]
    return ref_name(d, rec["cls"])


def skip_properties(data: bytes, p: int, names: list[str]) -> int:
    while p + 8 <= len(data):
        ni, _num = struct.unpack_from("<ii", data, p)
        if ni == 0:
            return p + 8
        p += 8
        ti, _tn, size, _idx = struct.unpack_from("<4i", data, p)
        p += 16
        typ = names[ti]
        if typ == "BoolProperty":
            p += 4
        elif typ == "StructProperty":
            p += 8
        p += size
    return p


def parse_function_tail(data: bytes, d: dict) -> dict:
    names = d["names"]
    p = skip_properties(data, 4, names)
    rest = data[p:]
    if len(rest) < 8:
        return {"parse_error": "tail too short", "rest_hex": rest.hex()}
    flags, script_size = struct.unpack_from("<II", rest, 0)
    script = rest[8 : 8 + script_size]
    return {
        "function_flags": hex(flags),
        "script_size": script_size,
        "script_hex": script.hex(),
        "script_ops": decode_script(script, d),
        "tail_hex": rest.hex(),
    }


def decode_script(script: bytes, d: dict) -> list[str]:
    out: list[str] = []
    i = 0
    while i < len(script):
        op = script[i]
        label = EXPR.get(op, f"EX_{op:#x}")
        i += 1
        if op in (0x60, 0x70) and i + 2 <= len(script):
            native = struct.unpack_from("<H", script, i)[0]
            out.append(f"{label} iNative={native:#x}")
            i += 2
        elif op in (0x00, 0x01, 0x04) and i + 4 <= len(script):
            prop = struct.unpack_from("<i", script, i)[0]
            try:
                pname = ref_name(d, prop)
            except (IndexError, AssertionError):
                pname = f"idx={prop}"
            out.append(f"{label} prop={pname}")
            i += 4
        else:
            out.append(label)
            if i >= len(script):
                break
    return out


def dump_package(path: Path, targets: list[tuple[str, str]]) -> dict:
    data = path.read_bytes()
    d = inspect(str(path), data, include_all=True)
    names = d["names"]
    by_key: dict[tuple[str, str], dict] = {}
    for rec in d["records"]:
        outer = ref_name(d, rec["outer"])
        for want_outer, want_name in targets:
            if rec["name"] == want_name and outer == want_outer:
                blob = data[rec["offset"] : rec["offset"] + rec["size"]]
                by_key[(want_outer, want_name)] = {
                    "export_index": d["records"].index(rec) + 1,
                    "size": rec["size"],
                    **parse_function_tail(blob, d),
                }
    missing = [f"{o}::{n}" for o, n in targets if (o, n) not in by_key]
    return {
        "schema": "singularityvr-ue3-script-bytecode-v1",
        "package": str(path),
        "functions": {f"{o}::{n}": by_key[(o, n)] for o, n in targets if (o, n) in by_key},
        "missing": missing,
    }


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--rvgame",
        type=Path,
        default=Path(r"C:\Users\Trilex\vr-mods\research\native-hands-20261007\RvGame.decompressed.xxx"),
    )
    args = ap.parse_args()
    targets = [
        ("RvUIGFxMovie_BarisovStation", "GetItemAt"),
        ("RvUIGFxMovie_BarisovStation", "TransferData"),
        ("RvUIGFxMovie_WeaponLocker", "Initialize"),
        ("RvUIGFxMovie_WeaponLocker", "LoadEquippedWeapons"),
        ("RvUIGFxMovie_WeaponLocker", "equip"),
        ("RvUIGFxMovie_WeaponLocker", "LoadUpgrades"),
    ]
    print(json.dumps(dump_package(args.rvgame, targets), indent=2))


if __name__ == "__main__":
    main()
