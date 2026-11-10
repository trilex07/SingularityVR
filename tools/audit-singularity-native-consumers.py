#!/usr/bin/env python3
"""Resolve remaining EN -> German consumers in owned PEs without executing them.

Reports addresses, hashes and evidence only. PASS means static correspondence,
not ABI, object ownership, runtime compatibility or headset acceptance.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys

import capstone

spec = importlib.util.spec_from_file_location('input_mapper', Path(__file__).with_name('map-singularity-input.py'))
mapper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mapper)
sys.path.insert(0, str(Path(__file__).parent / 'reload'))
from inspect_inventory_registration import resolve_wrapper
from inspect_native_reload import PE

BASE = 0x400000


def call_target(image, va):
    p = va - BASE
    if p < 0 or p + 5 > len(image) or image[p] != 0xe8:
        raise ValueError(f'expected bounded direct call at {va:08X}')
    return va + 5 + struct.unpack_from('<i', image, p + 1)[0]


def texture_class(image, constructor, text):
    def word(va):
        p = va - BASE
        if p < 0 or p + 4 > len(image):
            raise ValueError('texture class operand outside image')
        return struct.unpack_from('<I', image, p)[0]

    vtable = word(constructor + 0x69)
    deleting_entry = word(vtable)
    if not BASE + text[0] <= deleting_entry <= BASE + sum(text) - 32:
        raise ValueError('texture deleting entry outside text')
    destructor = call_target(image, deleting_entry + 6)
    if not BASE + text[0] <= destructor <= BASE + sum(text) - 100:
        raise ValueError('texture destructor outside text')
    if image[destructor-BASE+0x29:destructor-BASE+0x2b] != b'\xc7\x06' or word(destructor+0x2b) != vtable:
        raise ValueError('texture constructor/destructor class identities disagree')
    return dict(vtable=f'{vtable:08X}', deletingEntry=f'{deleting_entry:08X}', destructor=f'{destructor:08X}')


def audit(reference, candidate):
    source, relocations, text = mapper.load(reference)
    dest, _, dest_text = mapper.load(candidate)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True

    def matches(va, size):
        pattern, fixed = mapper.pattern(source, relocations, va - BASE, size, md)
        return [m.start() + BASE for m in pattern.finditer(dest, dest_text[0], sum(dest_text))], fixed

    pe_source, pe_dest = PE(reference.read_bytes()), PE(candidate.read_bytes())
    rows = []
    for name, va, size in mapper.REMAINING_BLOCKS:
        found, fixed = matches(va, size)
        row = dict(name=name, reference=f'{va:08X}', bytes=size, fixedBytes=fixed,
                   normalizedCandidates=[f'{p:08X}' for p in found])
        if name in ('load_upgrades', 'upgrade'):
            symbol = 'intURvUIGFxMovie_WeaponLockerexec' + ('LoadUpgrades' if name == 'load_upgrades' else 'Upgrade')
            en = resolve_wrapper(pe_source, source[text[0]:sum(text)], BASE + text[0], symbol)
            de = resolve_wrapper(pe_dest, dest[dest_text[0]:sum(dest_text)], BASE + dest_text[0], symbol)
            if en['wrapper'] != va or de['wrapper'] not in found:
                raise ValueError(f'{name}: registration disagrees with normalized candidates')
            row['registration'] = dict(reference=en, candidate=de)
            selected = de['wrapper']
        else:
            if len(found) != 1:
                raise ValueError(f'{name}: expected one normalized match, got {len(found)}')
            selected = found[0]
        row.update(candidate=f'{selected:08X}', candidateSha256=hashlib.sha256(dest[selected-BASE:selected-BASE+size]).hexdigest().upper())
        rows.append(row)

    globals_ = []
    for name, va in mapper.REMAINING_GLOBALS[:-1]:
        refs = []
        # Only relocated absolute operands inside independently unique contexts.
        for at in sorted(relocations):
            if not text[0] + 24 <= at < sum(text) - 56 or struct.unpack_from('<I', source, at)[0] != va:
                continue
            found, _ = matches(BASE + at - 24, 80)
            if len(found) == 1:
                operand = found[0] + 24
                refs.append(dict(referenceOperand=f'{BASE+at:08X}', candidateOperand=f'{operand:08X}',
                                 target=f'{struct.unpack_from("<I", dest, operand-BASE)[0]:08X}'))
                if len(refs) == 4:
                    break
        if not refs or len({r['target'] for r in refs}) != 1:
            raise ValueError(f'{name}: missing or disagreeing references')
        globals_.append(dict(name=name, reference=f'{va:08X}', references=refs))

    # A constructor template alone has four lookalikes. Two larger, unique
    # allocation callers must independently identify the SAME specialization.
    callers, constructors = [], set()
    for start, call, size in ((0x5140e0, 0x51426e, 498), (0x514910, 0x5149da, 302)):
        found, fixed = matches(start, size)
        if len(found) != 1 or call_target(source, call) != 0x520030:
            raise ValueError('texture allocation caller is not independently unique')
        mapped_call = found[0] + call - start
        ctor = call_target(dest, mapped_call)
        constructors.add(ctor)
        callers.append(dict(reference=f'{start:08X}', candidate=f'{found[0]:08X}', bytes=size,
                            fixedBytes=fixed, referenceCall=f'{call:08X}', candidateCall=f'{mapped_call:08X}',
                            candidateConstructor=f'{ctor:08X}'))
    if len(constructors) != 1:
        raise ValueError('texture allocation callers disagree')
    ctor = constructors.pop()
    for image, address in ((source, 0x520030), (dest, ctor)):
        if image[address-BASE+0x67:address-BASE+0x69] != b'\xc7\x06':
            raise ValueError('constructor final vtable store shape changed')
    en_vt = struct.unpack_from('<I', source, 0x520030-BASE+0x69)[0]
    de_vt = struct.unpack_from('<I', dest, ctor-BASE+0x69)[0]
    if en_vt != 0x17b441c:
        raise ValueError('reference texture identity changed')
    en_class = texture_class(source, 0x520030, text)
    de_class = texture_class(dest, ctor, dest_text)
    globals_.append(dict(name='rhi_texture_vtable', reference=f'{en_vt:08X}', candidate=f'{de_vt:08X}',
                         referenceConstructor='00520030', candidateConstructor=f'{ctor:08X}', callers=callers,
                         referenceClass=en_class, candidateClass=de_class))
    return dict(status='STATIC_CORRESPONDENCE_PASS', execution='NOT RUN',
                referenceSha256=hashlib.sha256(reference.read_bytes()).hexdigest().upper(),
                candidateSha256=hashlib.sha256(candidate.read_bytes()).hexdigest().upper(),
                blocks=rows, globals=globals_,
                limitation='Static mapping only. Runtime initialization remains separately gated.')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('reference', type=Path)
    ap.add_argument('candidate', type=Path)
    ap.add_argument('--output', required=True, type=Path)
    args = ap.parse_args()
    try:
        result = audit(args.reference, args.candidate)
    except (ValueError, struct.error, StopIteration) as error:
        result = dict(status='REFUSED', execution='NOT RUN', error=str(error))
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(result['status'])
    return 0 if result['status'] == 'STATIC_CORRESPONDENCE_PASS' else 2


if __name__ == '__main__':
    raise SystemExit(main())
