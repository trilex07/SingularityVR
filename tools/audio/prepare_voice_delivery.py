"""OFFLINE lookup/voice handoff. Native ownership only for fitted exact IDs.

New WAVs do not alone replace UE3. Preserve legacy lookup filenames, text tokens,
and all unrelated manifest rows. No game writes or generation here.
"""
import argparse,hashlib,json,re,shutil
from pathlib import Path
from patch_opening_dialogue import require

def sha(b):return hashlib.sha256(b).hexdigest()
def load(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def jsonl(path):return [json.loads(x) for x in path.read_text(encoding='utf-8-sig').splitlines() if x]
def tokens(s):return re.findall(r'`~|%[0-9$.+-]*[a-zA-Z]|<[^>]*>',s)
def main():
    ap=argparse.ArgumentParser()
    for name in ('candidate','side-bindings','review-bindings','native-report','game','out'):ap.add_argument('--'+name,type=Path,required=True)
    a=ap.parse_args();require(not a.out.exists(),'refuse existing handoff')
    rows=jsonl(a.candidate/'manifest.jsonl');qa=load(a.candidate/'qa.json');require(len(rows)==len(qa)==1886,'full pack identity')
    q={r['line_id']:r for r in qa};require(len(q)==1886,'ambiguous QA')
    side=load(a.side_bindings);review=load(a.review_bindings)
    require(sha(a.side_bindings.read_bytes())=='86d72c33429836313bdf24faf9af6852c9dfe0ab2d9add9cea36fd3221a1b637','old SIDE pin')
    require(sha(a.review_bindings.read_bytes())=='ef68e27c35879410136f06a8c5488b1d3cc594fdbc725a23b012a030998a3596','review SIDE pin')
    corrections={r['line_id']:r for r in side};corrections.update({r['line_id']:r for r in review})
    native={r['line_id']:r for bank in load(a.native_report) for r in bank['lines']}
    revised=[];metadata=[]
    for row in rows:
        row=dict(row);fix=corrections.get(row['line_id'])
        if fix:
            require(sha(Path(fix['file']).read_bytes())==fix['sha256'],'SIDE WAV changed')
            row.update(file=fix['file'],source_english=fix['source_english'],corrected_polish=fix['text_pl'],tts_text=fix['text_pl'],spoken_tts_text=fix['text_pl'])
            q[row['line_id']]=dict(q[row['line_id']],file=fix['file'],source_english=fix['source_english'],corrected_polish=fix['text_pl'],qa_status=fix.get('qa_status','REVIEW_OR_REPAIR'))
        revised.append(row)
    by={r['line_id']:r for r in revised};require(len(by)==1886,'ambiguous revoice ID')
    folder=a.game/'Binaries/vr-assets/localization/pl'
    a.out.mkdir(parents=True);pack=a.out/'revoice';pack.mkdir()
    # Existing guarded installer requires all WAV sources to live in this pack.
    for row in revised:
        source=Path(row['file']);dest=pack/source.name
        if dest.exists():require(sha(dest.read_bytes())==sha(source.read_bytes()),'conflicting WAV basename')
        else:shutil.copyfile(source,dest)
        require(sha(dest.read_bytes())==sha(source.read_bytes()),'staged WAV hash mismatch')
        row['file']=str(dest.resolve());q[row['line_id']]['file']=row['file']
    (pack/'manifest.jsonl').write_text(''.join(json.dumps(r,ensure_ascii=False)+'\n' for r in revised),encoding='utf-8')
    (pack/'qa.json').write_text(json.dumps(list(q.values()),ensure_ascii=False,indent=2),encoding='utf-8')
    notes=[]
    for filename in ('polish_text.jsonl','polish_voice.jsonl','polish_story_voice.jsonl'):
        path=folder/filename;data=path.read_bytes();original=jsonl(path);patched=[];changed=0
        for entry in original:
            entry=dict(entry)
            ident=entry.get('source') if filename=='polish_story_voice.jsonl' else entry.get('id')
            row=by.get(ident)
            if row:
                require(Path(row['previous_audio']).name==entry.get('file',Path(row['previous_audio']).name),'legacy lookup identity')
                pl=row['corrected_polish'];en=row['source_english']
                if filename=='polish_text.jsonl':
                    if tokens(en)==tokens(pl):entry.update(pl=pl,status='translated',tts_text=row['tts_text']);changed+=1
                    else:notes.append(dict(id=ident,reason='visual text tokens/markup not identical; retained original visual entry'))
                elif filename=='polish_voice.jsonl':entry.update(pl=pl,tts_text=row['tts_text']);changed+=1
                else:
                    entry.update(text_pl=pl,text_en=en,tts_text=row['tts_text']);changed+=1
                    if ident in native:
                        # Native identity is package/asset, not SIDE's reused _a WAV filename.
                        # Existing lookup name was independently checked against previous_audio above.
                        entry['native_replaced']='1'
            patched.append(entry)
        require(len(patched)==len(original),'lookup entries changed count')
        dest=a.out/filename;dest.write_text(''.join(json.dumps(r,ensure_ascii=False)+'\n' for r in patched),encoding='utf-8')
        metadata.append(dict(filename=filename,baseline_sha=sha(data),candidate_sha=sha(dest.read_bytes()),rows=len(patched),changed=changed))
    (a.out/'lookup-files.json').write_text(json.dumps(metadata,indent=2),encoding='utf-8')
    (a.out/'retained-text.json').write_text(json.dumps(notes,ensure_ascii=False,indent=2),encoding='utf-8')
    print('OFFLINE1886-WAV handoff; corrected lookup files; native fitted coverage',len(native),'visual token conflicts retained',len(notes))
if __name__=='__main__':main()
