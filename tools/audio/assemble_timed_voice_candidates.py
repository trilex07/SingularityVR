"""Hash-verify selected private offline takes; do not install or claim acceptance."""
import argparse
import array
import hashlib
import json
import shutil
import sys
import wave
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument('--choices',type=Path,required=True,help='asset to private fitted.json mapping')
ap.add_argument('--out',type=Path,required=True)
args = ap.parse_args()
if args.out.exists():
    raise ValueError('refuse existing output')
choices = json.loads(args.choices.read_text(encoding='utf-8'))
reports = {}
rows = []
for asset,path in choices.items():
    manifest = Path(path).resolve()
    if manifest not in reports:
        reports[manifest] = {r['asset']:r for r in json.loads(manifest.read_text(encoding='utf-8'))}
    row = dict(reports[manifest][asset])
    if row['timing_status'] != 'PASS_TIMING_ONLY_NOT_AUDITIONED':
        raise ValueError('refuse timing failure: '+asset)
    source = Path(row['file'])
    if hashlib.sha256(source.read_bytes()).hexdigest() != row['sha256']:
        raise ValueError('source hash changed: '+asset)
    with wave.open(str(source)) as wav:
        if (wav.getnchannels(),wav.getsampwidth(),wav.getframerate()) != (1,2,24000):
            raise ValueError('invalid format: '+asset)
        audio = array.array('h',wav.readframes(wav.getnframes()))
        if sys.byteorder != 'little':
            audio.byteswap()
        if not audio:
            raise ValueError('empty audio')
        peak = max(abs(value) for value in audio)
        clipped = sum(abs(value) >= 32760 for value in audio)/len(audio)
        if peak <= 0 or clipped > .001:
            raise ValueError('silent/clipped audio')
        row.update(structural_status='PASS_MONO_PCM16_24K_HASH_TIMING',sample_peak=peak/32768,
                   clipped_fraction=clipped,auditory_status='NOT_AUDITIONED',
                   native_playback_status='NOT_TESTED',selected_fitted_manifest=str(manifest))
    rows.append(row)
args.out.mkdir(parents=True)
for row in rows:
    target = args.out/(row['asset']+'.wav')
    shutil.copyfile(row['file'],target)
    if hashlib.sha256(target.read_bytes()).hexdigest() != row['sha256']:
        raise ValueError('copy changed')
    row['file'] = str(target.resolve())
(args.out/'selected.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(dict(selected=len(rows),structural_pass=len(rows),not_auditioned=len(rows))))
