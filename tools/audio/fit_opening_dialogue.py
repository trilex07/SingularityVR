"""Fit reviewed candidate lines to native mono/48k PCM frame budgets.

Does not truncate speech to make it fit: excessive tempo changes refuse.
Generated clips and manifests belong outside Git, not in release archives.
"""
import argparse
import hashlib
import json
import subprocess
import wave
from pathlib import Path
from ue3_sound_package import inspect
from patch_opening_dialogue import TARGETS, require

ap = argparse.ArgumentParser()
ap.add_argument('--logical', type=Path, required=True)
ap.add_argument('--manifest', type=Path, required=True)
ap.add_argument('--pilot-manifest', type=Path, required=True)
ap.add_argument('--ffmpeg', type=Path, required=True)
ap.add_argument('--out', type=Path, required=True)
args = ap.parse_args()
rows = json.loads(args.manifest.read_text(encoding='utf-8'))
pilots = json.loads(args.pilot_manifest.read_text(encoding='utf-8'))
require({r['asset'] for r in pilots} == {'INTRO_1006_CP','INTRO_1010_CP','INTRO_1016_CP'}, 'pilot coverage')
rows = {r['asset']:r for r in rows if r['role'] != 'pilot'}
rows.update({r['asset']:r for r in pilots})
require(set(rows) == TARGETS, 'dialogue coverage')
args.out.mkdir(parents=True, exist_ok=True)
results = []
for e in inspect(args.logical.read_bytes())['exports']:
    if e['name'] not in rows:
        continue
    row = rows[e['name']]
    source = Path(row['file'])
    require(hashlib.sha256(source.read_bytes()).hexdigest() == row['sha256'], 'generated clip changed')
    props = {p['name']:p['value'] for p in e['properties']}
    require(props['SampleRate'] == 48000 and props['NumChannels'] == 1, 'source format')
    require(props['RawPCMDataSize'] % 2 == 0, 'native PCM alignment')
    frames = props['RawPCMDataSize']//2
    budget = frames/48000
    trimmed = args.out/(e['name']+'.trim.wav')
    filtered = ('silenceremove=start_periods=1:start_duration=0.03:start_threshold=-50dB,'
                'areverse,silenceremove=start_periods=1:start_duration=0.03:start_threshold=-50dB,areverse')
    subprocess.run([str(args.ffmpeg),'-v','error','-n','-i',str(source),'-af',filtered,
                    '-ac','1','-ar','48000','-c:a','pcm_s16le',str(trimmed)], check=True)
    with wave.open(str(trimmed)) as wav:
        duration = wav.getnframes()/wav.getframerate()
    # Leave 20ms after the complete utterance; apad fills any longer native tail.
    tempo = max(1.0, duration/(budget-0.02))
    require(tempo <= 1.85, 'unsafe speech speed: '+e['name']+' '+str(tempo))
    target = args.out/(e['name']+'.ogg')
    subprocess.run([str(args.ffmpeg),'-v','error','-n','-i',str(trimmed),'-af',
                    f'atempo={tempo:.9f},apad=whole_len={frames},atrim=end_sample={frames}',
                    '-ac','1','-ar','48000','-c:a','libvorbis','-q:a','3',str(target)], check=True)
    decoded = args.out/(e['name']+'.decoded.wav')
    subprocess.run([str(args.ffmpeg),'-v','error','-n','-i',str(target),'-c:a','pcm_s16le',str(decoded)], check=True)
    with wave.open(str(decoded)) as wav:
        require(wav.getnframes() == frames and wav.getframerate() == 48000 and
                wav.getnchannels() == 1, 'decoded native frame budget mismatch')
    result = dict(asset=e['name'], source_file=str(source), file=str(target),
                  decoded_file=str(decoded), text_pl=row['text_pl'],
                  native_duration=budget, trimmed_duration=duration, tempo=tempo,
                  sha256=hashlib.sha256(target.read_bytes()).hexdigest(),
                  frames=frames, status='decoder_and_duration_PASS_not_auditioned')
    results.append(result)
    print(e['name'], 'tempo', round(tempo,3), 'frames', frames, flush=True)
(args.out/'fitted.json').write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding='utf-8')
