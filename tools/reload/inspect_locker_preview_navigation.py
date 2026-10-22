"""WeaponLocker preview vs commit — offline .text witness (no game launch)."""
from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path

TEXT_BASE = 0x402000

PINNED = {
    "equip_commit_hot": 0x00F00F50,
    "equip_script_full": 0x00F00FA0,
    "start": 0x00F11430,
    "load_equipped": 0x00F23840,
    "initialize": 0x00F237C0,
    "get_item_at": 0x00F23B60,
    "gfx_row_helper_208": 0x00F23CB0,
    "package_flag_native_1dc": 0x00F00B60,
    "package_followup_1dc": 0x00F00C50,
    "weapon_locker_vtable": 0x019A0948,
    "m_equipped_weapons_off": 0xB0,
    "m_requested_packages_off": 0xBC,
    "m_dirty_flag_off": 0xA4,
}


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


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--text",
        type=Path,
        default=Path(r"C:\Users\Trilex\vr-mods\research\etw\capture-20261006-205508\Singularity.text-memory.bin"),
    )
    args = ap.parse_args()
    text = args.text.read_bytes()

    equip_hot = text[PINNED["equip_commit_hot"] - TEXT_BASE : PINNED["equip_commit_hot"] - TEXT_BASE + 0x50]
    report = {
        "schema": "singularityvr-locker-preview-navigation-v1",
        "equip_at_f00f50": {
            "classification": "COMMIT — writes FName into mEquippedWeapons[slot*8] from script Weapon parm",
            "direct_call_sites_in_text": find_call_sites(text, PINNED["equip_commit_hot"]),
            "note": "Zero direct E8 calls; invoked only via GFx/script vtable slot 0x1F8.",
            "hot_body_writes_b0_slot": bytes([0x89, 0x04, 0x8A]) in equip_hot or bytes([0x89, 0x44, 0xC8]) in equip_hot,
            "full_script_at_f00fa0": "Uses movie vtable+0x208 (0xF23CB0) + inventory ownership before add",
        },
        "navigation_hypothesis": {
            "verdict": "LIKELY_GFX_SIDE for left/right preview",
            "evidence": [
                "PSVR2 sweep produced zero equip@F00F50 hits while UI changed",
                "LoadEquippedWeapons (F23840) builds GFx invoke payloads (4AE4C0/4498C0) — seeds list once",
                "Initialize (F237C0) calls F11570 then script vtable+0x130",
                "Start (F11430) clears mRequestedPackages (+0xBC) and resizes mEquippedWeapons (+0xB0)",
            ],
        },
        "mRequestedPackages": {
            "offset": hex(PINNED["m_requested_packages_off"]),
            "type": "IntProperty bitfield (script export witness)",
            "writer_native_1dc": hex(PINNED["package_flag_native_1dc"]),
            "writer_ret": "0x14 — args at [ebp+0x10..0x18] toggle bits 0/2/4 in +0xBC",
            "reader_sites_sample": ["0xF00BD0", "0xF00BF6", "0xF00C21", "0xF00CE2"],
            "semantics": "Tracks per-slot package request flags; updated when GFx calls vtable+0x1DC native (not equip commit)",
            "followup_1dc": hex(PINNED["package_followup_1dc"]),
            "followup_note": "After flag change, may call GGameSystem vtable+0x160 with mActiveWeaponSlot scalar (+0xB8)",
        },
        "next_witness_candidates_ranked": [
            "Option 3: bounded log of LoadEquippedWeapons / Initialize GFx invoke strings (seeded list once)",
            "Option 2: read-only log on vtable+0x1DC (F00B60) when LEFT/RIGHT changes mRequestedPackages",
            "Option 1: session-open scan all catalog rows wchar at +0 and +0xC (not wp_SP substring only)",
        ],
        "do_not_use": ["equip@F00F50 for preview navigation"],
    }
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
