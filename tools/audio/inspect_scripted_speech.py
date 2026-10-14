"""Read-only typed Matinee/PlaySound -> cue -> wave graph for observed UE3 v584.

Only consumes decoded user-owned packages; no game writes or runtime claims.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / 'title'))
from ue3_layout import inspect


class Graph:
    def __init__(self, path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        self.package = inspect(path, data=self.data, include_all=True)
        self.names = self.package['names']
        self.exports = {e['index']: e for e in self.package['exports']}
        self.imports = self.package['imports']

    def name(self, i):
        if not 0 <= i < len(self.names):
            raise ValueError('FName index outside table')
        return self.names[i]

    def ref(self, i):
        original = i
        chain, seen = [], set()
        kind = 'None'
        while i:
            if i in seen or len(seen) >= 64:
                raise ValueError('invalid outer chain')
            seen.add(i)
            if i > 0:
                e = self.exports[i]
                number = struct.unpack_from('<i', self.data, e['record_offset'] + 16)[0]
                leaf, outer, cls = e['name'], e['outer'], e['class_name']
            else:
                item = self.imports[-i - 1]
                leaf, number, outer, cls = self.name(item[5]), item[6], item[4], self.name(item[2])
            if len(chain) == 0:
                kind = cls
            chain.append(leaf + ('_' + str(number - 1) if number else ''))
            i = outer
        return dict(index=original, path='.'.join(reversed(chain)) or 'None', class_name=kind)

    def property_evidence(self, p):
        at, size = p['tag_offset'], p['tag_bytes']
        return dict(name=p['name'], type=p['type'], offset=at, bytes=size,
                    sha256=hashlib.sha256(self.data[at:at + size]).hexdigest())

    def properties(self, e):
        if 'parse_error' in e:
            raise ValueError('relevant object properties did not parse')
        return e.get('properties', [])

    def sound_entries(self, p):
        """Observed Sounds array: Time/Volume/Pitch floats and Sound object."""
        at = p['tag_offset'] + 24
        end = at + p['value']['bytes']
        count = struct.unpack_from('<i', self.data, at)[0]
        at += 4
        if not 0 <= count <= 1024:
            raise ValueError('unbounded sound entry count')
        rows = []
        for _ in range(count):
            row = {}
            terminated = False
            while at + 8 <= end:
                ni, number = struct.unpack_from('<2i', self.data, at)
                at += 8
                key = self.name(ni)
                if number:
                    raise ValueError('numbered sound field')
                if key == 'None':
                    terminated = True
                    break
                if at + 16 > end:
                    raise ValueError('truncated sound tag')
                ti, tn, size, ai = struct.unpack_from('<4i', self.data, at)
                at += 16
                if tn or ai or size != 4 or at + size > end or key in row:
                    raise ValueError('invalid sound field')
                typ = self.name(ti)
                if key in ('Time', 'Volume', 'Pitch') and typ == 'FloatProperty':
                    row[key] = struct.unpack_from('<f', self.data, at)[0]
                elif key == 'Sound' and typ == 'ObjectProperty':
                    row[key] = self.ref(struct.unpack_from('<i', self.data, at)[0])
                    row['sound_reference_offset'] = at
                else:
                    raise ValueError('unobserved sound structure')
                at += size
            if not terminated or not {'Time', 'Volume', 'Pitch', 'Sound'} <= row.keys():
                raise ValueError('incomplete sound entry')
            rows.append(row)
        if at != end:
            raise ValueError('sound array tail')
        return rows

    def facefx_entries(self, p):
        """Observed FaceFXSoundCueKeys tagged structures, not integer scan."""
        at = p['tag_offset'] + 24
        end = at + p['value']['bytes']
        count = struct.unpack_from('<i', self.data, at)[0]
        at += 4
        if not 0 <= count <= 1024:
            raise ValueError('unbounded FaceFX cue count')
        rows = []
        for _ in range(count):
            ni, nn, ti, tn, size, ai = struct.unpack_from('<6i', self.data, at)
            if (self.name(ni), self.name(ti), nn, tn, size, ai) != (
                    'FaceFXSoundCue', 'ObjectProperty', 0, 0, 4, 0):
                raise ValueError('unobserved FaceFX cue structure')
            at += 24
            row = dict(Sound=self.ref(struct.unpack_from('<i', self.data, at)[0]),
                       sound_reference_offset=at)
            at += 4
            ni, nn = struct.unpack_from('<2i', self.data, at)
            if self.name(ni) != 'None' or nn:
                raise ValueError('unterminated FaceFX cue structure')
            at += 8
            rows.append(row)
        if at != end:
            raise ValueError('FaceFX cue array tail')
        return rows

    def waves(self, i, seen=None):
        seen = set() if seen is None else seen
        if i in seen or len(seen) >= 64:
            raise ValueError('sound graph cycle or bound')
        seen.add(i)
        ref = self.ref(i)
        if i <= 0 or ref['class_name'] == 'SoundNodeWave':
            return [dict(**ref, status='import' if i < 0 else 'local')]
        result = []
        for p in self.properties(self.exports[i]):
            if p['name'] == 'FirstNode' and p['type'] == 'ObjectProperty':
                children = [struct.unpack_from('<i', self.data, p['tag_offset'] + 24)[0]]
            elif p['name'] == 'ChildNodes' and p['type'] == 'ArrayProperty':
                at = p['tag_offset'] + 24
                count = struct.unpack_from('<i', self.data, at)[0]
                if not 0 <= count <= 128 or p['value']['bytes'] != 4 + count * 4:
                    raise ValueError('invalid sound children')
                children = struct.unpack_from('<' + 'i' * count, self.data, at + 4)
            else:
                continue
            for child in children:
                if child:
                    for leaf in self.waves(child, set(seen)):
                        leaf.setdefault('edges', []).insert(0, dict(parent=ref, child=self.ref(child),
                                                                  property=self.property_evidence(p)))
                        result.append(leaf)
        return result

    def report(self):
        result = []
        for e in self.exports.values():
            if e['class_name'] not in ('InterpTrackSound', 'SeqAct_PlaySound', 'InterpTrackFaceFX'):
                continue
            for p in self.properties(e):
                if e['class_name'] == 'InterpTrackSound' and p['name'] == 'Sounds':
                    rows = self.sound_entries(p)
                elif e['class_name'] == 'SeqAct_PlaySound' and p['name'] == 'PlaySound':
                    rows = [dict(Sound=self.ref(struct.unpack_from('<i', self.data, p['tag_offset'] + 24)[0]))]
                elif e['class_name'] == 'InterpTrackFaceFX' and p['name'] == 'FaceFXSoundCueKeys':
                    rows = self.facefx_entries(p)
                else:
                    continue
                for row in rows:
                    row['waves'] = self.waves(row['Sound']['index']) if row['Sound']['index'] else []
                result.append(dict(producer=self.ref(e['index']), outer=self.ref(e['outer']),
                                   property=self.property_evidence(p), sounds=rows))
        return dict(logical_package=str(self.path.resolve()),
                    logical_sha256=hashlib.sha256(self.data).hexdigest(),
                    evidence_level='offline typed graph; not runtime playback', producers=result)


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('logical_package', type=Path)
    ap.add_argument('--output', type=Path)
    args = ap.parse_args()
    result = Graph(args.logical_package).report()
    if args.output:
        if args.output.exists():
            raise SystemExit('refusing existing output')
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    else:
        print(json.dumps(result, ensure_ascii=False, indent=2))
