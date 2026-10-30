"""Read-only verification of owned archived UE3 code; no process or game writes."""
import argparse
import hashlib
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32


def verify(text, base=0x402000):
    decoder=Cs(CS_ARCH_X86,CS_MODE_32)
    def block(address,size,sha):
        raw=text[address-base:address-base+size]
        assert hashlib.sha256(raw).hexdigest()==sha,hex(address)
        return {i.address:(i.mnemonic,i.op_str) for i in decoder.disasm(raw,address)}
    spawn=block(0xE7E5A0,431,'47218b694af0a504b038b6a1eb621c8413c2012c0471494af798be25a6cb8395')
    # Descriptor starts EBP-A4. Native args 8..11 populate +24,+28,+2C,+30.
    for address,op in {
        0xE7E6C6:'ecx, dword ptr [ebp + 0x24]',0xE7E6D2:'dword ptr [ebp - 0x80], ecx',
        0xE7E6F1:'edx, dword ptr [ebp + 0x28]',0xE7E6FC:'dword ptr [ebp - 0x7c], edx',
        0xE7E6CF:'eax, dword ptr [ebp + 0x2c]',0xE7E6D8:'dword ptr [ebp - 0x78], eax',
        0xE7E6D5:'ecx, dword ptr [ebp + 0x30]',0xE7E6DE:'dword ptr [ebp - 0x74], ecx',
    }.items(): assert spawn[address]==('mov',op)
    pool=block(0xFFE550,3168,'d217a308971e159f85a9878c100547c979e657062692b4d7efbd67b6693ed040')
    assert pool[0xFFEB97]==('cmp','dword ptr [edi + 0x24], edx')
    assert pool[0xFFEB9A]==('je','0xffedbb') # null base bypasses attachment, not particle creation
    assert pool[0xFFED8B]==('mov','eax, dword ptr [eax + 0x42c]')
    attach=block(0xF02D20,160,'b6fe179e8ec78d80ba56438e7defe87b2026563263e2567f8ed68edb5f2e72b0')
    assert attach[0xF02D8D]==('mov','edx, dword ptr [edx + 0x270]')
    actor=block(0x896480,1259,'6ca83043ea251ce2b2b5dae1ba3000b88ec6171760e651ee7af8f1a58d87ba1b')
    # Found socket -> overwrite relative position/rotation using socket data.
    # No socket -> different branch computes world-to-bone relative transform.
    assert actor[0x8966A4]==('test','edi, edi')
    assert actor[0x8966A6]==('je','0x8966e6')
    assert actor[0x8966A8]==('fld','dword ptr [edi + 0x4c]')
    assert actor[0x8966AE]==('fstp','dword ptr [esi + 0x168]')
    adapter=block(0xEDDAC0,84,'cb5fcabcdab987843ee110775ba50ab2a82f529beea96009fcba38991571d1f3')
    assert adapter[0xEDDAFF]==('call','0x896480')
    assert adapter[0xEDDB11]==('ret','0x20')
    return 'PASS: source arguments -> pool attachment -> named socket overwrite; native detach ret32'


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('text',type=Path)
    print(verify(p.parse_args().text.read_bytes()))
