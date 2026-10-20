"""Offline ownership/ABI audit. No process access or installed-file writes.

Report hashes and semantic contracts, not original game disassembly/assets.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_rifle_scope_backend import (PE, only, occurrences, u32, class_vtable,
    registration, block_contract, helper_contract)


def pawn_table(pe, text, base):
    string = only(list(occurrences(pe.data, 'RvPlayerPawn\0'.encode('utf-16le'))), 'pawn name')
    push = only(list(occurrences(text, b'\x68'+struct.pack('<I', pe.address(string)))), 'pawn push')
    rows = []
    for pos in range(push-48, push-4):
        if text[pos] != 0x68:
            continue
        callback = u32(text, pos+1)
        start = callback-base
        if start < 0 or start+18 > len(text):
            continue
        raw = text[start:start+18]
        if raw[:12] != bytes.fromhex('558bec8b4d0885c974065de9'):
            continue
        ctor = callback+16+struct.unpack_from('<i', raw, 12)[0]
        code = text[ctor-base:ctor-base+78]
        if len(code) == 78 and code[:4] == bytes.fromhex('568bf1e8') and code[8:10] == b'\xc7\x06' and code[-1] == 0xc3:
            rows.append(u32(code, 10))
    return only(rows, 'pawn ctor vtable')


def audit(exe, archive):
    pe = PE(Path(exe).read_bytes())
    text = Path(archive).read_bytes()
    section = only([s for s in pe.sections if s[0] == '.text'], 'text')
    if len(text) != section[1]:
        raise ValueError('archive extent')
    base = pe.base+section[2]
    wrapper = registration(pe, text, base, 'intARvPlayerPawnexecIsAiming')
    block_contract(text, base, wrapper, 58, 0x580673B4EE303833)
    aim = u32(pe.read(pawn_table(pe, text, base)+0xa98, 4), 0)
    aim_receipt = block_contract(text, base, aim, 13, 0x2C74B5D4D5842879)
    sniper = class_vtable(pe, text, base, 'RvWeaponShared_SniperRifle')['vtable']
    tick = u32(pe.read(sniper+0x21c, 4), 0)
    tick_receipt = block_contract(text, base, tick, 357, 0xD504DE25001FD2C5)
    energy = helper_contract(text, base, tick, 0x147, 512, 0xF11C945D7721EDC5)
    at = energy['target']-base
    if text[at+0xc8:at+0xd4] != bytes.fromhex('8b108bc88b82980a0000ffd0'):
        raise ValueError('energy exact IsAiming return-site')
    toggle = tick+0x170
    toggle_receipt = block_contract(text, base, toggle, 87, 0x05EFAB4C30C5C71D)
    if text[toggle-base+0x4a] != 0xe9:
        raise ValueError('toggle stop tail jump')
    stop = toggle+0x4f+struct.unpack_from('<i', text, toggle-base+0x4b)[0]
    stop_receipt = block_contract(text, base, stop, 128, 0x4DB623B969EF1939)
    params = registration(pe, text, base, 'intUSceneCapture2DComponentexecSetCaptureParameters')
    rebuild = helper_contract(text, base, params, 0x188, 373, 0x9D3D895A33EACBBD)
    # Derive the native normalization constant from the actual x87 operand,
    # not an external UE3 version or a guessed multiplier.
    raw = text[rebuild['target']-base:rebuild['target']-base+373]
    constants = []
    for pos in occurrences(raw, bytes.fromhex('d80d')):
        address = u32(raw, pos+2)
        constants.append(struct.unpack('<f', pe.read(address, 4))[0])
    if .75 not in constants:
        raise ValueError('native square-capture normalization not proven')
    energy.pop('target')
    rebuild.pop('target')
    return dict(status='OFFLINE_STRUCTURAL_PASS',
        exe_sha256=hashlib.sha256(pe.data).hexdigest(),
        text_sha256=hashlib.sha256(text).hexdigest(), aim=aim_receipt,
        sniper_tick=tick_receipt, energy=energy, toggle=toggle_receipt,
        stop=stop_receipt, projection_rebuild=rebuild,
        energy_aim_return_offset=0xd4, projection_normalization=.75,
        limitation='No Steam execution, GPU timing, magnification or physical L3 acceptance implied')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True)
    parser.add_argument('--text', required=True)
    parser.add_argument('--report', required=True)
    args = parser.parse_args()
    result = audit(args.exe, args.text)
    Path(args.report).write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(result['status'])
