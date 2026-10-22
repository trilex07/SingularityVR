"""Offline ABI witness for WeaponLocker native bodies (primary exit ret / prologue)."""
import argparse
import json
from pathlib import Path

ADDRESSES = {
    "Start": 0x00F11430,
    "equip": 0x00F00F50,
    "LoadEquippedWeapons": 0x00F23840,
    "PopMenu": 0x00F17F60,
}
# Scan windows must include the function epilogue (Start ret@+0x13A needs >0x13B).
SIZES = {
    "Start": 0x140,
    "equip": 0xA3,
    "PopMenu": 0x450,
    "LoadEquippedWeapons": 0x320,
}

EXPECTED = {
    "Start": {"primary_ret_imm": 4, "hook_typedef": "StartFn"},
    "equip": {"primary_ret_imm": 12, "hook_typedef": "EquipFn"},
    "LoadEquippedWeapons": {"primary_ret_imm": 4, "hook_typedef": "FrameFn"},
    "PopMenu": {"primary_ret_imm": 4, "hook_typedef": "FrameFn"},
}


def scan_ret_bytes(code, origin):
    out = []
    i = 0
    while i < len(code):
        if code[i] == 0xC3:
            out.append({"at": hex(origin + i), "kind": "ret", "imm": 0})
            i += 1
        elif code[i] == 0xC2 and i + 2 < len(code):
            imm = code[i + 1] | (code[i + 2] << 8)
            out.append({"at": hex(origin + i), "kind": "ret imm", "imm": imm})
            i += 3
        else:
            i += 1
    return out


def classify_primary(rets):
    if not rets:
        return "unknown", None
    primary = next((r for r in reversed(rets) if r["kind"] == "ret imm"), rets[-1])
    if primary["kind"] == "ret":
        return "thiscall-or-cdecl-0arg-stack", 0
    return f"stdcall-pop-{primary['imm']}-bytes", primary["imm"]


if __name__ == "__main__":
    import capstone

    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    ap = argparse.ArgumentParser()
    ap.add_argument("--text", type=Path, required=True)
    ap.add_argument("--text-base", type=lambda x: int(x, 0), default=0x402000)
    ap.add_argument("--strict", action="store_true")
    args = ap.parse_args()
    text = args.text.read_bytes()
    base = args.text_base
    report = {}
    ok = True
    for name, va in ADDRESSES.items():
        size = SIZES[name]
        code = text[va - base : va - base + size]
        if code[:3] != b"\x55\x8b\xec":
            raise ValueError(f"{name} prologue changed")
        rets = scan_ret_bytes(code, va)
        abi_class, imm = classify_primary(rets)
        exp = EXPECTED[name]
        match = imm == exp["primary_ret_imm"] if imm is not None else False
        if args.strict and not match:
            ok = False
        report[name] = {
            "va": hex(va),
            "size": size,
            "returns_tail": rets[-6:],
            "primary_ret_imm": imm,
            "abi_class": abi_class,
            "expected_hook_typedef": exp["hook_typedef"],
            "witness_ok": match,
        }
    report["_all_witness_ok"] = ok
    print(json.dumps(report, indent=2))
    if args.strict and not ok:
        raise SystemExit(1)
