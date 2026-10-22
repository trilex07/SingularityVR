"""Resolve WeaponLocker LoadUpgrades/Upgrade exec bodies from PE registration + .text ABI.

These execs are NOT in the WeaponLocker vtable (unlike Start/PopMenu/Load/equip).
Hooks target the registered native exec entry points in archived .text.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from inspect_inventory_registration import resolve_wrapper
from inspect_native_reload import PE


def primary_ret_imm(code, origin):
    rets = []
    i = 0
    while i < len(code):
        if code[i] == 0xC3:
            rets.append(0)
            i += 1
        elif code[i] == 0xC2 and i + 2 < len(code):
            rets.append(code[i + 1] | (code[i + 2] << 8))
            i += 3
        else:
            i += 1
    if not rets:
        return None
    for r in reversed(rets):
        if r != 0:
            return r
    return 0


def inspect(exe_bytes, text, base):
    pe = PE(exe_bytes)
    table = 0x019A0948
    vtable_core = {
        hex(off): hex(struct.unpack("<I", pe.read(table + off, 4))[0])
        for off in (0x11C, 0x190, 0x1F4, 0x1F8)
    }
    expected_core = {
        "0x11c": "0xf11430",
        "0x190": "0xf17f60",
        "0x1f4": "0xf23840",
        "0x1f8": "0xf00f50",
    }
    if vtable_core != expected_core:
        raise ValueError(f"core vtable drift: {vtable_core}")

    methods = {}
    for short in ("LoadUpgrades", "Upgrade"):
        name = f"intURvUIGFxMovie_WeaponLockerexec{short}"
        reg = resolve_wrapper(pe, text, base, name)
        body = reg["wrapper"]
        if not (base <= body <= base + len(text) - 16):
            raise ValueError(f"{short} body outside text")
        chunk = text[body - base : body - base + 16]
        if chunk[:3] != b"\x55\x8b\xec":
            raise ValueError(f"{short} prologue changed")
        ret = primary_ret_imm(text[body - base : body - base + 0x200], body)
        methods[short] = {
            "registration": name,
            "body_va": hex(body),
            "prefix16_sha256": hashlib.sha256(chunk).hexdigest(),
            "primary_ret_imm": ret,
            "hook_typedef": "ScriptExecFn",
            "in_weapon_locker_vtable": False,
        }
    return {
        "schema": "singularityvr-locker-page-methods-v1",
        "text_sha256": hashlib.sha256(text).hexdigest(),
        "exe_sha256": hashlib.sha256(exe_bytes).hexdigest(),
        "vtable": hex(table),
        "vtable_core_slots": vtable_core,
        "methods": methods,
        "conclusion": "LoadUpgrades/Upgrade are registered native exec entry points (not vtable slots). "
        "Hook body_va after prefix hash verification at install.",
    }


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--exe", type=Path, required=True)
    ap.add_argument("--text", type=Path, required=True)
    ap.add_argument("--text-base", type=lambda x: int(x, 0), default=0x402000)
    args = ap.parse_args()
    print(json.dumps(inspect(args.exe.read_bytes(), args.text.read_bytes(), args.text_base), indent=2))
