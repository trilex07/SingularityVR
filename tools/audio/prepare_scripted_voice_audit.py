"""Private native speech provenance and explicit semantic decisions, read-only.

No game writes or heuristic transcript translation. User-owned text/audio output
belongs in ignored private storage, never in Git. Encoding remains MAIN-owned.
"""
import argparse
import hashlib
import json
from pathlib import Path
from ue3_sound_package import inspect
from inspect_scripted_speech import Graph
from prepare_room_voice_audit import verify_wave


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(logicals, sources, pack, decisions, out):
    if out.exists():
        raise ValueError('refusing existing output')
    old = {row['line_id']:row for row in (json.loads(raw) for raw in
        (pack/'manifest.jsonl').read_text(encoding='utf-8').splitlines() if raw.strip())}
    qa = {row['line_id']:row for row in json.loads((pack/'qa.json').read_text(encoding='utf-8'))}
    inventory, generated, reuse = [], {}, {}
    for logical in logicals:
        source = sources/(logical.stem+'.xxx')
        data = logical.read_bytes()
        graph = Graph(logical)
        for e in inspect(data)['exports']:
            if e['class_name'] != 'SoundNodeWave':
                continue
            props = {p['name']:p for p in e['properties']}
            spoken = props['SpokenText']
            rate, pcm = props['SampleRate']['value'], props['RawPCMDataSize']['value']
            if props['NumChannels']['value'] != 1 or rate not in (24000,44100,48000) or pcm<=0 or pcm%2:
                raise ValueError('unsupported native timing')
            identity = logical.stem+'/SoundNodeWave/'+e['name']+'.ogg'
            row = dict(package=logical.stem,asset=e['name'],line_id=identity,
                wave_identity=graph.ref(e['index']),source_bank_path=str(source),
                source_logical_path=str(logical.resolve()),source_bank_sha256=sha(source),
                source_logical_sha256=hashlib.sha256(data).hexdigest(),source_english=spoken['value'],
                source_property_offset=spoken['value_offset'],source_property_size=spoken['size'],
                source_property_sha256=hashlib.sha256(data[spoken['value_offset']:spoken['value_offset']+spoken['size']]).hexdigest(),
                native_rate=rate,native_frames=pcm//2,native_duration=pcm/2/rate,
                native_properties_status='READ_ONLY_UNCHANGED',auditory_status='NOT_AUDITIONED',
                native_encoding_status='NOT_ENCODED',runtime_playback_status='NOT_PROVEN')
            choice = decisions.get(e['name'])
            if choice is None:
                # Unselected is not evidence of a nonverbal sound. The caller's
                # private audit must establish why a native export was retained.
                row.update(action='RETAIN_ORIGINAL_UNSELECTED',note='No translation selected; original audio retained, classification requires explicit review.')
                inventory.append(row)
                continue
            row.update(text_pl=choice['text_pl'],translation_note=choice['note'],
                       semantic_status='SIDE_FULL_NATIVE_TEXT_REVIEW',
                       action=choice.get('action','GENERATE_REPAIR'),
                       text_provenance='exact_native_SpokenText_property_plus_SIDE_translation')
            previous = old.get(identity)
            role = 'demichev_ruslan' if e['name'].endswith('_DEMY') else 'barisov_dmitri' if e['name'].endswith('_BARY') else 'narrator_user'
            reference = pack/'references'/(role+'_reference_pcm16.wav')
            row.update(role=role,reference=str(reference.resolve()),reference_sha256=sha(reference))
            if previous:
                row.update(previous_polish=previous['corrected_polish'],previous_source_english=previous['source_english'],
                    previous_file=previous['file'],previous_wav_sha256=sha(Path(previous['file'])),
                    previous_asr_status=qa[identity]['qa_status'])
            if row['action']=='REUSE_FAITHFUL':
                reused_identity=choice['reuse_line_id']
                previous=old[reused_identity]
                file=Path(previous['file'])
                digest,duration=verify_wave(file)
                row.update(file=str(file.resolve()),sha256=digest,selected_wav_sha256=digest,
                    duration=duration,qa_status=qa[reused_identity]['qa_status'],
                    reused_from=reused_identity,reused_source_english=previous['source_english'],
                    reused_text_pl=previous['corrected_polish'])
                reuse.setdefault(e['name'],row)
            else:
                row['tts_segments']=choice.get('segments',[row['text_pl']])
                if e['name'] in generated and (generated[e['name']]['source_english'],generated[e['name']]['native_frames'])!=(row['source_english'],row['native_frames']):
                    raise ValueError('duplicate speech has different native text/budget')
                generated.setdefault(e['name'],row)
            inventory.append(row)
    seen={r['asset'] for r in inventory if not r['action'].startswith('RETAIN_ORIGINAL')}
    if seen!=set(decisions):
        raise ValueError('decisions contain unmatched assets')
    out.mkdir(parents=True)
    for name,rows in [('semantic-audit.json',inventory),('generation-lines.json',list(generated.values())),('reuse.json',list(reuse.values()))]:
        (out/name).write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
    return dict(bindings=len(inventory),generate=len(generated),reuse=len(reuse),retain=sum(r['action'].startswith('RETAIN_ORIGINAL') for r in inventory))


if __name__=='__main__':
    ap=argparse.ArgumentParser()
    ap.add_argument('--logical',type=Path,nargs='+',required=True)
    for name in ('sources','pack','decisions','out'):
        ap.add_argument('--'+name,type=Path,required=True)
    args=ap.parse_args()
    print(json.dumps(prepare(args.logical,args.sources,args.pack,json.loads(args.decisions.read_text(encoding='utf-8')),args.out)))
