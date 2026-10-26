"""MAIN offline delivery: staged XTTS WAVs -> exact accepted native UE3 banks.

Never deploys. Reads hash-pinned accepted banks (including earlier Polish takes),
preserves unmatched waves, properties/cues and all non-PC payloads. Explicit
package list required; failed/overlong takes retain their accepted audio.
"""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

from native_story_package import build
from native_voice_fit import fit
from patch_opening_dialogue import require
from prepare_xlingual_banks import unpack_physical
from ue3_sound_package import inspect


def sha(data):
    return hashlib.sha256(data).hexdigest()


def select_rows(rows, package):
    selected = {}
    for row in rows:
        if row.get('package', '').casefold() != package.casefold():
            continue
        asset, name = row.get('asset', ''), row.get('file', '')
        require(bool(re.fullmatch(r'[A-Za-z0-9_.-]+', asset)), 'unsafe asset')
        require(bool(re.fullmatch(r'[A-Za-z0-9_.-]+\.wav', name)), 'unsafe WAV')
        require(asset not in selected, 'duplicate native identity: ' + asset)
        selected[asset] = row
    require(bool(selected), 'no exact lookup rows: ' + package)
    return selected


def restrict_native_scope(rows, manifest):
    """New approved deliveries may replace a subset of a bank.

    Legacy manifests retain their original whole-package behavior. The scope
    is an explicit exact package/asset set, never inferred from a WAV filename.
    """
    if 'nativeScope' not in manifest:
        return rows
    wanted = []
    for row in manifest['nativeScope']:
        key = (row['package'].casefold(), row['asset'])
        require(key not in wanted, 'duplicate approved native scope')
        wanted.append(key)
    require(bool(wanted), 'empty approved native scope')
    found = []
    for key in wanted:
        matches = [r for r in rows if (r.get('package', '').casefold(), r.get('asset')) == key]
        require(len(matches) == 1, 'approved native identity absent/ambiguous')
        found.extend(matches)
    return found


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for key in ('candidate', 'profile', 'cooked', 'decoder', 'ffmpeg', 'out'):
        ap.add_argument('--' + key, type=Path, required=True)
    ap.add_argument('--package', action='append', required=True)
    a = ap.parse_args()
    require(not a.out.exists(), 'output already exists')
    require(not a.out.resolve().is_relative_to(a.cooked.resolve()), 'output inside game refused')
    profile = json.loads(a.profile.read_text(encoding='utf-8-sig'))
    require(profile.get('approved') is True, 'unapproved baseline')
    pins = {r['path'].casefold(): r['sha256'].lower() for r in profile['files']}
    manifest_path = a.candidate / 'audio-payload-manifest.json'
    manifest = json.loads(manifest_path.read_text(encoding='utf-8-sig'))
    require(sha(a.profile.read_bytes()) == manifest['acceptedProfileSha256'].lower(), 'profile changed')
    lookup_path = a.candidate / 'payload/Binaries/vr-assets/localization/pl/polish_story_voice.jsonl'
    require(sha(lookup_path.read_bytes()) == manifest['lookupAfterSha256'].lower(), 'lookup changed')
    lookup = [json.loads(s) for s in lookup_path.read_text(encoding='utf-8').splitlines() if s.strip()]
    lookup = restrict_native_scope(lookup, manifest)
    wav_pins = {r['name']: r['candidateSha256'] for r in manifest['files']}
    packages = list(dict.fromkeys(a.package))
    require(len({p.casefold() for p in packages}) == len(packages), 'duplicate package')
    for package in packages:
        require(bool(re.fullmatch(r'[A-Za-z0-9_]+', package)), 'unsafe package')
        select_rows(lookup, package)
        relative = ('RvGame/CookedPC/' + package + '.xxx').casefold()
        require(relative in pins, 'native bank not pinned by accepted profile: ' + package)
        require(sha((a.cooked / (package + '.xxx')).read_bytes()) == pins[relative], 'installed bank drift')
    a.out.mkdir(parents=True)
    report = []
    def run(argv):
        return subprocess.run([str(a.ffmpeg), '-hide_banner', '-nostats', *argv],
                              capture_output=True, text=True, check=True).stderr
    for package in packages:
        folder = a.out / package
        folder.mkdir()
        source = a.cooked / (package + '.xxx')
        original = source.read_bytes()
        logical_path = folder / 'accepted.u'
        subprocess.run([str(a.decoder), str(source), str(logical_path)], capture_output=True, check=True)
        logical = logical_path.read_bytes()
        exports = {e['name']: e for e in inspect(logical)['exports'] if e['class_name'] == 'SoundNodeWave'}
        selected = select_rows(lookup, package)
        require(set(selected) <= set(exports), 'lookup/native export mismatch')
        replacements, lines, retained = {}, [], []
        for asset, row in selected.items():
            wav = a.candidate / 'payload/Binaries/vr-assets/audio/polish/story' / row['file']
            require(row['file'] in wav_pins and sha(wav.read_bytes()) == wav_pins[row['file']], 'WAV changed')
            props = {p['name']: p['value'] for p in exports[asset]['properties']}
            channels = props['NumChannels']
            require(channels in (1, 2) and props['SampleRate'] in (24000, 44100, 48000)
                    and props['RawPCMDataSize'] > 0 and props['RawPCMDataSize'] % (2 * channels) == 0, 'native format')
            target, receipt = fit(run, wav, folder, asset, props['SampleRate'],
                                  props['RawPCMDataSize'] // (2 * channels), channels=channels)
            if target is None or receipt['tempo'] > 1.35:
                retained.append(dict(receipt, asset=asset, disposition='accepted audio retained'))
                continue
            replacements[asset] = target.read_bytes()
            lines.append(dict(asset=asset, wav=row['file'], wav_sha256=wav_pins[row['file']],
                              spoken_text_matches=props.get('SpokenText') == row.get('text_en'), **receipt))
        if not replacements:
            report.append(dict(package=package, before=sha(original), after=None, lines=[],
                               retained=retained, status='NO_SAFE_TAKES_ACCEPTED_BANK_RETAINED'))
            (a.out / 'native-bank-report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
            print(package, 'NO SAFE TAKES; accepted bank retained', flush=True)
            continue
        expected, physical = build(original, logical, replacements, sha(original), sha(logical))
        require(unpack_physical(physical) == expected[0x71:], 'container inverse mismatch')
        packed = folder / 'candidate.xxx'
        packed.write_bytes(physical)
        decoded = folder / 'candidate-roundtrip.u'
        subprocess.run([str(a.decoder), str(packed), str(decoded)], capture_output=True, check=True)
        require(decoded.read_bytes() == expected, 'independent decoder mismatch')
        require(sha(source.read_bytes()) == sha(original), 'source changed during preparation')
        report.append(dict(package=package, before=sha(original), after=sha(physical),
                           lines=lines, retained=retained, status='OFFLINE_CONTAINER_PASS_RUNTIME_PENDING'))
        print(package, 'native takes', len(lines), 'retained', len(retained), flush=True)
        (a.out / 'native-bank-report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
