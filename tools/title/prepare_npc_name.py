"""Offline, length-preserving reduction of the original HUD's NPC name field.

Only the DefineEditText font height changes. No runtime hook, text substitution,
script edit, package offset relocation, or changes to subtitles/weapon labels.
Outputs must be outside Steam and independently roundtrip through the decoder.
"""
import argparse
import json
import struct
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / 'audio'))
from prepare_context_cursor import sha
from patch_opening_dialogue import literals
from swf_title_patch import split_tags, read_rect
from ue3_layout import inspect


def require(value, message):
    if not value:
        raise ValueError(message)


def patch_movie(movie):
    _, tags = split_tags(movie)
    candidates = []
    for tag in tags:
        if tag['code'] != 37:
            continue
        start = tag['pay_off']
        data = movie[start:start + tag['pay_len']]
        if not data.endswith(b'_root.name_NPC\0'):
            continue
        _, bounds, pos = read_rect(data, 2)
        # Authored native field: char184, WFontButton import102, font17px,
        # dynamic variable root.name_NPC. Keep alignment/bounds/alpha intact.
        require(struct.unpack_from('<H', data)[0] == 184, 'NPC character identity')
        require(bounds == [-40, 1940, -40, 474], 'NPC bounds identity')
        require(data[pos:pos+2] == b'\x0d\x31', 'NPC flags identity')
        require(struct.unpack_from('<HH', data, pos+2) == (102, 340), 'NPC font identity')
        candidates.append(start+pos+4)
    require(len(candidates) == 1, 'exclusive NPC variable field required')
    # Verify the character is the nameTxt child of the original sprite185,
    # rather than identifying a draw from its position or generic font atlas.
    owners = [movie[t['pay_off']:t['pay_off']+t['pay_len']] for t in tags
              if t['code'] == 39 and struct.unpack_from('<H', movie, t['pay_off'])[0] == 185]
    require(len(owners) == 1 and b'nameTxt\0' in owners[0] and
            b'\x26\x01\x01\x00\xb8\x00' in owners[0], 'NPC nameTxt placement identity')
    offset = candidates[0]
    out = bytearray(movie)
    struct.pack_into('<H', out, offset, 204)  # 60%, 10.2px instead of17px
    require(split_tags(out) == split_tags(movie), 'SWF layout changed')
    require(out[:offset] == movie[:offset] and out[offset+2:] == movie[offset+2:], 'unrelated movie edit')
    return bytes(out), dict(font_height_offset=offset, old_twips=340, new_twips=204,
                           character=184, sprite=185, variable='_root.name_NPC')


def patch_logical(logical):
    exports = inspect('memory', logical, include_all=True)['exports']
    hud = [e for e in exports if e['class_name'] == 'GFxMovieInfo' and e['name'] == 'HUD']
    require(len(hud) == 1, 'exclusive native HUD export required')
    raw = [p for p in hud[0]['properties'] if p['name'] == 'RawData' and p['type'] == 'ArrayProperty']
    require(len(raw) == 1, 'HUD RawData identity')
    at = raw[0]['tag_offset']+24
    count = struct.unpack_from('<I', logical, at)[0]
    require(count == raw[0]['value']['bytes']-4 and 8 <= count <= 2*1024*1024, 'RawData count')
    start = at+4
    movie = logical[start:start+count]
    changed, report = patch_movie(movie)
    result = logical[:start]+changed+logical[start+count:]
    offset = start+report['font_height_offset']
    require(len(result) == len(logical) and result[:offset] == logical[:offset] and
            result[offset+2:] == logical[offset+2:], 'unrelated package edit')
    report.update(package_offset=offset, original_movie_sha256=sha(movie), candidate_movie_sha256=sha(changed))
    return result, report


def repack_one_block(original, logical, changed):
    tag, block, compressed, size = struct.unpack_from('<4I', original)
    require(tag == 0x9e2a83c1 and block == 0x20000 and len(logical) == size == len(changed), 'whole package contract')
    count = (size+block-1)//block
    entries = [struct.unpack_from('<II', original, 16+8*i) for i in range(count)]
    require(sum(c for c, _ in entries) == compressed and sum(u for _, u in entries) == size, 'block totals')
    offset = 16+8*count
    chunks = []
    changed_blocks = []
    for i, (c, u) in enumerate(entries):
        raw = logical[i*block:i*block+u]
        new = changed[i*block:i*block+u]
        require(len(raw) == u and offset+c <= len(original), 'block bounds')
        if raw != new:
            chunks.append(literals(new)); changed_blocks.append(i)
        else:
            chunks.append(original[offset:offset+c])
        offset += c
    require(len(changed_blocks) == 1, 'exactly one changed compression block required')
    result = struct.pack('<4I', tag, block, sum(map(len, chunks)), size)
    result += b''.join(struct.pack('<II', len(c), u) for c, (_, u) in zip(chunks, entries))
    result += b''.join(chunks)
    return result+b'\0'*((-len(result)) % 65536)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--input', type=Path, required=True)
    ap.add_argument('--input-sha256', required=True)
    ap.add_argument('--decoder', type=Path, required=True)
    ap.add_argument('--out-dir', type=Path, required=True)
    args = ap.parse_args()
    output = str(args.out_dir.resolve()).replace('\\', '/').lower()
    require('steamapps' not in output and 'program files' not in output, 'output must be outside Steam')
    require(not args.out_dir.exists(), 'refuse existing output directory')
    original = args.input.read_bytes()
    require(len(original) <= 64*1024*1024 and sha(original) == args.input_sha256.lower(), 'input SHA/size mismatch')
    require(struct.unpack_from('<II', original) == (0x9e2a83c1, 0x20000), 'whole package compression required')
    args.out_dir.mkdir(parents=True)
    control = args.out_dir/'control.u'
    subprocess.run([str(args.decoder), str(args.input), str(control)], check=True, capture_output=True)
    logical = control.read_bytes()
    changed, report = patch_logical(logical)
    encoded = repack_one_block(original, logical, changed)
    candidate = args.out_dir/'RvGame.xxx'
    candidate.write_bytes(encoded)
    decoded = args.out_dir/'candidate.u'
    subprocess.run([str(args.decoder), str(candidate), str(decoded)], check=True, capture_output=True)
    require(decoded.read_bytes() == changed, 'independent native decoder mismatch')
    report.update(input=str(args.input.resolve()), input_sha256=sha(original),
                  candidate=str(candidate.resolve()), candidate_sha256=sha(encoded),
                  native_decoder_roundtrip=True, status='OFFLINE_PASS_PHYSICAL_PENDING')
    (args.out_dir/'report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
