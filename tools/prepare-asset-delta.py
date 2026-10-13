"""Offline exact32-file delta stage; no install. Log-only/apply are separate leases."""
import argparse,hashlib,json,re,shutil
from pathlib import Path
def sha(b):return hashlib.sha256(b).hexdigest().upper()
def require(ok,why):
    if not ok:raise ValueError(why)
def load(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def main():
    ap=argparse.ArgumentParser()
    for name in ('native','title','materials','locale','game','out'):ap.add_argument('--'+name,type=Path,required=True)
    ap.add_argument('--mode',choices=['log-only','apply'],required=True);a=ap.parse_args()
    require(not a.out.exists(),'refuse existing artifact');a.out.mkdir(parents=True);files=[]
    def add(rel,source,expected,baseline=None):
        data=source.read_bytes();require(sha(data)==expected.upper(),'source SHA mismatch')
        relative=rel.replace('/','\\');dst=a.out/'payload'/Path(rel);dst.parent.mkdir(parents=True,exist_ok=True);dst.write_bytes(data)
        installed=a.game/Path(rel);actual=sha(installed.read_bytes()) if installed.exists() else 'ABSENT'
        if baseline is not None:require(actual==baseline.upper(),'original delta baseline changed')
        files.append(dict(RelativePath=relative,Source='payload\\'+relative,BaselineSha256=actual,CandidateSha256=sha(data)))
    for bank in load(a.native/'native-story.json'):
        if bank['package']=='SP_VI_intro_Kismet_LOC_INT':continue # belongs ONLY to four-file lease
        add('RvGame/CookedPC/'+bank['package']+'.xxx',a.native/bank['package']/'candidate.xxx',bank['candidate_sha'],bank['source_sha'])
    title=load(a.title/'title.json')
    add('RvGame/CookedPC/UI_SinglePlayer_SF.xxx',a.title/'candidate.xxx',title['candidate_sha'],title['source_sha'])
    for f in load(a.materials/'material-files.json'):
        add('Binaries/texture-override-remaster/'+f['filename'],a.materials/f['filename'],f['sha256'],f['baseline_sha256'] or 'ABSENT')
    for f in load(a.locale/'lookup-files.json'):
        add('Binaries/vr-assets/localization/pl/'+f['filename'],a.locale/f['filename'],f['candidate_sha'],f['baseline_sha'])
    original=a.game/'Binaries/texture_override.ini';data=original.read_bytes()
    require(sha(data)=='146140CD8DAB8867CBAA15BD13BC983EE90E30EE73E80A6325E83FE2962E5093','accepted texture config changed')
    text=data.decode('latin1');require('bridge_owned' not in text and 'remaster_apply' not in text,'existing bridge/remaster key')
    require(len(re.findall(r'^mode_apply=1\r?$',text,re.M))==1 and text.count('[general]')==1,'config layout')
    edited=text.replace('[general]','[general]\r\nbridge_owned=1\r\nremaster_apply='+('0' if a.mode=='log-only' else '1')+'\r\nremasterdir=texture-override-remaster',1)
    # mode_apply1, accepted base folder/groups/tier remain unchanged.
    config=a.out/'texture_override.ini';config.write_bytes(edited.encode('latin1'))
    add('Binaries/texture_override.ini',config,sha(config.read_bytes()),sha(data))
    require(len(files)==32 and len({f['RelativePath'] for f in files})==32,'exact32 unique targets')
    manifest=dict(Kind='SingularityVR prepared asset delta v1',Mode=a.mode,Files=files,
        StartRequired=True,SteamWritePerformed=False,PhysicalAcceptance='PENDING',SourceSourceOnly=True)
    (a.out/'asset-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print('OFFLINE32-file delta READY',a.mode,'No Steam file changed')
if __name__=='__main__':main()
