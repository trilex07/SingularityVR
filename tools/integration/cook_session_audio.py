"""Offline native campaign cook from the verified session intake.

Uses the CURRENT native packages as the baseline, not old stock/recovery banks.
Full utterances are fitted by the audited native_voice_fit path. Oversized or
unsupported takes retain baseline audio and are explicitly reported. No launch,
native property edits, MP/UI changes, mute hack, or installed writes.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import wave

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'tools/audio'))
from native_story_package import build
from native_voice_fit import fit
from patch_opening_dialogue import bulks, require
from ue3_sound_package import inspect
from prepare_session_assets import leaf, sha


def jsonl(path):
    return [json.loads(x) for x in path.read_text(encoding='utf-8-sig').splitlines() if x.strip()]


def legacy_metadata(path):
    # SIDE's legacy remaining-voices inventory mixes valid UTF-8 and CP1250
    # bytes. Only binding IDs are authoritative here; English always comes from
    # the independently decoded native wave. Do not rewrite the source file.
    text = path.read_bytes().decode('utf-8-sig', errors='surrogateescape')
    text = ''.join(bytes([ord(c)-0xdc00]).decode('cp1250') if 0xdc80 <= ord(c) <= 0xdcff else c for c in text)
    return json.loads(text)


def bindings(a, intake):
    sources = {(r['pack'], r['file']): r for r in intake['audio'] if r['pack'] != 'mp'}
    chosen = {}

    def add(package, asset, pack, filename, metadata):
        require(re.fullmatch(r'(?i:SP_)[A-Za-z0-9_]+_LOC_INT', package) and
                re.fullmatch(r'[A-Za-z0-9_]+', asset), 'invalid campaign wave identity')
        key = (package.lower(), asset)
        source = sources.get((pack, filename))
        if not source:
            return
        row = dict(package=package, asset=asset, pack=pack, file=filename,
                   sha256=source['staged_sha256'], metadata=metadata)
        if key in chosen:
            # A corrected story SD_* take supersedes the old bark take, but
            # never let a second bank/name guess supersede a declared binding.
            if chosen[key]['pack'] == 'story' and pack != 'story':
                return
            require(chosen[key]['sha256'] == row['sha256'], 'competing wave bindings: '+str(key))
        chosen[key] = row

    for r in jsonl(a.story):
        package = r.get('package', '')
        if not package.lower().startswith('sp_'):
            continue
        filename = r.get('file', '')
        if ('story', filename) in sources:
            add(package, r['asset'], 'story', filename, r)
        else:
            add(package, r['asset'], 'barks', r['asset']+'.pl.wav', r)
    remaining = legacy_metadata(a.remaining)
    for r in remaining['skipped']:
        # Never overwrite a reviewed story binding with the older skipped take.
        key = (r['package'].lower(), r['asset'])
        if key not in chosen:
            add(r['package'], r['asset'], 'skipped', r['source_wav'], r)
    intro = json.loads(a.intro.read_text(encoding='utf-8-sig'))
    for r in intro:
        add('SP_VI_intro_Kismet_LOC_INT', r['asset'], 'intro', r['asset']+'.pl.wav', r)
    for r in remaining['vignette']:
        add('SP_RT3_Vignette_LOC_INT', r['asset'], 'vignette', r['source_wav'], r)
    require(chosen, 'no native bindings')
    groups = {}
    for row in chosen.values():
        groups.setdefault(row['package'].lower(), []).append(row)
    return groups


def run_tool(argv):
    result = subprocess.run([str(x) for x in argv], capture_output=True, text=True)
    require(result.returncode == 0, 'tool failed: '+str(argv[0])+' '+result.stderr[-1200:])
    return result.stderr


def canonical_decode(data):
    # The independent native decoder clears these compressed-summary fields;
    # native_story_package keeps them for its on-disk packer. Every other byte
    # must match, including wave properties and non-target audio/cue exports.
    result = bytearray(data)
    result[0x61:0x69] = b'\0'*8
    return bytes(result)


def cook_one(a, package, rows, folder):
    original = (folder/'baseline.xxx').read_bytes()
    run_tool([a.decoder, folder/'baseline.xxx', folder/'baseline.u'])
    logical = (folder/'baseline.u').read_bytes()
    exports = inspect(logical)['exports']
    by_name = {e['name']: e for e in exports}
    require(len(by_name) == len(exports), 'ambiguous native exports')
    replacements, done, retained = {}, [], []

    def ff(args):
        return run_tool([a.ffmpeg, '-hide_banner', '-nostats', *args])

    for r in rows:
        asset = r['asset']
        e = by_name.get(asset)
        require(e and e['class_name'] == 'SoundNodeWave', 'mapped native wave absent: '+asset)
        props = {p['name']: p['value'] for p in e['properties']}
        rate, channels, size = props['SampleRate'], props['NumChannels'], props['RawPCMDataSize']
        if channels != 1 or rate not in (44100, 48000) or size % 2:
            retained.append(dict(asset=asset, reason='unsupported native PCM metadata; baseline retained'))
            continue
        frames = size//2
        source = a.intake/'audio-sources'/r['pack']/r['file']
        require(sha(source.read_bytes()) == r['sha256'], 'staged voice changed')
        ref = folder/(asset+'.reference.ogg')
        ref.write_bytes(bulks(logical, e)[1][1])
        decoded = folder/(asset+'.reference.wav')
        ff(['-v', 'error', '-n', '-i', ref, '-c:a', 'pcm_s16le', decoded])
        with wave.open(str(decoded)) as w:
            actual = (w.getnchannels(), w.getframerate(), w.getnframes())
        if actual != (1, rate, frames):
            # Some currently installed waves already decode 128 frames shorter
            # than their immutable RawPCMDataSize. Do not guess padding/delay,
            # "repair" metadata or abort unrelated proven waves in this bank.
            retained.append(dict(asset=asset, reason='baseline decode/sample budget differs; baseline retained',
                                 declared=[1, rate, frames], decoded=list(actual)))
            continue
        target, fitting = fit(ff, source, folder, asset, rate, frames)
        if target is None:
            retained.append(dict(asset=asset, **fitting))
            continue
        # No rewrite of SampleRate/Duration/RawPCMDataSize or cue timing.
        loud = ff(['-i', target, '-af', 'loudnorm=I=-24:TP=-1:LRA=11:print_format=json', '-f', 'null', '-'])
        measured = json.loads(loud[loud.rfind('{'):loud.rfind('}')+1])
        require(float(measured['input_tp']) < -.2, 'native encode true-peak gate')
        replacements[asset] = target.read_bytes()
        done.append(dict(asset=asset, source_sha256=r['sha256'], pack=r['pack'], file=r['file'],
                         english=props.get('SpokenText', ''),
                         polish=r['metadata'].get('text_pl', r['metadata'].get('pl', r['metadata'].get('polish', ''))),
                         true_peak_dbtp=float(measured['input_tp']), **fitting))
    if not replacements:
        return dict(package=package, status='UNCHANGED_NO_ELIGIBLE_TAKES', lines=[], retained=retained)
    # Repack-only first, then changed candidate. Independent C++ LZO decoder
    # checks the entire result rather than an inverse sharing packer code.
    for label, clips in (('control', {}), ('candidate', replacements)):
        expected, physical = build(original, logical, clips, sha(original), sha(logical))
        packed = folder/(label+'.xxx')
        packed.write_bytes(physical)
        run_tool([a.decoder, packed, folder/(label+'.u')])
        require((folder/(label+'.u')).read_bytes() == canonical_decode(expected), 'independent native roundtrip mismatch')
    return dict(package=package, status='OFFLINE_PASS_RUNTIME_AUDITORY_PENDING',
                before_sha256=sha(original), candidate_sha256=sha((folder/'candidate.xxx').read_bytes()),
                lines=done, retained=retained, native_decoder_roundtrip=True)


def main(a):
    a.intake = a.intake.resolve()
    allowed = (ROOT/'build/next-weapon-reload').resolve()
    out = a.out.resolve()
    require(a.intake.is_relative_to(allowed) and out.is_relative_to(allowed) and out != allowed
            and not out.exists(), 'use fresh private output beneath build/next-weapon-reload')
    intake = json.loads((a.intake/'intake.json').read_text(encoding='utf-8'))
    groups = bindings(a, intake)
    if a.only:
        requested = {p.lower() for p in a.only}
        require(requested <= groups.keys(), 'unknown --only package')
        groups = {p: rows for p, rows in groups.items() if p in requested}
    # Snapshot every targeted current package before cooking; no live changes.
    out.mkdir(parents=True)
    for package in groups:
        folder = out/package
        folder.mkdir()
        source = a.cooked/(package+'.xxx')
        shutil.copyfile(source, folder/'baseline.xxx')
        require(sha(source.read_bytes()) == sha((folder/'baseline.xxx').read_bytes()), 'native snapshot drift')
    report = []
    with ThreadPoolExecutor(max_workers=a.workers) as pool:
        jobs = {pool.submit(cook_one, a, p, r, out/p): p for p, r in groups.items()}
        for job in as_completed(jobs):
            package = jobs[job]
            try:
                result = job.result()
            except Exception as ex:
                result = dict(package=package, status='FAILED', reason=str(ex), lines=[], retained=[])
            report.append(result)
            (out/'bank-report.json').write_text(json.dumps(sorted(report, key=lambda r:r['package']),
                                                           indent=2, ensure_ascii=False), encoding='utf-8')
            print(package, result['status'], 'replaced', len(result['lines']), 'retained', len(result['retained']), flush=True)
    failed = sum(r['status'] == 'FAILED' for r in report)
    print('OFFLINE cook:', len(report), 'banks;', failed, 'failed;', sum(len(r['lines']) for r in report), 'takes')
    return 1 if failed else 0


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    for name in ('intake', 'cooked', 'story', 'remaining', 'intro', 'decoder', 'ffmpeg', 'out'):
        ap.add_argument('--'+name, type=Path, required=True)
    ap.add_argument('--workers', type=int, choices=range(1, 7), default=4)
    ap.add_argument('--only', nargs='+', help='Targeted offline retry; still requires a fresh output directory')
    sys.exit(main(ap.parse_args()))
