"""Offline exact34 delta: retain previous32 except notes/story lookups +two room banks."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
from patch_opening_dialogue import require


def sha(data): return hashlib.sha256(data).hexdigest().upper()


def story_manifest(data, bindings):
    rows=[json.loads(l) for l in data.decode('utf-8-sig').splitlines() if l]
    by={(r['package'],r['asset']):r for r in bindings}
    require(len(by)==19 and len(bindings)==19,'exact19 bindings')
    seen=set()
    for row in rows:
        key=(row.get('package'),row.get('asset'))
        if key not in by: continue
        require(key not in seen,'ambiguous existing story identity')
        seen.add(key); b=by[key]
        row.update(text_en=b['source_english'],text_pl=b['text_pl'],tts_text=b['text_pl'],native_replaced='1',translation_status='full_native_text_review')
    for key in set(by)-seen:
        b=by[key]
        # The two short native1505 lines had no legacy WAV. Native ownership and
        # text mapping still load before the manager's empty-audio-entry gate.
        require(b['asset']=='VI1_1505_ANN','unexpected absent story entry')
        rows.append(dict(package=b['package'],asset=b['asset'],source=b['line_id'],file='',
            text_en=b['source_english'],text_pl=b['text_pl'],tts_text=b['text_pl'],native_replaced='1',translation_status='full_native_text_review'))
    return ('\n'.join(json.dumps(r,ensure_ascii=False) for r in rows)+'\n').encode('utf-8')


def main():
    ap=argparse.ArgumentParser()
    for arg in ('prior-manifest','room-report','bindings','notes','out'):ap.add_argument('--'+arg,type=Path,required=True)
    a=ap.parse_args();require(not a.out.exists(),'refuse existing output')
    prior=json.loads(a.prior_manifest.read_text(encoding='utf-8-sig'))
    require(prior['Kind']=='SingularityVR prepared asset delta v1' and len(prior['Files'])==32,'accepted32 input required')
    bindings=json.loads(a.bindings.read_text(encoding='utf-8-sig'))
    rooms=json.loads(a.room_report.read_text(encoding='utf-8-sig'))
    require({r['package'] for r in rooms}=={'SP_VI2_zone02a_LOC_INT','SP_VI2_zone02a_2010_LOC_INT'} and sum(len(r['lines']) for r in rooms)==19,'exact2/19 room report')
    planned=[]
    for f in prior['Files']:
        source=(a.prior_manifest.parent/f['Source']).resolve()
        require(source.is_relative_to(a.prior_manifest.parent.resolve()),'source escape')
        data=source.read_bytes();require(sha(data)==f['CandidateSha256'],'accepted source changed')
        if f['RelativePath']=='Binaries\\vr-assets\\localization\\pl\\polish_text.jsonl':data=a.notes.read_bytes()
        elif f['RelativePath']=='Binaries\\vr-assets\\localization\\pl\\polish_story_voice.jsonl':data=story_manifest(data,bindings)
        planned.append((copy.deepcopy(f),data))
    for room in rooms:
        require(room['status']=='OFFLINE_PASS_NOT_NATIVE_LOAD_OR_AUDITORY_ACCEPTANCE','unvalidated room bank')
        data=(a.room_report.parent/room['package']/'candidate.xxx').read_bytes()
        require(sha(data)==room['candidate_sha256'].upper(),'room package changed')
        rel='RvGame\\CookedPC\\'+room['package']+'.xxx'
        planned.append((dict(RelativePath=rel,Source='payload\\'+rel,BaselineSha256=room['source_sha256'].upper(),CandidateSha256=sha(data)),data))
    require(len(planned)==34 and len({f['RelativePath'] for f,_ in planned})==34,'duplicate target')
    a.out.mkdir(parents=True)
    files=[]
    for f,data in planned:
        dest=(a.out/f['Source']).resolve();require(dest.is_relative_to(a.out.resolve()),'output escape')
        dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
        f['CandidateSha256']=sha(data);files.append(f)
    manifest=dict(Kind='SingularityVR post-opening asset delta v2',Mode=prior['Mode'],Files=files,
        Notes='Two new native room banks, reviewed first-area notes; other prior assets byte-identical. Default remaster apply remains0.')
    (a.out/'asset-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print('PASS: exact34 delta staged privately; no Steam writes')


if __name__=='__main__': main()
