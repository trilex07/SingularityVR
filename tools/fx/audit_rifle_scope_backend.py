"""Read-only native FX / SceneCapture audit against owned, archived inputs.

Outputs identities and hashes, never proprietary disassembly or game assets.
No process access, Steam mutation, or guessed runtime addresses.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'reload'))
from inspect_native_reload import PE, occurrences, only, u32
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'title'))
from ue3_layout import inspect


def registration(pe, text, base, name):
    string = only(list(occurrences(pe.data, (name+'\0').encode('utf-16le'))), name)
    row = only([p for p in occurrences(pe.data, struct.pack('<I', pe.address(string)))
                if p % 4 == 0], name+' row')
    address = u32(pe.data, row+4)
    # Engine bindings are preinitialised in .rdata; Raven also uses static
    # initialisers. Accept only the measured mov-immediate / absolute-store form.
    if not address:
        dest = pe.address(row)+4
        assignments = [p for p in occurrences(text, b'\xa3'+struct.pack('<I', dest))
                       if p >= 5 and text[p-5] == 0xb8]
        at = only(assignments, name+' initializer')
        address = u32(text, at-4)
    if not base <= address < base+len(text):
        raise ValueError(name+' wrapper outside archived code')
    return address


def body(text, base, address, limit=4096):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    if not base <= address < base+len(text):
        raise ValueError('method outside archived code')
    start = address-base
    instructions = []
    for ins in Cs(CS_ARCH_X86, CS_MODE_32).disasm(text[start:start+limit], address):
        instructions.append(ins)
        if ins.mnemonic == 'ret':
            end = ins.address+ins.size-base
            raw = text[start:end]
            return instructions, dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest(),
                fnv1a64=f'{fnv(raw):016X}')
    raise ValueError('bounded method has no complete epilogue')


def fnv(raw):
    h = 14695981039346656037
    for byte in raw:
        h = ((h ^ byte)*1099511628211) & 0xffffffffffffffff
    return h


def direct_calls(instructions):
    return [int(i.op_str, 16) for i in instructions
            if i.mnemonic == 'call' and i.op_str.startswith('0x')]


def helper_contract(text, base, wrapper, offset, length, expected):
    """Decode the actual relative call; refuse truncation/opcode/hash drift."""
    pos = wrapper - base + offset
    if pos < 0 or pos + 5 > len(text) or text[pos] != 0xE8:
        raise ValueError('helper direct-call contract')
    target = wrapper + offset + 5 + struct.unpack_from('<i', text, pos + 1)[0]
    start = target - base
    if start < 0 or start + length > len(text):
        raise ValueError('helper code bounds')
    raw = text[start:start+length]
    if fnv(raw) != expected:
        raise ValueError('helper full-body fingerprint')
    return dict(bytes=length, fnv1a64=f'{expected:016X}', target=target)


def audit(exe, capture, engine, rifle):
    pe = PE(Path(exe).read_bytes())
    text = Path(capture).read_bytes()
    section = only([s for s in pe.sections if s[0] == '.text'], '.text')
    if len(text) != section[1]:
        raise ValueError('archive length does not match executable .text')
    base = pe.base+section[2]
    names = ('intURvWeaponFXexecPlayMuzzleFX',
             'intUSceneCapture2DComponentexecSetView',
             'intUSceneCapture2DComponentexecSetCaptureParameters',
             'intUSceneCaptureComponentexecSetFrameRate',
             'intUTextureRenderTarget2DexecCreate', 'intAActorexecAttachComponent',
             'intAActorexecDetachComponent')
    methods = {}
    for name in names:
        address = registration(pe, text, base, name)
        instructions, receipt = body(text, base, address)
        receipt.update(rva=address-pe.base,
                       direct_call_rvas=[v-pe.base for v in direct_calls(instructions)])
        methods[name] = receipt
    helpers = {}
    for stage, name, offset, length, expected in (
        ('construct', 'intUTextureRenderTarget2DexecCreate', 0x159, 528, 0x80DA10E4224515C4),
        ('init', 'intUTextureRenderTarget2DexecCreate', 0x1dd, 44, 0xEC5B63A8BE57148F),
        ('view', 'intUSceneCapture2DComponentexecSetView', 0x98, 246, 0x78C58D8F4194A021),
        ('rebuild', 'intUSceneCapture2DComponentexecSetCaptureParameters', 0x188, 373, 0x9D3D895A33EACBBD),
        ('rate', 'intUSceneCaptureComponentexecSetFrameRate', 0x52, 31, 0x67123D70BB9B9A26),
        ('attach', 'intAActorexecAttachComponent', 0x4e, 117, 0xA43F290EA3F897E0),
        ('detach', 'intAActorexecDetachComponent', 0x4e, 41, 0x04E0A3B0BBD603DB),
    ):
        receipt = helper_contract(text, base, pe.base+methods[name]['rva'], offset, length, expected)
        receipt['rva'] = receipt.pop('target') - pe.base
        helpers[stage] = receipt
    pkg = inspect(engine, include_all=True)
    classes = {e['index']: e for e in pkg['exports']}
    contract = {}
    for name in ('SceneCapture2DComponent', 'SceneCaptureComponent', 'TextureRenderTarget2D'):
        cls = only([e for e in classes.values() if e['name'] == name and e['cls'] == 0], name)
        contract[name] = [dict(name=e['name'], type=e['class_name']) for e in classes.values()
                          if e['outer'] == cls['index']]
    pkg = inspect(rifle, include_all=True)
    fx = [e for e in pkg['exports'] if e['class_name'] == 'RvWeaponFX']
    # The first-person effect is LOOPING and GPU-cloud authored, unlike the
    # accepted pistol's immediate user-pointer sprite draws. This distinction
    # is structural proof, not identification of a captured draw's visible pixels.
    templates = [dict(export=e['index'], properties=e.get('properties', [])) for e in fx
                 if any(p['name']=='mLoopingMuzzleFlash1stPerson'
                        for p in e.get('properties', []))]
    if len(templates) != 1:
        raise ValueError('ambiguous rifle first-person template')
    return dict(status='OFFLINE_STRUCTURAL_ONLY',
        exe_sha256=hashlib.sha256(pe.data).hexdigest(),
        text_sha256=hashlib.sha256(text).hexdigest(), methods=methods,
        reflected_contract=contract, helper_contract=helpers, rifle_fx=templates,
        warning='No live execution, completed RTT pixels, native emitter identity or PSVR2 proof.')


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    for key in ('exe', 'archived-text', 'engine', 'rifle', 'output'):
        ap.add_argument('--'+key, required=True, type=Path)
    a = ap.parse_args()
    result = audit(a.exe, a.archived_text, a.engine, a.rifle)
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print('Offline native FX/SceneCapture contracts audited; private receipt:', a.output)
