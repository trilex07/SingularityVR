"""Offline native-registration -> wrapper -> class-constructor -> vtable mapper.

Reads an owned PE, an ARCHIVED text capture and an already decompressed UE3
package. Never opens a process, launches Steam, patches an executable or outputs
proprietary function bodies. Narrow measured x86 forms reject ambiguity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'title'))
from ue3_layout import inspect


def u32(b, p):
    if p < 0 or p+4 > len(b):
        raise ValueError('truncated integer')
    return struct.unpack_from('<I', b, p)[0]


def only(rows, what):
    if len(rows) != 1:
        raise ValueError(f'{what}: expected one match, got {len(rows)}')
    return rows[0]


def occurrences(b, pattern):
    p = 0
    while (p := b.find(pattern, p)) >= 0:
        yield p
        p += 1


class PE:
    def __init__(self, b):
        if not 256 <= len(b) <= 64*1024*1024 or b[:2] != b'MZ':
            raise ValueError('not a bounded PE file')
        pe = u32(b, 60)
        if b[pe:pe+4] != b'PE\0\0' or pe+24 > len(b):
            raise ValueError('invalid PE header')
        machine, count = struct.unpack_from('<HH', b, pe+4)
        opt = struct.unpack_from('<H', b, pe+20)[0]
        if machine != 0x14c or not 1 <= count <= 32 or opt < 96 or pe+24+opt+count*40 > len(b):
            raise ValueError('unsupported PE layout')
        if struct.unpack_from('<H', b, pe+24)[0] != 0x10b:
            raise ValueError('requires x86 PE32')
        self.base = u32(b, pe+24+28)
        self.data = b
        self.sections = []
        for i in range(count):
            p = pe+24+opt+i*40
            name = b[p:p+8].rstrip(b'\0').decode('ascii')
            size, rva, raw_size, offset = struct.unpack_from('<4I', b, p+8)
            if offset+raw_size > len(b):
                raise ValueError('section outside file')
            self.sections.append((name, size, rva, raw_size, offset))

    def address(self, offset):
        matches = [self.base+rva+offset-raw for _, _, rva, n, raw in self.sections if raw <= offset < raw+n]
        return only(matches, 'file mapping')

    def read(self, va, n):
        matches = [raw+va-self.base-rva for _, _, rva, size, raw in self.sections
                   if self.base+rva <= va and va+n <= self.base+rva+size]
        p = only(matches, 'VA mapping')
        return self.data[p:p+n]


def wrapper_slot(b):
    # Inspect the short tail only; the independent C++ decoder checks the WHOLE
    # wrapper, parameter consumption and ret8 before allowing a runtime hook.
    if len(b) < 58 or b[:9] != bytes.fromhex('558bec8b4508ff4018'):
        raise ValueError('unknown wrapper prologue')
    if b[36:40] != bytes.fromhex('8b068b90') or b[44:58] != bytes.fromhex('8bceffd28b4d0c89015e5dc20800'):
        raise ValueError('unknown no-param bool dispatch')
    slot = u32(b, 40)
    if slot & 3 or not 0x100 <= slot <= 0x1000:
        raise ValueError('invalid virtual displacement')
    return slot


def registration(pe, text, base, name):
    pos = only(list(occurrences(pe.data, (name+'\0').encode('utf-16le'))), 'registration name')
    va = pe.address(pos)
    row = only([p for p in occurrences(pe.data, struct.pack('<I', va)) if p % 4 == 0], 'registration row')
    destination = pe.address(row)+4
    # Static initializer: mov eax, native-wrapper; mov [lookup-row+4],eax.
    assignments = [p for p in occurrences(text, b'\xa3'+struct.pack('<I', destination))
                   if p >= 5 and text[p-5] == 0xb8]
    p = only(assignments, 'registration initializer')
    wrapper = u32(text, p-4)
    if not base <= wrapper < base+len(text)-58:
        raise ValueError('wrapper outside archived text')
    return dict(row=pe.address(row), initializer=base+p-5, wrapper=wrapper,
                slot=wrapper_slot(text[wrapper-base:wrapper-base+58]))


def class_vtable(pe, text, base, name):
    pos = only(list(occurrences(pe.data, (name+'\0').encode('utf-16le'))), 'class name')
    va = pe.address(pos)
    push = only(list(occurrences(text, b'\x68'+struct.pack('<I', va))), 'class name push')
    candidates = []
    for p in range(max(0, push-48), push-4):
        if text[p] != 0x68:
            continue
        thunk = u32(text, p+1)
        if not base <= thunk < base+len(text)-32:
            continue
        raw = text[thunk-base:thunk-base+32]
        if raw[:12] != bytes.fromhex('558bec8b4d0885c974065de9'):
            continue
        constructor = thunk+16+struct.unpack_from('<i', raw, 12)[0]
        if not base <= constructor < base+len(text)-80:
            continue
        code = text[constructor-base:constructor-base+80]
        if code[:4] != bytes.fromhex('568bf1e8'):
            continue
        end = code.find(b'\xc3')
        if end < 0:
            continue
        stores = [u32(code, j+2) for j in occurrences(code[:end], b'\xc7\x06') if j+6 <= end]
        if len(stores) == 2:
            candidates.append(dict(name_push=base+push, thunk=thunk,
                                   constructor=constructor, vtable=stores[-1]))
    return only(candidates, 'class constructor/vtable')


def package_calls(path):
    data = Path(path).read_bytes()
    pkg = inspect(path, data=data, include_all=True)
    exports = {e['index']: e for e in pkg['exports']}
    begin = only([e for e in exports.values() if e['name'] == 'BeginFire' and
                  exports.get(e['outer'], {}).get('name') == 'Active' and
                  exports.get(exports.get(e['outer'], {}).get('outer'), {}).get('name') == 'RvWeaponShared'],
                 'RvWeaponShared.Active.BeginFire')
    raw = data[begin['offset']:begin['offset']+begin['size']]
    length = u32(raw, 40)
    if 44+length+15 != len(raw) or length < 2 or raw[44+length-1] != 0x53:
        raise ValueError('unsupported serialized function layout')
    code = raw[44:44+length]
    found = {}
    for name in ('HasAmmo', 'HasAnyAmmo', 'AttemptReload'):
        index = pkg['names'].index(name)
        needle = b'\x1b'+struct.pack('<ii', index, 0)
        found[name] = only(list(occurrences(code, needle)), name+' call')
    state_index = pkg['names'].index('WaitingForTriggerRelease')
    wait = only(list(occurrences(code, b'\x21'+struct.pack('<ii', state_index, 0))), 'fallback state')
    attempt = found['AttemptReload']
    # Stronger than a string hit: no-param call under unary bool-not, argument
    # terminators followed immediately by the fallback state transition.
    if code[attempt-1] != 0x81 or code[attempt+9:attempt+13] != b'\x16\x16\x16\x71' or wait != attempt+13:
        raise ValueError('unexpected AttemptReload/fallback expression')
    if not found['HasAmmo'] < found['HasAnyAmmo'] < attempt < wait:
        raise ValueError('unexpected fire/reload order')
    return dict(package_sha256=hashlib.sha256(data).hexdigest(), export=begin['index'],
                function='RvWeaponShared.Active.BeginFire', calls=found,
                fallback_name='WaitingForTriggerRelease', fallback_offset=wait,
                note='Narrow bytecode evidence, not a general decompiler or live execution proof')


def describe(exe, capture, package, base):
    pe = PE(Path(exe).read_bytes())
    text = Path(capture).read_bytes()
    section = only([s for s in pe.sections if s[0] == '.text'], '.text')
    if not text or len(text) != section[1] or base != pe.base+section[2]:
        raise ValueError('capture base/length does not match PE .text')
    reg = registration(pe, text, base, 'intARvWeaponSharedexecAttemptReload')
    cls = class_vtable(pe, text, base, 'RvWeaponShared')
    method = u32(pe.read(cls['vtable']+reg['slot'], 4), 0)
    if not base <= method < base+len(text)-0x1d3:
        raise ValueError('method outside capture')
    code = text[method-base:method-base+0x1d3]
    if code[:6] != bytes.fromhex('558bec83ec08') or code[-10:] != bytes.fromhex('8b45f85f5e5b8be55dc3'):
        raise ValueError('unreviewed native method body')
    return dict(scope='OFFLINE, no game process/file mutations, not runtime acceptance',
                exe_sha256=hashlib.sha256(pe.data).hexdigest(),
                text_sha256=hashlib.sha256(text).hexdigest(), text_path=str(Path(capture).resolve()),
                text_base=base, registration=reg, class_chain=cls, method=method,
                method_bytes=0x1d3, package=package_calls(package))


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--archived-text', type=Path, required=True)
    ap.add_argument('--package', type=Path, required=True)
    ap.add_argument('--text-base', type=lambda s: int(s, 0), required=True)
    ap.add_argument('--output', type=Path, required=True, help='private evidence JSON outside tracked source')
    args = ap.parse_args()
    report = describe(args.exe, args.archived_text, args.package, args.text_base)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, indent=2))
