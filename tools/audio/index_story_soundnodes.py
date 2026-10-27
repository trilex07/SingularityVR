from pathlib import Path
import csv

root = Path(r'C:\Users\Trilex\vr-mods\SingularityVR-cutscene\audio\story_en_reference')
out = root / 'soundnodes_manifest.csv'
rows = []
for p in sorted(root.glob('SP_*_LOC_INT/SoundNodeWave/*.ogg')):
    asset = p.stem
    code = asset.rsplit('_', 1)[-1] if '_' in asset else ''
    if asset.endswith('_BAR') or asset.rsplit('_', 1)[-1].startswith('BAR'):
        speaker = 'barisov'
    elif '_DEM' in asset or '_DEMY' in asset:
        speaker = 'demichev'
    elif '_KAT' in asset or '_KATH' in asset:
        speaker = 'kathryn'
    elif '_DEV' in asset:
        speaker = 'devlin'
    elif '_REN' in asset:
        speaker = 'renko'
    elif asset.startswith(('SD_', 'FX_', 'SND_', 'UI_')):
        speaker = 'generic_or_non_dialogue'
    else:
        speaker = 'unclassified'
    rows.append({
        'package': p.parts[-3],
        'asset': asset,
        'speaker_guess': speaker,
        'path': p.relative_to(root).as_posix(),
        'bytes': p.stat().st_size,
    })
with out.open('w', newline='', encoding='utf-8') as f:
    w = csv.DictWriter(f, fieldnames=rows[0].keys())
    w.writeheader()
    w.writerows(rows)
print('indexed', len(rows), 'soundnodes')
