"""Build private voice provenance from native properties and explicit translations.

No game writes, transcription guessing, model downloads or copyrighted fixtures.
The output and translation input must remain private, outside tracked source.
"""
import argparse
import hashlib
import json
from pathlib import Path
from ue3_sound_package import inspect


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory(path):
    data = path.read_bytes()
    sha = hashlib.sha256(data).hexdigest()
    rows = []
    for export in inspect(data)['exports']:
        if export['class_name'] != 'SoundNodeWave':
            continue
        props = {p['name']: p for p in export['properties']}
        if 'SpokenText' not in props:
            continue
        rate = props['SampleRate']['value']
        pcm = props['RawPCMDataSize']['value']
        channels = props['NumChannels']['value']
        if channels != 1 or rate not in (24000, 44100, 48000) or pcm <= 0 or pcm % 2:
            raise ValueError('unsupported native format: ' + export['name'])
        text = props['SpokenText']
        rows.append(dict(package=path.stem, asset=export['name'],
                         line_id=path.stem+'/SoundNodeWave/'+export['name']+'.ogg',
                         source_english=text['value'], source_logical_sha256=sha,
                         source_property_offset=text['value_offset'],
                         source_property_sha256=hashlib.sha256(data[text['value_offset']:
                             text['value_offset']+text['size']]).hexdigest(),
                         native_frames=pcm//2, native_rate=rate,
                         native_duration=(pcm//2)/rate))
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--logical', type=Path, action='append', required=True)
    ap.add_argument('--translations', type=Path, required=True)
    ap.add_argument('--old-manifest', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    if args.out.exists():
        raise ValueError('refuse existing output')
    natives = [row for path in args.logical for row in inventory(path)]
    by_id = {row['line_id']: row for row in natives}
    if len(by_id) != len(natives):
        raise ValueError('duplicate native line identity')
    old = {r['line_id']: r for r in map(json.loads,
        args.old_manifest.read_text(encoding='utf-8').splitlines())}
    translations = json.loads(args.translations.read_text(encoding='utf-8'))
    if not translations or len({r['line_id'] for r in translations}) != len(translations):
        raise ValueError('empty/duplicate translation input')
    output = []
    for translation in translations:
        row = dict(by_id[translation['line_id']])
        if translation['expected_english'] != row['source_english']:
            raise ValueError('source text mismatch: '+row['line_id'])
        if not translation['text_pl'].strip() or not translation['translation_note'].strip():
            raise ValueError('missing reviewed translation/provenance')
        row.update(translation)
        row['reference_sha256'] = digest(Path(row['reference']))
        if row['line_id'] in old:
            previous = old[row['line_id']]
            row['old_source_english'] = previous['source_english']
            row['old_polish'] = previous['corrected_polish']
            row['old_wav_sha256'] = digest(Path(previous['file']))
            row['old_wav'] = previous['file']
            row['legacy_wav'] = Path(previous['previous_audio']).name
        row['text_provenance'] = 'native_SpokenText_property; SIDE manual Polish translation'
        output.append(row)
    args.out.mkdir(parents=True)
    (args.out/'inventory.json').write_text(json.dumps(natives,ensure_ascii=False,indent=2),encoding='utf-8')
    (args.out/'lines.json').write_text(json.dumps(output,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(dict(native_count=len(natives), selected=len(output),
        source_manifest_sha256=digest(args.old_manifest),
        translation_sha256=digest(args.translations))))


if __name__ == '__main__':
    main()
