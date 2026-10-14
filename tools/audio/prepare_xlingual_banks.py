"""Offline xlingual-to-native VI2 bank integration (MAIN, no Steam writes).

Replicates the accepted v4 production shape (prepare_native_story.py /
prepare_room_banks.py + native_story_package.build) for the NEW xlingual
voice set. Differences from the old flow, all explicit:
- mapping comes from manifest_xlingual.jsonl (package, asset, file) joined
  on native export identity; text_en vs SpokenText is recorded as an
  explicitly accepted near-match, never a silent equality claim;
- unfit utterances retain English with --allow-partial (else abort);
- independent decoder roundtrip is UNAVAILABLE for sound containers
  (uedecompress parses script packages only); replaced by a fresh Python
  container unpack check written against the pack() layout, plus build()'s
  own structural verification. Game load (L3) still required before install.
"""
import argparse
import hashlib
import json
import struct
import subprocess
import wave
from pathlib import Path

from native_story_package import build, TAG
from patch_opening_dialogue import bulks, require
from ue3_sound_package import inspect
from native_voice_fit import fit

STOCK = {
    'SP_VI2_kismet01_LOC_INT': 'CFCFBD65F00320414B74FF8532959F253E0F47AEECA26460EE2F69997C7E05BA',
    'SP_VI2_kismet01_a_LOC_INT': 'A00679CE2F9B7505BFB56D31006CA2D32D64514AC9E8F8C46A59AE4440B22F1A',
    'SP_VI2_kismet01_b_LOC_INT': 'A8A570AFBCBB4B6CED486CA809F19EA7BF41B190713D41B68EC6CE67F570E348',
    'SP_VI2_kismet02_LOC_INT': '44C42E06F8C5830F1FF7DEB01231B6262374957EF76AAD31E8C3D814F334DA56',
    'SP_VI2_kismet03_LOC_INT': 'CD1D4A0565E803366D9BBA59F4AD01FBD97519741B71158F1C8E5E53FC40CA17',
    'SP_VI2_kismet04_LOC_INT': '84773631379D0AF88D61DB8371E8D3EB17CBFFDAA279410B849E3B8CA67DB80B',
    'SP_VI2_zone02a_LOC_INT': '578AD1547A69CCDC7F50D2D9C616c5ecc8c038ba0e388c8de218225f35be3c7d',
    'SP_VI2_zone02a_2010_LOC_INT': 'D79485C60A05BA05FBDD64C61FD367BC5C8390F6226F31EBD5625D69FE712070',
}
LOGICAL = {
    'SP_VI2_kismet01_LOC_INT': 'f179bea43d05b4e5260c9f2bc778adb7619b23eda507cbdd2b782aac2d7e05fb',
    'SP_VI2_kismet01_a_LOC_INT': 'f7f484445c7887ba25cdf3176f6d1da6625adf1e9675cf0eeb2ec0d761816ff8',
    'SP_VI2_kismet01_b_LOC_INT': '75910d0d70ee81f68075dd470bda4a2b178894e329839fc3a6cf1a00c8f6b89b',
    'SP_VI2_kismet02_LOC_INT': 'f6a2441645034a84010f0f4eaedf60c78017021d1c4157155f21ba64b8af08c6',
    'SP_VI2_kismet03_LOC_INT': '696d64d860114d2cd1ee6ce9982d061651e3fa5c37850e79144c7ac71b87d990',
    'SP_VI2_kismet04_LOC_INT': '164720d7d90cd2ce32f56a03865d596f11e7d0c48190018c391f055348ddc7f5',
    'SP_VI2_zone02a_LOC_INT': 'd15ec25249d8a1e7dc487bbb8ab0f60be8416d59076a51ec63582b79a290ae65',
    'SP_VI2_zone02a_2010_LOC_INT': 'f8d541723290d55c85ecfea7bb86198d66eb541bd505e77f282167cdd9728cc1',
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def unpack_physical(physical):
    """Fresh inverse of native_story_package.pack(): container -> logical body.

    Only inverts literal-block pieces (what pack() emits for >=19B chunks)
    and raw pieces (<19B). Anything else refuses: this check shares no code
    with pack() beyond the documented layout.
    """
    require(len(physical) > 0x81 and struct.unpack_from('<I', physical)[0] == TAG, 'package tag')
    flag, count = struct.unpack_from('<2I', physical, 0x61)
    require(flag == 2 and count == 1, 'unobserved compression summary')
    require(struct.unpack_from('<I', physical, 0x69)[0] == 0x71, 'unobserved logical header')
    rec_off, rec_size, cont_off, cont_size = struct.unpack_from('<4I', physical, 0x69)
    require(rec_off == 0x71, 'unobserved record offset')
    container = physical[cont_off:cont_off + cont_size]
    ctag, block, total_comp, total_raw = struct.unpack_from('<4I', container)
    require(ctag == TAG and block == 0x20000, 'container header')
    at = 16
    comp_sizes = []
    raw_sizes = []
    # count unknown up front: entries are 8B pairs until payload start; find by walk
    entries = []
    while True:
        require(at + 8 <= len(container), 'chunk table overrun')
        cl, rl = struct.unpack_from('<2I', container, at)
        at += 8
        # payload region begins where cumulative comps would start; detect by sanity:
        entries.append((cl, rl))
        # stop when next bytes look like a literal/raw piece start for remaining data
        # simpler: stop when consumed table + sum(comp) == len(container)
        if at + sum(c for c, _ in entries) == len(container):
            break
        require(len(entries) < 4096, 'chunk table runaway')
    payload_at = at
    raws = []
    for cl, rl in entries:
        piece = container[payload_at:payload_at + cl]
        require(len(piece) == cl, 'truncated piece')
        payload_at += cl
        if rl < 19:
            require(cl == rl, 'raw piece size')
            raws.append(piece)
            continue
        # literal block: [0x00, 0x00*(k), tail][data][0x11 0x00 0x00]
        require(piece[0] == 0 and piece[-3:] == b'\x11\0\0', 'not a literal piece')
        i = 1
        remaining = 0
        while True:
            require(i < len(piece), 'literal prefix overrun')
            b = piece[i]
            i += 1
            if b != 0:
                remaining += b
                break
            remaining += 255
        # remaining = len(data)-18 per literals(): prefix consumed i bytes
        data = piece[i:-3]
        require(len(data) - 18 == remaining and len(data) == rl, 'literal size')
        raws.append(data)
    require(payload_at == len(container), 'container tail')
    body = b''.join(raws)
    require(len(body) == total_raw, 'raw size')
    return body


def main():
    ap = argparse.ArgumentParser()
    for arg in ('cooked', 'logical', 'xlingual', 'ffmpeg', 'out'):
        ap.add_argument('--' + arg, type=Path, required=True)
    ap.add_argument('--allow-partial', action='store_true')
    ap.add_argument('--allow-pending-qa', action='store_true')
    a = ap.parse_args()
    require(not a.out.exists(), 'refuse existing output')
    xl = {}
    for line in (a.xlingual / 'manifest_xlingual.jsonl').read_text(encoding='utf-8-sig').splitlines():
        if line.strip():
            r = json.loads(line)
            xl[(r.get('package'), r.get('asset'))] = r
    recall = {}
    for qf in ['ASR_QA.json', 'ASR_QA_runda1.json']:
        p = a.xlingual / qf
        if p.exists():
            for e in json.loads(p.read_text(encoding='utf-8-sig')):
                recall[e['file']] = e.get('recall')
    qaflags = {}
    p = a.xlingual / 'QA_FLAGS.json'
    if p.exists():
        qaflags = json.loads(p.read_text(encoding='utf-8-sig'))
    flagged_files = {f for cat in qaflags.values() if isinstance(cat, list)
                     for row in cat for f in [row[0]] if isinstance(row, list) and row}
    ffmpeg = str(a.ffmpeg)

    def run(argv):
        return subprocess.run([ffmpeg, '-hide_banner', '-nostats', *argv],
                              capture_output=True, text=True, check=True).stderr

    a.out.mkdir(parents=True)
    report = []
    for package, source_sha in STOCK.items():
        folder = a.out / package
        folder.mkdir()
        original = (a.cooked / (package + '.xxx')).read_bytes()
        require(sha(original) == source_sha.lower(), 'stock bank changed: ' + package)
        logical = (a.logical / (package + '.u')).read_bytes()
        logical_sha = sha(logical)
        require(logical_sha == LOGICAL[package].lower(), 'logical bank mismatch: ' + package)
        exports = inspect(logical)['exports']
        by = {e['name']: e for e in exports}
        selected = [(e, xl[(package, e['name'])]) for e in exports
                    if e['class_name'] == 'SoundNodeWave' and (package, e['name']) in xl]
        require(bool(selected), 'empty xlingual target group: ' + package)
        replacements = {}
        lines = []
        skipped = []
        for e, r in selected:
            asset = e['name']
            require(e['class_name'] == 'SoundNodeWave', 'wrong export class')
            source = a.xlingual / r['file']
            props = {x['name']: x['value'] for x in e['properties']}
            rate = props['SampleRate']
            require(props['NumChannels'] == 1 and rate in (44100, 48000)
                    and props['RawPCMDataSize'] % 2 == 0, 'unsupported PCM metadata')
            frames = props['RawPCMDataSize'] // 2
            spoken = next(x for x in e['properties'] if x['name'] == 'SpokenText')
            text_match = (r.get('text_en') == props['SpokenText'])
            qa_recall = recall.get(r['file'])
            qa_flag = r['file'] in flagged_files
            if qa_flag and not a.allow_pending_qa:
                skipped.append(dict(asset=asset, reason='QA-flagged file needs explicit flag'))
                continue
            reference = folder / (asset + '.native.ogg')
            reference.write_bytes(bulks(logical, e)[1][1])
            decoded = folder / (asset + '.native.wav')
            run(['-v', 'error', '-n', '-i', str(reference), '-c:a', 'pcm_s16le', str(decoded)])
            with wave.open(str(decoded)) as w:
                require(w.getnframes() == frames and w.getframerate() == rate
                        and w.getnchannels() == 1, 'native PCM mismatch')
            target, fitting = fit(run, source, folder, asset, rate, frames)
            if target is None:
                require(a.allow_partial, 'utterance too long: ' + asset)
                skipped.append(dict(asset=asset, **fitting))
                print(package, asset, 'RETAIN ENGLISH: unfit at safe tempo', flush=True)
                continue
            loud = run(['-i', str(target), '-af', 'loudnorm=I=-24:TP=-1:LRA=11:print_format=json',
                        '-f', 'null', '-'])
            measurement = json.loads(loud[loud.rfind('{'):loud.rfind('}') + 1])
            require(float(measurement['input_tp']) < -.2, 'encoded peak gate')
            replacements[asset] = target.read_bytes()
            lines.append(dict(asset=asset, source_wav=r['file'],
                              source_sha256=sha(source.read_bytes()),
                              english=props['SpokenText'], polish=r.get('text_pl_machine', ''),
                              text_exact_match=text_match,
                              qa_recall=qa_recall, qa_flagged=qa_flag,
                              true_peak_dbtp=float(measurement['input_tp']), **fitting))
            print(package, asset, 'fit tempo=%.2f peak=%s' % (fitting['tempo'], measurement['input_tp']), flush=True)
        require(bool(replacements), 'no eligible voices: ' + package)
        expected, physical = build(original, logical, replacements, source_sha, logical_sha)
        packed = folder / 'candidate.xxx'
        packed.write_bytes(physical)
        # Fresh container-integrity check (not a game-load proof).
        body = unpack_physical(physical)
        header = bytearray(logical[:0x71])
        # logical body starts at 0x71 per pack()
        require(body == expected[0x71:], 'container roundtrip mismatch: ' + package)
        report.append(dict(package=package, source_sha256=source_sha,
                           candidate_sha256=sha(physical), lines=lines, skipped=skipped,
                           status='OFFLINE_PASS_NOT_NATIVE_LOAD_OR_AUDITORY_ACCEPTANCE'))
        print(package, 'candidate', sha(physical)[:16], 'lines', len(lines),
              'retained', len(skipped), flush=True)
    (a.out / 'xlingual-banks.json').write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding='utf-8')
    print('OFFLINE ONLY: no Steam file changed; game-load and headset gates pending', flush=True)


if __name__ == '__main__':
    main()
