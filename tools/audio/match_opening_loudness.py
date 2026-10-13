"""Match native opening voice loudness; no music/FX/mixer changes or installs."""
import argparse
import json
import math
import subprocess
import wave
from pathlib import Path
from ue3_sound_package import inspect
from patch_opening_dialogue import bulks, TARGETS, require

ap = argparse.ArgumentParser()
ap.add_argument('--logical', type=Path, required=True)
ap.add_argument('--input', type=Path, required=True)
ap.add_argument('--ffmpeg', type=Path, required=True)
ap.add_argument('--out', type=Path, required=True)
ap.add_argument('--boost-db', type=float, default=0.0,
                help='Explicit speech-only loudness target above native; 0 preserves original policy')
ap.add_argument('--max-corrections', type=int, default=5)
args = ap.parse_args()
require(math.isfinite(args.boost_db) and 0.0 <= args.boost_db <= 6.0, 'boost outside safe experiment range')
require(1 <= args.max_corrections <= 8, 'unbounded corrections refused')
require(not args.out.exists(), 'refuse overwrite')
data = args.logical.read_bytes()
exports = inspect(data)['exports']
args.out.mkdir(parents=True)
(args.out/'references').mkdir()
(args.out/'passes').mkdir()


def run(arguments):
    return subprocess.run([str(args.ffmpeg), '-hide_banner', '-nostats', *arguments],
                          capture_output=True, text=True, check=True).stderr


def measure(path):
    text = run(['-i', str(path), '-af', 'loudnorm=I=-24:TP=-1:LRA=11:print_format=json', '-f', 'null', '-'])
    p = text.rfind('{')
    q = text.find('}', p)
    require(p >= 0 and q > p, 'missing loudness result')
    result = json.loads(text[p:q+1])
    require(all(math.isfinite(float(result[k])) for k in ('input_i','input_tp','input_lra')), 'silent/invalid speech')
    return result


results = []
for e in exports:
    if e['name'] not in TARGETS:
        continue
    original = args.out/'references'/(e['name']+'.ogg')
    original.write_bytes(bulks(data,e)[1][1])
    source = args.input/(e['name']+'.ogg')
    native = measure(original)
    before = measure(source)
    props = {p['name']:p['value'] for p in e['properties']}
    frames = props['RawPCMDataSize']//2
    target = args.out/(e['name']+'.ogg')
    original_i = float(native['input_i'])
    target_i = original_i + args.boost_db
    # Short spoken lines can defeat loudnorm's long-window adaptive gain.
    # Derive gain from measured gated loudness, constrain peaks with a
    # look-ahead limiter, then verify the ENCODED result. Never hard-clip or
    # change the global game mixer. Five bounded corrections at most: measured
    # short radio lines converged monotonically but exceeded 1 LU after three.
    gain_db = target_i-float(before['input_i'])
    # Original crash shouting has unusually dense radio compression. Keep up
    # to 1.5 LU below it rather than forcing stronger limiting; other dialogue
    # stays within 1 LU. This exception is explicit in the output manifest.
    tolerance = 1.5 if e['name'] == 'INTRO_1016_CP' else 1.0
    for attempt in range(args.max_corrections):
        temp = args.out/'passes'/(e['name']+f'.pass{attempt+1}.ogg')
        filters = (f'volume={gain_db:.6f}dB,alimiter=limit=0.8:level=disabled:latency=1,'
                   f'aresample=48000,apad=whole_len={frames},atrim=end_sample={frames}')
        run(['-n','-i',str(source),'-af',filters,'-ac','1','-ar','48000',
             '-c:a','libvorbis','-q:a','3',str(temp)])
        after = measure(temp)
        delta = target_i-float(after['input_i'])
        if abs(delta) <= tolerance:
            temp.rename(target)
            break
        gain_db += delta
    require(target.exists(), 'bounded loudness corrections failed')
    require(abs(float(after['input_i'])-target_i) <= tolerance, 'loudness outside native budget')
    require(float(after['input_tp']) < -0.2, 'encoded true-peak headroom failed')
    decoded = args.out/(e['name']+'.decoded.wav')
    run(['-n','-i',str(target),'-c:a','pcm_s16le',str(decoded)])
    with wave.open(str(decoded)) as wav:
        require(wav.getnframes()==frames and wav.getframerate()==48000 and
                wav.getnchannels()==1, 'native PCM budget changed')
    results.append(dict(asset=e['name'], original_lufs=original_i,
                        target_lufs=target_i, speech_target_boost_db=args.boost_db,
                        before_lufs=float(before['input_i']), after_lufs=float(after['input_i']),
                        true_peak_dbtp=float(after['input_tp']), frames=frames,
                        applied_gain_db=gain_db,
                        allowed_difference_lu=tolerance,
                        status='offline_loudness_and_decoder_PASS_not_auditioned'))
    (args.out/'loudness.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
    print(e['name'], target_i, before['input_i'], after['input_i'], flush=True)
(args.out/'loudness.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
