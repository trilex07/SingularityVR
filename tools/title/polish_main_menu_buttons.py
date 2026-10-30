"""Offline main-menu polish of the pinned, already remastered private UI package.

Only SPMenus.Button text definitions and its four V2.1 bitmap payloads change.
Native timelines, labels, placements, actions, hit targets and ButtonLeft stay
byte-identical. Output is private; this tool never touches the game install.
"""
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path

from PIL import Image, ImageDraw

from prepare_menu_remaster import offsets
from prepare_title_package import pack_summary
from remaster_native_buttons import movie_records, tag, bitmap_payload
from swf_title_patch import split_tags, read_rect
from ue3_layout import inspect

SOURCE_SHA = '9482bcc00ba91377e5d7aecc18dc3ee35ea1ed37bbd0bee4c696d19f7d8acc9a'
LOGICAL_SHA = 'f46e78e950fe1067b8ee20be185b8ed2b4c094d3c9a836fe175429102c3a230c'
MENU_SHA = 'b371cada18282798acec8d20cc7e3bcaafbcfb76715173f4ae00f63c1a61d952'
IIS_SHA = 'fc843c08fe55da875bfd5498917c7e7909343b39afb20524414462288889f384'
TEXT_IDS = (99, 102, 103)
BITMAP_IDS = (358, 359, 360, 361)
LABELS = {'EN': ('Start Game', 'Quit Game'), 'PL': ('Rozpocznij grę', 'Wyjdź z gry')}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def require(condition, why):
    if not condition:
        raise ValueError(why)


def button_art(state):
    """Original procedural metal plate at the established 480x54 bitmap size."""
    colors = {
        'up': ((27, 33, 38), (76, 89, 94), (112, 127, 126), (116, 98, 58)),
        'over': ((32, 43, 47), (52, 150, 166), (107, 223, 232), (177, 128, 61)),
        'down': ((15, 19, 22), (153, 107, 46), (222, 154, 63), (93, 79, 56)),
        'disabled': ((23, 27, 30), (56, 65, 67), (83, 94, 94), (72, 68, 55)),
    }
    base, edge, accent, bolt = colors[state]
    image = Image.new('RGBA', (480, 54), (0, 0, 0, 0))
    d = ImageDraw.Draw(image)
    for y in range(3, 51):
        light = (51-y)//11 if state != 'down' else (y-27)//12
        fill = tuple(max(0, min(255, c+light)) for c in base)+(255,)
        d.line((4, y, 475, y), fill=fill)
    d.rounded_rectangle((2, 2, 477, 51), radius=5, outline=edge+(255,), width=2)
    d.line((8, 5, 471, 5), fill=accent+(120 if state == 'over' else 60,), width=1)
    d.line((8, 48, 471, 48), fill=(4, 8, 10, 170), width=1)
    if state == 'over':
        d.line((12, 8, 467, 8), fill=accent+(110,), width=1)
        d.rectangle((5, 16, 7, 37), fill=accent+(220,))
    elif state == 'down':
        d.line((10, 10, 469, 10), fill=(0, 0, 0, 220), width=2)
        d.line((9, 47, 470, 47), fill=accent+(90,), width=1)
    for x in (16, 464):
        d.ellipse((x-2, 25, x+2, 29), fill=bolt+(225,))
        d.line((x-1, 27, x+1, 27), fill=(13, 16, 18, 220), width=1)
    return image


def text_field(payload):
    out = bytearray(payload)
    _, bounds, at = read_rect(payload, 2)
    require(bounds in ([-40, 6360, -40, 577], [-40, 6360, -40, 652]), 'text bounds drift')
    require(payload[at:at+2] in (b'\x0d\x31', b'\x8d\x31'), 'text flags drift')
    require(struct.unpack_from('<HH', payload, at+2) in ((15, 480), (19, 480)), 'font identity drift')
    # Existing field has centered layout, zero margins and 2px leading.
    require(payload[at+10:at+19] == b'\x02\0\0\0\0\0\0\x28\0', 'text layout drift')
    struct.pack_into('<H', out, at+4, 420)  # 21px: extra vertical breathing room.
    struct.pack_into('<HH', out, at+11, 300, 300)  # 15px each side at design resolution.
    return bytes(out)


def patch_movie(movie):
    require(sha(movie) == MENU_SHA, 'unreviewed SPMenus movie')
    head, records = movie_records(movie)
    seen_text, seen_bitmap = set(), set()
    out = bytearray(movie[:head['body']])
    for code, payload in records:
        char = struct.unpack_from('<H', payload)[0] if code in (37, 36) else None
        if code == 37 and char in TEXT_IDS:
            payload = text_field(payload); seen_text.add(char)
        elif code == 36 and char in BITMAP_IDS:
            state = ('up', 'over', 'down', 'disabled')[BITMAP_IDS.index(char)]
            payload = bitmap_payload(char, button_art(state)); seen_bitmap.add(char)
        out += tag(code, payload)
    require(seen_text == set(TEXT_IDS) and seen_bitmap == set(BITMAP_IDS), 'native definitions missing')
    struct.pack_into('<I', out, 4, len(out))
    split_tags(out)
    return bytes(out)


def build(original, logical):
    require(sha(original) == SOURCE_SHA and sha(logical) == LOGICAL_SHA, 'unreviewed private package')
    info = inspect('memory', logical, include_all=True)
    exports = info['exports']
    require(len(exports) == 45 and all('parse_error' not in e for e in exports), 'export layout drift')
    by = {e['name']: e for e in exports}
    require(by['SPMenus']['class_name'] == by['iis']['class_name'] == 'GFxMovieInfo', 'movie identity')
    def raw_movie(e):
        prop = next(p for p in e['properties'] if p['name'] == 'RawData')
        at = prop['tag_offset']+24
        size = prop['value']['bytes']
        require(struct.unpack_from('<I', logical, at)[0] == size-4, 'RawData count')
        return prop, at, size, logical[at+4:at+size]
    _, _, _, iis = raw_movie(by['iis'])
    require(sha(iis) == IIS_SHA, 'iis movie drift')
    menu = by['SPMenus']; prop, at, size, before = raw_movie(menu)
    after = patch_movie(before)
    require(after != before, 'no visual change')
    blob = bytearray(logical[menu['offset']:at+4] + after + logical[at+size:menu['offset']+menu['size']])
    struct.pack_into('<I', blob, prop['tag_offset']-menu['offset']+16, len(after)+4)
    struct.pack_into('<I', blob, at-menu['offset'], len(after))
    ordered = sorted(exports, key=lambda e: e['offset'])
    cursor = ordered[0]['offset']; out = bytearray(logical[:cursor]); expected = {}
    for e in ordered:
        require(e['offset'] == cursor, 'export gap'); cursor += e['size']
        new = len(out); part = bytearray(blob if e['index'] == menu['index'] else logical[e['offset']:cursor])
        if e['class_name'] in ('Texture2D', 'TextureRenderTarget2D'):
            for field in offsets(logical, e):
                local = field-e['offset']
                value = struct.unpack_from('<I', part, local)[0]
                struct.pack_into('<I', part, local, value+new-e['offset'])
        expected[e['index']] = bytes(part)
        out += part
        struct.pack_into('<2I', out, e['record_offset']+32, len(part), new)
    require(cursor == len(logical), 'tail drift')
    check = inspect('output', out, include_all=True)
    require([(e['name'], e['class_name'], e['outer']) for e in exports] ==
            [(e['name'], e['class_name'], e['outer']) for e in check['exports']], 'export identity changed')
    for old, new in zip(exports, check['exports']):
        require(bytes(out[new['offset']:new['offset']+new['size']]) == expected[old['index']],
                'export content changed unexpectedly')
        if old['class_name'] in ('Texture2D', 'TextureRenderTarget2D'):
            offsets(out, new)
    return bytes(out), {'input_sha256': sha(original), 'input_logical_sha256': sha(logical),
                        'menu_before_sha256': sha(before), 'menu_after_sha256': sha(after),
                        'iis_unchanged_sha256': sha(iis), 'text_ids': TEXT_IDS,
                        'bitmap_ids': BITMAP_IDS, 'font_twips': [480, 420],
                        'margin_twips': [0, 300], 'labels': LABELS,
                        'exports_verified': len(exports), 'physical': 'PENDING'}


def main():
    ap = argparse.ArgumentParser()
    for name in ('original', 'logical', 'decoder', 'out'):
        ap.add_argument('--'+name, type=Path, required=True)
    args = ap.parse_args()
    require(not args.out.exists(), 'refuse existing output')
    logical, report = build(args.original.read_bytes(), args.logical.read_bytes())
    args.out.mkdir(parents=True)
    candidate = args.out/'UI_SinglePlayer_SF.xxx'
    candidate.write_bytes(pack_summary(logical))
    decoded = args.out/'independent.u'
    subprocess.run([str(args.decoder), str(candidate), str(decoded)], check=True)
    require(decoded.read_bytes() == logical, 'independent roundtrip failed')
    report.update(candidate_sha256=sha(candidate.read_bytes()),
                  candidate_logical_sha256=sha(logical), roundtrip='PASS')
    (args.out/'manifest.json').write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding='utf-8')
    print(json.dumps(report, indent=2, ensure_ascii=False))


if __name__ == '__main__':
    main()
