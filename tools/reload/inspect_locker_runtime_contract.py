"""Offline locker RUNTIME contract witness from an archived live .text capture.

The on-disk Singularity.exe is packed; vtable/registration identity therefore
needs the memory image (see inspect_locker_contract.py). This tool instead pins
the four locker methods inside the retained capture
(research/etw/capture-20261006-205508/Singularity.text-memory.bin, base
0x402000) by stable x86 byte encodings, and reports identity-only structure.
A matched contract is still NOT a four-slot implementation or runtime proof.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

ADDRESSES = {'Start': 0x00F11430, 'equip': 0x00F00F50,
             'PopMenu': 0x00F17F60, 'LoadEquippedWeapons': 0x00F23840}
SIZES = {'Start': 0x13B, 'equip': 0xA3, 'PopMenu': 0x42B, 'LoadEquippedWeapons': 0x100}
HELPERS = {'tarrayFind': 0x00447BA0, 'tarrayAppend': 0x0044B0F0,
           'destroyItem': 0x00A05790, 'spawnByName': 0x00F00FA0,
           'scriptCallback': 0x00F0B470, 'baseTail': 0x00EF72F0,
           'controllerSlotByte': 0xC0}


def calls(code, origin, target):
    out = []
    for i in range(len(code) - 5):
        if code[i] == 0xE8:
            dest = origin + i + 5 + struct.unpack_from('<i', code, i + 1)[0]
            if dest == target:
                out.append(origin + i)
    return out


def find_all(code, pattern):
    out, p = [], 0
    while (p := code.find(pattern, p)) >= 0:
        out.append(p)
        p += 1
    return out


def window(text, base, method):
    va = ADDRESSES[method]
    if not base <= va < base + len(text):
        raise ValueError(f'{method} outside capture; no conclusion')
    code = text[va - base:va - base + SIZES[method]]
    if not code.startswith(b'\x55\x8b\xec'):
        raise ValueError(f'{method} prologue changed; no conclusion')
    return va, code


def inspect_start(text, base):
    va, code = window(text, base, 'Start')
    # GetWeaponSlotName is controller vtable slot 0x740 bytes (index 0x1D0).
    getters = (find_all(code, b'\x8b\x92\x40\x07\x00\x00') +
               find_all(code, b'\x8b\x90\x40\x07\x00\x00'))
    pushes = [code[i + 1] for i in range(len(code) - 1)
              if code[i] == 0x6A and code[i + 1] in (0, 1)]
    appends = calls(code, va, HELPERS['tarrayAppend'])
    if len(getters) != 2 or sorted(pushes) != [0, 1] or len(appends) != 2:
        raise ValueError('Start slot contract changed; no conclusion')
    if b'\x8b\x87\xb4\x00\x00\x00' in code or b'\x8b\x47\xb4' in code:
        raise ValueError('Start unexpectedly reads equipped count; no conclusion')
    return dict(getter_dispatches=[hex(va + g) for g in getters[:2]],
                slot_indices=pushes, append_sites=[hex(a) for a in appends],
                conclusion='Start rebuilds equipped ONLY from controller slots 0 and 1')


def inspect_equip(text, base):
    va, code = window(text, base, 'equip')
    if calls(code, va, HELPERS['tarrayAppend']) or calls(code, va, HELPERS['tarrayFind']):
        raise ValueError('equip allocates or searches; contract changed; no conclusion')
    slot = find_all(code, b'\x8b\x4d\x0c')
    store = find_all(code, b'\x89\x14\xc8')  # mov [eax+ecx*8], edx
    ret = find_all(code, b'\xc2\x0c\x00')
    if not slot or not store or not ret:
        raise ValueError('equip unbounded-write shape changed; no conclusion')
    if code.find(b'\x3b', slot[0], store[0]) >= 0 and b'\x83\xf9' in code[slot[0]:store[0]]:
        raise ValueError('equip grew a bounds check; re-audit; no conclusion')
    return dict(slot_source='[ebp+0xc] (script slotNum)', store_site=hex(va + store[0]),
                conclusion='equip writes equipped.data[slotNum] with no bounds check '
                           'and no spare-slot allocation; GFx slotNum > count-1 is a heap write')


def inspect_popmenu(text, base):
    va, code = window(text, base, 'PopMenu')
    setters = find_all(code, b'\x8b\x92\x44\x07\x00\x00')  # [edx+0x744] = slot 0x1D1
    destroys = calls(code, va, HELPERS['destroyItem'])
    finds = calls(code, va, HELPERS['tarrayFind'])
    spawns = calls(code, va, HELPERS['spawnByName'])
    # The new-equip script callback is pushed as an immediate and dispatched
    # through the generic 0x41F380 native->script bridge, never called directly.
    callback = find_all(code, b'\x68' + struct.pack('<I', HELPERS['scriptCallback']))
    dispatch = calls(code, va, 0x41F380)
    active = find_all(code, b'\x0f\xb6\x86\xc0\x00\x00\x00')  # movzx eax, byte [esi+0xC0]
    if len(setters) != 1 or len(destroys) != 1 or len(finds) != 2 or not active:
        raise ValueError('PopMenu commit contract changed; no conclusion')
    if len(spawns) != 1 or len(callback) != 1 or not dispatch:
        raise ValueError('PopMenu re-equip callback shape changed; no conclusion')
    return dict(setter_dispatch=hex(va + setters[0]), destroy_site=hex(destroys[0]),
                find_sites=[hex(f) for f in finds],
                callback_site=hex(va + callback[0]),
                active_slot_byte=HELPERS['controllerSlotByte'],
                conclusion='PopMenu writes controller slot i for EVERY equipped[i] '
                           '(growing equipped past 2 overwrites special slots 2/3); '
                           'destroys original weapons absent from equipped; byte +0xC0 '
                           're-hands the active slot after close')


def inspect_load(text, base):
    va, code = window(text, base, 'LoadEquippedWeapons')
    count = find_all(code, b'\x8b\x87\xb4\x00\x00\x00')  # mov eax, [edi+0xb4]
    data = find_all(code, b'\x8b\x81\xb0\x00\x00\x00')   # mov eax, [ecx+0xb0]
    pushes = [code[i + 1] for i in range(len(code) - 1) if code[i] == 0x6A]
    if not count or not data:
        raise ValueError('LoadEquippedWeapons array shape changed; no conclusion')
    if 0 in pushes and 1 in pushes:
        raise ValueError('LoadEquippedWeapons became a hardcoded two-iteration loop; no conclusion')
    return dict(count_field='+0xB4', data_field='+0xB0',
                conclusion='LoadEquippedWeapons enumerates the equipped TArray; it is '
                           'the load-time grant producer, not a fixed two-slot loop')


def inspect(text, base):
    if not text or not text.startswith(b'\x55\x8b\xec'):
        raise ValueError('capture does not start with a function prologue; wrong file?')
    report = dict(scope='OFFLINE capture-only identity; the packed disk PE cannot '
                       'verify vtables, so the memory-image tier (inspect_locker_contract) '
                       'remains the stronger witness',
                  text_sha256=hashlib.sha256(text).hexdigest(),
                  text_base=hex(base), methods={})
    for name, fn in (('Start', inspect_start), ('equip', inspect_equip),
                     ('PopMenu', inspect_popmenu), ('LoadEquippedWeapons', inspect_load)):
        report['methods'][name] = fn(text, base)
    return report


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--text', type=Path, required=True)
    ap.add_argument('--text-base', type=lambda x: int(x, 0), required=True)
    args = ap.parse_args()
    print(json.dumps(inspect(args.text.read_bytes(), args.text_base), indent=2))
