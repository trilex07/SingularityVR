"""Export unambiguous selected hashes and source-verified native bindings offline.

Historical input hashes are provenance, never identities of the selected WAV.
Original speech/text and output reports remain private. No game writes/launches.
"""
import argparse
import hashlib
import json
import wave
from pathlib import Path
from prepare_timed_voice_manifest import inventory


def selected_identity(row):
    source = Path(row['file']).resolve()
    actual = hashlib.sha256(source.read_bytes()).hexdigest()
    if actual.lower()!=row['sha256'].lower():
        raise ValueError('selected WAV hash mismatch: '+row['asset'])
    with wave.open(str(source)) as wav:
        if (wav.getnchannels(),wav.getsampwidth(),wav.getframerate())!=(1,2,24000):
            raise ValueError('selected WAV format mismatch')
        duration = wav.getnframes()/wav.getframerate()
    if duration>row['native_frames']/row['native_rate']-.005:
        raise ValueError('selected WAV exceeds native budget')
    fields = ('asset','line_id','package','text_pl','source_english','translation_note',
              'text_provenance','tts_text','tts_segments','pronunciation_note','reference',
              'reference_sha256','generation_seed','generation_temperature','generation_segments',
              'generation_max_tokens','generation_frames_after_eos','native_frames','native_rate',
              'trim_evidence','generated_source_wav_sha256','generated_source_file',
              'native_duration','required_tempo','structural_status','qa_status','asr_detected',
              'asr_word_error','asr_flags','auditory_status','native_playback_status')
    output = {key:row[key] for key in fields if key in row}
    output.update(file=str(source),sha256=actual,selected_wav_sha256=actual,
                  selected_wav_duration=duration,
                  native_encoding_status='NOT_ENCODED_OR_DECODE_COUNT_VERIFIED',
                  historical_input_hashes={key:row[key] for key in (
                      'candidate_wav_sha256','old_wav_sha256','upstream_input_wav_sha256',
                      'previous_selected_sha256','source_sha') if row.get(key)})
    return output


def bind(selected,native):
    if selected['asset']!=native['asset'] or selected['source_english']!=native['source_english']:
        raise ValueError('native source identity/text mismatch')
    if (selected['native_frames'],selected['native_rate'])!=(native['native_frames'],native['native_rate']):
        raise ValueError('native timing budget mismatch')
    return dict(selected,**{key:native[key] for key in ('package','line_id',
        'source_logical_sha256','source_property_offset','source_property_sha256')},
        binding_status='NATIVE_TEXT_BUDGET_IDENTITY_MATCH_OFFLINE')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--selected',type=Path,required=True)
    ap.add_argument('--logical',type=Path,action='append',required=True)
    ap.add_argument('--existing-manifest',type=Path)
    ap.add_argument('--coverage-prefix',default='VI1_')
    ap.add_argument('--out',type=Path,required=True)
    args = ap.parse_args()
    if args.out.exists():
        raise ValueError('refuse existing output')
    selected = [selected_identity(r) for r in json.loads(args.selected.read_text(encoding='utf-8'))]
    if len({r['asset'] for r in selected})!=len(selected):
        raise ValueError('duplicate selected asset')
    by_asset = {r['asset']:r for r in selected}
    native = [r for path in args.logical for r in inventory(path)]
    bindings = []
    for row in native:
        if row['asset'] in by_asset and row['source_english']==by_asset[row['asset']]['source_english']:
            bindings.append(bind(by_asset[row['asset']],row))
    if {r['asset'] for r in bindings}!=set(by_asset):
        raise ValueError('selected asset lacks native evidence')
    existing = {}
    if args.existing_manifest:
        existing = {r['line_id']:r for r in (json.loads(raw) for raw in
            args.existing_manifest.read_text(encoding='utf-8').splitlines() if raw.strip())}
    coverage = []
    for row in native:
        if not row['asset'].startswith(args.coverage_prefix):
            continue
        previous = existing.get(row['line_id'])
        audio = Path(previous['file']) if previous else None
        coverage.append(dict(row,in_selected=row['asset'] in by_asset,
            in_existing_pack=previous is not None,
            existing_audio_present=audio is not None and audio.is_file(),
            existing_audio_sha256=hashlib.sha256(audio.read_bytes()).hexdigest()
                if audio is not None and audio.is_file() else None,
            runtime_playback_identity='NOT_OBSERVED'))
    args.out.mkdir(parents=True)
    for name,rows in (('selected.json',selected),('native-bindings.json',bindings),('coverage.json',coverage)):
        (args.out/name).write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
    gates = [dict(asset=row['asset'],line_id=row['line_id'],
                  selected_wav_sha256=row['selected_wav_sha256'],
                  required_decoded_native_frames=row['native_frames'],
                  required_decoded_native_rate=row['native_rate'],
                  required_decoded_native_channels=1,
                  status='NOT_ENCODED_OR_DECODE_COUNT_VERIFIED',
                  rule='Exact independent decode count required; do not crop speech or alter native properties to pass')
             for row in bindings]
    (args.out/'native-encoding-gates.json').write_text(json.dumps(gates,indent=2),encoding='utf-8')
    print(json.dumps(dict(selected=len(selected),bindings=len(bindings),
        inventory_radio_assets=len(coverage),radio_in_existing_pack=sum(r['in_existing_pack'] for r in coverage))))


if __name__=='__main__':
    main()
