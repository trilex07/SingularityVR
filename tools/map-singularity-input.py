#!/usr/bin/env python3
"""Offline native input mapping between two owned PE32 images.

Requires pefile and capstone. Never loads/executes an image or emits game bytes.
Candidates are evidence for review, not automatically accepted runtime hooks.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path
import capstone
import pefile


BLOCKS = [
    ('pad_enum', 0x11d2fc0, 29), ('pad_callback', 0x11d2180, 0x313),
    ('ui_poll', 0x11d31c0, 128), ('get_x', 0x11cfc10, 48),
    ('get_y', 0x11cfc40, 48), ('get_pos', 0x11cfc70, 48),
    ('haptic_start', 0x100d790, 248), ('haptic_stop', 0x100aec0, 194),
    ('mouse_init', 0x11d7439, 90), ('mouse_poll', 0x11d325b, 1629),
    ('mouse_coop', 0x11d5f2b, 32), ('mouse_helper', 0x11cf3e0, 96),
]
GLOBALS = [
    ('engine', 0x1cf8778), ('viewports', 0x1d1fcc4), ('mouse_x', 0x1cf566c),
    ('mouse_y', 0x1cf5674), ('left_button', 0x1cf568c),
    ('pad_identity', 0x1d1fc7c), ('mouse_slot', 0x1d1fc78),
]
REMAINING_BLOCKS = [
    ('start',0xf11430,128),('pop',0xf17f60,128),('load',0xf23840,128),
    ('equip',0xf00f50,128),('load_upgrades',0xf87c80,128),('upgrade',0xf87e60,128),
    ('destroy',0xa05790,128),('pop_destroy_call',0xf18059,32),
    ('append_name',0x4affd0,64),('factory',0xfd2320,128),
    ('get_game_system',0xefa070,32),('has_weapon',0x102f700,128),
    ('scale_getter',0xb3e400,64),('scale_consumer',0xaec25d,26),
]
REMAINING_GLOBALS = [
    ('locker_vtable',0x19a0948),('inventory_vtable',0x19cb480),
    ('game_system_vtable',0x19cdbf8),('game_system_ptr',0x1cf394c),
    ('inventory_global',0x1cf74e4),('empty_string',0x179e4d0),
    ('rhi_surface_vtable',0x17b4474),('rhi_texture_vtable',0x17b441c),
]


def load(path):
    image = pefile.PE(str(path), fast_load=True)
    if image.FILE_HEADER.Machine != 0x14c or image.OPTIONAL_HEADER.ImageBase != 0x400000:
        raise ValueError('expected original-base x86 PE32 image')
    image.parse_data_directories(directories=[5])
    relocs = {e.rva for block in getattr(image, 'DIRECTORY_ENTRY_BASERELOC', [])
              for e in block.entries if e.type == 3}
    text = next(s for s in image.sections if s.Name.rstrip(b'\0') == b'.text')
    return image.get_memory_mapped_image(), relocs, (text.VirtualAddress, text.Misc_VirtualSize)


def pattern(image, relocs, start, size, disassembler):
    chunk = image[start:start+size]
    if len(chunk) != size:
        raise ValueError('pattern outside image')
    mask = bytearray([1]*size)
    for i in range(size):
        if start+i in relocs:
            mask[i:min(i+4, size)] = bytes(min(4, size-i))
    for ins in disassembler.disasm(chunk, start+0x400000):
        if ins.group(capstone.CS_GRP_JUMP) or ins.group(capstone.CS_GRP_CALL):
            first = ins.address-0x400000-start+ins.imm_offset
            for k in range(first, min(first+ins.imm_size, size)):
                mask[k] = 0
    raw = b''.join(re.escape(bytes([b])) if mask[i] else b'.' for i, b in enumerate(chunk))
    return re.compile(raw, re.DOTALL), sum(mask)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference', type=Path)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--scope', choices=['input','remaining'], default='input')
    args = parser.parse_args()
    source, relocations, text = load(args.reference)
    candidate, _, candidate_text = load(args.candidate)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    rows = []
    blocks, globals_ = (BLOCKS,GLOBALS) if args.scope=='input' else (REMAINING_BLOCKS,REMAINING_GLOBALS)
    for name, va, size in blocks:
        regex, fixed = pattern(source, relocations, va-0x400000, size, md)
        found = [m.start() for m in regex.finditer(candidate, candidate_text[0], sum(candidate_text))]
        row = dict(name=name, reference=f'{va:08X}', bytes=size, fixedBytes=fixed,
                   candidates=[f'{a+0x400000:08X}' for a in found])
        if len(found) == 1:
            row['candidateSha256'] = hashlib.sha256(candidate[found[0]:found[0]+size]).hexdigest().upper()
        rows.append(row)
    globals_report = []
    for name, va in globals_:
        refs = []
        for match in re.finditer(re.escape(struct.pack('<I', va)), source[text[0]:sum(text)]):
            at = match.start()+text[0]
            if at not in relocations:
                continue
            regex, _ = pattern(source, relocations, at-24, 80, md)
            found = list(regex.finditer(candidate, candidate_text[0], sum(candidate_text)))
            if len(found) != 1:
                continue
            operand = found[0].start()+24
            dest = struct.unpack_from('<I', candidate, operand)[0]
            refs.append(dict(referenceOperand=f'{at+0x400000:08X}',
                             candidateOperand=f'{operand+0x400000:08X}', target=f'{dest:08X}'))
            if len(refs) == 4:
                break
        globals_report.append(dict(name=name, reference=f'{va:08X}', references=refs,
                                   agrees=bool(refs) and len({r['target'] for r in refs}) == 1))
    passed = all(len(r['candidates']) == 1 for r in rows) and all(r['agrees'] for r in globals_report)
    result = dict(status='PASS' if passed else 'REVIEW_REQUIRED', execution='NOT RUN',
                  referenceSha256=hashlib.sha256(args.reference.read_bytes()).hexdigest().upper(),
                  candidateSha256=hashlib.sha256(args.candidate.read_bytes()).hexdigest().upper(),
                  blocks=rows, globals=globals_report,
                  limitation='Unique normalized matches require ABI/ownership review and runtime acceptance.')
    args.output.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(result['status'], f'{len(rows)} code blocks, {len(globals_report)} globals; no image execution')
    return 0 if passed else 2


if __name__ == '__main__':
    raise SystemExit(main())
