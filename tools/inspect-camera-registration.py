"""Read-only string/xref inspection of owned PE and previously captured plaintext .text."""
import argparse
import struct
from pathlib import Path
import capstone

ap=argparse.ArgumentParser()
ap.add_argument('exe',type=Path)
ap.add_argument('text',type=Path)
ap.add_argument('names',nargs='+')
args=ap.parse_args()
pe=args.exe.read_bytes(); code=args.text.read_bytes()
nt=struct.unpack_from('<I',pe,0x3c)[0]
sections=struct.unpack_from('<H',pe,nt+6)[0]
optional=struct.unpack_from('<H',pe,nt+20)[0]
base=struct.unpack_from('<I',pe,nt+24+28)[0]
table=nt+24+optional
def va(offset):
    for j in range(sections):
        at=table+j*40
        rva,size,raw=struct.unpack_from('<III',pe,at+12)
        if raw<=offset<raw+size:return base+rva+offset-raw
    return None
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
for name in args.names:
    needle=(name+'\0').encode('utf-16le');pos=pe.find(needle)
    print('NAME',name)
    while pos>=0:
        address=va(pos)
        if address:
            print('string',hex(address))
            dataref=pe.find(struct.pack('<I',address))
            while dataref>=0:
                if va(dataref):
                    print('PE pointer table',hex(va(dataref)),pe[max(0,dataref-8):dataref+24].hex(' '))
                dataref=pe.find(struct.pack('<I',address),dataref+4)
            ref=code.find(struct.pack('<I',address))
            while ref>=0:
                # Context starts on x86 PUSH imm32, when present, for aligned decoding.
                start=ref-1 if code[ref-1]==0x68 else ref
                print('xref',hex(base+0x2000+ref))
                for ins in md.disasm(code[start:start+100],base+0x2000+start):
                    print(hex(ins.address),ins.mnemonic,ins.op_str)
                ref=code.find(struct.pack('<I',address),ref+4)
        pos=pe.find(needle,pos+len(needle))
