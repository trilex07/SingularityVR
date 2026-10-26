"""Build a private native-menu package; never install it or edit Steam.

Pinned input preserves the accepted title/cursor/pause RTT. All non-movie
exports are byte-identical apart from proven inline bulk-data relocations.
"""
import argparse
import json
import struct
import subprocess
from pathlib import Path
from prepare_title_package import pack_summary, offsets, sha, require
from ue3_layout import inspect
from remaster_native_buttons import patch_movie

SOURCE_SHA = '51cdab5c892a7b682d89319721330aa3e78c7013d1c60e1476a0115431f01a1a'
LOGICAL_SHA = '39fa0746a02334cd1c7db52464ca24796d8f04d70f1e7c9814b0068c673c863d'


def build(original, logical, assets):
    require(sha(original) == SOURCE_SHA and sha(logical) == LOGICAL_SHA, 'unreviewed UI source')
    info = inspect('memory', logical, include_all=True)
    exports = info['exports']
    require(all('parse_error' not in e for e in exports), 'property parse')
    require(any(e['name'] == 'RTT_VRPause' for e in exports), 'pause RTT missing')
    blobs = {}; reports = []
    for e in exports:
        if e['class_name'] != 'GFxMovieInfo' or e['name'] not in ('iis', 'SPMenus'):
            continue
        raw = next(p for p in e['properties'] if p['name'] == 'RawData')
        require(raw['type'] == 'ArrayProperty', 'RawData type')
        value = raw['tag_offset'] + 24; size = raw['value']['bytes']
        require(struct.unpack_from('<I', logical, value)[0] == size-4, 'RawData count')
        before = logical[value+4:value+size]
        after, report = patch_movie(before, e['name'], assets)
        part = bytearray(logical[e['offset']:value+4] + after + logical[value+size:e['offset']+e['size']])
        struct.pack_into('<I', part, raw['tag_offset']-e['offset']+16, len(after)+4)
        struct.pack_into('<I', part, value-e['offset'], len(after))
        blobs[e['index']] = part
        report.update(before_sha256=sha(before), after_sha256=sha(after))
        reports.append(report)
    require(len(blobs) == 2, 'two native movies required')
    ordered = sorted(exports, key=lambda e: e['offset'])
    cursor = ordered[0]['offset']; out = bytearray(logical[:cursor]); expected = {}
    relocations = []
    for e in ordered:
        require(e['offset'] == cursor, 'unknown export gap'); cursor += e['size']
        new = len(out); part = bytearray(blobs.get(e['index'], logical[e['offset']:cursor]))
        cls = e['class_name']
        if cls in ('Texture2D', 'TextureRenderTarget2D'):
            for field in offsets(logical, e):
                local = field-e['offset']; old = struct.unpack_from('<I', part, local)[0]
                struct.pack_into('<I', part, local, old+new-e['offset'])
                relocations.append(dict(export=e['name'], local=local, delta=new-e['offset']))
        elif cls == 'ShaderCache':
            p = e['property_end']; end = cursor
            require(logical[p] == 0, 'shader platform'); p += 1
            for section in range(2):
                n = struct.unpack_from('<I', logical, p)[0]; p += 4
                require(n <= 1024, 'shader CRC count')
                for _ in range(n):
                    ni, num, _ = struct.unpack_from('<3I', logical, p); p += 12
                    require(ni < len(info['names']) and num == 0, 'shader CRC name')
                if section == 0:
                    require(struct.unpack_from('<I', logical, p)[0] == 0, 'nonempty shader body'); p += 4
            require(logical[p:end] == b'\0'*4, 'nonempty material cache')
        else:
            require(cls in ('GFxMovieInfo', 'ObjectRedirector', 'ObjectReferencer', 'Package') or
                    cls.startswith('RvUIGFxMovie'), 'unreviewed export class')
        out += part; expected[e['index']] = bytes(part)
        struct.pack_into('<2I', out, e['record_offset']+32, len(part), new)
    require(cursor == len(logical), 'unknown native tail')
    verified = inspect('result', out, include_all=True)
    for old, new in zip(exports, verified['exports']):
        require((old['name'], old['cls'], old['outer']) == (new['name'], new['cls'], new['outer']), 'export identity')
        require(bytes(out[new['offset']:new['offset']+new['size']]) == expected[old['index']], 'export content')
        if old['class_name'] in ('Texture2D', 'TextureRenderTarget2D'):
            offsets(out, new)
    return bytes(out), dict(movies=reports, exports_verified=len(exports), relocations=relocations,
                           source_sha256=sha(original), logical_sha256=sha(logical), physical='PENDING')


def main():
    ap = argparse.ArgumentParser()
    for key in ('original', 'logical', 'decoder', 'out'):
        ap.add_argument('--'+key, type=Path, required=True)
    ap.add_argument('--assets', type=Path, default=Path('assets/processed/ui_remaster/pause_menu'))
    args = ap.parse_args()
    require(not args.out.exists(), 'refuse existing output')
    logical, report = build(args.original.read_bytes(), args.logical.read_bytes(), args.assets)
    args.out.mkdir(parents=True)
    physical = pack_summary(logical)
    dest = args.out/'UI_SinglePlayer_SF.xxx'; dest.write_bytes(physical)
    decoded = args.out/'independent.u'
    subprocess.run([str(args.decoder), str(dest), str(decoded)], capture_output=True, check=True)
    require(decoded.read_bytes() == logical, 'independent LZO roundtrip')
    report.update(candidate_sha256=sha(physical), candidate_logical_sha256=sha(logical),
                  compression='independent decoder byte-exact PASS')
    (args.out/'manifest.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
