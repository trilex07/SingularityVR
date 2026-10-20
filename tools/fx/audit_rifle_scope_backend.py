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
from inspect_native_reload import PE, occurrences, only, u32, class_vtable
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


def block_contract(text, base, address, length, expected):
    """Whole known extent, including branches after an early return."""
    start = address-base
    if start < 0 or length <= 0 or start+length > len(text):
        raise ValueError('complete block bounds')
    raw = text[start:start+length]
    if fnv(raw) != expected:
        raise ValueError('complete block fingerprint')
    return dict(bytes=length, fnv1a64=f'{expected:016X}', sha256=hashlib.sha256(raw).hexdigest())


def tracer_visibility(text, base, address):
    """Original tracer's random gate, NOT a collision/bullet gate.

    Whole body includes every branch; refuse a changed random/comparison path.
    Runtime reflection must additionally establish the field's semantic name.
    """
    receipt = block_contract(text, base, address, 1694, 0xB7DDF9B80E425AAB)
    at = address-base
    if (text[at+0x227:at+0x22b] != bytes.fromhex('837f4800') or
            text[at+0x24a] != 0xe8 or
            text[at+0x24f:at+0x258] != bytes.fromhex('d9474cdff1ddd80f86') or
            text[at+0x2d0:at+0x2d4] != bytes.fromhex('8b474850')):
        raise ValueError('tracer template/random/compare/spawn contract')
    random = address+0x24f+struct.unpack_from('<i', text, at+0x24b)[0]
    skip = address+0x25c+struct.unpack_from('<i', text, at+0x258)[0]
    if skip != address+0x2ec:
        raise ValueError('tracer chance skip destination')
    receipt.update(chance_field=0x4c, template_field=0x48,
                   random_helper=random, skip_offset=0x2ec,
                   note='cosmetic spawn is conditional; source getter runs before the random gate')
    return receipt


def capture_vtable(pe, text, base):
    # Measured native registration callback has a short tail jump to the
    # component ctor, unlike Raven's placement-new thunk. Refuse ambiguity.
    name = only(list(occurrences(pe.data, ('SceneCapture2DComponent\0').encode('utf-16le'))), 'capture class')
    push = only(list(occurrences(text, b'\x68'+struct.pack('<I', pe.address(name)))), 'capture class push')
    rows = []
    for pos in range(max(0, push-48), push-4):
        if text[pos] != 0x68:
            continue
        callback = u32(text, pos+1)
        try:
            block_contract(text, base, callback, 18, 0x9DB79AF8F2F197BB)
            off = callback-base
            if text[off+11] != 0xe9:
                continue
            ctor = callback+16+struct.unpack_from('<i', text, off+12)[0]
            block_contract(text, base, ctor, 134, 0xA1AFCED08AA559A7)
            if text[ctor-base+12:ctor-base+14] != b'\xc7\x06':
                continue
            rows.append(u32(text, ctor-base+14))
        except ValueError:
            continue
    return only(rows, 'capture ctor vtable')


def audit(exe, capture, engine, rifle):
    pe = PE(Path(exe).read_bytes())
    text = Path(capture).read_bytes()
    section = only([s for s in pe.sections if s[0] == '.text'], '.text')
    if len(text) != section[1]:
        raise ValueError('archive length does not match executable .text')
    base = pe.base+section[2]
    names = ('intURvWeaponFXexecPlayMuzzleFX', 'intURvWeaponFXexecPlayTracerFX',
             'intUSceneCapture2DComponentexecSetView',
             'intUSceneCapture2DComponentexecSetCaptureParameters',
             'intUSceneCaptureComponentexecSetFrameRate',
             'intUTextureRenderTarget2DexecCreate', 'intAActorexecAttachComponent',
             'intAActorexecDetachComponent', 'intAActorexecSetLocation',
             'intAActorexecSetRotation')
    methods = {}
    expected_wrappers = {
        'intURvWeaponFXexecPlayMuzzleFX': (197, 0x1153B073558EDE80),
        'intURvWeaponFXexecPlayTracerFX': (141, 0xF637C8C8C78F8557),
        'intUSceneCapture2DComponentexecSetView': (165, 0x6AE16D2B13BDB57C),
        'intUSceneCapture2DComponentexecSetCaptureParameters': (455, 0x3EE3F90D93363B69),
        'intUSceneCaptureComponentexecSetFrameRate': (95, 0x4E3E6769F20272E3),
        'intUTextureRenderTarget2DexecCreate': (496, 0x55FEB9FBD0D3F0A5),
        'intAActorexecAttachComponent': (91, 0xB40193EDB4C6A38B),
        'intAActorexecDetachComponent': (91, 0x87A7C6584939B487),
        'intAActorexecSetLocation': (121, 0xD75050CD7801E36B),
        'intAActorexecSetRotation': (109, 0xCB407E96A5DEE5B3),
    }
    for name in names:
        address = registration(pe, text, base, name)
        instructions, receipt = body(text, base, address)
        expected_bytes, expected_hash = expected_wrappers[name]
        if (receipt['bytes'], receipt['fnv1a64']) != (expected_bytes, f'{expected_hash:016X}'):
            raise ValueError(name+' full wrapper contract')
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
        ('emitter-location', 'intAActorexecSetLocation', 0x67, 29, 0x8A32507F9F7A934A),
        ('emitter-rotation', 'intAActorexecSetRotation', 0x5b, 139, 0xAA63F1F63DEFF3D8),
    ):
        receipt = helper_contract(text, base, pe.base+methods[name]['rva'], offset, length, expected)
        receipt['rva'] = receipt.pop('target') - pe.base
        helpers[stage] = receipt
    table = capture_vtable(pe, text, base)
    factory = u32(pe.read(table+0x164, 4), 0)
    block_contract(text, base, factory, 205, 0xC87668CD09D29B85)
    ctor = helper_contract(text, base, factory, 0xb7, 195, 0xF2ECDE1DE85EA63A)['target']
    if text[ctor-base+0x63:ctor-base+0x65] != b'\xc7\x06':
        raise ValueError('capture-info vtable store')
    info = u32(text, ctor-base+0x65)
    render = u32(pe.read(info+4, 4), 0)
    isolation = block_contract(text, base, render, 737, 0x9CCA1CFB4E9B6123)
    isolation.update(factory_rva=factory-pe.base, constructor_rva=ctor-pe.base, render_rva=render-pe.base,
                     target_field=0x10, note='own target equality required; not a size heuristic')
    resource_marker = bytes.fromhex('8bf18b466033db3998ac000000')
    resource_init = base + only(list(occurrences(text, resource_marker)), 'RT resource init') - 0x28
    resolve = block_contract(text, base, resource_init, 645, 0x65B4A8A44C0157E5)
    at = resource_init-base
    if (text[at+0xbb:at+0xbe] != bytes.fromhex('8d7e64') or
            text[at+0x159:at+0x15c] != bytes.fromhex('8d4e48')):
        raise ValueError('separate TextureRHI / SurfaceRHI storage contract')
    resolve.update(init_rva=resource_init-pe.base, texture_rhi=0x64, surface_rhi=0x48,
                   note='separate handles; no surface GetContainer dependency')
    weapon = class_vtable(pe, text, base, 'RvWeaponShared')
    # Native default ctor writes pure-interface tables first, implementations
    # last. Do not confuse it with the destructor's reversed sequence.
    block_contract(text, base, weapon['constructor'], 74, 0x76B208AF5D452BBD)
    spread_wrapper=registration(pe,text,base,'intARvWeaponSharedexecGetWeaponSpread')
    block_contract(text,base,spread_wrapper,58,0x63D37CBC1C2582C4)
    spread=u32(pe.read(weapon['vtable']+0x45c,4),0)
    # Includes the fallback branch AFTER the first return, unlike body().
    spread_receipt=block_contract(text,base,spread,117,0xA07CE9D71DC4EC2A)
    add_wrapper=registration(pe,text,base,'intARvWeaponSharedexecAddSpread')
    block_contract(text,base,add_wrapper,132,0x4BD340D01E7502E5)
    add_spread=u32(pe.read(weapon['vtable']+0x428,4),0)
    spread_receipt['add']=block_contract(text,base,add_spread,536,0x90E1F845D48FC976)
    spread_receipt.update(get_rva=spread-pe.base,add_rva=add_spread-pe.base,
                          note='original random radial spread precedes the mod collision-ray hook; do not zero it to hide an axis error')
    start = weapon['constructor']-base
    pattern = b'\xc7\x86\xd4\x02\x00\x00'
    stores = list(occurrences(text[start:start+74], pattern))
    if len(stores) != 2:
        raise ValueError('weapon source-interface construction/unwind stores')
    source_table = u32(text, start+stores[-1]+len(pattern))
    getter = u32(pe.read(source_table+8, 4), 0)
    tracer_source = block_contract(text, base, getter, 156, 0xDC2F29F0817B2902)
    fx_name = only(list(occurrences(pe.data, ('RvWeaponFX\0').encode('utf-16le'))), 'FX class')
    fx_push = only(list(occurrences(text, b'\x68'+struct.pack('<I', pe.address(fx_name)))), 'FX class push')
    fx_tables = []
    for pos in range(max(0, fx_push-48), fx_push-4):
        if text[pos] != 0x68:
            continue
        callback = u32(text, pos+1)
        try:
            block_contract(text, base, callback, 27, 0x64F1971B60F898FB)
            if text[callback-base+18:callback-base+20] == b'\xc7\x06':
                fx_tables.append(u32(text, callback-base+20))
        except ValueError:
            continue
    tracer = u32(pe.read(only(fx_tables, 'FX native vtable')+0x128, 4), 0)
    visibility = tracer_visibility(text, base, tracer)
    visibility['random_helper_rva'] = visibility.pop('random_helper')-pe.base
    if text[tracer-base+0x8c:tracer-base+0x8e] != b'\xff\xd2':
        raise ValueError('tracer source getter callsite')
    tracer_source.update(interface_offset=0x2d4, getter_rva=getter-pe.base,
                         tracer_rva=tracer-pe.base, caller_rva=tracer+0x8e-pe.base,
                         note='both return branches; endpoint never replaced')
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
    template = only([p['value'] for p in templates[0]['properties'] if p['name']=='mLoopingMuzzleFlash1stPerson'], 'native loop')
    loop = only([e['index'] for e in pkg['exports'] if e['name']==template and e['class_name']=='ParticleSystem'], 'loop export')
    materials = [dict(export=e['index'], properties=[p for p in e['properties']
                 if p['name'] in ('Material', 'bUseLocalSpace')]) for e in pkg['exports']
                 if e['outer']==loop and e['class_name']=='ParticleModuleRequired']
    # Preserve export/archetype identity: single-player and multiplayer FX
    # share display names but override tracer chance independently.
    chances = []
    raw_rifle = Path(rifle).read_bytes()
    for e in fx:
        for p in e.get('properties', []):
            if p['name'] == 'mTracerChance' and p['type'] == 'FloatProperty':
                if p['tag_bytes'] != 28:
                    raise ValueError('tracer chance FloatProperty size')
                chance = struct.unpack_from('<f', raw_rifle, p['tag_offset']+24)[0]
                if not 0 <= chance <= 1:
                    raise ValueError('tracer chance range')
                chances.append(dict(export=e['index'], outer=e['outer'],
                                    outer_name=pkg['records'][e['outer']-1]['name'],
                                    archetype=e['archetype'], chance=chance))
    weapon_defaults=[]
    for e in pkg['exports']:
        values={}
        for p in e.get('properties',[]):
            if 'Spread' not in p['name']: continue
            if p['type']=='FloatProperty' and p['tag_bytes']==28:
                values[p['name']]=struct.unpack_from('<f',raw_rifle,p['tag_offset']+24)[0]
            elif p['type']=='BoolProperty': values[p['name']]=p['value']
        if values: weapon_defaults.append(dict(export=e['index'],name=e['name'],
                                               archetype=e['archetype'],values=values))
    return dict(status='OFFLINE_STRUCTURAL_ONLY',
        exe_sha256=hashlib.sha256(pe.data).hexdigest(),
        text_sha256=hashlib.sha256(text).hexdigest(), methods=methods,
        reflected_contract=contract, helper_contract=helpers, rifle_fx=templates,
        capture_isolation=isolation, resolve_texture=resolve, tracer_source=tracer_source,
        tracer_visibility=visibility, tracer_chances=chances, native_spread=spread_receipt,
        weapon_spread_defaults=weapon_defaults, native_loop_materials=materials,
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
