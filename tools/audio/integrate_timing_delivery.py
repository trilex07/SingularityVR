"""Offline exact-identity timing delta onto preserved V3 native banks.

Never installs or launches. Retains refused takes and every previously packed
PC payload, metadata and non-PC export. A technical pack is NOT audio approval
or clearance of the separately recorded V3 startup crash.
"""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import struct
import subprocess
import wave

from native_story_package import build
from native_voice_fit import MAX_DEPLOYMENT_TEMPO, fit, native_tempo, trim_guarded
from patch_opening_dialogue import bulks, require
from ue3_sound_package import inspect


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def write(path, value):
    path.write_text(json.dumps(value, indent=2, ensure_ascii=True)+'\n', encoding='utf-8')


def identity(row):
    return row['package'].casefold(), row['asset']


def validate_rows(delivery, plan, review, lookup):
    expected = {identity(r): r for r in plan['timing_cases']}
    require(len(expected) == len(plan['timing_cases']), 'duplicate plan identity')
    rows, filenames = {}, set()
    for r in delivery['records']:
        key = identity(r)
        require(re.fullmatch(r'SP_[A-Za-z0-9_]+_LOC_INT', r['package'], re.I), 'not a campaign bank')
        require(re.fullmatch(r'[A-Za-z0-9_.-]+', r['asset']), 'unsafe asset')
        require(re.fullmatch(r'[A-Za-z0-9_.-]+\.wav', r['file']), 'unsafe WAV')
        require(r['file'].casefold() not in filenames, 'filename collision')
        require(re.fullmatch(r'[0-9a-f]{64}', r['sha256']), 'bad WAV hash')
        filenames.add(r['file'].casefold())
        require(key in expected and key not in rows, 'unknown or duplicate delivery identity')
        require(key in lookup and lookup[key]['file'] == r['file'], 'native lookup/filename mismatch')
        require(r['text_en'] == expected[key]['text_en'], 'source text identity changed')
        require(math.isclose(r['native_duration'], expected[key]['native_duration'], abs_tol=1e-9),
                'native budget differs from MAIN plan')
        require(r['status'] == 'PASS', 'SIDE record is not technical PASS')
        rows[key] = r
    require(rows.keys() == expected.keys(), 'delivery does not cover the exact MAIN plan')
    blocked = {identity(r): r for r in review['blocked']}
    require(len(blocked) == len(review['blocked']) and blocked.keys() <= rows.keys(), 'bad exclusion scope')
    for key, r in blocked.items():
        require(r['sha256'] == rows[key]['sha256'] and r['tts_text'] == rows[key]['tts_text'],
                'review no longer matches the refused take')
    return rows, blocked


def verify_wav(row, source, trimmed):
    require(sha(source.read_bytes()) == row['sha256'], 'WAV hash changed')
    with wave.open(str(source)) as w:
        require((w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getcomptype()) ==
                (1, 2, 24000, 'NONE'), 'delivery must be mono PCM16 24 kHz')
        samples = w.readframes(w.getnframes())
        require(len(samples) == 2*w.getnframes(), 'truncated delivery PCM')
    peak = max((abs(x[0]) for x in struct.iter_unpack('<h', samples)), default=0)
    require(0 < peak <= 29204, 'silent or excessive peak')
    duration = trim_guarded(source, trimmed)
    tempo = native_tempo(duration, row['native_duration'])
    require(abs(duration-row['guarded_duration']) <= 1/24000 and
            abs(tempo-row['computed_tempo']) < 1e-5, 'reported timing disagrees with actual PCM')
    require(tempo <= MAX_DEPLOYMENT_TEMPO+1e-9, 'recomputed tempo exceeds MAIN limit')
    return dict(guarded_duration=duration, tempo=tempo, peak=peak)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for key in ('base', 'delivery', 'plan', 'review', 'wav-dir', 'lookup', 'cooked',
                'decoder', 'ffmpeg', 'runtime-receipt', 'source-profile', 'out'):
        ap.add_argument('--'+key, type=Path, required=True)
    ap.add_argument('--base-manifest-sha256', required=True)
    ap.add_argument('--source-profile-sha256', required=True)
    a = ap.parse_args()
    root = Path(__file__).resolve().parents[2]
    allowed = (root/'build/next-weapon-reload').resolve()
    require(a.out.resolve().is_relative_to(allowed) and a.out.resolve() != allowed and
            not a.out.exists() and not a.out.resolve().is_relative_to(a.base.resolve()),
            'fresh private candidate directory required')
    raw = (a.base/'manifest.json').read_bytes()
    require(sha(raw) == a.base_manifest_sha256.lower(), 'base manifest drift')
    manifest = json.loads(raw)
    report = read(a.base/'combined-report.json')
    profile = read(a.base/'accepted-profile-ab348e06.json')
    require(profile.get('approved') is True, 'unapproved baseline')
    pins = {r['path'].casefold(): r['sha256'].lower() if r['sha256'] else None
            for r in profile['files']}
    require(sha(a.source_profile.read_bytes()) == a.source_profile_sha256.lower(), 'source profile drift')
    source_profile = read(a.source_profile)
    require(source_profile.get('approved') is True, 'unapproved source snapshot')
    for r in source_profile['files']:
        key = r['path'].casefold(); value = r['sha256'].lower()
        require(key not in pins or pins[key] == value, 'source/baseline profile conflict')
        pins[key] = value
    require(sha(a.lookup.read_bytes()) == pins['binaries/vr-assets/localization/pl/polish_story_voice.jsonl'],
            'installed native lookup drift')
    lookup = {identity(r): r for r in map(json.loads, a.lookup.read_text(encoding='utf-8-sig').splitlines())}
    review = read(a.review)
    require(sha(a.delivery.read_bytes()) == review['delivery_manifest_sha256'], 'SIDE manifest drift')
    rows, blocked = validate_rows(read(a.delivery), read(a.plan), review, lookup)
    require(len(rows) == 551, 'this reviewed delivery must cover exactly 551 identities')
    files = {r['path'].casefold(): r for r in manifest['files']}
    for r in files.values():
        require(sha((a.base/'payload'/r['path']).read_bytes()) == r['after'], 'V3 payload drift')
    runtime = read(a.runtime_receipt)
    for r in runtime['runtime']:
        require(sha((a.base/'runtime'/Path(r['path']).name).read_bytes()) == r['sha256'].lower(),
                'V3 runtime provenance drift')
    banks = {r['package'].casefold(): r for r in report['banks']}
    previous = {(b['package'].casefold(), r['asset']) for b in banks.values() for r in b['lines']}
    require(len(previous) == 707 and not (previous & rows.keys()), 'would overwrite previous 707 takes')
    for package_key in {key[0] for key in rows}:
        bank = banks[package_key]
        key = ('RvGame/CookedPC/'+bank['package']+'.xxx').casefold()
        require(pins.get(key) == bank['before'], 'native baseline identity mismatch')
        source = a.base/'payload'/files[key]['path'] if key in files else a.cooked/(bank['package']+'.xxx')
        require(sha(source.read_bytes()) == (bank['after'] or bank['before']), 'source bank drift')
    a.out.mkdir(parents=True)
    audit = a.out/'delivery-audit'; audit.mkdir()
    verified = []
    for key, r in rows.items():
        verified.append(dict(package=r['package'], asset=r['asset'],
                             **verify_wav(r, a.wav_dir/r['file'], audit/r['file'])))
    write(a.out/'delivery-audit.json', dict(source_manifest_sha256=sha(a.delivery.read_bytes()),
          technical_pass=len(verified), records=verified, excluded=review['blocked'],
          listening_performed=False, creative_approval=False))
    shutil.copytree(a.base/'payload', a.out/'payload')
    shutil.copytree(a.base/'runtime', a.out/'runtime')
    for r in runtime['runtime']:
        require(sha((a.out/'runtime'/Path(r['path']).name).read_bytes()) == r['sha256'].lower(),
                'copied runtime differs')
    groups = defaultdict(list)
    for key, r in rows.items():
        if key not in blocked:
            groups[key[0]].append(r)
    receipts, refused = [], []
    def run(argv):
        return subprocess.run([str(a.ffmpeg), '-hide_banner', '-nostats', *argv],
                              check=True, capture_output=True, text=True).stderr
    for number, (package_key, selected) in enumerate(groups.items(), 1):
        bank = banks[package_key]; package = bank['package']
        relative = 'RvGame/CookedPC/'+package+'.xxx'; key = relative.casefold()
        require(pins[key] == bank['before'], 'native baseline identity mismatch')
        source = a.base/'payload'/files[key]['path'] if key in files else a.cooked/(package+'.xxx')
        original = source.read_bytes()
        require(sha(original) == (bank['after'] or bank['before']), 'source bank drift')
        folder = a.out/'timing-delta'/package; folder.mkdir(parents=True)
        logical_path = folder/'base.u'
        subprocess.run([str(a.decoder), str(source), str(logical_path)], check=True, capture_output=True)
        logical = logical_path.read_bytes()
        exports = {e['name']: e for e in inspect(logical)['exports']}
        replacements, added = {}, []
        for r in selected:
            asset = r['asset']; e = exports[asset]
            require(e['class_name'] == 'SoundNodeWave', 'not a native wave')
            props = {p['name']: p['value'] for p in e['properties']}
            rate, channels = props['SampleRate'], props['NumChannels']
            require(channels in (1, 2) and props['RawPCMDataSize'] % (2*channels) == 0, 'native PCM layout')
            frames = props['RawPCMDataSize']//(2*channels)
            require(math.isclose(frames/rate, r['native_duration'], abs_tol=1e-9), 'real native budget mismatch')
            target, receipt = fit(run, a.wav_dir/r['file'], folder, asset, rate, frames, channels)
            if target is None or receipt['tempo'] > MAX_DEPLOYMENT_TEMPO:
                refused.append(dict(package=package, asset=asset, **receipt)); continue
            second = folder/(asset+'.libvorbis.wav')
            run(['-v','error','-n','-c:a','libvorbis','-i',str(target),'-c:a','pcm_s16le',str(second)])
            with wave.open(str(second)) as w:
                parity = (w.getnframes(),w.getframerate(),w.getnchannels()) == (frames,rate,channels)
            if not parity:
                refused.append(dict(package=package, asset=asset, reason='second decoder frame mismatch')); continue
            replacements[asset] = target.read_bytes()
            added.append(dict(asset=asset,wav=r['file'],wav_sha256=r['sha256'],
                              tts_text=r['tts_text'],source_delivery_sha256=review['delivery_manifest_sha256'],
                              second_decoder='PASS',**receipt))
        if replacements:
            expected, physical = build(original, logical, replacements, sha(original), sha(logical))
            dest = a.out/'payload'/relative; dest.parent.mkdir(parents=True, exist_ok=True); dest.write_bytes(physical)
            decoded = folder/'roundtrip.u'
            subprocess.run([str(a.decoder),str(dest),str(decoded)], check=True,capture_output=True)
            require(decoded.read_bytes() == expected, 'independent container roundtrip failed')
            parsed = {e['name']:e for e in inspect(expected)['exports']}
            for old in bank['lines']:
                asset=old['asset']
                require(bulks(logical,exports[asset])[1][1] == bulks(expected,parsed[asset])[1][1],
                        'previous packed take changed: '+asset)
            bank['lines'].extend(added)
            bank['retained'] = [r for r in bank['retained'] if r['asset'] not in replacements]
            bank['after'] = sha(physical); bank['candidate_private_path'] = str(dest)
            files[key] = dict(path=relative,before=bank['before'],after=sha(physical))
        receipts.append(dict(package=package,added=len(added),previous_preserved=len(bank['lines'])-len(added),
                             container_roundtrip='PASS' if replacements else 'UNCHANGED'))
        write(a.out/'progress.json',dict(banks=receipts,codec_refusals=refused))
        print(f'{number}/{len(groups)} {package}: added={len(added)}',flush=True)
    for bank in banks.values():
        for row in bank['retained']:
            key = (bank['package'].casefold(),row['asset'])
            if key in blocked:
                row.update(reason='new delivery content refusal: '+blocked[key]['reason'],
                           disposition='base audio retained; corrected SIDE take required')
    retained = [dict(r,package=b['package']) for b in banks.values() for r in b['retained']]
    report['retained'] = retained
    report['summary'].update(takes_replaced=sum(len(b['lines']) for b in banks.values()),
          takes_retained=len(retained),banks_changed=sum(b['after'] is not None for b in banks.values()),
          reasons=dict(Counter(r.get('reason','retained') for r in retained)))
    manifest['files'] = list(files.values())
    manifest['protected'] = [r for r in manifest['protected'] if r['path'].casefold() not in files]
    manifest.update(status='OFFLINE_ONLY_STARTUP_BLOCKED_AUDIO_UNAPPROVED',requires_new_user_start=True,
                    steam_written=False,physical_acceptance='NOT_RUN')
    for r in manifest['files']:
        require(sha((a.out/'payload'/r['path']).read_bytes()) == r['after'], 'final payload drift')
    require(sha(a.lookup.read_bytes()) == pins['binaries/vr-assets/localization/pl/polish_story_voice.jsonl'],
            'lookup changed during offline preparation')
    write(a.out/'manifest.json',manifest); write(a.out/'combined-report.json',report)
    write(a.out/'timing-delta.json',dict(previous_packed_takes_preserved=707,
          source_manifest_sha256=review['delivery_manifest_sha256'],source_commit=review['source_commit'],
          source_profile_sha256=a.source_profile_sha256.lower(),
          technically_verified=551,semantic_refusals=len(blocked),codec_refusals=refused,
          new_takes_packed=report['summary']['takes_replaced']-707,summary=report['summary'],
          current_runtime_source='38ca72f88112bfcbd26addb0d6aa4f76095aa15e',
          startup_status='UNRESOLVED_V3_CRASH_NO_LAUNCH',listening_performed=False,
          creative_approval=False,steam_written=False,banks=receipts))
    print(json.dumps(report['summary']),flush=True)


if __name__ == '__main__':
    main()
