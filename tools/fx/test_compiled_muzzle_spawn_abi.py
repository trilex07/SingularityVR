"""Check the actual optimized PE32 hook forwarding, not source spelling."""
import re
import sys
from pathlib import Path
import capstone
import pefile

p = Path(sys.argv[1])
pe = pefile.PE(str(p))
assert pe.FILE_HEADER.Machine == 0x14c and pe.OPTIONAL_HEADER.Magic == 0x10b
mapping = p.with_suffix('.map').read_text().splitlines()
hook = next(l for l in mapping if '?SpawnHook@NativeRifleFx@' in l and ' f ' in l)
original = next(l for l in mapping if '?spawnOriginal@NativeRifleFx@' in l)
va = int(re.search(r'\s([0-9a-fA-F]{8}) f\s', hook)[1], 16)
target = int(re.search(r'\s([0-9a-fA-F]{8})\s+d3d9.obj', original)[1], 16)
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
ins = list(md.disasm(pe.get_data(va-pe.OPTIONAL_HEADER.ImageBase, 0x400), va))
end = next(i for i, x in enumerate(ins) if x.mnemonic == 'ret')
ins = ins[:end+1]
assert ins[-1].op_str == '0x2c'
call = next(i for i, x in enumerate(ins) if x.mnemonic == 'call' and x.op_str == f'dword ptr [{target:#x}]')
start = call-1
while start >= 0 and ins[start].mnemonic not in ('jmp', 'xor'):
    start -= 1
args = ins[start+1:call]
assert sum(x.mnemonic == 'push' for x in args) == 11
assert any((x.mnemonic, x.op_str) == ('mov', 'ecx, esi') for x in args)
assert [(x.mnemonic,x.op_str) for x in ins[call+1:]] == [
    ('pop','edi'),('pop','esi'),('pop','ebp'),('pop','ebx'),('add','esp, 0x18'),('ret','0x2c')]
print('PASS: optimized PE32 SpawnHook forwards 11 DWORDs/ECX; trampoline cleans arguments; nonvolatile restoration and ret2C')
