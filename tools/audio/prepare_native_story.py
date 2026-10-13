"""Fit source-reviewed revoice files into three gameplay +one intro bank, offline.

Not full-game localization. Asset/QA/source identity pinned to the verified WAV
lease; Original PCM duration/event/props remain. No clipping utterances to fit.
"""
import argparse
import hashlib
import json
import subprocess
import wave
from pathlib import Path
from native_story_package import build
from patch_opening_dialogue import bulks,require
from ue3_sound_package import inspect
from native_voice_fit import fit

SOURCES={
    'SP_VI2_kismet01_LOC_INT':'CFCFBD65F00320414B74FF8532959F253E0F47AEECA26460EE2F69997C7E05BA',
    'SP_VI2_kismet01_a_LOC_INT':'A00679CE2F9B7505BFB56D31006CA2D32D64514AC9E8F8C46A59AE4440B22F1A',
    'SP_VI2_kismet01_b_LOC_INT':'A8A570AFBCBB4B6CED486CA809F19EA7BF41B190713D41B68EC6CE67F570E348',
    'SP_VI_intro_Kismet_LOC_INT':'890C61720ABC0BB5AFAD86F16433A811A6F21BA6FC6284563BC5875E52642E85',
}
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    ap=argparse.ArgumentParser()
    for arg in ('candidate','voice-lease','logical-directory','cooked-directory','ffmpeg','decoder','out','semantic-audit','side-bindings','intro-candidate','review-bindings'):
        ap.add_argument('--'+arg,type=Path,required=True)
    ap.add_argument('--allow-pending-qa',action='store_true')
    ap.add_argument('--allow-partial',action='store_true',help='Explicit vertical slice; oversized utterances retain original audio')
    args=ap.parse_args()
    require(not args.out.exists(),'refuse existing output')
    lease=json.loads(args.voice_lease.read_text(encoding='utf-8-sig'))
    manifest=args.candidate/'manifest.jsonl';qa=args.candidate/'qa.json'
    require(sha(manifest.read_bytes())==lease['CandidateManifestSha256'].lower(),'manifest changed since WAV lease')
    require(sha(qa.read_bytes())==lease['QaSha256'].lower(),'QA changed since WAV lease')
    rows=[json.loads(x) for x in manifest.read_text(encoding='utf-8-sig').splitlines() if x]
    old_by_id={r['line_id']:r for r in rows};require(len(old_by_id)==len(rows),'duplicate old identity')
    q={r['line_id']:r for r in json.loads(qa.read_text(encoding='utf-8-sig'))}
    pinned={r['Source'].lower():r['CandidateSha256'].lower() for r in lease['Files']}
    audit=json.loads(args.semantic_audit.read_text(encoding='utf-8-sig'))
    require(sha(args.semantic_audit.read_bytes())=='34894e07c4e67ea820da4e7074e28d016bd6fe29031ada7f3df2e9de6bac2d86','semantic audit changed')
    reviewed={r['line_id']:r for r in audit if r['status']=='FAITHFUL_TEXT_REVIEW'}
    side=json.loads(args.side_bindings.read_text(encoding='utf-8-sig'))
    require(sha(args.side_bindings.read_bytes())=='86d72c33429836313bdf24faf9af6852c9dfe0ab2d9add9cea36fd3221a1b637','SIDE binding manifest changed')
    require(sha(args.review_bindings.read_bytes())=='ef68e27c35879410136f06a8c5488b1d3cc594fdbc725a23b012a030998a3596','review bindings changed')
    improved=json.loads(args.review_bindings.read_text(encoding='utf-8-sig'))
    side_by={r['line_id']:r for r in side}
    require(len(side_by)==len(side),'ambiguous SIDE identity')
    for r in improved:
        require(r['line_id'] in side_by,'review identity not in original SIDE handoff')
        side_by[r['line_id']]=r
    side=list(side_by.values())
    require(sha(args.intro_candidate.read_bytes())=='84b9a419b3843e73defd3a96e63ae7c5a0e7f73d932d69d532c5bed7bae07565','accepted helicopter candidate changed')
    # Preserve previous native helicopter voices while adding gameplay/bark voices.
    args.out.mkdir(parents=True)
    intro_logical=args.out/'accepted-intro.u'
    require(not intro_logical.exists(),'intro scratch exists')
    subprocess.run([str(args.decoder),str(args.intro_candidate),str(intro_logical)],capture_output=True,check=True)
    report=[]
    def run(argv):
        return subprocess.run([str(args.ffmpeg),'-hide_banner','-nostats',*argv],capture_output=True,text=True,check=True).stderr
    for package,source_sha in SOURCES.items():
        folder=args.out/package;folder.mkdir();original=(args.cooked_directory/(package+'.xxx')).read_bytes()
        require(sha(original)==source_sha.lower(),'native bank changed')
        logical=(args.logical_directory/(package+'.u')).read_bytes();logical_sha=sha(logical)
        exports=inspect(logical)['exports'];by={e['name']:e for e in exports}
        selected=[dict(r,provenance=reviewed[r['line_id']],source_hash=reviewed[r['line_id']]['candidate_wav_sha256'])
                  for r in rows if r['pack']=='story_nllb' and r['line_id'].startswith(package+'/SoundNodeWave/') and r['line_id'] in reviewed]
        for r in side:
            if r['package']==package:
                require(r['line_id'] not in {s['line_id'] for s in selected},'duplicate native selection')
                selected.append(dict(r,provenance=r,source_hash=r['sha256'],previous_audio=old_by_id.get(r['line_id'],{}).get('previous_audio',''),side=True))
        require(bool(selected),'empty native target group')
        replacements={};lines=[];skipped=[]
        if package=='SP_VI_intro_Kismet_LOC_INT':
            old_intro=intro_logical.read_bytes()
            for native in inspect(old_intro)['exports']:
                if native['name'].startswith('INTRO_') and native['name']!='INTRO_1015_GST':
                    replacements[native['name']]=bulks(old_intro,native)[1][1]
        for r in selected:
            asset=Path(r['line_id']).stem;source=Path(r['file']);e=by[asset]
            if asset=='SD_GST_REPORT_RELOAD1':
                skipped.append(dict(asset=asset,reason='SIDE full-word guarded review exceeds safe tempo; old take not accepted'))
                continue
            require(e['class_name']=='SoundNodeWave','wrong export class')
            expected=r['source_hash'];require(sha(source.read_bytes())==expected,'voice source changed')
            if not r.get('side'):require(str(source).lower() in pinned and pinned[str(source).lower()]==expected,'old revoice pin changed')
            pending=(r.get('qa_status','REVIEW') not in ('PASS_AUTOMATED_SCREEN','PASS')) if r.get('side') else q[r['line_id']]['qa_status']!='PASS_AUTOMATED_SCREEN'
            require(not pending or args.allow_pending_qa,'pending QA requires explicit flag')
            p={x['name']:x['value'] for x in e['properties']}
            proof=r['provenance'];spoken=next(x for x in e['properties'] if x['name']=='SpokenText')
            require(proof['source_logical_sha256']==logical_sha and proof['source_english']==p['SpokenText'],'native text/source identity mismatch')
            require(proof['source_property_offset']==spoken['value_offset'] and sha(logical[spoken['value_offset']:spoken['value_offset']+spoken['size']])==proof['source_property_sha256'],'native text property changed')
            rate=p['SampleRate'];require(p['NumChannels']==1 and rate in (44100,48000) and p['RawPCMDataSize']%2==0,'unsupported PCM metadata')
            frames=p['RawPCMDataSize']//2;budget=frames/rate
            # Verify native decoder metadata before using it as the fitting budget.
            reference=folder/(asset+'.native.ogg');reference.write_bytes(bulks(logical,e)[1][1])
            decoded=folder/(asset+'.native.wav')
            run(['-v','error','-n','-i',str(reference),'-c:a','pcm_s16le',str(decoded)])
            with wave.open(str(decoded)) as w:require(w.getnframes()==frames and w.getframerate()==rate and w.getnchannels()==1,'native PCM mismatch')
            target,fitting=fit(run,source,folder,asset,rate,frames)
            if target is None:
                require(args.allow_partial,'utterance too long: '+asset)
                skipped.append(dict(asset=asset,**fitting))
                print(package,asset,'RETAIN ENGLISH: native timing budget exceeded',flush=True)
                continue
            loud=run(['-i',str(target),'-af','loudnorm=I=-24:TP=-1:LRA=11:print_format=json','-f','null','-'])
            measurement=json.loads(loud[loud.rfind('{'):loud.rfind('}')+1]);require(float(measurement['input_tp'])<-.2,'encoded peak gate')
            replacements[asset]=target.read_bytes()
            lines.append(dict(asset=asset,legacy_wav=Path(r['previous_audio']).name,source_sha=expected,
                              english=p['SpokenText'],polish=r.get('text_pl',r.get('text','')),line_id=r['line_id'],
                              **fitting,pending_qa=pending,true_peak_dbtp=float(measurement['input_tp'])))
        require(bool(replacements),'no eligible voices; no candidate')
        # Control first: exact logical roundtrip, then candidate independently decoded.
        for label,clips in [('control',{}),('candidate',replacements)]:
            expected,physical=build(original,logical,clips,source_sha,logical_sha)
            packed=folder/(label+'.xxx');packed.write_bytes(physical)
            roundtrip=folder/(label+'.roundtrip.u')
            subprocess.run([str(args.decoder),str(packed),str(roundtrip)],capture_output=True,text=True,check=True)
            require(roundtrip.read_bytes()==expected,'independent LZO roundtrip mismatch')
        report.append(dict(package=package,source_sha=source_sha,candidate_sha=sha((folder/'candidate.xxx').read_bytes()),lines=lines,skipped=skipped,
                           status='OFFLINE_PASS_NOT_NATIVE_LOAD_OR_AUDITORY_ACCEPTANCE'))
        (args.out/'native-story.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
        print(package,'native candidate lines',len(lines),'retained originals',len(skipped),'PASS',flush=True)
    print('OFFLINE ONLY: no Steam file changed; native bank runtime/physical gates pending',flush=True)
if __name__=='__main__':main()
