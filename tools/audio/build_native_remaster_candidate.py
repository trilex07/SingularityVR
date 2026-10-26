"""Build private native sound remaster banks. Never deploys or launches a game.

Only eight pinned SoundNodeWave PC payloads are eligible. Fixed-size valid Ogg
comments retain EVERY package offset and all non-target logical bytes. No cue,
attenuation, pitch, native event, timing property or spatial metadata is edited.
"""
import argparse, hashlib, json, struct, subprocess, wave
from array import array
from pathlib import Path
from native_story_package import pack
from patch_opening_dialogue import bulks, literals, TAG, require
from ue3_sound_package import inspect
from vorbis_fixed_allocation import pad_to_allocation

ROOT=Path(__file__).resolve().parents[2]
SUMMARY=ROOT/'docs/handoffs/final-audio-inventory-20261026.json'

def sha(b):return hashlib.sha256(b).hexdigest()
def offline_path(path):
    path=Path(path).resolve()
    require(not any(p.lower() in ('steam','steamapps') for p in path.parts),'Steam access forbidden')
    return path

def wav_info(path):
    with wave.open(str(path),'rb') as w:
        return dict(frames=w.getnframes(),channels=w.getnchannels(),sample_width=w.getsampwidth(),sample_rate=w.getframerate()),w.readframes(w.getnframes())

def whole_pack(logical):
    raw=[logical[p:p+0x20000] for p in range(0,len(logical),0x20000)]
    parts=[literals(p) if len(p)>=19 else p for p in raw]
    return struct.pack('<4I',TAG,0x20000,sum(map(len,parts)),len(logical))+b''.join(struct.pack('<2I',len(c),len(r)) for c,r in zip(parts,raw))+b''.join(parts)

def replace_payload(logical,asset,replacement):
    found=[e for e in inspect(logical)['exports'] if e['name']==asset and e['class_name']=='SoundNodeWave']
    require(len(found)==1,'ambiguous native wave')
    e=found[0];head,old=bulks(logical,e)[1];require(len(old)==len(replacement),'allocation changed')
    props={p['name']:p['value'] for p in e['properties']}
    at=head+16;out=logical[:at]+replacement+logical[at+len(old):]
    require(out[:at]==logical[:at] and out[at+len(old):]==logical[at+len(old):],'non-target bytes changed')
    require(bulks(out,e)[1][1]==replacement,'native payload roundtrip')
    return out,old,props,at

def run(command):
    p=subprocess.run([str(x) for x in command],capture_output=True,text=True)
    require(p.returncode==0,'command failed: '+str(command[0])+' '+p.stderr[-1500:]+p.stdout[-500:])
    return p.stdout

def decode(decoder,source,destination):
    raw=source.read_bytes();whole=struct.unpack_from('<I',raw,4)[0]==0x20000
    exe=decoder/('uedecompress.exe' if whole else 'unpack_chunks.exe')
    run([exe,source,destination]);return destination.read_bytes(),whole

def storm_reference(logical):
    # Typed SoundCue.FirstNode -> SoundNodeAmbient.Wave import, not name similarity.
    import sys
    sys.path.insert(0,str(ROOT/'tools/title'))
    from ue3_layout import inspect as layout
    x=layout('storm',data=logical,include_all=True);exports={e['index']:e for e in x['exports']};names=x['names']
    hits=[]
    for node in exports.values():
        if node['class_name']!='SoundNodeAmbient' or 'parse_error' in node:continue
        for p in node['properties']:
            if p['name']!='Wave' or p['type']!='ObjectProperty':continue
            i=struct.unpack_from('<i',logical,p['tag_offset']+24)[0]
            if i>=0:continue
            imp=x['imports'][-i-1]
            if names[imp[2]]!='SoundNodeWave' or names[imp[5]]!='windStormHigh01_R':continue
            for cue in exports.values():
                if cue['class_name']!='SoundCue' or 'parse_error' in cue:continue
                for cp in cue['properties']:
                    if cp['name']=='FirstNode' and cp['type']=='ObjectProperty' and struct.unpack_from('<i',logical,cp['tag_offset']+24)[0]==node['index']:
                        hits.append(dict(cue_export=cue['index'],ambient_node_export=node['index'],wave_import=i,wave='windStormHigh01_R',scope='Freighter high-wind layer only; not a global weather replacement'))
    require(len(hits)==1,'storm producer is not uniquely proven')
    return hits[0]

def music_reference(logical,asset):
    import sys
    sys.path.insert(0,str(ROOT/'tools/title'))
    from ue3_layout import inspect as layout
    x=layout('music',data=logical,include_all=True);names=x['names'];hits=[]
    for e in x['exports']:
        if e['class_name'] not in ('SoundCue','DynamicMusic') or 'parse_error' in e:continue
        for p in e['properties']:
            refs=[]
            if p['name']=='FirstNode' and p['type']=='ObjectProperty':refs=[struct.unpack_from('<i',logical,p['tag_offset']+24)[0]]
            if p['name']=='MusicTracks' and p['type']=='ArrayProperty':
                at=p['tag_offset']+24;count=struct.unpack_from('<i',logical,at)[0]
                require(0<=count<=64 and p['value']['bytes']==4+4*count,'invalid MusicTracks')
                refs=list(struct.unpack_from('<'+'i'*count,logical,at+4))
            for ref in refs:
                if ref>=0:continue
                imp=x['imports'][-ref-1]
                if names[imp[2]]=='SoundNodeWave' and names[imp[5]]==asset:
                    hits.append(dict(consumer_export=e['index'],consumer_class=e['class_name'],property=p['name'],wave_import=ref,wave=asset))
    require(len(hits)==1,'music native producer not uniquely proven')
    return hits[0]

def build(args):
    sources=offline_path(args.sources);audition=offline_path(args.audition);decoder=offline_path(args.decoder);ff=offline_path(args.ffmpeg);out=offline_path(args.output)
    require(out.is_relative_to(ROOT/'build'),'output must be in private build directory')
    out.mkdir(parents=True,exist_ok=True)
    manifest=out/'candidate-manifest.json'
    # A failed rebuild must never leave a stale success receipt beside new banks.
    if manifest.exists():manifest.replace(out/'previous-manifest.json')
    incomplete=out/'INCOMPLETE';incomplete.write_text('Do not deploy: offline build has not completed.\n',encoding='utf-8')
    summary=json.loads(SUMMARY.read_text(encoding='utf-8-sig'));rows=[]
    storm_data,_=decode(decoder,sources/'SP_Freight_Audio_2010.xxx',out/'storm-consumer.u')
    proof=storm_reference(storm_data)
    report=dict(schema='singularityvr-native-remaster-candidate-v1',selection_authority='User explicitly delegated variant selection to MAIN/SIDE, 2026-10-26; no claim of human listening',selection='restored_conservative.wav for all eight sets: existing native content, minimal processing, exact duration/channel parity, no invented assets. Highest enumerated Vorbis quality fitting native allocation; up to -3 dB codec headroom only if decoded saturation requires it',listening_performed=False,physical_acceptance='PENDING',source_provenance='Private user game-source lab; no third-party generated audio; proprietary content remains local, not distributable in Git',ffmpeg_sha256=sha(ff.read_bytes()),decoder_hashes={n:sha((decoder/n).read_bytes()) for n in ['unpack_chunks.exe','uedecompress.exe']},storm_reference=dict(**proof,consumer_source_sha256=sha((sources/'SP_Freight_Audio_2010.xxx').read_bytes()),consumer_logical_sha256=sha(storm_data)),banks=rows)
    for row in summary['remaster']:
        print('Building '+row['id'],flush=True)
        folder=audition/row['id'];selected=folder/'restored_conservative.wav';original_wav=folder/'original.wav';meta=folder/'metadata.json'
        require(sha(meta.read_bytes())==row['metadata_sha256'],'metadata drift')
        selected_record=next(v for v in row['variants'] if v['file']==selected.name)
        original_record=next(v for v in row['variants'] if v['file']==original_wav.name)
        require(sha(selected.read_bytes())==selected_record['sha256'] and sha(original_wav.read_bytes())==original_record['sha256'],'WAV drift')
        expected={k:selected_record[k] for k in ['frames','channels','sample_width','sample_rate']}
        info,_=wav_info(selected);require(info==expected and wav_info(original_wav)[0]==expected,'PCM parity')
        source=sources/row['package'];src=source.read_bytes();logical,whole=decode(decoder,source,out/(row['id']+'-source.u'))
        if row['id'].startswith(('06_','07_')):
            companion=row['package'].replace('_XSA','');companion_data,_=decode(decoder,sources/companion,out/(row['id']+'-consumer.u'))
            reference=music_reference(companion_data,row['asset']);reference.update(consumer_package=companion,consumer_package_sha256=sha((sources/companion).read_bytes()),consumer_logical_sha256=sha(companion_data))
        elif row['id'].startswith('08_'):
            reference=report['storm_reference']
        else:
            from extract_audio_inventory import parse_package_full,analyze_sound_cues
            cue_map,confidence,paths=analyze_sound_cues(logical,parse_package_full(logical))
            require(confidence.get(row['asset'])=='VERIFIED_REFERENCE' and row['cue'] in cue_map.get(row['asset'],[]),'native SoundCue producer not proven')
            reference=dict(cues=cue_map[row['asset']],paths=paths[row['asset']],confidence='VERIFIED_REFERENCE')
        found=[e for e in inspect(logical)['exports'] if e['name']==row['asset'] and e['class_name']=='SoundNodeWave'];require(len(found)==1,'missing wave')
        native=bulks(logical,found[0])[1][1];(out/'native-source.ogg').write_bytes(native)
        run([ff,'-v','error','-y','-i',out/'native-source.ogg','-c:a','pcm_s16le',out/'native-source.wav'])
        require(wav_info(out/'native-source.wav')==wav_info(original_wav),'audition original is not the decoded native source')
        attempts=[];padded=None
        # Pick highest available quality fitting the unchanged native allocation.
        for q in [6,5,4,3,2.5,2,1.5,1,0.5,0,-0.5,-1]:
            for codec_gain_db in [0,-1,-2,-3]:
                command=[ff,'-v','error','-y','-i',selected,'-map_metadata','-1']
                if codec_gain_db:command+=['-af','volume='+str(codec_gain_db)+'dB']
                run(command+['-c:a','libvorbis','-q:a',q,out/'encoded.ogg'])
                encoded=(out/'encoded.ogg').read_bytes()
                try:padded=pad_to_allocation(encoded,len(native))
                except ValueError as e:
                    attempts.append(dict(q=q,gain_db=codec_gain_db,bytes=len(encoded),refusal=str(e)));padded=None;break
                (out/'codec-check.ogg').write_bytes(padded)
                run([ff,'-v','error','-y','-i',out/'codec-check.ogg','-c:a','pcm_s16le',out/'codec-check.wav'])
                codec_info,codec_pcm=wav_info(out/'codec-check.wav')
                samples=array('h');samples.frombytes(codec_pcm)
                if codec_info!=expected or samples.count(-32768)+samples.count(32767):
                    attempts.append(dict(q=q,gain_db=codec_gain_db,bytes=len(encoded),refusal='codec timing mismatch or saturation'));padded=None;continue
                break
            if padded is not None:break
        require(padded is not None,'no acceptable encode fits native allocation')
        ogg=out/(row['id']+'.ogg');ogg.write_bytes(padded)
        run([ff,'-v','error','-y','-i',ogg,'-c:a','pcm_s16le',out/'decoded.wav'])
        decoded_info,decoded_pcm=wav_info(out/'decoded.wav')
        require(decoded_info==expected,'encoded duration/channels/sample rate changed')
        samples=array('h');samples.frombytes(decoded_pcm)
        saturated=samples.count(-32768)+samples.count(32767)
        require(saturated==0,'lossy encode introduced saturated PCM16 samples')
        candidate,old,props,offset=replace_payload(logical,row['asset'],padded)
        require(props['NumChannels']==expected['channels'] and props['SampleRate']==expected['sample_rate'] and props['RawPCMDataSize']==expected['frames']*expected['channels']*2,'native metadata mismatch')
        bank=out/row['package'];bank.write_bytes(whole_pack(candidate) if whole else pack(candidate,src))
        unpacked,_=decode(decoder,bank,out/(row['id']+'-roundtrip.u'));require(unpacked==candidate,'independent UE3 decoder roundtrip failed')
        rows.append(dict(id=row['id'],package=row['package'],asset=row['asset'],cue=row['cue'] if row['id']!=proof.get('id','08_ambience_ocean_storm') else 'native SoundCue export '+str(proof['cue_export']),selected_wav_sha256=selected_record['sha256'],native_consumer=reference,decoded_saturated_samples=saturated,source_package_sha256=sha(src),source_logical_sha256=sha(logical),source_ogg_sha256=sha(native),candidate_sha256=sha(bank.read_bytes()),candidate_logical_sha256=sha(candidate),candidate_ogg_sha256=sha(padded),bytes=bank.stat().st_size,source_bytes=len(src),payload_offset=offset,payload_bytes=len(native),vorbis_quality=q,codec_headroom_gain_db=codec_gain_db,pcm=expected,failed_higher_quality_attempts=attempts,native_properties_unchanged=True,non_target_logical_bytes_unchanged=True,roundtrip_decoder='PASS',decoded_timing_channels='PASS',relative_candidate_path=bank.relative_to(ROOT).as_posix()))
    report['builder_file_hashes']={n:sha((ROOT/'tools/audio'/n).read_bytes()) for n in ['build_native_remaster_candidate.py','vorbis_fixed_allocation.py','native_story_package.py','patch_opening_dialogue.py']}
    report['status']='COMPLETE_OFFLINE_ONLY'
    temporary=out/'candidate-manifest.partial.json'
    temporary.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');temporary.replace(manifest);incomplete.unlink()
    print('PASS: '+str(len(rows))+' native candidate banks; '+str(out/'candidate-manifest.json'))
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['sources','audition','decoder','ffmpeg','output']:p.add_argument('--'+name,required=True,type=Path)
    build(p.parse_args())
