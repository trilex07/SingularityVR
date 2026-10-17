"""Offline no-param native refill/getter evidence; no process or Steam access."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from inspect_native_reload import PE, only, occurrences, u32, class_vtable, wrapper_slot

SPECS = (('GetAmmoInClip', 53, 14, 0x814290bcd87ed31d),
         ('GetAmmoInBin', 53, 14, 0x9d0fbaa54f040869),
         ('GetClipSize', 77, 23, 0x837eb5e669b80c72),
         ('RefillClip', 164, 12, 0x5626d93f8679565f))


def fingerprint(raw, rel):
    if not 1 <= rel <= len(raw)-4 or raw[rel-1] != 0xe8:
        raise ValueError('not the audited cast call')
    h = 14695981039346656037
    for i, v in enumerate(raw):
        h = ((h ^ (0 if rel <= i < rel+4 else v))*1099511628211) & ((1 << 64)-1)
    return h


def void_slot(raw):
    if len(raw) < 53 or raw[44:53] != bytes.fromhex('8bceffd25e5dc20800'):
        raise ValueError('unknown void native wrapper')
    # Independently reuse the existing no-param integer-dispatch checker by
    # substituting only the result-store tail; the native ABI test checks both.
    return wrapper_slot(raw[:44]+bytes.fromhex('8bceffd28b4d0c89015e5dc20800'))


def describe(exe, capture, base):
    pe = PE(Path(exe).read_bytes())
    text = Path(capture).read_bytes()
    section = only([s for s in pe.sections if s[0] == '.text'], '.text')
    if len(text) != section[1] or base != pe.base+section[2]:
        raise ValueError('capture base/length mismatch')
    cls = class_vtable(pe, text, base, 'RvWeaponShared')
    rows = []
    casts = set()
    for name, size, rel, expected in SPECS:
        pos = only(list(occurrences(pe.data, ('intARvWeaponSharedexec'+name+'\0').encode('utf-16le'))), name)
        va = pe.address(pos)
        row = only([p for p in occurrences(pe.data, struct.pack('<I', va)) if p % 4 == 0], 'row')
        dest = pe.address(row)+4
        init = only([p for p in occurrences(text, b'\xa3'+struct.pack('<I', dest))
                     if p >= 5 and text[p-5] == 0xb8], 'initializer')
        wrapper = u32(text, init-4)
        if not base <= wrapper <= base+len(text)-58:
            raise ValueError('wrapper bounds')
        raw = text[wrapper-base:wrapper-base+58]
        slot = void_slot(raw) if name == 'RefillClip' else wrapper_slot(raw)
        method = u32(pe.read(cls['vtable']+slot, 4), 0)
        if not base <= method <= base+len(text)-size:
            raise ValueError('method bounds')
        raw = text[method-base:method-base+size]
        actual = fingerprint(raw, rel)
        if actual != expected:
            raise ValueError('different method/version: '+name)
        cast = method+rel+4+struct.unpack_from('<i', raw, rel)[0]
        casts.add(cast)
        rows.append(dict(name=name, wrapper=wrapper, slot=slot, method=method,
                         size=size, rel=rel, fingerprint=f'{actual:016x}',
                         return_type='void' if name == 'RefillClip' else 'int32'))
    if len(casts) != 1:
        raise ValueError('different inventory cast')
    return dict(scope='OFFLINE_STATIC_ONLY', exe_sha256=hashlib.sha256(pe.data).hexdigest(),
                text_path=str(Path(capture).resolve()), text_base=base,
                text_sha256=hashlib.sha256(text).hexdigest(),
                inventory_cast=only(list(casts), 'cast'), methods=rows)


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--archived-text', type=Path, required=True)
    ap.add_argument('--text-base', type=lambda s: int(s, 0), required=True)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args()
    report = describe(args.exe, args.archived_text, args.text_base)
    if args.output.exists():
        raise SystemExit('refusing to overwrite evidence')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, indent=2))
