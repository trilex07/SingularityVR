"""Offline field-use discriminator; output stays in the private RE lab."""
import argparse
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_OP_MEM

p = argparse.ArgumentParser()
p.add_argument('snapshot', type=Path)
p.add_argument('begin', type=lambda x: int(x, 16))
p.add_argument('end', type=lambda x: int(x, 16))
p.add_argument('field', type=lambda x: int(x, 16))
a = p.parse_args()
b = a.snapshot.read_bytes()
c = Cs(CS_ARCH_X86, CS_MODE_32)
c.detail = True
c.skipdata = True
instructions = list(c.disasm(b[a.begin-0x402000:a.end-0x402000], a.begin))
hits = 0
for i, instruction in enumerate(instructions):
    if instruction.id == 0: continue
    if not any(op.type == CS_OP_MEM and op.mem.base and op.mem.disp == a.field for op in instruction.operands): continue
    print(f'FIELD-USE {instruction.address:#x}')
    for x in instructions[max(0, i-3):i+10]:
        print(f'{x.address:08x} {x.mnemonic} {x.op_str}')
    hits += 1
    if hits >= 128: break
