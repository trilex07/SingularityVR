"""Offline V2.1 plates in the ORIGINAL GFx button timelines, not a second UI.

Private game input/output only. Text, AVM1, filters, mouse handlers, placement
matrices and timeline length survive. A new depth below the native children is
reserved for the plate. The original hover geometry remains in the display list
with zero alpha, retaining the native bounds/height used by AddButtonBase.
"""
import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path

from PIL import Image
from inspect_gameplay_hud import nested_tags, matrix, skip_cxform
from swf_title_patch import split_tags, encode_tag_header, read_rect, write_rect
from swf_ui_polish import Bits, signed_width, exported_char, require

STATES = ('up', 'over', 'down', 'disabled')
PLATES = ('idle', 'hover', 'pressed', 'disabled')
# Exact native symbol -> measured intersection of its text-field bounds in ALL
# states (twips). Background cannot enlarge the native button or change listY.
TARGETS = {
    'iis': {'Button': (8, (-3946, 5400, -19, 597))},
    'SPMenus': {'Button': (104, (0, 6400, 0, 617)),
                'ButtonLeft': (155, (640, 6640, 0, 692))},
}
SOURCE_MOVIES = {
    'iis': 'ad34885ad6e7256abb1476b327b1fceec4c81652bb7f0c2a039445d759cc497e',
    'SPMenus': 'c5547e15f7782bb3878b84ba3cc1520876a053ecb009a2d4d995e63b47b50b1a',
}


def tag(code, payload):
    return encode_tag_header(code, len(payload)) + payload


def swf_matrix(sx=1.0, sy=1.0, x=0, y=0):
    bits = Bits()
    bits.put(1, 1)
    fixed = (round(sx * 65536), round(sy * 65536))
    width = signed_width(fixed)
    bits.put(width, 5)
    for value in fixed:
        bits.put(value, width)
    bits.put(0, 1)
    width = signed_width((x, y))
    bits.put(width, 5); bits.put(x, width); bits.put(y, width)
    return bits.bytes()


def bitmap_payload(character, image):
    image = image.convert('RGBA')
    rgba = image.tobytes()
    argb = bytearray(len(rgba))
    # DefineBitsLossless2 format 5 uses premultiplied ARGB, not PNG RGBA.
    for at in range(0, len(rgba), 4):
        r, g, b, a = rgba[at:at + 4]
        argb[at:at + 4] = bytes((a, (r*a+127)//255, (g*a+127)//255, (b*a+127)//255))
    return struct.pack('<HBHH', character, 5, *image.size) + zlib.compress(argb, 9)


def plate_shape(character, bitmap, rect, size):
    left, right, top, bottom = rect
    styles = b'\x01\x41' + struct.pack('<H', bitmap)
    styles += swf_matrix((right-left)/size[0], (bottom-top)/size[1], left, top)
    styles += b'\0'  # no line styles
    bits = Bits(); bits.put(1, 4); bits.put(0, 4)
    bits.put(0, 1); bits.put(5, 5)  # moveTo + fill1
    width = signed_width((left, top))
    bits.put(width, 5); bits.put(left, width); bits.put(top, width); bits.put(1, 1)
    for dx, dy in ((right-left, 0), (0, bottom-top), (left-right, 0), (0, top-bottom)):
        width = signed_width((dx, dy))
        require(width <= 17, 'shape edge too wide')
        bits.put(1, 1); bits.put(1, 1); bits.put(width-2, 4); bits.put(1, 1)
        bits.put(dx, width); bits.put(dy, width)
    bits.put(0, 6)
    return struct.pack('<H', character) + write_rect(rect) + styles + bits.bytes()


def placement_fields(code, payload):
    require(code in (26, 70), 'unsupported placement')
    flags = payload[0]
    depth_at = 2 if code == 70 else 1
    # Target buttons contain filter lists but no class/image/clip-depth records.
    require(not (flags & 0x40), 'unexpected clip depth')
    if code == 70:
        require(not (payload[1] & 0x18), 'unexpected class/image placement')
    at = depth_at + 2
    character_at = at if flags & 2 else None
    character = struct.unpack_from('<H', payload, at)[0] if flags & 2 else None
    if flags & 2:
        at += 2
    if flags & 4:
        _, at = matrix(payload, at)
    cx_at = at
    cx_end = skip_cxform(payload, at) if flags & 8 else at
    return depth_at, character_at, character, cx_at, cx_end


def invisible_alpha():
    bits = Bits(); bits.put(0, 1); bits.put(1, 1); bits.put(10, 4)
    for value in (256, 256, 256, 0):
        bits.put(value, 10)
    return bits.bytes()


def patch_timeline(payload, shapes, hover_character=None):
    output = bytearray(payload[:4])
    state = None
    covered_depths = set()
    labels = []
    for code, data in nested_tags(payload):
        if code == 43:
            state = data.rstrip(b'\0').decode('ascii')
            require(state in STATES, 'unknown native button label')
            labels.append(state)
            output += tag(code, data)
            # Replace the plate at a reserved depth; never duplicate labels.
            if len(labels) > 1:
                output += tag(28, struct.pack('<H', 1))
            output += tag(26, b'\x06' + struct.pack('<HH', 1, shapes[STATES.index(state)]) + b'\0')
            continue
        if code in (26, 70):
            depth_at, _, child, cx_at, cx_end = placement_fields(code, data)
            old_depth = struct.unpack_from('<H', data, depth_at)[0]
            require(old_depth < 65535, 'cannot reserve background depth')
            if child is not None:
                covered_depths.discard(old_depth)
                if child == hover_character:
                    covered_depths.add(old_depth)
            if old_depth in covered_depths:
                # Do not delete the animated highlight: its dimensions also
                # participate in native button layout. Only its visual alpha.
                data = bytes((data[0] | 8,)) + data[1:cx_at] + invisible_alpha() + data[cx_end:]
            data = bytearray(data)
            struct.pack_into('<H', data, depth_at, old_depth + 1)
        elif code == 28:
            old_depth = struct.unpack('<H', data)[0]
            covered_depths.discard(old_depth)
            data = struct.pack('<H', old_depth + 1)
        else:
            require(code in (0, 1, 12), 'unexpected native button timeline tag')
        output += tag(code, data)
    require(tuple(labels) == STATES, 'missing/reordered button states')
    return bytes(output)


def movie_records(movie):
    header, tags = split_tags(movie)
    return header, [(t['code'], movie[t['pay_off']:t['pay_off']+t['pay_len']]) for t in tags]


def patch_movie(movie, name, asset_dir):
    require(name in TARGETS, 'unknown movie')
    require(hashlib.sha256(movie).hexdigest() == SOURCE_MOVIES[name], 'unreviewed native movie')
    header, records = movie_records(movie)
    defined = {struct.unpack_from('<H', p)[0] for c, p in records
               if c in (2, 6, 7, 10, 11, 14, 20, 21, 22, 32, 33, 34, 35, 36, 37, 39, 46, 48, 60, 75, 83, 84, 87, 90, 91, 1001)}
    for code, data in records:
        if code in (57, 71):
            at = data.index(0) + 1 + (2 if code == 71 else 0)
            count = struct.unpack_from('<H', data, at)[0]; at += 2
            for _ in range(count):
                defined.add(struct.unpack_from('<H', data, at)[0]); at += 2
                at = data.index(0, at) + 1
            require(at == len(data), 'unknown import table tail')
    next_id = max(defined) + 1
    definitions = bytearray()
    bitmaps = []; sizes = []; assets = []
    for variant in PLATES:
        path = asset_dir / ('btn_plate_' + variant + '_neutral.png')
        image = Image.open(path).convert('RGBA')
        require(image.size == (480, 54), 'V2.1 plate dimensions drifted')
        bitmaps.append(next_id); sizes.append(image.size)
        definitions += tag(36, bitmap_payload(next_id, image)); next_id += 1
        assets.append(dict(file=path.name, sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    changes = {}; report = []
    for symbol, (expected_id, rect) in TARGETS[name].items():
        character = exported_char(movie, symbol)
        require(character == expected_id, 'native symbol identity drifted')
        original = [p for c, p in records if c == 39 and struct.unpack_from('<H', p)[0] == character]
        require(len(original) == 1, 'missing native sprite')
        shapes = []
        for bitmap, size in zip(bitmaps, sizes):
            shapes.append(next_id)
            definitions += tag(32, plate_shape(next_id, bitmap, rect, size)); next_id += 1
        changes[character] = patch_timeline(original[0], shapes, 101 if name == 'SPMenus' else None)
        report.append(dict(symbol=symbol, character=character, plate_bounds_twips=rect,
                           original_sprite_sha256=hashlib.sha256(original[0]).hexdigest(), shapes=shapes))
    require(next_id < 65535, 'character dictionary exhausted')
    out = bytearray(movie[:header['body']]); inserted = False
    for original, (code, payload) in zip(split_tags(movie)[1], records):
        # Keep the native exporter/FileAttributes header FIRST. Dependencies must
        # precede the button sprites, but must not displace file metadata tags.
        if not inserted and code == 2:
            out += definitions; inserted = True
        character = struct.unpack_from('<H', payload)[0] if code == 39 else None
        if character in changes:
            out += tag(code, changes[character])
        else:
            out += movie[original['tag_off']:original['pay_off']+original['pay_len']]
    struct.pack_into('<I', out, 4, len(out))
    require(inserted, 'no safe definition insertion point')
    split_tags(out)
    return bytes(out), dict(movie=name, buttons=report, assets=assets, bitmap_ids=bitmaps,
                           text_and_actions='unchanged', physical='PENDING')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('input', type=Path); ap.add_argument('output', type=Path)
    ap.add_argument('--movie', choices=TARGETS, required=True)
    ap.add_argument('--assets', type=Path, default=Path('assets/processed/ui_remaster/pause_menu'))
    args = ap.parse_args()
    require(not args.output.exists(), 'refuse overwrite')
    out, report = patch_movie(args.input.read_bytes(), args.movie, args.assets)
    args.output.write_bytes(out)
    args.output.with_suffix('.json').write_text(json.dumps(report, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
