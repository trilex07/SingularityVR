"""Overlay only safe-duration codec refusals onto a pinned private candidate.

Never installs. Existing replaced speech remains byte-identical. Native package
builder checks every property, platform payload and non-wave export unchanged.
"""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import wave

from native_story_package import build
from native_voice_fit import MAX_DEPLOYMENT_TEMPO, fit
from patch_opening_dialogue import bulks, require
from ue3_sound_package import inspect


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def write(path, data):
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for arg in ('base', 'report', 'plan', 'staged', 'ffmpeg', 'decoder', 'out'):
        ap.add_argument('--'+arg, type=Path, required=True)
    ap.add_argument('--base-manifest-sha256', required=True)
    a = ap.parse_args()
    private_root = Path(__file__).resolve().parents[2] / 'build'
    require(a.out.resolve().is_relative_to(private_root), 'output outside private build refused')
    require(not a.out.exists() and not a.out.resolve().is_relative_to(a.base.resolve()), 'output exists/overlaps base')
    base_manifest_path = a.base / 'manifest.json'
    require(sha(base_manifest_path.read_bytes()) == a.base_manifest_sha256.lower(), 'base manifest changed')
    manifest, report, plan = read(base_manifest_path), read(a.report), read(a.plan)
    cases = plan['codec_only_cases']
    require(bool(cases) and all(r['tempo'] <= MAX_DEPLOYMENT_TEMPO for r in cases), 'not codec-only')
    require(len({(r['package'], r['asset']) for r in cases}) == len(cases), 'duplicate codec identity')
    pins = {r['path']: r for r in manifest['files']}
    for row in pins.values():
        require(sha((a.base / 'payload' / row['path']).read_bytes()) == row['after'], 'base payload drift')
    delivery = read(a.staged / 'audio-payload-manifest.json')
    lookup_path = a.staged / 'payload/Binaries/vr-assets/localization/pl/polish_story_voice.jsonl'
    require(sha(lookup_path.read_bytes()) == delivery['lookupAfterSha256'], 'lookup drift')
    lookup = {(r['package'].casefold(), r['asset']): r
              for r in map(json.loads, lookup_path.read_text(encoding='utf-8-sig').splitlines())}
    wav_pins = {r['name']: r['candidateSha256'] for r in delivery['files']}
    groups = defaultdict(list)
    for row in cases:
        relative = 'RvGame/CookedPC/'+row['package']+'.xxx'
        require(relative in pins, 'codec bank absent in base candidate')
        groups[row['package']].append(row)
    a.out.mkdir(parents=True)
    shutil.copytree(a.base/'payload', a.out/'payload')
    shutil.copytree(a.base/'runtime', a.out/'runtime')
    evidence = []
    for package, rows in groups.items():
        folder = a.out/'codec-recovery'/package
        folder.mkdir(parents=True)
        relative = 'RvGame/CookedPC/'+package+'.xxx'
        source = a.base/'payload'/relative
        logical_path = folder/'base.u'
        subprocess.run([str(a.decoder), str(source), str(logical_path)], check=True, capture_output=True)
        original, logical = source.read_bytes(), logical_path.read_bytes()
        exports = {e['name']: e for e in inspect(logical)['exports']}
        bank = next(r for r in report['banks'] if r['package'] == package)
        replacements, new_lines = {}, []
        def run(args):
            return subprocess.run([str(a.ffmpeg), '-hide_banner', '-nostats', *args],
                                  check=True, capture_output=True, text=True).stderr
        for row in rows:
            asset = row['asset']; lk = lookup[(package.casefold(), asset)]
            wav = a.staged/'payload/Binaries/vr-assets/audio/polish/story'/lk['file']
            require(sha(wav.read_bytes()) == wav_pins[lk['file']], 'source WAV drift')
            props = {p['name']: p['value'] for p in exports[asset]['properties']}
            rate, channels = props['SampleRate'], props['NumChannels']
            frames = props['RawPCMDataSize']//(2*channels)
            target, receipt = fit(run, wav, folder, asset, rate, frames, channels)
            require(target is not None and receipt['tempo'] <= MAX_DEPLOYMENT_TEMPO, 'codec repair failed')
            require(receipt['repacketized_single_page'] and receipt['measured_tail_padding_frames'] == 0,
                    'unexpected codec repair route')
            # libvorbis as a second decoder; both share Ogg demuxing, so retain
            # packet/EOS invariance evidence too, not just decoder agreement.
            second = folder/(asset+'.libvorbis.wav')
            run(['-v','error','-n','-c:a','libvorbis','-i',str(target),'-c:a','pcm_s16le',str(second)])
            with wave.open(str(second)) as w:
                require((w.getnframes(), w.getframerate(), w.getnchannels()) == (frames, rate, channels),
                        'second decoder disagreement')
            replacements[asset] = target.read_bytes()
            new_lines.append(dict(asset=asset, wav=lk['file'], wav_sha256=wav_pins[lk['file']], **receipt))
        expected, physical = build(original, logical, replacements, sha(original), sha(logical))
        candidate = a.out/'payload'/relative
        candidate.write_bytes(physical)
        decoded = folder/'candidate-roundtrip.u'
        subprocess.run([str(a.decoder), str(candidate), str(decoded)], check=True, capture_output=True)
        require(decoded.read_bytes() == expected, 'independent package roundtrip failed')
        new_exports = {e['name']: e for e in inspect(expected)['exports']}
        for line in bank['lines']:
            asset = line['asset']
            require(bulks(logical, exports[asset])[1][1] == bulks(expected, new_exports[asset])[1][1],
                    'previously packed speech changed: '+asset)
        evidence.append(dict(package=package, before=sha(original), after=sha(physical),
                             added=[r['asset'] for r in new_lines],
                             previous_packed_takes_preserved=len(bank['lines']),
                             container_roundtrip='PASS', native_properties_and_other_exports='PASS'))
        recovered = set(replacements)
        bank['lines'].extend(new_lines)
        bank['retained'] = [r for r in bank['retained'] if r['asset'] not in recovered]
        bank['after'] = sha(physical); bank['candidate_private_path'] = str(candidate.resolve())
        pins[relative]['after'] = sha(physical)
    frozen = report['summary']['takes_replaced']
    retained = [dict(row, package=b['package']) for b in report['banks'] for row in b['retained']]
    report['retained'] = retained
    report['summary'].update(takes_replaced=sum(len(b['lines']) for b in report['banks']),
                             takes_retained=len(retained),
                             reasons=dict(Counter(r.get('reason','tempo exceeds 1.35') for r in retained)))
    for r in manifest['files']:
        require(sha((a.out/'payload'/r['path']).read_bytes()) == r['after'], 'final payload drift')
    write(a.out/'manifest.json', manifest)
    write(a.out/'combined-report.json', report)
    write(a.out/'codec-recovery.json', dict(base_manifest_sha256=a.base_manifest_sha256,
          recovered=len(cases), previous_packed_takes_preserved=frozen, banks=evidence,
          decoder_frames='PASS (Vorbis and libvorbis)', compressed_speech_packets='IDENTICAL',
          speech_cropped=False, native_metadata_changed=False, steam_written=False,
          physical_acceptance='PENDING'))
    print(json.dumps(report['summary']))


if __name__ == '__main__':
    main()
