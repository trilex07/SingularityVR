"""Offline exact37/38 delta: retain tested34 + UI/NPC + optional SIDE font.

No TTS, installed-file writes, launch or automatic acceptance.
"""
import argparse
import copy
import json
from pathlib import Path
from prepare_post_opening_assets import sha,story_manifest
from patch_opening_dialogue import require
from prepare_room_banks import NPC_BANKS


def main():
    ap=argparse.ArgumentParser()
    for name in ('prior-manifest','npc-report','bindings','title-report','out'):
        ap.add_argument('--'+name,type=Path,required=True)
    ap.add_argument('--bindings-sha256',required=True)
    ap.add_argument('--font-report',type=Path,help='reviewed SIDE original-font package patch; exact additional file')
    a=ap.parse_args();require(not a.out.exists(),'refuse existing output')
    prior=json.loads(a.prior_manifest.read_text(encoding='utf-8-sig'))
    require(prior['Kind']=='SingularityVR post-opening asset delta v2' and len(prior['Files'])==34,'exact34 input')
    binding_bytes=a.bindings.read_bytes()
    require(sha(binding_bytes)==a.bindings_sha256.upper(),'SIDE binding changed')
    bindings=json.loads(binding_bytes)
    npc=json.loads(a.npc_report.read_text(encoding='utf-8-sig'))
    title=json.loads(a.title_report.read_text(encoding='utf-8-sig'))
    require({r['package'] for r in npc}==set(NPC_BANKS) and sum(len(r['lines']) for r in npc)==23,'exact3/23 report')
    require(title['status']=='OFFLINE_PASS_PHYSICAL_PENDING' and title['movie']=='iis+SPMenus','scoped UI report')
    planned=[]
    for entry in prior['Files']:
        source=(a.prior_manifest.parent/entry['Source']).resolve()
        require(source.is_relative_to(a.prior_manifest.parent.resolve()),'source escape')
        data=source.read_bytes();require(sha(data)==entry['CandidateSha256'],'prior candidate changed')
        if entry['RelativePath']=='RvGame\\CookedPC\\UI_SinglePlayer_SF.xxx':
            require(entry['BaselineSha256']==title['source_sha'].upper(),'UI baseline mismatch')
            data=(a.title_report.parent/'candidate.xxx').read_bytes()
            require(sha(data)==title['candidate_sha'].upper(),'UI candidate changed')
        elif entry['RelativePath']=='Binaries\\vr-assets\\localization\\pl\\polish_story_voice.jsonl':
            data=story_manifest(data,bindings,23)
        planned.append((copy.deepcopy(entry),data))
    for bank in npc:
        require(bank['status']=='OFFLINE_PASS_NOT_NATIVE_LOAD_OR_AUDITORY_ACCEPTANCE','unvalidated bank')
        require(bank['source_sha256']==NPC_BANKS[bank['package']][0],'unexpected original bank')
        data=(a.npc_report.parent/bank['package']/'candidate.xxx').read_bytes()
        require(sha(data)==bank['candidate_sha256'].upper(),'NPC candidate changed')
        rel='RvGame\\CookedPC\\'+bank['package']+'.xxx'
        planned.append((dict(RelativePath=rel,Source='payload\\'+rel,
            BaselineSha256=bank['source_sha256'].upper()),data))
    if a.font_report:
        font=json.loads(a.font_report.read_text(encoding='utf-8-sig'))
        require(font['status']=='OFFLINE_PASS_PHYSICAL_PENDING' and
                font['input_sha256']=='3f7bf0bef08d73aed3b1ba249730b4bbb16ec427dd49992a6f4c4a7ce6c8b51b' and
                font['output_sha256']=='60e7287185300caa2a8e64fbe14b741475d4d959944b6b0e51f84547841e8db8',
                'unreviewed font identity')
        data=(a.font_report.parent/'ui_GFx_fonts.xxx').read_bytes()
        require(sha(data)==font['output_sha256'].upper(),'font candidate changed')
        rel='RvGame\\CookedPC\\ui_GFx_fonts.xxx'
        planned.append((dict(RelativePath=rel,Source='payload\\'+rel,
                            BaselineSha256=font['input_sha256'].upper()),data))
    total=38 if a.font_report else 37
    require(len(planned)==total and len({f['RelativePath'] for f,_ in planned})==total,'exact unique target count')
    a.out.mkdir(parents=True)
    files=[]
    for entry,data in planned:
        dest=(a.out/entry['Source']).resolve();require(dest.is_relative_to(a.out.resolve()),'output escape')
        dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
        entry['CandidateSha256']=sha(data);files.append(entry)
    manifest=dict(Kind='SingularityVR first-area font polish delta v4' if a.font_report else
                       'SingularityVR first-area polish delta v3',Mode=prior['Mode'],Files=files,
        StartRequired=True,SteamWritePerformed=False,PhysicalAcceptance='PENDING',
        FontEnabled=bool(a.font_report),
        Notes='Native cursor geometry and scoped runtime title;23 NPC utterances,9 nonverbal exports retained; separate font package when declared. Not full-game dubbing or physical acceptance.')
    (a.out/'asset-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print(f'OFFLINE exact{total} ready; UI/NPC/fonts unverified physically; no Steam writes')


if __name__=='__main__':main()
