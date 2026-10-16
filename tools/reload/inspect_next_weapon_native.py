"""Offline inheritance proof for the THREE weapon native reload entry points.

Explicit private original inputs only. No process access, game launch or install.
Addresses are evidence metadata, NEVER compiled into the runtime.
"""
import argparse
import hashlib
import json
from pathlib import Path
from inspect_native_reload import PE, class_vtable, registration, package_calls, u32
from inspect_native_reload import inspect

def common_method(rows):
    if len(rows) != 3 or {r['class_name'] for r in rows} != {
        'RvWeaponShared', 'RvWeaponShared_Shotgun', 'RvWeaponShared_SniperRifle'}:
        raise ValueError('missing/duplicate audited class')
    if len({r['method'] for r in rows}) != 1:
        raise ValueError('classes override AttemptReload; single-hook policy invalid')
    return rows[0]['method']


def package_inheritance(package):
    exports = inspect(package, include_all=True)['exports']
    classes = []
    for name in ('RvWeaponShared', 'RvWeaponShared_Shotgun', 'RvWeaponShared_SniperRifle'):
        matches = [e for e in exports if e['name'] == name and e['cls'] == 0]
        if len(matches) != 1:
            raise ValueError('missing/ambiguous class export')
        classes.append(matches[0])
    result = []
    for cls in classes[1:]:
        if cls['super_index'] != classes[0]['index']:
            raise ValueError('derived class no longer inherits Shared')
        children = [e for e in exports if e['outer'] == cls['index']]
        if any(e['class_name'] == 'State' or e['name'] in
               ('BeginFire', 'AttemptReload', 'ServerStartReload', 'RefillClip') for e in children):
            raise ValueError('derived script override requires review')
        result.append(dict(class_name=cls['name'], super_name=classes[0]['name'],
                           class_export=cls['index'], super_export=cls['super_index'],
                           direct_children=[e['name'] for e in children],
                           relevant_script_override=False))
    return result


def describe(exe, capture, package, base):
    pe = PE(Path(exe).read_bytes())
    text = Path(capture).read_bytes()
    sections = [s for s in pe.sections if s[0] == '.text']
    if len(sections) != 1 or len(text) != sections[0][1] or base != pe.base+sections[0][2]:
        raise ValueError('capture does not match PE .text')
    reg = registration(pe, text, base, 'intARvWeaponSharedexecAttemptReload')
    rows = []
    for name in ('RvWeaponShared', 'RvWeaponShared_Shotgun', 'RvWeaponShared_SniperRifle'):
        row = class_vtable(pe, text, base, name)
        method = u32(pe.read(row['vtable']+reg['slot'], 4), 0)
        if not base <= method < base+len(text)-0x1d3:
            raise ValueError('native method outside archived text')
        code = text[method-base:method-base+0x1d3]
        if code[:6] != bytes.fromhex('558bec83ec08') or code[-10:] != bytes.fromhex('8b45f85f5e5b8be55dc3'):
            raise ValueError('unreviewed method body')
        rows.append(dict(class_name=name, **row, method=method,
                         method_sha256=hashlib.sha256(code).hexdigest()))
    common_method(rows)
    return dict(scope='OFFLINE inheritance/ABI evidence; NOT live execution proof',
                exe_sha256=hashlib.sha256(pe.data).hexdigest(),
                text_sha256=hashlib.sha256(text).hexdigest(), registration=reg,
                classes=rows, shared_method=True, package=package_calls(package),
                script_inheritance=package_inheritance(package))


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    for key in ('exe', 'archived-text', 'package', 'output'):
        ap.add_argument('--'+key, type=Path, required=True)
    ap.add_argument('--text-base', type=lambda s: int(s, 0), required=True)
    args = ap.parse_args()
    report = describe(args.exe, args.archived_text, args.package, args.text_base)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, indent=2))
