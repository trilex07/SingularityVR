"""Prepare exact, length-preserving native cursor cores outside Steam.

The mod supplies the animated ring. Native GFx retains a small centred core
(also usable with VR disabled), its Mouse sprite, actions and input handling.
Only the exclusive cursor DefineShape payload changes. No package offsets,
script exports, audio, titles, fonts or other shapes are relocated.
"""
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path

from swf_title_patch import read_rect, split_tags
from swf_ui_polish import Bits, dot_shape, exported_char, require
from inspect_ui_movie_fingerprints import fnv
from ue3_layout import inspect
from prepare_title_package import pack_summary
from patch_opening_dialogue import literals

ARROW_HASH = 'c7e04db2f2b65640893c7681966775e3b3f27a8398ca4a204353d63dfc45c864'
PROFILES = {
    'single-player': {
        'physical': '51cdab5c892a7b682d89319721330aa3e78c7013d1c60e1476a0115431f01a1a',
        'logical': '39fa0746a02334cd1c7db52464ca24796d8f04d70f1e7c9814b0068c673c863d',
        'movies': {'iis': '978D81A0A2F0D401', 'SPMenus': 'D2E964C936CCE902'},
        'container': 'summary',
    },
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def core_shape(character, size):
    """Four-edge, 4px-wide centred diamond plus legal MoveTo-only padding.

    Padding is actual SWF SHAPE grammar, not ignored trailing bytes. Keeping
    the complete payload length permits byte-exact preservation of every
    enclosing movie/property/export offset, including the RvGame script bank.
    """
    rect = Bits(); rect.put(7, 5)
    for value in (-48, 48, -48, 48):
        rect.put(value, 7)
    prefix = struct.pack('<H', character) + rect.bytes() + bytes((1, 0, 245, 245, 245, 0))
    edges = Bits(); edges.put(1, 4); edges.put(0, 4)
    edges.put(0, 1); edges.put(5, 5)  # MoveTo + FillStyle1
    edges.put(7, 5); edges.put(40, 7); edges.put(0, 7); edges.put(1, 1)
    points = [(40, 0), (0, 40), (-40, 0), (0, -40), (40, 0)]
    for a, b in zip(points, points[1:]):
        edges.put(1, 1); edges.put(1, 1); edges.put(5, 4); edges.put(1, 1)
        edges.put(b[0]-a[0], 7); edges.put(b[1]-a[1], 7)
    gap = (size-len(prefix))*8-len(edges.bits)-6
    require(gap >= 0, 'cursor payload too small')
    widths = None
    for count in range(gap//13+1):
        for padding in range(8):
            rest = gap-padding-11*count
            if rest < 0 or rest % 2 or not count <= rest//2 <= 31*count:
                continue
            widths = [1]*count; spare = rest//2-count
            for i in range(count):
                extra = min(30, spare); widths[i] += extra; spare -= extra
            break
        if widths is not None:
            break
    require(widths is not None, 'no exact SHAPE padding solution')
    for width in widths:
        edges.put(0, 1); edges.put(1, 5); edges.put(width, 5)
        edges.put(0, width); edges.put(0, width)
    edges.put(0, 6)
    result = prefix + edges.bytes()
    require(len(result) == size, 'cursor payload length changed')
    return result


def patch_movie(movie, allowed_hashes=None):
    character = exported_char(movie, 'Mouse')
    _, tags = split_tags(movie)
    sprites = [t for t in tags if t['code'] == 39 and
               struct.unpack_from('<H', movie, t['pay_off'])[0] == character]
    require(len(sprites) == 1, 'Mouse sprite identity')
    sprite = movie[sprites[0]['pay_off']:sprites[0]['pay_off']+sprites[0]['pay_len']]
    require(len(sprite) == 16 and sprite[2:9] == bytes.fromhex('01008606060100') and
            sprite[11:] == bytes.fromhex('0040000000'), 'unknown Mouse sprite layout')
    leaf = struct.unpack_from('<H', sprite, 9)[0]
    shapes = [t for t in tags if t['code'] == 2 and
              struct.unpack_from('<H', movie, t['pay_off'])[0] == leaf]
    require(len(shapes) == 1, 'cursor leaf identity')
    tag = shapes[0]; start = tag['pay_off']; end = start+tag['pay_len']
    payload = movie[start:end]
    known = {ARROW_HASH, sha(dot_shape(leaf)[2:])} if allowed_hashes is None else allowed_hashes
    require(sha(payload[2:]) in known, 'unknown cursor geometry')
    reference = b'\x06\x01\x00' + struct.pack('<H', leaf) + b'\x00'
    require(movie.count(reference) == 1, 'cursor leaf is shared')
    changed = movie[:start]+core_shape(leaf, len(payload))+movie[end:]
    require(split_tags(changed) == split_tags(movie), 'movie tag layout changed')
    require(changed[:start] == movie[:start] and changed[end:] == movie[end:], 'unrelated movie bytes changed')
    return changed, dict(sprite=character, leaf=leaf, offset=start, size=len(payload),
                         old_shape_sha256=sha(payload), new_shape_sha256=sha(changed[start:end]))


def patch_logical(logical, profile):
    require(sha(logical) == profile['logical'], 'logical package identity mismatch')
    result = bytearray(logical); changed = []
    exports = inspect('memory', logical, include_all=True)['exports']
    for name, fingerprint in profile['movies'].items():
        matches = [e for e in exports if e['name'] == name and e['class_name'] == 'GFxMovieInfo']
        require(len(matches) == 1 and 'parse_error' not in matches[0], 'movie export identity')
        export = matches[0]
        prop = next(p for p in export['properties'] if p['name'] == 'RawData')
        at = prop['tag_offset']+24
        require(struct.unpack_from('<I', logical, at)[0] == prop['value']['bytes']-4, 'RawData length')
        start = at+4; end = at+prop['value']['bytes']; movie = logical[start:end]
        require(fnv(movie) == fingerprint, 'runtime movie fingerprint mismatch')
        replacement, report = patch_movie(movie)
        result[start:end] = replacement
        report.update(movie=name, original_fnv64=fnv(movie), candidate_fnv64=fnv(replacement),
                      package_offset=start+report['offset'])
        changed.append(report)
    # An independent whole-file diff can contain ONLY the declared leaf ranges.
    cursor = 0
    for report in sorted(changed, key=lambda r: r['package_offset']):
        start = report['package_offset']; end = start+report['size']
        require(logical[cursor:start] == result[cursor:start], 'unrelated package bytes changed')
        cursor = end
    require(logical[cursor:] == result[cursor:] and len(logical) == len(result), 'package tail/layout changed')
    return bytes(result), changed


def pack_whole(logical):
    blocks = [logical[i:i+0x20000] for i in range(0, len(logical), 0x20000)]
    compressed = [literals(b) for b in blocks]
    header = struct.pack('<4I', 0x9E2A83C1, 0x20000, sum(map(len, compressed)), len(logical))
    data = header + b''.join(struct.pack('<2I', len(c), len(b)) for c, b in zip(compressed, blocks)) + b''.join(compressed)
    return data+b'\0'*((-len(data)) % 65536)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--profile', choices=PROFILES, required=True)
    ap.add_argument('--input', type=Path, required=True)
    ap.add_argument('--decoder', type=Path, required=True)
    ap.add_argument('--out-dir', type=Path, required=True)
    args = ap.parse_args(); profile = PROFILES[args.profile]
    output = str(args.out_dir.resolve()).replace('\\', '/').lower()
    require('steamapps' not in output and 'program files' not in output, 'output must be outside the installation')
    require(not args.out_dir.exists(), 'refuse existing output directory')
    require(args.input.stat().st_size <= 64*1024*1024, 'input size cap')
    original = args.input.read_bytes(); require(sha(original) == profile['physical'], 'physical package identity mismatch')
    args.out_dir.mkdir(parents=True)
    logical_path = args.out_dir/'control.u'
    subprocess.run([str(args.decoder), str(args.input), str(logical_path)], check=True, capture_output=True)
    logical = logical_path.read_bytes(); patched, changes = patch_logical(logical, profile)
    encoded = pack_summary(patched) if profile['container'] == 'summary' else pack_whole(patched)
    candidate = args.out_dir/args.input.name; candidate.write_bytes(encoded)
    decoded = args.out_dir/'candidate.u'
    subprocess.run([str(args.decoder), str(candidate), str(decoded)], check=True, capture_output=True)
    require(decoded.read_bytes() == patched, 'independent native decoder roundtrip failed')
    report = dict(profile=args.profile, input=str(args.input.resolve()), original_sha256=sha(original),
                  candidate=str(candidate.resolve()), candidate_sha256=sha(encoded), changes=changes,
                  logical_size_unchanged=True, unrelated_bytes_unchanged=True,
                  native_decoder_roundtrip=True, status='OFFLINE_PASS_PHYSICAL_PENDING')
    (args.out_dir/'report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
