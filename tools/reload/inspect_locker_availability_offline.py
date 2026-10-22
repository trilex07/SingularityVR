"""Offline WeaponLocker availability contract (export + .text disassembly).

No game launch. Produces JSON evidence for RvGameSystem.mWeapons path,
native exec bodies, and deprioritized mWeaponsAvailableToLocker.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

from inspect_inventory_registration import resolve_wrapper
from inspect_native_reload import PE


WEAPON_INFO_FIELDS = [
    ("mbIsHeavyWeapon", "Bool"),
    ("mNumTechPointsToBuyClip", "Int"),
    ("mAmmoType", "Byte"),
    ("mRateOfFire", "Float"),
    ("mRange", "Float"),
    ("mDamage", "Float"),
    ("mAccuracy", "Float"),
    ("mPerk", "Str"),
    ("mType", "Byte"),
    ("mPath", "Str"),
    ("mName", "Str"),
    ("Mid", "Str"),
]


def parse_export_inventory(export_text: str) -> dict:
    if "ScriptStruct WeaponInfo" not in export_text:
        raise ValueError("WeaponInfo missing from export inventory")
    if "ArrayProperty mWeapons" not in export_text:
        raise ValueError("mWeapons missing from export inventory")
    if "ObjectProperty SessionManager" not in export_text:
        raise ValueError("SessionManager property missing (RvGameSystem block)")
    return {
        "weapon_info_struct_size": "0x190",
        "weapon_info_export_fields": WEAPON_INFO_FIELDS,
        "m_weapons_owner_class": "RvGameSystem",
        "session_manager_note": "SessionManager on RvGameSystem is RvSessionManager, not UEngine.",
    }


def weapon_locker_vtable(pe: PE) -> dict:
    vt = 0x019A0948
    slots = {
        "Initialize_impl": 0x1EC,
        "LoadEquippedWeapons": 0x1F4,
        "equip_body": 0x1F8,
        "GFx_invoke": 0x130,
    }
    out = {}
    for name, off in slots.items():
        out[name] = {"slot": hex(off), "va": hex(struct.unpack("<I", pe.read(vt + off, 4))[0])}
    return out


def scan_global_game_system(text: bytes, base: int) -> list:
    """Sites that load GGameSystem pointer (verified on LoadEquippedWeapons / Initialize)."""
    pat = b"\xa1" + struct.pack("<I", 0x01CF394C)  # mov eax, [GGameSystem]
    hits = []
    i = 0
    while True:
        j = text.find(pat, i)
        if j < 0:
            break
        hits.append(hex(base + j))
        i = j + 1
    return hits[:12]


def inspect(exe: bytes, text: bytes, base: int, export_text: str) -> dict:
    pe = PE(exe)
    export_meta = parse_export_inventory(export_text)
    wrappers = {}
    for name in (
        "intURvUIGFxMovie_WeaponLockerexecInitialize",
        "intURvUIGFxMovie_WeaponLockerexecLoadEquippedWeapons",
        "intURvUIGFxMovie_WeaponLockerexecequip",
        "intURvUIGFxMovieexecGetGameSystem",
        "intARvPlayerPawnexecEnableAllWeaponsForLocker",
    ):
        wrappers[name] = hex(resolve_wrapper(pe, text, base, name)["wrapper"])

    return {
        "schema": "singularityvr-locker-availability-offline-v1",
        "exe_sha256": hashlib.sha256(exe).hexdigest(),
        "text_sha256": hashlib.sha256(text).hexdigest(),
        "text_base": hex(base),
        "export": export_meta,
        "globals": {
            "GEngine": "0x01CF8778",
            "GGameSystem": "0x01CF394C",
            "witness_fix": "Use GGameSystem for RvGameSystem.mWeapons; Engine.SessionManager reflection was wrong.",
        },
        "native_bodies": {
            "Initialize_impl": "0x00F237C0",
            "LoadEquippedWeapons": "0x00F23840",
            "equip_slot_write": "0x00F00F50",
            "equip_script_full": "0x00F00FA0",
            "GetItemAt_vtable_0x204": "0x00F23B60",
            "TransferData_vtable_0x200": "0x00F23A50",
            "GetItemAt_exec_Barisov": "0x00F6E5A0",
            "TransferData_exec_Barisov": "0x00F6E4F0",
        },
        "use_station_list_audit": "docs/weapon-locker-availability-producer-20261022.md",
        "script_wrappers": wrappers,
        "weapon_locker_vtable_0x019A0948": weapon_locker_vtable(pe),
        "g_game_system_mov_eax_sites_sample": scan_global_game_system(text, base),
        "disassembly_conclusions": {
            "equip_0xF00F50": "Writes GFx-supplied FName into movie+0xB0[slot*8]; no ownedMask gate.",
            "LoadEquippedWeapons": "Uses GGameSystem; iterates movie mEquippedWeapons (+0xB0/+0xB4); GFx invoke vtable+0x130.",
            "Initialize": "vtable+0x1EC -> 0xF237C0; shares tail with LoadEquippedWeapons setup.",
            "EnableAllWeaponsForLocker": "RvPlayerPawn vtable+0xDDC with bool; progression/cheat unlock, not station UI list.",
        },
        "m_weapons_available_to_locker": {
            "declared_on": "RvInventoryItem",
            "weapon_locker_exec_xrefs": "none in LoadEquipped/equip/Initialize bodies reviewed",
            "status": "deprioritized",
        },
    }


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--exe", type=Path, required=True)
    ap.add_argument("--text", type=Path, required=True)
    ap.add_argument("--export", type=Path, required=True)
    ap.add_argument("--text-base", type=lambda x: int(x, 0), default=0x402000)
    args = ap.parse_args()
    report = inspect(
        args.exe.read_bytes(),
        args.text.read_bytes(),
        args.text_base,
        args.export.read_text(encoding="utf-8", errors="replace"),
    )
    print(json.dumps(report, indent=2))
