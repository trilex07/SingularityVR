"""Fail-closed audit of the actual MSVC /O2 gun-scale adapter (DLL + MAP)."""
import re
import sys
from pathlib import Path
import capstone
from inspect_native_reload import PE

dll=Path(sys.argv[1]);pe=PE(dll.read_bytes());mapping=dll.with_suffix('.map').read_text()
def symbol(name):
    row=next(x for x in mapping.splitlines() if name in x and ' f ' in x)
    return int(re.search(r'\s([0-9a-fA-F]{8}) f\s',row)[1],16)
hook=symbol('?Hook@NativeDualTmdGunScale@');resolve=symbol('?Resolve@NativeDualTmdGunScale@')
instructions=[]
for i in capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32).disasm(pe.read(hook,128),hook):
    instructions.append(i)
    if i.mnemonic=='ret':break
code=[(i.mnemonic,i.op_str) for i in instructions]
# S=entry ESP. Original return is [S], bone[S+4], mesh[S+8]. Save EBX and
# local pointer; push mesh then bone. This remains in ECX. RET8 pops both.
assert code[0]==('push','ecx') and code[1][0]=='mov' and code[1][1].startswith('eax, dword ptr [0x')
assert code[2:9]==[('push','ebx'),('push','dword ptr [esp + 0x10]'),
    ('mov','dword ptr [esp + 8], eax'),('mov','ebx, ecx'),
    ('push','dword ptr [esp + 0x10]'),('call','dword ptr [esp + 0xc]'),
    ('mov','eax, dword ptr [esp + 8]')]
# Copy original ST(0) to float argument, not an integer or double; pass the
# ORIGINAL native return address to Resolve (not the trampoline return).
assert code[9:]==[('push','ecx'),('fstp','dword ptr [esp + 8]'),
    ('movss','xmm0, dword ptr [esp + 8]'),('movss','dword ptr [esp], xmm0'),
    ('push','eax'),('push','dword ptr [esp + 0x18]'),('push','dword ptr [esp + 0x18]'),
    ('push','ebx'),('call',hex(resolve)),('add','esp, 0x14'),('pop','ebx'),('pop','ecx'),('ret','8')]
for i in instructions:print(f'{i.address:08X} {i.mnemonic} {i.op_str}')
print('PASS compiled PE32 adapter: ECX=this, bone/mesh stack, original once, x87 float, caller witness, EBX restore, ret8')
