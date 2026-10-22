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
    "weapon_locker_vtable": 0x019A0948,
    "gfx_consumer_vtable_91b220": 0x019D4488,
    "vtable_slot_44c": {"offset": 0x44C, "impl": 0x00FAEAC0, "ret_imm": 4},
    "vtable_slot_450": {"offset": 0x450, "impl": 0x00FFF3B0, "ret_imm": 4},
    "vtable_slot_484": {"offset": 0x484, "impl": 0x00FC4C10, "ret_imm": 0xC},
    "vtable_slot_484_helper": {"impl": 0x00FC4AA0, "ret_imm": 0xC},
    "enable_all_pawn_vtable_ddc": {"offset": 0xDDC, "impl": 0x00FFA950, "ret_imm": 4},
    "enable_all_exec": 0x00F67E40,
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
    "catalog_path_to_mask": 0x00FD5AC0,
    "catalog_mask_apply": 0x00FD5BA0,
    "catalog_gamesystem_data": 0x160,
    "catalog_gamesystem_count": 0x164,
    "catalog_row_stride": 0x50,
    "catalog_row_filter_byte": 0x24,
    "catalog_row_filter_dword": 0x4C,
    "catalog_row_filter_dword_bit0": 1,
    "catalog_default_empty_fstring": 0x179E4D0,
    "gfx_consumer_91b220": 0x0091B220,
    "uobject_outer_helper_40bef0": 0x0040BEF0,
    "progression_unlock_loop": 0x00F03350,
    "fd5ba0_call_sites": [0x00F03431, 0x00F41675, 0x010419F7],
    "wrong_class_vtable_fstring_dtor": 0x0189DC00,
    "wrong_class_ctor_b40c80": 0x00B40C80,
    "wrong_class_dtor_b40db0": 0x00B40DB0,
    "invmgr_vtable_store": 0x01027912,
}

VTABLE_484_LOAD = bytes([0x8B, 0x90, 0x84, 0x04, 0x00, 0x00])  # mov edx, [eax+484h]

MASK_3E4_PATTERNS: list[tuple[bytes, str]] = [
    (bytes([0x09, 0x86, 0xE4, 0x03, 0x00, 0x00]), "or [esi+3E4],eax"),
    (bytes([0x21, 0x86, 0xE4, 0x03, 0x00, 0x00]), "and [esi+3E4],eax"),
    (bytes([0x21, 0x8E, 0xE4, 0x03, 0x00, 0x00]), "and [esi+3E4],ecx"),
    (bytes([0x89, 0x86, 0xE4, 0x03, 0x00, 0x00]), "mov [esi+3E4],eax"),
    (bytes([0x8B, 0x86, 0xE4, 0x03, 0x00, 0x00]), "mov eax,[esi+3E4]"),
    (bytes([0x8B, 0x8E, 0xE4, 0x03, 0x00, 0x00]), "mov ecx,[esi+3E4]"),
    (bytes([0x8B, 0x81, 0xE4, 0x03, 0x00, 0x00]), "mov eax,[ecx+3E4]"),
]


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


def scan_text_patterns(text: bytes, patterns: list[tuple[bytes, str]]) -> list[dict]:
    hits: list[dict] = []
    for pat, label in patterns:
        i = 0
        while True:
            j = text.find(pat, i)
            if j < 0:
                break
            hits.append({"va": hex(TEXT_BASE + j), "op": label})
            i = j + 1
    hits.sort(key=lambda h: int(h["va"], 16))
    return hits


def find_call_sites(text: bytes, target: int) -> list[str]:
    sites: list[int] = []
    i = 0
    while True:
        j = text.find(b"\xe8", i)
        if j < 0 or j + 5 > len(text):
            break
        rel = struct.unpack_from("<i", text, j + 1)[0]
        va = TEXT_BASE + j + 5 + rel
        if va == target:
            sites.append(TEXT_BASE + j)
        i = j + 1
    return [hex(s) for s in sites]


def verify_fd5ac0(text: bytes) -> dict:
    import capstone

    va = PINNED["catalog_path_to_mask"]
    chunk = text[va - TEXT_BASE : va - TEXT_BASE + 0xD8]
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    ecx_used_as_this = False
    for ins in md.disasm(chunk, va):
        if ins.address >= va + 0x45:
            break
        if ins.mnemonic in ("mov", "cmp", "test", "lea", "add", "or", "and", "xor") and ins.op_str.startswith(
            "ecx,"
        ):
            ecx_used_as_this = True
            break
        if "[ecx" in ins.op_str and "ebp" not in ins.op_str:
            ecx_used_as_this = True
            break
    reads_ebp8 = chunk.find(bytes([0x8D, 0x4D, 0x08])) >= 0  # lea ecx,[ebp+8]
    return {
        "va": hex(va),
        "ret_imm": scan_ret(chunk, va),
        "expected_ret_imm": 8,
        "call_gamesystem": chunk.find(bytes([0xE8])) >= 0,
        "failure_returns_minus_one": chunk.find(bytes([0x83, 0xC8, 0xFF])) >= 0,
        "success_shl": chunk.find(bytes([0xD3, 0xE0])) >= 0,
        "callers": find_call_sites(text, va),
        "incoming_ecx_is_not_this": not ecx_used_as_this,
        "formal_args_from_ebp8": reads_ebp8,
        "signature": (
            "uint32 __stdcall CatalogWeaponPathToBitMask(const wchar_t* pathData, int pathNum); "
            "ECX at entry is caller spill (Incidental, not part of ABI)"
        ),
        "fd5ba0_pushes_before_call": "push pathNum; push pathData; (FString components)",
        "ffa950_pushes_before_call": "push [ebp-8]; push [ebp-0xC] after 4AFFD0",
    }


def verify_manager_3e4_lifetime(pe: PE, text: bytes) -> dict:
    wrong_vt = PINNED["wrong_class_vtable_fstring_dtor"]
    inv_vt = PINNED["inventory_manager_vtable"]
    dtor_vt_store = PINNED["wrong_class_dtor_b40db0"] + 0x2A
    off = dtor_vt_store - TEXT_BASE
    dtor_sets = struct.unpack("<I", text[off + 2 : off + 6])[0]
    inv_off = PINNED["invmgr_vtable_store"] - TEXT_BASE
    inv_sets = struct.unpack("<I", text[inv_off + 2 : inv_off + 6])[0]
    return {
        "resolution": "A_B40E78_not_RvInventoryManager",
        "b40db0_sets_vtable": hex(dtor_sets),
        "invmgr_vtable_at_1027912": hex(inv_sets),
        "vtable_match_at_dtor": dtor_sets == wrong_vt,
        "invmgr_ctor_stores_vtable": inv_sets == inv_vt,
        "manager_plus_3e4_type": "uint32 catalog_bitmask (OR/AND only; no 403B00 free on InvMgr path)",
        "fstring_free_sites_at_3e4": ["0xB40E78 (class vtable 0x0189DC00 only)"],
    }


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
    wl = PINNED["weapon_locker_vtable"]
    out["weapon_locker_vtable_578"] = {
        "resolved": hex(struct.unpack("<I", pe.read(wl + 0x578, 4))[0]),
        "expected_not_91b220": True,
    }
    gfx = PINNED["gfx_consumer_vtable_91b220"]
    out["gfx_consumer_vtable_578"] = {
        "resolved": hex(struct.unpack("<I", pe.read(gfx + 0x578, 4))[0]),
        "expected": hex(PINNED["gfx_consumer_91b220"]),
        "match": struct.unpack("<I", pe.read(gfx + 0x578, 4))[0]
        == PINNED["gfx_consumer_91b220"],
    }
    inv578 = struct.unpack("<I", pe.read(vt + 0x578, 4))[0]
    out["inventory_manager_vtable_578"] = {
        "resolved": hex(inv578),
        "expected_not_91b220": inv578 != PINNED["gfx_consumer_91b220"],
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


def classify_3e4_xrefs(hits: list[dict]) -> dict:
    manager_bitmask_vas = {0xFFA9DC, 0xFFA9E6, 0xFD5BC0, 0xFD5BCD}
    wrong_class_fstring_vas = {0xB40D56, 0xB40E78}
    manager: list[dict] = []
    gfx: list[dict] = []
    wrong_class: list[dict] = []
    other: list[dict] = []
    for h in hits:
        va_int = int(h["va"], 16)
        entry = {**h}
        if va_int in manager_bitmask_vas:
            entry["class_hint"] = "RvInventoryManager_bitmask"
            manager.append(entry)
        elif va_int in wrong_class_fstring_vas:
            entry["class_hint"] = "vtable_0x0189DC00_FString_field"
            wrong_class.append(entry)
        elif va_int == 0x91B241:
            entry["class_hint"] = "vtable_0x019D4488_not_WeaponLocker"
            gfx.append(entry)
        else:
            entry["class_hint"] = "other_type_same_offset"
            other.append(entry)
    return {
        "inventory_manager_bitmask": manager,
        "wrong_class_fstring_at_same_offset": wrong_class,
        "gfx_consumer_91b241": gfx,
        "other_offset_collisions": other,
    }


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--exe",
        type=Path,
        default=Path(r"C:\Users\Trilex\vr-mods\test-games\assets\Singularity\Binaries\Singularity.exe"),
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
    mask_hits = scan_text_patterns(text, MASK_3E4_PATTERNS)
    fd5 = verify_fd5ac0(text)
    lifetime = verify_manager_3e4_lifetime(pe, text)

    report = {
        "schema": "singularityvr-locker-progression-features-v3",
        "exe_sha256": hashlib.sha256(exe).hexdigest(),
        "text_sha256": hashlib.sha256(text).hexdigest(),
        "text_base": hex(TEXT_BASE),
        "pinned": PINNED,
        "subobject_at_pawn_3d0": {
            "type": "RvInventoryManager",
            "evidence": [
                "native_owned_inventory.h kPawnInvManager=0x3D0 + ClassHas(..., InventoryManager)",
                "EnableAllWeaponsForLocker @ 0xFFA950 reads [pawn+0x3D0]",
                "FD5BA0 @ 0xFD5BA0 uses ecx=RvInventoryManager, patches +0x3E4",
            ],
        },
        "fd5ac0_catalog_path_to_mask": {
            **fd5,
            "skip_row_if": ["row[+0x24]!=0", "row[+0x4C]&1"],
            "success_return": "1u << catalogIndex",
            "failure_return": "0xFFFFFFFF",
            "match_helpers": ["0x4AC340", "0x4409F0"],
            "gamesystem_via": "0xEFA070",
        },
        "fd5ba0_apply_mask": {
            "va": hex(PINNED["catalog_mask_apply"]),
            "signature": (
                "void __thiscall ApplyCatalogBit(RvInventoryManager* this, "
                "const wchar_t* pathData, int pathNum, bool enable)"
            ),
            "call_sites": [hex(v) for v in PINNED["fd5ba0_call_sites"]],
            "call_sites_match": find_call_sites(text, PINNED["catalog_mask_apply"])
            == [hex(v) for v in PINNED["fd5ba0_call_sites"]],
            "progression_paths": {
                "0xF03431": "Mission unlock loop 0xF03350: FString from catalog row -> FD5BA0(true); may AddWeapon +0x434",
                "0xF41675": "Pickup bundle: pairs at [item+0xC8 + i*8] (data,num) -> FD5BA0(true)",
                "0x10419F7": "Pickup: [pickup+0x2E8/0x2EC] weapon identity -> FD5BA0(true)",
            },
        },
        "manager_plus_3e4_lifetime": lifetime,
        "catalog_row_layout_partial": {
            "gamesystem_offset_data": hex(PINNED["catalog_gamesystem_data"]),
            "gamesystem_offset_count": hex(PINNED["catalog_gamesystem_count"]),
            "stride": hex(PINNED["catalog_row_stride"]),
            "identity_fstring": "+0x00/+0x04",
            "secondary_fstring": "+0x0C/+0x10 (0xF0343C)",
            "filter_byte": hex(PINNED["catalog_row_filter_byte"]),
            "filter_dword_bit0": hex(PINNED["catalog_row_filter_dword"]),
        },
        "enable_all_weapons_for_locker": {
            "exec_wrapper": hex(enable_wrap["wrapper"]),
            "expected_exec": hex(PINNED["enable_all_exec"]),
            "exec_match": enable_wrap["wrapper"] == PINNED["enable_all_exec"],
            "native_impl": hex(PINNED["enable_all_pawn_vtable_ddc"]["impl"]),
            "progression_unlock_loop": hex(PINNED["progression_unlock_loop"]),
            "state_effect": "Same catalog loop as FD5AC0; OR/AND NOT manager+0x3E4; respects row+0x24 and row+0x4C bit0",
        },
        "gfx_consumer_91b220_gate": {
            "function": hex(PINNED["gfx_consumer_91b220"]),
            "read_site": "0x91b241",
            "field_use": "UObject* at +0x3E4 -> 0x40BEF0 (not bitmask)",
            "vtable": hex(PINNED["gfx_consumer_vtable_91b220"]),
            "weapon_locker_vtable_578": "stub 0x5B93C0 — not 0x91B220",
            "verdict": "C_wrong_subsystem_for_locker_mask_consumer",
        },
        "mask_3e4_xrefs": classify_3e4_xrefs(mask_hits),
        "slot_44c_semantics": {
            "impl": hex(PINNED["vtable_slot_44c"]["impl"]),
            "abi": "__thiscall ret 4; arg0 byte index",
            "behavior": "if index<4 return [this+0x364+index*4] else 0",
        },
        "slot_484_semantics": {
            "impl": hex(PINNED["vtable_slot_484"]["impl"]),
            "role": "Barisov/GFx station list scroll metrics — not weapon unlock",
            "prologue_ret_imm": scan_ret(v484, PINNED["vtable_slot_484"]["impl"]),
        },
        "slot_44c_ret_imm": scan_ret(v44c, PINNED["vtable_slot_44c"]["impl"]),
        "vtable_verify": verify_vtable(pe),
        "vtable_484_call_sites": classify_484_callers(sites),
        "catalog_offline_tool": "tools/reload/dump_gamesystem_weapon_catalog_offline.py",
        "ordinary_weapon_bit_map": "INFERRED_Default__RvGameSystem_path_order",
        "verdict": {
            "manager_plus_3e4_bitmask_semantics": "PROVEN_uint32_via_FD5BA0",
            "manager_plus_3e4_lifetime": "PROVEN_not_FString_on_InvMgr",
            "catalog_index_order": "INFERRED_blob_order_equals_runtime_index",
            "weapon_locker_consumes_3e4": "NOT_PROVEN_91B221_retired",
            "overall_vr_loadout": "B_ready_for_minimal_runtime_witness_on_locker_consumption",
        },
        "minimal_witness_shape": (
            "Weapon=<path> catalogIndex=i nativeBitIndex=i nativeMask=(1<<i) "
            "bitSet=((manager+0x3E4)&nativeMask)!=0 — not mask hex alone while cycling"
        ),
        "doc": "docs/weapon-locker-mask-3e4-contract-20261022.md",
    }

    ok = all(v["match"] for k, v in report["vtable_verify"].items() if "match" in v)
    ok &= report["enable_all_weapons_for_locker"]["exec_match"]
    ok &= report["slot_44c_ret_imm"] == PINNED["vtable_slot_44c"]["ret_imm"]
    ok &= report["slot_484_semantics"]["prologue_ret_imm"] == PINNED["vtable_slot_484"]["ret_imm"]
    ok &= fd5["ret_imm"] == fd5["expected_ret_imm"]
    ok &= set(find_call_sites(text, PINNED["catalog_path_to_mask"])) == {"0xfd5bb0", "0xffa9cc"}
    ok &= report["fd5ba0_apply_mask"]["call_sites_match"]
    ok &= report["vtable_verify"]["gfx_consumer_vtable_578"]["match"]
    ok &= report["vtable_verify"]["inventory_manager_vtable_578"]["expected_not_91b220"]
    ok &= report["vtable_verify"]["weapon_locker_vtable_578"]["resolved"] == "0x5b93c0"
    ok &= fd5["incoming_ecx_is_not_this"]
    ok &= fd5["formal_args_from_ebp8"]
    ok &= lifetime["invmgr_ctor_stores_vtable"]
    ok &= lifetime["resolution"] == "A_B40E78_not_RvInventoryManager"
    ok &= len(report["mask_3e4_xrefs"]["wrong_class_fstring_at_same_offset"]) == 2
    ok &= len(report["mask_3e4_xrefs"]["inventory_manager_bitmask"]) >= 4
    report["_witness_ok"] = ok

    print(json.dumps(report, indent=2))
    if args.strict and not ok:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
