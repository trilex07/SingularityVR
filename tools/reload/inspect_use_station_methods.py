"""Offline ABI + disassembly witness for UseStation list natives (GetItemAt / TransferData).

Pins registration wrappers, WeaponLocker vtable slots (+0x200..+0x20C), and primary
ret immediates against archived Singularity.text-memory.bin. No game launch.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

from inspect_inventory_registration import resolve_wrapper
from inspect_native_reload import PE

TEXT_BASE = 0x402000
G_GAME_SYSTEM = 0x01CF394C
WEAPON_LOCKER_VTABLE = 0x019A0948

# Verified on capture-20261006-205508 / Steam Singularity.exe
EXPECTED = {
    "script_wrappers": {
        "intURvUIGFxMovie_BarisovStationexecGetItemAt": 0x00F6E5A0,
        "intURvUIGFxMovie_BarisovStationexecTransferData": 0x00F6E4F0,
    },
    "vtable_impl": {
        "TransferData": {"slot": 0x200, "va": 0x00F23A50, "primary_ret_imm": 4},
        "GetItemAt": {"slot": 0x204, "va": 0x00F23B60, "primary_ret_imm": 4},
        "GFx_WeaponTech_row": {"slot": 0x208, "va": 0x00F23CB0, "primary_ret_imm": 4},
        "GFx_lookup_by_name": {"slot": 0x20C, "va": 0x00F23DF0, "primary_ret_imm": 8},
    },
    "helper_calls": {
        "GGameSystem_deref": 0x00EAE080,
        "pawn_from_game_system_v194": 0x194,
        "player_subobject_3d0": 0x3D0,
        "subobject_vtable_484_keys": (0x19, 0x37),
        "subobject_vtable_44c_transfer": 2,
    },
}


def scan_ret(code: bytes, origin: int) -> list[dict]:
    out: list[dict] = []
    i = 0
    while i < len(code):
        if code[i] == 0xC3:
            out.append({"at": hex(origin + i), "imm": 0})
            i += 1
        elif code[i] == 0xC2 and i + 2 < len(code):
            imm = code[i + 1] | (code[i + 2] << 8)
            out.append({"at": hex(origin + i), "imm": imm})
            i += 3
        else:
            i += 1
    return out


def primary_ret_imm(rets: list[dict]) -> int | None:
    if not rets:
        return None
    tail = next((r for r in reversed(rets) if r["imm"]), rets[-1])
    return tail["imm"]


def disasm_summary(text: bytes, va: int, size: int) -> list[str]:
    import capstone

    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    lines: list[str] = []
    for ins in md.disasm(text[va - TEXT_BASE : va - TEXT_BASE + size], va):
        op = ins.op_str
        if ins.mnemonic == "call" or "1cf394c" in op or "+ 0x484" in op or "+ 0x44c" in op:
            lines.append(f"{ins.address:#x}: {ins.mnemonic} {op}")
    return lines


def inspect(exe: bytes, text: bytes) -> dict:
    pe = PE(exe)
    wrappers = {}
    for name, exp_va in EXPECTED["script_wrappers"].items():
        resolved = resolve_wrapper(pe, text, TEXT_BASE, name)
        wrappers[name] = {
            "expected": hex(exp_va),
            "resolved": hex(resolved["wrapper"]),
            "match": resolved["wrapper"] == exp_va,
            "form": resolved["form"],
        }

    vtable = {}
    for label, meta in EXPECTED["vtable_impl"].items():
        slot = meta["slot"]
        va = struct.unpack("<I", pe.read(WEAPON_LOCKER_VTABLE + slot, 4))[0]
        code = text[va - TEXT_BASE : va - TEXT_BASE + 0x180]
        rets = scan_ret(code, va)
        imm = primary_ret_imm(rets)
        vtable[label] = {
            "slot": hex(slot),
            "va": hex(va),
            "expected_va": hex(meta["va"]),
            "va_match": va == meta["va"],
            "primary_ret_imm": imm,
            "ret_imm_match": imm == meta["primary_ret_imm"],
            "disasm_markers": disasm_summary(text, va, 0x180),
        }

    get_item = text[0xF23B60 - TEXT_BASE : 0xF23B60 - TEXT_BASE + 0x160]
    transfer = text[0xF23A50 - TEXT_BASE : 0xF23A50 - TEXT_BASE + 0x120]
    conclusions = {
        "package_exports": (
            "GetItemAt / TransferData UFunction outers are RvUIGFxMovie_BarisovStation only; "
            "RvUIGFxMovie_WeaponLocker exports Initialize, LoadEquippedWeapons, LoadUpgrades, equip — not GetItemAt."
        ),
        "indexing": (
            "GetItemAt/TransferData impls load GGameSystem and call player subobject vtable+0x484 "
            "with fixed keys 0x19/0x37 (TransferData also +0x44C with 2). No imul 0x190 / no [Index] on mWeapons."
        ),
        "equip_tab_hypothesis": (
            "WeaponLocker cyclable FNames correlate with GFx equip(Weapon,slot) + HUD WeaponNames map; "
            "not proven as GetItemAt filtered mWeapons[] (see docs/weapon-locker-availability-producer-20261022.md)."
        ),
    }

    return {
        "schema": "singularityvr-use-station-methods-v1",
        "exe_sha256": hashlib.sha256(exe).hexdigest(),
        "text_sha256": hashlib.sha256(text).hexdigest(),
        "text_base": hex(TEXT_BASE),
        "GGameSystem": hex(G_GAME_SYSTEM),
        "weapon_locker_vtable": hex(WEAPON_LOCKER_VTABLE),
        "script_wrappers": wrappers,
        "vtable_impl": vtable,
        "helper_calls": EXPECTED["helper_calls"],
        "conclusions": conclusions,
    }


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
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
    ap.add_argument("--strict", action="store_true")
    args = ap.parse_args()
    exe, text = args.exe.read_bytes(), args.text.read_bytes()
    report = inspect(exe, text)
    ok = all(w["match"] for w in report["script_wrappers"].values())
    ok &= all(v["va_match"] and v["ret_imm_match"] for v in report["vtable_impl"].values())
    report["_witness_ok"] = ok
    print(json.dumps(report, indent=2))
    if args.strict and not ok:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
