"""Read-only observed v584 RadioConversation -> cue -> wave inspector.

Consumes an already decoded user's package. Reports typed references, not
integer/substring guesses. No game writes, triggers, audio or generated assets.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / 'title'))
from ue3_layout import inspect


def conversations(path):
    data = Path(path).read_bytes()
    package = inspect(path, data=data, include_all=True)
    names = package['names']
    exports = {e['index']: e for e in package['exports']}
    imports = package['imports']

    def name(index):
        if not 0 <= index < len(names):
            raise ValueError('name outside table')
        return names[index]

    def ref(index):
        if index == 0:
            return dict(index=0, name='None', path='None')
        chain = []; seen = set(); cursor = index
        while cursor:
            if cursor in seen or len(seen) >= 64:
                raise ValueError('invalid outer chain')
            seen.add(cursor)
            if cursor > 0:
                e = exports[cursor]
                number = struct.unpack_from('<i', data, e['record_offset'] + 16)[0]
                leaf = e['name']; cursor = e['outer']
            else:
                item = imports[-cursor - 1]
                leaf = name(item[5]); number = item[6]; cursor = item[4]
            chain.append(leaf + ('_' + str(number - 1) if number else ''))
        return dict(index=index, name=chain[0], path='.'.join(reversed(chain)))

    def radio_array(prop):
        start = prop['tag_offset'] + 24
        end = start + prop['value']['bytes']
        count = struct.unpack_from('<i', data, start)[0]; cursor = start + 4
        if not 0 < count <= 64:
            raise ValueError('unobserved radio array count')
        rows = []
        for _ in range(count):
            row = {}
            while cursor + 8 <= end:
                ni, number = struct.unpack_from('<2i', data, cursor); cursor += 8
                key = name(ni)
                if number: raise ValueError('numbered radio property')
                if key == 'None': break
                ti, tn, size, array_index = struct.unpack_from('<4i', data, cursor); cursor += 16
                if tn or array_index or cursor + size > end or key in row:
                    raise ValueError('invalid radio property')
                typ = name(ti)
                if key == 'mCharacter' and typ == 'ByteProperty' and size == 8:
                    enum, num = struct.unpack_from('<2i', data, cursor)
                    if num: raise ValueError('numbered character enum')
                    row[key] = name(enum)
                elif key == 'mCharacterDelay' and typ == 'FloatProperty' and size == 4:
                    row[key] = struct.unpack_from('<f', data, cursor)[0]
                elif key == 'mCharacterSoundCue' and typ == 'ObjectProperty' and size == 4:
                    row[key] = ref(struct.unpack_from('<i', data, cursor)[0])
                else: raise ValueError('unobserved radio structure')
                cursor += size
            else: raise ValueError('unterminated radio entry')
            if set(row) != {'mCharacter', 'mCharacterDelay', 'mCharacterSoundCue'}:
                raise ValueError('incomplete radio entry')
            rows.append(row)
        if cursor != end: raise ValueError('radio array tail')
        return rows

    def waves(index, seen=None):
        seen = set() if seen is None else seen
        if index in seen or len(seen) >= 64: raise ValueError('sound chain guard')
        seen.add(index)
        if index <= 0:
            kind = name(imports[-index - 1][2]) if index < 0 else 'None'
            return [dict(**ref(index), class_name=kind,
                         status='import wave' if kind == 'SoundNodeWave' else 'import not traversed')]
        e = exports[index]
        if 'parse_error' in e: raise ValueError('sound-node property parse failed')
        if e['class_name'] == 'SoundNodeWave': return [dict(**ref(index), status='wave')]
        found = []
        for p in e['properties']:
            if p['name'] == 'FirstNode' and p['type'] == 'ObjectProperty':
                child = struct.unpack_from('<i', data, p['tag_offset'] + 24)[0]
                if child: found.extend(waves(child, set(seen)))
            elif p['name'] == 'ChildNodes' and p['type'] == 'ArrayProperty':
                at = p['tag_offset'] + 24; size = p['value']['bytes']
                count = struct.unpack_from('<i', data, at)[0]
                if not 0 <= count <= 64 or size != 4 + count * 4:
                    raise ValueError('invalid sound children')
                for child in struct.unpack_from('<' + 'i' * count, data, at + 4):
                    if child: found.extend(waves(child, set(seen)))
        return found

    result = []
    for e in exports.values():
        if e['class_name'] != 'RvSeqAct_PlayerRadio': continue
        if 'parse_error' in e: raise ValueError('radio top-level parse failed')
        prop = next(p for p in e['properties'] if p['name'] == 'RadioConversation')
        rows = radio_array(prop)
        for row in rows:
            row['waves'] = waves(row['mCharacterSoundCue']['index'])
        result.append(dict(object=ref(e['index']), conversation=rows))
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('logical_package', type=Path)
    args = parser.parse_args()
    print(json.dumps(conversations(args.logical_package), indent=2))
