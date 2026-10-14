"""Offline volume-only radio candidate using typed cue/wave evidence.

Retains all other prior candidate voices, native timing/properties, mixer and
cue graph. Reuses generated speech; no TTS or new translation generation.
"""
import argparse
import hashlib
import json
import math
import subprocess
import wave
from pathlib import Path
from inspect_radio_conversations import conversations
from native_story_package import build
from patch_opening_dialogue import bulks, require
from ue3_sound_package import inspect


def measure(run, path):
    text = run(['-i', str(path), '-af',
                'loudnorm=I=-24:TP=-1:LRA=11:print_format=json', '-f', 'null', '-'])
    start = text.rfind('{'); end = text.find('}', start)
    require(start >= 0 and end > start, 'missing loudness measurement')
    result = json.loads(text[start:end + 1])
    require(all(math.isfinite(float(result[k])) for k in ('input_i', 'input_tp')),
            'silent/invalid speech')
    return result


def match(run, source, reference, folder, asset, rate, frames):
    native = measure(run, reference); before = measure(run, source)
    target_i = float(native['input_i'])
    require(-40 <= target_i <= -3, 'native loudness outside observed safety range')
    gain = target_i - float(before['input_i'])
    require(abs(gain) <= 24, 'excessive gain refused')
    target = folder / (asset + '.ogg')
    # Real VI1_0110 measurements converge monotonically under the peak limiter:
    # -14.94,-14.27,-13.83,-13.46,-13.19 vs native -12.11 LUFS. Five passes
    # miss the 1 LU gate by .08 LU. Keep the established absolute eight-pass
    # ceiling, re-rendering from the same source rather than accumulating encodes.
    for attempt in range(8):
        require(abs(gain) <= 30, 'cumulative gain safety limit')
        temporary = folder / (asset + f'.gain{attempt + 1}.ogg')
        filters = (f'volume={gain:.6f}dB,alimiter=limit=0.8:level=disabled:latency=1,'
                   f'aresample={rate},apad=whole_len={frames},atrim=end_sample={frames}')
        run(['-v', 'error', '-n', '-i', str(source), '-af', filters,
             '-ac', '1', '-ar', str(rate), '-c:a', 'libvorbis', '-q:a', '3', str(temporary)])
        after = measure(run, temporary)
        delta = target_i - float(after['input_i'])
        if abs(delta) <= 1.0:
            require(float(after['input_tp']) < -.2, 'encoded true peak gate')
            temporary.rename(target)
            break
        gain += delta
    require(target.exists(), 'bounded radio loudness correction failed')
    decoded = folder / (asset + '.decoded.wav')
    run(['-v', 'error', '-n', '-i', str(target), '-c:a', 'pcm_s16le', str(decoded)])
    with wave.open(str(decoded)) as w:
        require(w.getnframes() == frames and w.getframerate() == rate and
                w.getnchannels() == 1 and w.getsampwidth() == 2, 'native PCM budget changed')
    return target, dict(native_lufs=target_i, before_lufs=float(before['input_i']),
                        after_lufs=float(after['input_i']), true_peak=float(after['input_tp']),
                        gain_db=gain, attempts=attempt + 1, frames=frames, rate=rate)


def sha(data): return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    for arg in ('prep', 'logical-directory', 'cooked-directory', 'radio-graph',
                'ffmpeg', 'decoder', 'out'):
        parser.add_argument('--' + arg, type=Path, required=True)
    parser.add_argument('--asset', action='append', required=True,
                        help='Explicit proven wave identity; no blanket gain changes')
    args = parser.parse_args()
    require(not args.out.exists(), 'output already exists')
    graph = conversations(args.radio_graph)
    radio = {w['name'] for e in graph for row in e['conversation'] for w in row['waves']
             if w.get('class_name') == 'SoundNodeWave' and w['status'] == 'import wave'}
    require(len(radio) == 17 and all(n.startswith('VI1_') for n in radio),
            'original radio graph identity changed')
    selected = set(args.asset)
    require(len(selected) == len(args.asset) and selected <= radio,
            'duplicate/unproven radio target')
    manifests = json.loads((args.prep / 'native-voices-v3/native-story.json').read_text())
    args.out.mkdir(parents=True)
    (args.out / 'radio-graph.json').write_text(json.dumps(graph, indent=2), encoding='utf-8')
    result = []

    def run(argv):
        return subprocess.run([str(args.ffmpeg), '-hide_banner', '-nostats', *argv],
                              capture_output=True, text=True, check=True).stderr

    for bank in manifests:
        package = bank['package']
        if package == 'SP_VI_intro_Kismet_LOC_INT': continue
        original = (args.cooked_directory / (package + '.xxx')).read_bytes()
        require(sha(original) == bank['source_sha'].lower(), 'original bank changed')
        logical = (args.logical_directory / (package + '.u')).read_bytes()
        prior_file = args.prep / 'native-voices-v3' / package / 'candidate.xxx'
        require(sha(prior_file.read_bytes()) == bank['candidate_sha'].lower(), 'prior candidate changed')
        prior = (prior_file.parent / 'candidate.roundtrip.u').read_bytes()
        old = {e['name']: e for e in inspect(logical)['exports'] if e['class_name'] == 'SoundNodeWave'}
        current = {e['name']: e for e in inspect(prior)['exports'] if e['class_name'] == 'SoundNodeWave'}
        require(set(old) == set(current), 'bank identity mismatch')
        folder = args.out / package; folder.mkdir()
        original_decoded = folder / 'original.roundtrip.u'
        subprocess.run([str(args.decoder), str(args.cooked_directory / (package + '.xxx')),
                        str(original_decoded)], capture_output=True, check=True)
        require(original_decoded.read_bytes() == logical, 'inventory differs from original package decode')
        replacements = {}; lines = []
        for asset, e in old.items():
            native = bulks(logical, e)[1][1]; candidate = bulks(prior, current[asset])[1][1]
            if native == candidate: continue  # includes refused English short lines
            replacements[asset] = candidate  # keep all previously reviewed non-radio replacements
            if asset not in selected: continue
            source = folder / (asset + '.before.ogg'); source.write_bytes(candidate)
            reference = folder / (asset + '.native.ogg'); reference.write_bytes(native)
            props = {p['name']: p['value'] for p in e['properties']}
            frames = props['RawPCMDataSize'] // 2; rate = props['SampleRate']
            decoded = folder / (asset + '.before.wav')
            run(['-v', 'error', '-n', '-i', str(source), '-c:a', 'pcm_s16le', str(decoded)])
            with wave.open(str(decoded)) as w:
                require(w.getnframes() == frames and w.getframerate() == rate,
                        'prior timing budget mismatch; volume cannot crop speech')
            target, proof = match(run, source, reference, folder, asset, rate, frames)
            replacements[asset] = target.read_bytes()
            lines.append(dict(asset=asset, **proof))
            print(package, asset, proof['before_lufs'], '->', proof['after_lufs'], flush=True)
        if lines:
            expected, packed = build(original, logical, replacements, bank['source_sha'], sha(logical))
        else:
            expected, packed = prior, prior_file.read_bytes()  # unrelated banks byte-identical
        candidate_path = folder / 'candidate.xxx'; candidate_path.write_bytes(packed)
        decoded_path = folder / 'candidate.roundtrip.u'
        subprocess.run([str(args.decoder), str(candidate_path), str(decoded_path)],
                       capture_output=True, check=True)
        require(decoded_path.read_bytes() == expected, 'independent LZO mismatch')
        after = {e['name']: e for e in inspect(expected)['exports'] if e['class_name'] == 'SoundNodeWave'}
        changed = {r['asset'] for r in lines}
        for asset, e in current.items():
            require(prior[e['offset']:e['property_end']] ==
                    expected[after[asset]['offset']:after[asset]['property_end']], 'candidate properties changed')
            if asset not in changed:
                require(bulks(prior, e)[1][1] == bulks(expected, after[asset])[1][1],
                        'non-radio candidate audio changed')
        result.append(dict(package=package, source_sha=bank['source_sha'],
                           prior_candidate_sha=bank['candidate_sha'], candidate_sha=sha(packed), lines=lines,
                           status='OFFLINE_VOLUME_ONLY_PASS_NOT_EVENT_OR_AUDITORY_ACCEPTANCE' if lines else 'UNCHANGED_PRIOR_BANK'))
        (args.out / 'radio-loudness.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    require({line['asset'] for bank in result for line in bank['lines']} == selected,
            'selected target was not a prior translated wave')
    print('OFFLINE ONLY: radio timing/properties and non-radio speech preserved; no Steam writes', flush=True)


if __name__ == '__main__': main()
