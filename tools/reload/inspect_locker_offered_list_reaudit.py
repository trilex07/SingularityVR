"""WeaponLocker offered-list offline re-audit (2026-10-22) — JSON witness for docs/tests."""
from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

TEXT_BASE = 0x402000

PINNED = {
    "initialize": 0x00F237C0,
    "build_gfx_payload": 0x00F11570,
    "load_equipped": 0x00F23840,
    "set_requested_packages": 0x00F00B60,
    "apply_requested_packages": 0x00F00C50,
    "refresh_subobject_1dc": 0x00EF7370,
    "weapon_locker_vtable": 0x019A0948,
    "gamesystem_vtable_160": 0x00D80EF0,
    "gamesystem_vtable_15c": 0x00D80E30,
    "equip_commit": 0x00F00F50,
    "start": 0x00F11430,
}


def vtable_slot(pe, vt_va: int, slot: int) -> int:
    return struct.unpack("<I", pe.read(vt_va + slot, 4))[0]


def scan_bc_sites(text: bytes) -> dict[str, list[str]]:
    sigs = {
        "write_mov_esi": bytes([0x89, 0xBE, 0xBC, 0, 0, 0]),
        "read_mov_esi": bytes([0x8B, 0x86, 0xBC, 0, 0, 0]),
        "read_mov_edi": bytes([0x8B, 0x8E, 0xBC, 0, 0, 0]),
    }
    out: dict[str, list[str]] = {}
    for name, sig in sigs.items():
        hits: list[str] = []
        i = 0
        while True:
            j = text.find(sig, i)
            if j < 0:
                break
            va = TEXT_BASE + j
            if 0xED0000 <= va <= 0xF05000:
                hits.append(hex(va))
            i = j + 1
        out[name] = hits
    out["start_clear"] = ["0xf11497"]
    return out


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--text",
        type=Path,
        default=Path(r"C:\Users\Trilex\vr-mods\research\etw\capture-20261006-205508\Singularity.text-memory.bin"),
    )
    ap.add_argument(
        "--exe",
        type=Path,
        default=Path(r"C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\Singularity.exe"),
    )
    args = ap.parse_args()
    text = args.text.read_bytes()
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
    from inspect_native_reload import PE  # noqa: E402

    pe = PE(args.exe.read_bytes())

    vt = PINNED["weapon_locker_vtable"]
    slot_1dc = vtable_slot(pe, vt, 0x1DC)

    report = {
        "schema": "singularityvr-locker-offered-list-reaudit-v1",
        "verdict": "B",
        "verdict_note": (
            "Strongest GFx list producer candidate: Initialize -> F11570 -> vtable+0x130; "
            "not A until init payload logged at runtime."
        ),
        "f00b60": {
            "va": hex(PINNED["set_requested_packages"]),
            "export_name": None,
            "best_supported_label": "Native_SetRequestedPackageState",
            "abi": "__thiscall, ret 0x14",
            "args": {
                "ebp+0xC": "float -> mActiveWeaponSlot (+0xB8)",
                "ebp+0x10": "bit0 of mRequestedPackages (+0xBC)",
                "ebp+0x14": "bit2 (mask 4)",
                "ebp+0x18": "bit1 (mask 2)",
            },
            "is_vtable_1dc": False,
            "calls_vtable_1dc_first": True,
            "vtable_1dc_impl": hex(slot_1dc),
            "preview_callback_proven": False,
            "direct_e8_call_sites": [],
        },
        "f00c50": {
            "va": hex(PINNED["apply_requested_packages"]),
            "best_supported_label": "Native_ApplyRequestedPackages",
            "role": "Propagate +0xBC bits and +0xB8 float into profile/game; GGameSystem subobject streaming",
            "gamesystem_vtable_160": hex(PINNED["gamesystem_vtable_160"]),
            "gamesystem_vtable_15c": hex(PINNED["gamesystem_vtable_15c"]),
            "weapon_identity_in_path": False,
        },
        "load_equipped_weapons": {
            "va": hex(PINNED["load_equipped"]),
            "classification": "A",
            "classification_meaning": "current equipped slots only (movie+0xB4 count, +0xB0 FNames)",
        },
        "initialize": {
            "va": hex(PINNED["initialize"]),
            "helper": hex(PINNED["build_gfx_payload"]),
            "payload_audit": "progression FName array (FE2500) x catalog+0x160 rows -> GFx invoke buffer",
            "reads_manager_3e4": False,
            "strongest_gfx_list_producer": True,
        },
        "mRequestedPackages_bc": {
            "offset": "0xBC",
            "semantics": "package load/request bitfield — not offeredOrdinaryMask",
            "sites": scan_bc_sites(text),
        },
        "catalog_v2_log_contract": {
            "per_row": [
                "primary.Data",
                "primary.Num",
                "primary.Max",
                "primary bounded wchar full string",
                "secondary bounded wchar at row+0x0C",
                "filter24",
                "filter4C",
            ],
            "line_example": 'catalog[i] primary="<full>" secondary="<...>" num=N filter24=X filter4C=Y',
            "no_wp_sp_prefilter": True,
        },
        "witness_v2_design": [
            "1: one-shot log after F11570 in Initialize (GFx seed payload)",
            "2: optional vtable+0x130 invoke args",
            "3: once/session catalog contract (17 rows max)",
            "avoid: equip@F00F50, LockerNativeTrace, LoadUpgrades, F00B60 unless preview proven",
        ],
    }
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
