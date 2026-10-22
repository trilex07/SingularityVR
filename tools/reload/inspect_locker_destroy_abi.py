"""Offline ABI witness for RvInventoryManager::destroyItem and PopMenu callsite."""
import argparse
import json
import struct
from pathlib import Path

BASE_DEFAULT = 0x402000
DESTROY = 0x00A05790
POP_CALL = 0x00F18059
POP_RET = POP_CALL + 5


def scan_ret_imm(code, origin):
    for i in range(len(code)):
        if code[i] == 0xC3:
            return {"at": hex(origin + i), "imm": 0, "kind": "ret"}
        if code[i] == 0xC2 and i + 2 < len(code):
            imm = code[i + 1] | (code[i + 2] << 8)
            return {"at": hex(origin + i), "imm": imm, "kind": "ret imm"}
    return None


def popmenu_destroy_window(text, base):
    off = POP_CALL - base
    win = text[off - 0x20 : off + 0x20]
    call_idx = 0x20
    if win[call_idx] != 0xE8:
        raise ValueError("PopMenu destroy site is not E8 rel32")
    dest = POP_CALL + 5 + struct.unpack_from("<i", win, call_idx + 1)[0]
    after = POP_CALL + 5
    post = win[call_idx + 5 : call_idx + 8]
    return {
        "call_va": hex(POP_CALL),
        "return_va": hex(after),
        "callee": hex(dest),
        "post_call_bytes": post.hex(),
        "post_call_is_inc_edi": post[0] == 0x47,  # inc edi
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--text", type=Path, required=True)
    ap.add_argument("--text-base", type=lambda x: int(x, 0), default=BASE_DEFAULT)
    ap.add_argument("--strict", action="store_true")
    args = ap.parse_args()
    text = args.text.read_bytes()
    base = args.text_base
    destroy_code = text[DESTROY - base : DESTROY - base + 0xD0]
    if destroy_code[:3] != b"\x55\x8b\xec":
        raise ValueError("destroyItem prologue changed")
    primary = scan_ret_imm(destroy_code, DESTROY)
    pop = popmenu_destroy_window(text, base)
    report = {
        "destroyItem": {
            "va": hex(DESTROY),
            "convention": "__thiscall",
            "ecx": "inventory manager (PopMenu loads global before call)",
            "stack_args": ["item*", "int arg2 (PopMenu=0)", "int arg3 (PopMenu=1)"],
            "primary_return": primary,
            "neutral_return_eax": 0,
            "popmenu_uses_return": False,
        },
        "popmenu_destroy": pop,
        "policy_return_gate": hex(POP_RET),
    }
    ok = (
        pop["callee"] == hex(DESTROY)
        and pop["return_va"] == hex(POP_RET)
        and pop["post_call_is_inc_edi"]
        and primary
        and primary["kind"] == "ret imm"
        and primary["imm"] == 12
    )
    report["_witness_ok"] = ok
    print(json.dumps(report, indent=2))
    if args.strict and not ok:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
