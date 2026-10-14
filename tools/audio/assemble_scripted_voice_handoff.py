"""Source-bound offline scripted voice handoff; no native encoding or deploy."""
import argparse
import hashlib
import json
import shutil
from collections import Counter
from pathlib import Path
from assemble_room_voice_handoff import bind_voice
from prepare_room_voice_audit import verify_wave
from inspect_scripted_speech import Graph


def assemble(audit, voices, graphs, out):
    if out.exists():
        raise ValueError('refusing existing output')
    by_asset={r['asset']:r for r in voices}
    if len(by_asset)!=len(voices):
        raise ValueError('duplicate selected asset')
    speech=[r for r in audit if not r['action'].startswith('RETAIN_ORIGINAL')]
    retained=[r for r in audit if r['action'].startswith('RETAIN_ORIGINAL')]
    if set(by_asset)!={r['asset'] for r in speech}:
        raise ValueError('speech coverage mismatch')
    routes={}
    producers={}
    graph_evidence=[]
    for path in graphs:
        graph=Graph(path)
        report=graph.report()
        graph_evidence.append(report)
        for entry in report['producers']:
            for sound in entry['sounds']:
                for leaf in sound['waves']:
                    if leaf['class_name']=='SoundNodeWave':
                        producers.setdefault(leaf['path'],[]).append(dict(
                            logical_source_sha256=report['logical_sha256'],producer=entry['producer'],
                            producer_property=entry['property'],cue=sound['Sound'],wave=leaf))
        for export in graph.exports.values():
            if export['class_name']!='SoundCue':
                continue
            for leaf in graph.waves(export['index']):
                if leaf['class_name']=='SoundNodeWave':
                    routes.setdefault(leaf['path'],[]).append(dict(logical_source_sha256=report['logical_sha256'],
                        cue=graph.ref(export['index']),wave=leaf))
    # Validate and pin all selected audio before writing delivery files.
    selected=[]
    for row in voices:
        if row['timing_status']!='PASS_TIMING_ONLY_NOT_AUDITIONED':
            raise ValueError('unsafe timing')
        digest,duration=verify_wave(Path(row['file']),row['sha256'])
        if duration>row['native_frames']/row['native_rate'] or row['required_tempo']>1.85:
            raise ValueError('native timing violation')
        selected.append(dict(row,sha256=digest,selected_wav_sha256=digest,
            selected_wav_duration=duration,auditory_status='NOT_AUDITIONED',
            native_encoding_status='NOT_ENCODED_OR_DECODE_COUNT_VERIFIED'))
    by_asset={r['asset']:r for r in selected}
    bindings=[]
    for native in speech:
        voice=by_asset[native['asset']]
        binding=bind_voice(native,voice)
        # Chosen take may use an independently reviewed faithful wording variant.
        binding.update(text_pl=voice['text_pl'],tts_segments=voice.get('tts_segments'),
                       prior_audit_text_pl=native['text_pl'])
        wave=native['wave_identity']['path']
        binding.update(cue_routes=routes.get(wave,[]),scripted_producers=producers.get(wave,[]),
                       producer_evidence='OFFLINE_TYPED_GRAPH' if producers.get(wave) else 'CUE_ONLY_OR_UNRESOLVED_PRODUCER')
        bindings.append(binding)
    out.mkdir(parents=True)
    for voice in selected:
        target=out/(voice['asset']+'.wav')
        shutil.copyfile(voice['file'],target)
        if hashlib.sha256(target.read_bytes()).hexdigest()!=voice['sha256']:
            raise ValueError('delivery copy hash mismatch')
        voice['file']=str(target.resolve())
    for binding in bindings:
        binding['file']=by_asset[binding['asset']]['file']
    banks={r['package']:dict(source_bank_sha256=r['source_bank_sha256'],
        source_logical_sha256=r['source_logical_sha256'],source_bank_path=r['source_bank_path'],
        source_logical_path=r['source_logical_path']) for r in audit}
    gates=[dict(line_id=r['line_id'],selected_wav_sha256=r['selected_wav_sha256'],
        required_native_decoded_frames=r['native_frames'],required_native_rate=r['native_rate'],
        required_native_channels=1,status='NOT_ENCODED_OR_DECODE_COUNT_VERIFIED') for r in bindings]
    for filename,data in [('selected.json',selected),('native-bindings.json',bindings),
        ('retained-original.json',retained),('source-banks.json',banks),
        ('native-encoding-gates.json',gates),('scripted-graph.json',graph_evidence)]:
        (out/filename).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
    return dict(selected=len(selected),bindings=len(bindings),retained=len(retained),
                qa=dict(Counter(r['qa_status'] for r in selected)),source_banks=len(banks),
                offline_only=True)


if __name__=='__main__':
    ap=argparse.ArgumentParser()
    for key in ('audit','voices','out'):
        ap.add_argument('--'+key,type=Path,required=True)
    ap.add_argument('--graph',type=Path,nargs='+',required=True)
    args=ap.parse_args()
    print(json.dumps(assemble(json.loads(args.audit.read_text(encoding='utf-8')),
        json.loads(args.voices.read_text(encoding='utf-8')),args.graph,args.out)))
