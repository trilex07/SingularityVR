"""Inspect the original RvGame/HUD GFx movie without exporting game assets."""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

from swf_title_patch import _take_bits, read_rect, split_tags
from ue3_layout import inspect


def raw_hud(package):
    exports = inspect('memory', package, include_all=True)['exports']
    matches = [e for e in exports if e['name'] == 'HUD' and e['class_name'] == 'GFxMovieInfo']
    if len(matches) != 1:
        raise ValueError('unique HUD GFx movie required')
    props = [p for p in matches[0]['properties'] if p['name'] == 'RawData']
    if len(props) != 1:
        raise ValueError('unique RawData required')
    at = props[0]['tag_offset'] + 24
    size = struct.unpack_from('<I', package, at)[0]
    movie = package[at + 4:at + 4 + size]
    split_tags(movie)
    return movie


def export_assets(movie, tags):
    rows = []
    for tag in tags:
        if tag['code'] != 56:
            continue
        data = movie[tag['pay_off']:tag['pay_off'] + tag['pay_len']]
        count = struct.unpack_from('<H', data)[0]
        at = 2
        for _ in range(count):
            char = struct.unpack_from('<H', data, at)[0]
            at += 2
            end = data.index(0, at)
            rows.append(dict(character=char, name=data[at:end].decode('latin1'), tag=tag['tag_off']))
            at = end + 1
        if at != len(data):
            raise ValueError('ExportAssets trailing data')
    return rows


def edit_fields(movie, tags):
    fields = []
    for tag in tags:
        if tag['code'] != 37:
            continue
        data = movie[tag['pay_off']:tag['pay_off'] + tag['pay_len']]
        fields.append(dict(character=struct.unpack_from('<H', data)[0],
                           bounds=read_rect(data, 2)[1],
                           strings=[s.decode('latin1') for s in re.findall(rb'[ -~]{3,}', data)],
                           tag=tag['tag_off']))
    return fields


def nested_tags(data, offset=4):
    while offset + 2 <= len(data):
        head = struct.unpack_from('<H', data, offset)[0]
        code, size = head >> 6, head & 63
        at = offset + 2
        if size == 63:
            size = struct.unpack_from('<I', data, at)[0]
            at += 4
        if at + size > len(data):
            raise ValueError('nested sprite tag out of range')
        yield code, data[at:at + size]
        offset = at + size
        if code == 0:
            break


def matrix(data, at):
    bit = at * 8
    scale, bit = _take_bits(data, bit, 1)
    if scale:
        width, bit = _take_bits(data, bit, 5)
        sx, bit = _take_bits(data, bit, width, signed=True)
        sy, bit = _take_bits(data, bit, width, signed=True)
    else:
        sx = sy = 65536
    rotate, bit = _take_bits(data, bit, 1)
    if rotate:
        width, bit = _take_bits(data, bit, 5)
        r0, bit = _take_bits(data, bit, width, signed=True)
        r1, bit = _take_bits(data, bit, width, signed=True)
    else:
        r0 = r1 = 0
    width, bit = _take_bits(data, bit, 5)
    x, bit = _take_bits(data, bit, width, signed=True)
    y, bit = _take_bits(data, bit, width, signed=True)
    return dict(x_twips=x, y_twips=y, sx=sx / 65536, sy=sy / 65536,
                r0=r0 / 65536, r1=r1 / 65536), (bit + 7) // 8


def skip_cxform(data, at):
    bit = at * 8
    additive, bit = _take_bits(data, bit, 1)
    multiplicative, bit = _take_bits(data, bit, 1)
    width, bit = _take_bits(data, bit, 4)
    bit += width * 4 * (additive + multiplicative)
    return (bit + 7) // 8


def sprite_placements(movie, tags):
    rows = []
    for tag in tags:
        if tag['code'] != 39:
            continue
        data = movie[tag['pay_off']:tag['pay_off'] + tag['pay_len']]
        owner = struct.unpack_from('<H', data)[0]
        for code, child in nested_tags(data):
            if code not in (26, 70):
                continue
            flags = child[0]
            at = 4 if code == 70 else 3
            character = None
            if flags & 2:
                character = struct.unpack_from('<H', child, at)[0]
                at += 2
            transform = None
            if flags & 4:
                transform, at = matrix(child, at)
            if flags & 0x08:
                at = skip_cxform(child, at)
            if flags & 0x10:
                at += 2
            name = None
            if flags & 0x20:
                end = child.index(0, at)
                name = child[at:end].decode('latin1')
            rows.append(dict(owner=owner, character=character, depth=struct.unpack_from('<H', child, 2 if code == 70 else 1)[0],
                             name=name, flags=flags, matrix=transform))
    return rows


def inspect_hud(movie):
    _, tags = split_tags(movie)
    return dict(bytes=len(movie), sha256=hashlib.sha256(movie).hexdigest(),
                exports=export_assets(movie, tags),
                fields=edit_fields(movie, tags),
                placements=sprite_placements(movie, tags),
                script_names=sorted(set(s.decode('latin1') for s in
                    re.findall(rb'[A-Za-z_][A-Za-z0-9_.$]{3,}', movie))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('rvgame', type=Path)
    args = parser.parse_args()
    print(json.dumps(inspect_hud(raw_hud(args.rvgame.read_bytes())), indent=2))
