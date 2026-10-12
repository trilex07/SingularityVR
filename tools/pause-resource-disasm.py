"""Bounded offline disassembly of an owned, existing UE3 text snapshot.

Output is private game-derived evidence; never commit or redistribute it.
"""
import argparse
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

parser = argparse.ArgumentParser()
parser.add_argument("snapshot", type=Path)
parser.add_argument("addresses", nargs="+", type=lambda value: int(value, 16))
parser.add_argument("--base", type=lambda value: int(value, 16), default=0x402000)
parser.add_argument("--bytes", type=int, default=512)
args = parser.parse_args()
if not 1 <= args.bytes <= 4096:
    parser.error("--bytes must be between 1 and 4096")
blob = args.snapshot.read_bytes()
decoder = Cs(CS_ARCH_X86, CS_MODE_32)
for address in args.addresses:
    offset = address - args.base
    if offset < 0 or offset >= len(blob):
        parser.error(f"address {address:#x} outside snapshot")
    print(f"FUNCTION-CANDIDATE {address:#x}")
    for instruction in decoder.disasm(blob[offset:offset + args.bytes], address):
        print(f"{instruction.address:08x} {instruction.mnemonic} {instruction.op_str}")
        if instruction.mnemonic == "ret":
            break
