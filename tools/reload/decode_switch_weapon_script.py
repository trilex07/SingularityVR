"""Offline decode of the audited SwitchWeaponByArchetype script token stream.

Reads the private original package fixture only. Never opens a process and
never writes to the installation. Prints the token stream with the audited
operand positions resolved to names, so the throw expression can be located
exactly instead of guessed.
"""
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/title'))
from ue3_layout import inspect

FIXTURE = ROOT / 'build/next-weapon-reload/npc-name-60pct-oneblock-20261019/control.u'
POLICY = ROOT / 'spikes/view_matrix/inventory_retention_policy.h'


def policy_refs():
    text = POLICY.read_text(encoding='utf-8')
    objects, names = text.split('inline constexpr Ref Names[]=', 1)
    obj = {int(o): n for o, n in re.findall(r'\{(\d+),"(\w+)"\}', objects.split(';', 1)[0])}
    nam = {int(o): n for o, n in re.findall(r'\{(\d+),"(\w+)"\}', names.split(';', 1)[0])}
    return obj, nam


def main():
    data = FIXTURE.read_bytes()
    package = inspect('memory', data, include_all=True)
    fn = package['exports'][10581]
    raw = data[fn['offset']:fn['offset'] + fn['size']]
    length = struct.unpack_from('<I', raw, 40)[0]
    code = raw[44:44 + length]
    objects, names = policy_refs()

    print('function', fn['name'], 'size', fn['size'], 'code', length)
    print('outer', package['records'][fn['outer'] - 1]['name'])
    print()
    for base in range(0, len(code), 16):
        chunk = code[base:base + 16]
        print('%04X  %s  |%s|' % (base, ' '.join('%02X' % b for b in chunk),
                                  ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)))
    print()
    print('--- audited operands ---')
    for off, name in sorted(objects.items()):
        ref = struct.unpack_from('<i', code, off)[0]
        actual = (package['records'][ref - 1]['name'] if ref > 0 else
                  package['names'][package['imports'][-ref - 1][5]])
        print('obj  @%3d  %-24s -> %s' % (off, name, actual))
    for off, name in sorted(names.items()):
        index, number = struct.unpack_from('<II', code, off)
        print('name @%3d  %-24s -> %s (#%d)' % (off, name, package['names'][index], number))


if __name__ == '__main__':
    main()
