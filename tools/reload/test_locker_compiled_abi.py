"""Check the emitted x86 factory call, not C++ spelling. No game access."""
import pathlib
import re
import sys
import pefile
import capstone

dll = pathlib.Path(sys.argv[1])
pe = pefile.PE(str(dll))
assert pe.FILE_HEADER.Machine == 0x14c and pe.OPTIONAL_HEADER.Magic == 0x10b
mapping = dll.with_suffix('.map').read_text()
line = next(x for x in mapping.splitlines() if '?TryPickupOrdinary@LockerVrLoadoutAcquire@' in x)
va = int(re.search(r'\s([0-9a-fA-F]{8}) f\s', line).group(1), 16)
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
ins = list(md.disasm(pe.get_data(va - pe.OPTIONAL_HEADER.ImageBase, 0x400), va))
slots = [i for i, x in enumerate(ins) if x.mnemonic == 'mov' and x.op_str == 'eax, dword ptr [eax + 0x20c]']
assert len(slots) == 1, 'factory must use literal vtable displacement 0x20c'
i = slots[0]
assert ins[i+1].mnemonic == 'call' and ins[i+1].op_str == 'eax'
assert ins[i-1].op_str == 'eax, dword ptr [ecx]'
prefix = [(x.mnemonic, x.op_str) for x in ins[max(0,i-14):i]]
assert ('sub', 'esp, 8') in prefix and ('mov', 'ecx, esp') in prefix
j = next(j for j,x in enumerate(prefix) if x == ('sub', 'esp, 8'))
assert prefix[j-2:j] == [('push','1'),('push','0')]
assert prefix[j+1:j+4] == [('mov','ecx, esp'),('push','1'),('push','1')]
for x in ins[i-13:i+4]:
    print(f'{x.address:08x} {x.mnemonic} {x.op_str}')
print('PASS: PE32 factory literal displacement and FName stack setup')
