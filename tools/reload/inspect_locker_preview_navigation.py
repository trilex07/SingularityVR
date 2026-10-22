"""WeaponLocker preview vs commit — offline .text witness (no game launch)."""
from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

TEXT_BASE = 0x402000

PINNED = {
    "equip_commit_hot": 0x00F00F50,
    "equip_script_full": 0x00F00FA0,
    "start": 0x00F11430,
    "load_equipped": 0x00F23840,
    "initialize": 0x00F237C0,
    "build_gfx_payload": 0x00F11570,
    "get_item_at": 0x00F23B60,
    "gfx_row_helper_208": 0x00F23CB0,
    "set_requested_packages_native": 0x00F00B60,
    "apply_requested_packages_native": 0x00F00C50,
    "refresh_subobject_vt_1dc": 0x00EF7370,
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
    slot_1dc = struct.unpack("<I", pe.read(vt + 0x1DC, 4))[0]

    equip_hot = text[PINNED["equip_commit_hot"] - TEXT_BASE : PINNED["equip_commit_hot"] - TEXT_BASE + 0x50]
    report = {
        "schema": "singularityvr-locker-preview-navigation-v2",
        "equip_at_f00f50": {
            "classification": "COMMIT — writes FName into mEquippedWeapons[slot*8] from script Weapon parm",
            "direct_call_sites_in_text": find_call_sites(text, PINNED["equip_commit_hot"]),
            "note": "Zero direct E8 calls; invoked only via GFx/script vtable slot 0x1F8.",
            "hot_body_writes_b0_slot": bytes([0x89, 0x04, 0x8A]) in equip_hot or bytes([0x89, 0x44, 0xC8]) in equip_hot,
            "full_script_at_f00fa0": "Uses movie vtable+0x208 (0xF23CB0) + inventory ownership before add",
        },
        "load_equipped_classification": {
            "code": "A",
            "meaning": "Iterates movie+0xB4 (equipped count) and mEquippedWeapons+0xB0 FNames only — not full catalog",
        },
        "navigation_hypothesis": {
            "verdict": "LIKELY_GFX_SIDE for left/right preview",
            "evidence": [
                "PSVR2 sweep produced zero equip@F00F50 hits while UI changed",
                "LoadEquippedWeapons (F23840) is classification A (equipped slots only)",
                "Initialize (F237C0) calls F11570 then script vtable+0x130 — strongest offered-list seed candidate",
                "Start (F11430) clears mRequestedPackages (+0xBC) and resizes mEquippedWeapons (+0xB0)",
            ],
        },
        "mRequestedPackages": {
            "offset": hex(PINNED["m_requested_packages_off"]),
            "type": "IntProperty bitfield (script export witness)",
            "writer_native_f00b60": hex(PINNED["set_requested_packages_native"]),
            "writer_ret": "0x14 — args at [ebp+0x10..0x18] toggle bits 0/1/2 in +0xBC; [ebp+0xC] float -> +0xB8",
            "reader_sites_sample": ["0xF00BD0", "0xF00BF6", "0xF00C21", "0xF00CE2", "0xF04740", "0xEFC0C5"],
            "semantics": "Package load/request state — not offeredOrdinaryMask",
            "apply_native_f00c50": hex(PINNED["apply_requested_packages_native"]),
            "followup_note": "F00C50 mirrors +0xBC into profile/game; GGameSystem subobject vtable+0x160/0x15C streaming",
        },
        "vtable_1dc_correction": {
            "slot": "0x1DC",
            "actual_impl": hex(slot_1dc),
            "expected_if_was_f00b60": False,
            "note": "F00B60 and F00C50 call vtable+0x1DC first; they are not the vtable slot itself",
        },
        "next_witness_candidates_ranked": [
            "Option 1: one-shot log Initialize -> F11570 GFx payload (offered list seed)",
            "Option 2: session-open catalog v2 contract (all 17 rows, full wchar, no wp_SP filter)",
            "Option 3: F00B60 only if runtime proves preview dispatches it (unproven offline)",
        ],
        "do_not_use": [
            "equip@F00F50 for preview navigation",
            "LoadEquippedWeapons as full offered-list witness",
            "mRequestedPackages as offeredOrdinaryMask",
        ],
    }
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
