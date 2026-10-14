"""Offline private source-bound room voice delivery, not native packaging/deploy."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path
from prepare_room_voice_audit import verify_wave


def bind_voice(native,voice):
    if (native['asset'],native['source_english'],native['native_frames'],native['native_rate'])!=(
            voice['asset'],voice['source_english'],voice['native_frames'],voice['native_rate']):
        raise ValueError('native voice content/budget mismatch')
    output = dict(native)
    for key in ('file','sha256','selected_wav_sha256','selected_wav_duration','qa_status',
                'asr_detected','asr_word_error','asr_flags','required_tempo','trim_evidence',
                'generated_source_wav_sha256','generated_source_file','generation_seed',
                'generation_temperature','generation_max_tokens','generation_frames_after_eos'):
        if key in voice:
            output[key] = voice[key]
    output.update(auditory_status='NOT_AUDITIONED',native_encoding_status='NOT_ENCODED_OR_DECODE_COUNT_VERIFIED',
                  runtime_playback_identity='NOT_OBSERVED',
                  qa_limit='ASR screen only, not acting/translation/native playback approval')
    return output


def main():
    ap = argparse.ArgumentParser()
    for arg in ('audit','voices','qa','out'):
        ap.add_argument('--'+arg,type=Path,required=True)
    args = ap.parse_args()
    if args.out.exists():
        raise ValueError('refuse existing output')
    native = json.loads(args.audit.read_text(encoding='utf-8'))
    source = json.loads(args.voices.read_text(encoding='utf-8'))
    qa = {r['asset']:r for r in json.loads(args.qa.read_text(encoding='utf-8'))}
    if len({r['asset'] for r in source})!=len(source) or set(qa)!={r['asset'] for r in source}:
        raise ValueError('candidate/QA uniqueness or coverage failure')
    output = []
    for voice in source:
        if voice.get('timing_status','PASS_TIMING_ONLY_NOT_AUDITIONED')!='PASS_TIMING_ONLY_NOT_AUDITIONED':
            raise ValueError('unsafe timing candidate cannot enter delivery')
        current,duration = verify_wave(Path(voice['file']),voice['sha256'])
        if duration>voice['native_frames']/voice['native_rate']:
            raise ValueError('candidate exceeds native budget')
        row = dict(voice,sha256=current,selected_wav_sha256=current,selected_wav_duration=duration)
        for key in ('qa_status','asr_detected','asr_word_error','asr_flags'):
            row[key] = qa[voice['asset']][key]
        row.update(auditory_status='NOT_AUDITIONED',native_encoding_status='NOT_ENCODED_OR_DECODE_COUNT_VERIFIED')
        output.append(row)
    by_asset = {r['asset']:r for r in output}
    if set(by_asset)!={r['asset'] for r in native}:
        raise ValueError('native inventory not completely covered')
    # Validate all nineteen native identities BEFORE writing any delivery file.
    bindings = [bind_voice(r,by_asset[r['asset']]) for r in native]
    args.out.mkdir(parents=True)
    for voice in output:
        target = args.out/(voice['asset']+'.wav')
        shutil.copyfile(voice['file'],target)
        if hashlib.sha256(target.read_bytes()).hexdigest()!=voice['selected_wav_sha256']:
            raise ValueError('copied WAV hash mismatch')
        voice['file'] = str(target.resolve())
    bindings = [bind_voice(r,by_asset[r['asset']]) for r in native]
    banks = {r['package']:dict(source_bank_sha256=r['source_bank_sha256'],
                             source_logical_sha256=r['source_logical_sha256']) for r in native}
    for filename,data in (('selected.json',output),('native-bindings.json',bindings),('source-banks.json',banks)):
        (args.out/filename).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
    gates = [dict(line_id=r['line_id'],selected_wav_sha256=r['selected_wav_sha256'],
                  required_native_decoded_frames=r['native_frames'],required_native_rate=r['native_rate'],
                  required_native_channels=1,status='NOT_ENCODED_OR_DECODE_COUNT_VERIFIED') for r in bindings]
    (args.out/'native-encoding-gates.json').write_text(json.dumps(gates,indent=2),encoding='utf-8')
    from collections import Counter
    print(json.dumps(dict(selected=len(output),bindings=len(bindings),source_banks=len(banks),
                         qa_status=dict(Counter(r['qa_status'] for r in output)),
                         no_native_package_or_Steam_writes=True)))


if __name__=='__main__':
    main()
