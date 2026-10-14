"""Private native-room voice inventory and explicit source-text semantic audit.

Read-only game sources; no audio extraction, native writes or model downloads.
Full source speech/translation and generated output must remain outside Git.
"""
import argparse
import array
import hashlib
import json
import wave
import sys
from pathlib import Path
from ue3_sound_package import inspect


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory(logical, source, expected):
    data = logical.read_bytes()
    source_sha, logical_sha = sha(source), hashlib.sha256(data).hexdigest()
    if source_sha!=expected['source_sha256'].lower() or logical_sha!=expected['logical_sha256'].lower():
        raise ValueError('source bank pin mismatch')
    rows = []
    for export in inspect(data)['exports']:
        if export['class_name']!='SoundNodeWave':
            continue
        props = {p['name']:p for p in export['properties']}
        spoken = props['SpokenText']
        rate,pcm = props['SampleRate']['value'],props['RawPCMDataSize']['value']
        if props['NumChannels']['value']!=1 or rate!=48000 or pcm<=0 or pcm%2:
            raise ValueError('unsupported native timing metadata')
        rows.append(dict(package=logical.stem,asset=export['name'],
            line_id=logical.stem+'/SoundNodeWave/'+export['name']+'.ogg',
            source_bank_sha256=source_sha,source_logical_sha256=logical_sha,
            source_english=spoken['value'],source_property_offset=spoken['value_offset'],
            source_property_size=spoken['size'],
            source_property_sha256=hashlib.sha256(data[spoken['value_offset']:
                spoken['value_offset']+spoken['size']]).hexdigest(),
            native_rate=rate,native_frames=pcm//2,native_duration=(pcm//2)/rate,
            native_properties_status='READ_ONLY_UNCHANGED'))
    return rows


def verify_wave(path, expected_sha=None):
    actual = sha(path)
    if expected_sha is not None and actual!=expected_sha.lower():
        raise ValueError('reused wave hash mismatch')
    with wave.open(str(path)) as wav:
        if (wav.getnchannels(),wav.getsampwidth(),wav.getframerate())!=(1,2,24000):
            raise ValueError('voice format mismatch')
        duration = wav.getnframes()/wav.getframerate()
        samples = array.array('h',wav.readframes(wav.getnframes()))
        if sys.byteorder!='little':
            samples.byteswap()
        if (not samples or max(abs(value) for value in samples)==0 or
                sum(abs(value)>=32760 for value in samples)/len(samples)>.001):
            raise ValueError('empty/silent/clipped candidate')
    return actual,duration


def main():
    ap = argparse.ArgumentParser()
    for arg in ('logical-directory','source-directory','pack','decisions','out'):
        ap.add_argument('--'+arg,type=Path,required=True)
    args = ap.parse_args()
    if args.out.exists():
        raise ValueError('refuse existing output')
    decisions = json.loads(args.decisions.read_text(encoding='utf-8'))
    old = {r['line_id']:r for r in (json.loads(raw) for raw in
        (args.pack/'manifest.jsonl').read_text(encoding='utf-8').splitlines() if raw.strip())}
    old_qa = {r['line_id']:r for r in json.loads((args.pack/'qa.json').read_text(encoding='utf-8'))}
    rows = []
    for package,pin in decisions['banks'].items():
        rows.extend(inventory(args.logical_directory/(package+'.u'),
                              args.source_directory/(package+'.xxx'),pin))
    if {r['asset'] for r in rows}!=set(decisions['lines']):
        raise ValueError('semantic decisions do not cover native inventory exactly')
    generated, reused, audit = {},{},[]
    for native in rows:
        choice = decisions['lines'][native['asset']]
        if choice['action'] not in ('GENERATE_REPAIR','GENERATE_MISSING','REUSE_FAITHFUL'):
            raise ValueError('unknown decision action')
        if not choice['text_pl'].strip() or not choice['note'].strip():
            raise ValueError('translation/note required')
        previous = old.get(native['line_id'])
        row = dict(native,text_pl=choice['text_pl'],translation_note=choice['note'],
                   semantic_status='SIDE_FULL_NATIVE_TEXT_REVIEW',action=choice['action'],
                   role='narrator_user',reference=decisions['narrator_reference'],
                   auditory_status='NOT_AUDITIONED',native_encoding_status='NOT_ENCODED',
                   text_provenance='exact_native_SpokenText_property_plus_SIDE_translation')
        if previous:
            row.update(previous_polish=previous['corrected_polish'],previous_source_english=previous['source_english'],
                       previous_file=previous['file'],previous_wav_sha256=sha(Path(previous['file'])),
                       previous_asr_status=old_qa[native['line_id']]['qa_status'],
                       role=previous['profile_before'],reference=previous['reference_file'])
        row['reference_sha256'] = sha(Path(row['reference']))
        if choice['action']=='REUSE_FAITHFUL':
            source = Path(choice['file']).resolve()
            actual,duration = verify_wave(source,choice['sha256'])
            row.update(file=str(source),sha256=actual,selected_wav_sha256=actual,
                       duration=duration,qa_status=choice['qa_status'],
                       reused_from=choice['reused_from'])
            reused.setdefault(native['asset'],row)
        else:
            # Explicit sentences preserve all translated content under short-text model EOS.
            row['tts_segments'] = choice['segments']
            if native['asset'] in generated:
                existing = generated[native['asset']]
                if (existing['source_english'],existing['native_frames'],existing['native_rate'])!=(
                    row['source_english'],row['native_frames'],row['native_rate']):
                    raise ValueError('duplicate asset differs in native content/timing')
            generated.setdefault(native['asset'],row)
        audit.append(row)
    args.out.mkdir(parents=True)
    for name,output in (('inventory.json',rows),('semantic-audit.json',audit),
                        ('generation-lines.json',list(generated.values())),('reuse.json',list(reused.values()))):
        (args.out/name).write_text(json.dumps(output,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(dict(bindings=len(rows),unique_assets=len({r['asset'] for r in rows}),
                         generate=len(generated),reuse=len(reused),
                         decisions_sha256=sha(args.decisions))))


if __name__=='__main__':
    main()
