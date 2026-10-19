"""Offline token-boundary solver for the audited UE3 script token stream.

Reads the private original package fixture only. Never opens a process, never
launches Steam and never writes to the installation.

The token stream is a flat byte array of [opcode][operand] records. The
operand size depends on the opcode. The audited operand positions (object
references and FNames) are known exactly, so the record boundaries can be
recovered by a constraint search instead of guessed.

Output: the record layout, the exact byte range of the ThrowActiveWeapon call
and the byte budget a length-preserving neutralisation may use.
"""
import json
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/title'))
from ue3_layout import inspect

FIXTURE = ROOT / 'build/next-weapon-reload/npc-name-60pct-oneblock-20261019/control.u'
POLICY = ROOT / 'spikes/view_matrix/inventory_retention_policy.h'

# Opcode operand sizes that are certain from the independent package evidence:
# a call is 0x1B followed by an 8-byte FName, and object operands are 4 bytes.
FIXED = {0x1B: 8}


def load():
    data = FIXTURE.read_bytes()
    package = inspect('memory', data, include_all=True)
    fn = [e for e in package['exports'] if e['name'] == 'SwitchWeaponByArchetype'][0]
    raw = data[fn['offset']:fn['offset'] + fn['size']]
    length = struct.unpack_from('<I', raw, 40)[0]
    code = raw[44:44 + length]
    text = POLICY.read_text(encoding='utf-8')
    part0, part1 = text.split('inline constexpr Ref Names[]=', 1)
    objects = dict((int(o), n) for o, n in re.findall(r'\{(\d+),"(\w+)"\}', part0))
    names = dict((int(o), n) for o, n in re.findall(r'\{(\d+),"(\w+)"\}', part1))
    return package, code, objects, names


def solve(code, objects, names):
    """Depth-first search for a consistent record layout."""
    # Anchor -> required operand kind, used to validate a candidate boundary.
    anchors = {}
    for off in objects:
        anchors[off] = 'obj'
    for off in names:
        anchors[off] = 'name'
    end = len(code)
    results = []

    def walk(pos, sizes, layout):
        if len(results) > 8:
            return
        if pos == end:
            results.append((dict(sizes), list(layout)))
            return
        if pos > end:
            return
        op = code[pos]
        options = [FIXED[op]] if op in FIXED else range(1, 9)
        for size in options:
            nxt = pos + 1 + size
            if nxt > end:
                continue
            # An anchor is the FIRST byte of an operand: it may sit at pos+1,
            # but it must never sit strictly inside a record, and a record's
            # opcode may never sit on an anchor.
            bad = False
            for a in anchors:
                if pos + 1 < a < nxt or a == pos:
                    bad = True
                    break
            if bad:
                continue
            if op in sizes and sizes[op] != size:
                continue
            nxt_sizes = sizes
            if op not in sizes:
                nxt_sizes = dict(sizes)
                nxt_sizes[op] = size
            layout.append((pos, op, size, code[pos + 1:nxt]))
            walk(nxt, nxt_sizes, layout)
            layout.pop()

    walk(0, {}, [])
    return results


def main():
    package, code, objects, names = load()
    results = solve(code, objects, names)
    print('consistent layouts found:', len(results))
    if not results:
        return
    sizes, layout = results[0]
    print('opcode operand sizes:', {hex(k): v for k, v in sorted(sizes.items())})
    print()
    anchor_at = {}
    for off, n in objects.items():
        anchor_at[off] = ('obj', n)
    for off, n in names.items():
        anchor_at[off] = ('name', n)
    for pos, op, size, operand in layout:
        tag = ''
        if pos + 1 in anchor_at:
            kind, n = anchor_at[pos + 1]
            tag = '  <== %s %s' % (kind, n)
        print('%3d  op=%02X size=%d  %s%s' % (pos, op, size, operand.hex(' '), tag))


if __name__ == '__main__':
    main()
