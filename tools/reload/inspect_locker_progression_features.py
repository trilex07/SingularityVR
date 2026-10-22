"""Offline progression / feature subsystem witness (pawn+0x3D0, vtable +0x484/+0x44C).

Pins against archived Singularity.text-memory.bin. No game launch.
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

PINNED = {
    "pawn_inv_manager_offset": 0x3D0,
    "inventory_manager_vtable": 0x019CB480,
    "vtable_slot_44c": {"offset": 0x44C, "impl": 0x00FAEAC0, "ret_imm": 4},
    "vtable_slot_450": {"offset": 0x450, "impl": 0x00FFF3B0, "ret_imm": 4},
    "vtable_slot_484": {"offset": 0x484, "impl": 0x00FC4C10, "ret_imm": 0xC},
    "vtable_slot_484_helper": {"impl": 0x00FC4AA0, "ret_imm": 0xC},
    "enable_all_pawn_vtable_ddc": {"offset": 0xDDC, "impl": 0x00FFA950, "ret_imm": 4},
    "enable_all_exec": 0x00F67E40,
    "enable_all_catalog_helper": 0x00FD5AC0,
    "enable_all_bit_or": 0x00FFA9DC,
    "manager_locker_bitmask_offset": 0x3E4,
    "manager_ui_counters_offset": 0x364,
    "manager_ui_floats_offset": 0x374,
    "manager_ui_flag_offset": 0x3D4,
    "cast_helpers": {
        "EAE020_inventory_manager": 0x00EAE020,
        "EAEFB0_inventory_manager": 0x00EAEFB0,
        "EADF60_game_system_cast": 0x00EADF60,
        "EADA30_pawn_cast": 0x00EADA30,
    },
    "get_features_unlocked_exec": 0x00F55B30,
    "set_features_unlocked_exec": 0x00F55B70,
    "GGameSystem": 0x01CF394C,
}

VTABLE_484_LOAD = bytes([0x8B, 0x90, 0x84, 0x04, 0x00, 0x00])  # mov edx, [eax+484h]


def scan_ret(code: bytes, origin: int) -> int | None:
    i = 0
    last = None
    while i < len(code):
        if code[i] == 0xC3:
            last = 0
            i += 1
        elif code[i] == 0xC2 and i + 2 < len(code):
            last = code[i + 1] | (code[i + 2] << 8)
            i += 3
        else:
            i += 1
    return last


def verify_vtable(pe: PE) -> dict:
    vt = PINNED["inventory_manager_vtable"]
    out = {}
    for key in ("vtable_slot_44c", "vtable_slot_450", "vtable_slot_484"):
        meta = PINNED[key]
        va = struct.unpack("<I", pe.read(vt + meta["offset"], 4))[0]
        out[key] = {
            "resolved": hex(va),
            "expected": hex(meta["impl"]),
            "match": va == meta["impl"],
        }
    pawn_vt = 0x019D0788  # RvPlayerPawnSP (verify_pawn_weapon_selector.py)
    ddc = struct.unpack("<I", pe.read(pawn_vt + 0xDDC, 4))[0]
    out["enable_all_pawn_vtable_ddc"] = {
        "resolved": hex(ddc),
        "expected": hex(PINNED["enable_all_pawn_vtable_ddc"]["impl"]),
        "match": ddc == PINNED["enable_all_pawn_vtable_ddc"]["impl"],
    }
    return out


def scan_484_sites(text: bytes) -> list[dict]:
    import capstone

    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    sites = []
    i = 0
    while True:
        j = text.find(VTABLE_484_LOAD, i)
        if j < 0:
            break
        site = TEXT_BASE + j
        start = site - 0x40
        chunk = text[start - TEXT_BASE : site - TEXT_BASE + 0x20]
        pushes: list[str] = []
        for ins in md.disasm(chunk, start):
            if ins.address >= site:
                break
            if ins.mnemonic == "push":
                pushes.append(ins.op_str)
        sites.append({"load_va": hex(site), "pushes_before_call": pushes[-8:]})
        i = j + 1
    return sites


def classify_484_callers(sites: list[dict]) -> dict:
    barisov_keys = []
    other = []
    for s in sites:
        va = int(s["load_va"], 16)
        pushes = s["pushes_before_call"]
        if va in (0xF23C1C, 0xF23C3B):
            barisov_keys.append({**s, "role": "GetItemAt_impl"})
        elif any("0x19" in p or "0x37" in p for p in pushes):
            barisov_keys.append({**s, "role": "barisov_style_keys"})
        else:
            other.append(s)
    return {"barisov_station_list": barisov_keys, "other_callers": other}


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
    pe = PE(exe)

    enable_wrap = resolve_wrapper(pe, text, TEXT_BASE, "intARvPlayerPawnexecEnableAllWeaponsForLocker")

    v484 = text[PINNED["vtable_slot_484"]["impl"] - TEXT_BASE : PINNED["vtable_slot_484"]["impl"] - TEXT_BASE + 0x120]
    v44c = text[PINNED["vtable_slot_44c"]["impl"] - TEXT_BASE : PINNED["vtable_slot_44c"]["impl"] - TEXT_BASE + 0x30]

    sites = scan_484_sites(text)
    report = {
        "schema": "singularityvr-locker-progression-features-v1",
        "exe_sha256": hashlib.sha256(exe).hexdigest(),
        "text_sha256": hashlib.sha256(text).hexdigest(),
        "text_base": hex(TEXT_BASE),
        "pinned": PINNED,
        "subobject_at_pawn_3d0": {
            "type": "RvInventoryManager",
            "evidence": [
                "native_owned_inventory.h kPawnInvManager=0x3D0 + ClassHas(..., InventoryManager)",
                "EnableAllWeaponsForLocker @ 0xFFA950 reads [pawn+0x3D0]",
                "GetItemAt chain: [pawn+0x3D0] -> 0xEAE020 (IsA cache 0x1D18AB0)",
                "EAEFB0 @ 0xEAEFB0 same IsA pattern (cache 0x1D18AB8)",
            ],
        },
        "vtable_verify": verify_vtable(pe),
        "enable_all_weapons_for_locker": {
            "exec_wrapper": hex(enable_wrap["wrapper"]),
            "expected_exec": hex(PINNED["enable_all_exec"]),
            "exec_match": enable_wrap["wrapper"] == PINNED["enable_all_exec"],
            "native_impl": hex(PINNED["enable_all_pawn_vtable_ddc"]["impl"]),
            "state_effect": (
                "Loops GGameSystem+0x160 catalog (count +0x164, stride 0x50); FD5AC0 resolves row to "
                "bit index; OR/AND NOT into RvInventoryManager+0x3E4 from bool script arg."
            ),
            "does_not_write_mWeapons_array_directly": True,
        },
        "slot_44c_semantics": {
            "impl": hex(PINNED["vtable_slot_44c"]["impl"]),
            "abi": "__thiscall ret 4; arg0 byte index",
            "behavior": "if index<4 return [this+0x364+index*4] else 0",
            "transfer_data_arg2": "reads manager+0x36C (category slot 2 counter)",
        },
        "slot_484_semantics": {
            "impl": hex(PINNED["vtable_slot_484"]["impl"]),
            "abi": "__thiscall ret 0xC; stack args (byte category, int, int deltaKey)",
            "get_item_at_calls": [
                "category=2, int=1, key=0x19 -> add to [this+0x364+2*4]",
                "category=3, int=1, key=0x37 -> add to [this+0x364+3*4]",
            ],
            "role": "Barisov/GFx station list scroll metrics (uses +0x450 float table +0x374), not weapon unlock",
            "prologue_ret_imm": scan_ret(v484, PINNED["vtable_slot_484"]["impl"]),
            "helper_ret_imm": scan_ret(
                text[PINNED["vtable_slot_484_helper"]["impl"] - TEXT_BASE : PINNED["vtable_slot_484_helper"]["impl"] - TEXT_BASE + 0x80],
                PINNED["vtable_slot_484_helper"]["impl"],
            ),
        },
        "slot_44c_ret_imm": scan_ret(v44c, PINNED["vtable_slot_44c"]["impl"]),
        "vtable_484_call_sites": classify_484_callers(sites),
        "locker_progression_candidate": {
            "field": "RvInventoryManager+0x3E4 (native bitmask, export name not recovered)",
            "related_script_field": "RvInventoryItem.mWeaponsAvailableToLocker (IntProperty, separate object)",
            "game_system_byte": "RvGameSystem.mFeaturesUnlocked via Get/SetFeaturesUnlocked (+0x1F0/+0x1F4)",
            "weapon_locker_falsifier": (
                "+0x484/+0x44C only reached from Barisov GetItemAt/TransferData in locker vtable block; "
                "WeaponLocker equip/Initialize/LoadEquippedWeapons do not reference +0x484 or +0x3E4."
            ),
        },
        "verdict": "B_for_locker_3E4_bitmask; C_for_vtable_484_44C_locker_availability",
        "minimal_witness": (
            "Read-only: log RvInventoryManager+0x3E4 (hex) at WeaponLocker equip(Weapon) time; "
            "optional bit index from FD5AC0 match for equip FName. Do not log full GGameSystem catalog."
        ),
    }

    ok = all(v["match"] for v in report["vtable_verify"].values())
    ok &= report["enable_all_weapons_for_locker"]["exec_match"]
    ok &= report["slot_44c_ret_imm"] == PINNED["vtable_slot_44c"]["ret_imm"]
    ok &= report["slot_484_semantics"]["prologue_ret_imm"] == PINNED["vtable_slot_484"]["ret_imm"]
    report["_witness_ok"] = ok

    print(json.dumps(report, indent=2))
    if args.strict and not ok:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
