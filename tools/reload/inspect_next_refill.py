"""Prove the three original classes share ALL four refill/getter native methods."""
import argparse
import json
from pathlib import Path
from inspect_refill_only import describe as shared
from inspect_native_reload import PE, class_vtable, u32
from inspect_next_weapon_native import package_inheritance

CLASSES = ('RvWeaponShared', 'RvWeaponShared_Shotgun', 'RvWeaponShared_SniperRifle')


def shared_bindings(rows, methods):
    if len(rows) != 3 or {r['class_name'] for r in rows} != set(CLASSES):
        raise ValueError('unknown/missing/duplicate class')
    expected = {m['name']: m['method'] for m in methods}
    if len(expected) != 4 or any(r['methods'] != expected for r in rows):
        raise ValueError('native refill/getter override requires separate RE')
    return True


def describe(exe, capture, package, base):
    report = shared(exe, capture, base)
    pe = PE(Path(exe).read_bytes());text = Path(capture).read_bytes()
    rows = []
    for name in CLASSES:
        cls = class_vtable(pe, text, base, name)
        methods = {m['name']: u32(pe.read(cls['vtable']+m['slot'], 4), 0)
                   for m in report['methods']}
        rows.append(dict(class_name=name, vtable=cls['vtable'], methods=methods))
    report['shared_bindings'] = shared_bindings(rows, report['methods'])
    report['classes'] = rows
    report['script_inheritance'] = package_inheritance(package)
    return report


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    for key in ('exe', 'archived-text', 'package', 'output'):
        ap.add_argument('--'+key, type=Path, required=True)
    ap.add_argument('--text-base', type=lambda s: int(s, 0), required=True)
    args = ap.parse_args()
    report = describe(args.exe, args.archived_text, args.package, args.text_base)
    if args.output.exists():
        raise SystemExit('refusing to overwrite evidence')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, indent=2))
