"""Disassembly-driven reconstruction of GGameSystem catalog row identity (FD5AC0).

Uses archived Singularity.text-memory.bin only. Documents runtime 0x50 layout and
the exact compare path — NOT 'read wchar* at row+0' without FString rules.
"""
from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path

TEXT_BASE = 0x402000
FD5AC0 = 0x00FD5AC0
COPY_HELPER = 0x004AC340
STREQ_HELPER = 0x004409F0
ROW_STRIDE = 0x50

PSEUDOCODE = """
// uint32 __stdcall CatalogWeaponPathToBitMask(const wchar_t* queryData, int queryNum);  ret 8
// ECX on entry is NOT used by FD5AC0.
uint32 CatalogWeaponPathToBitMask(const wchar_t* queryData, int queryNum) {
    RvGameSystem* gs = GetGameSystem();  // call 0xEFA070
    if (!gs || gs->catalogCount_164 <= 0) return 0xFFFFFFFFu;
    for (int i = 0; i < gs->catalogCount_164; ++i) {
        uint8_t* row = gs->catalogData_160 + i * 0x50;
        if (row[0x24] != 0) continue;
        if (row[0x4C] & 1) continue;
        FString tmp;                      // [ebp-0x14]
        FString query;                    // views [ebp+8] queryData/queryNum
        CopyAssignFString(&tmp, &query);  // 0x4AC340 — NOT a raw memcpy from row
        if (FStringEquals(row, &tmp))    // 0x4409F0 — ecx=row FString, arg=tmp
            return 1u << i;
    }
    return 0xFFFFFFFFu;
}

// EnableAll (0xFFA950) builds query FString from row PRIMARY field only:
//   wchar_t* p = row[+4] ? row[+0] : defaultEmpty_179E4D0;
//   then pushes (p, len) into FD5AC0 — same identity field as the compare above.

// Unlock loop (0xF033F0) additionally reads SECONDARY FString at row+0x0C/+0x10
// for GGameSystem vtable+0x20C (display/alternate path); not used in FD5AC0 match.
"""


def verify_prologue(text: bytes) -> dict:
    chunk = text[FD5AC0 - TEXT_BASE : FD5AC0 - TEXT_BASE + 0xD0]
    return {
        "stride_0x50_add_edi": bytes([0x83, 0xC7, 0x50]) in chunk or bytes([0x81, 0xC7, 0x50, 0x00, 0x00, 0x00]) in chunk,
        "filter_byte_0x24": bytes([0x80, 0x78, 0x24, 0x00]) in chunk,
        "filter_dword_0x4c_bit0": bytes([0xF6, 0x40, 0x4C, 0x01]) in chunk,
        "call_4ac340_before_4409f0": chunk.find(bytes([0xE8])) < chunk.rfind(bytes([0xE8])),
        "success_shl_index": bytes([0xD3, 0xE0]) in chunk,
    }


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--text",
        type=Path,
        default=Path(r"C:\Users\Trilex\vr-mods\research\etw\capture-20261006-205508\Singularity.text-memory.bin"),
    )
    args = ap.parse_args()
    text = args.text.read_bytes()
    report = {
        "schema": "singularityvr-catalog-row-fd5ac0-v2",
        "runtime_row_stride": hex(ROW_STRIDE),
        "runtime_fields_proven": {
            "+0x00/+0x04": "FString primary identity (4409F0 compares row base as FString)",
            "+0x0C/+0x10": "FString secondary (0xF033F0 unlock / vtable+0x20C only)",
            "+0x24": "uint8 exclude-if-nonzero (FD5AC0, EnableAll, unlock loop)",
            "+0x4C": "uint32 exclude-if-bit0 (FD5AC0, EnableAll, unlock loop)",
        },
        "witness_correction": (
            "LockerAvailabilityTrace ReadFStringAscii(row+0) assumed plain inline path text. "
            "FD5AC0 uses FString layout and 4409F0; live PSVR2 run had count=17 but zero "
            "wp_SP_* exact matches — likely string content/encoding/pointer validity, NOT "
            "'row+0 is not FString'. Next witness must log bounded wchar at +0 and +0xC "
            "for all count rows."
        ),
        "helpers": {"copy_assign": hex(COPY_HELPER), "fstring_equals": hex(STREQ_HELPER)},
        "pseudocode": PSEUDOCODE.strip(),
        "prologue_checks": verify_prologue(text),
    }
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
