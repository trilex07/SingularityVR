"""Stage existing SIDE materials byte-exact; never regenerates/deploys textures."""
import argparse,hashlib,json,re,struct,subprocess
from pathlib import Path

REFS={'prototype':'0dcdcd5c6dc677573df24ea5616b69ef71ebb8ff','batch':'188c10a67b1cf2b0f784818b5bc3bc2daed6835f'}
def sha(b):return hashlib.sha256(b).hexdigest()
def require(ok,why):
    if not ok:raise ValueError(why)
def dds(b):
    require(len(b)>=128 and b[:4]==b'DDS ' and struct.unpack_from('<I',b,4)[0]==124,'DDS header')
    h,w=struct.unpack_from('<2I',b,12);m=struct.unpack_from('<I',b,28)[0];fc=b[84:88].decode('ascii')
    require(0<w<=8192 and 0<h<=8192 and fc in ('DXT1','DXT5'),'DDS format/dimensions')
    require(m==max(w,h).bit_length(),'complete mip chain required')
    block=8 if fc=='DXT1' else 16
    expected=128+sum(max(1,((w>>i)+3)//4)*max(1,((h>>i)+3)//4)*block for i in range(m))
    require(len(b)==expected,'DDS mip byte budget');return dict(w=w,h=h,mips=m,fourcc=fc,bytes=len(b))
def main():
    ap=argparse.ArgumentParser()
    for name in ('assets-local','game','out'):ap.add_argument('--'+name,type=Path,required=True)
    a=ap.parse_args();require(not a.out.exists(),'refuse existing output')
    repo=Path(__file__).resolve().parents[1]
    def git(ref,path):return subprocess.run(['git','show',ref+':'+path],cwd=repo,capture_output=True,check=True).stdout
    table=(repo/'spikes/texture_cache/targets_generated.h').read_text(encoding='utf-8')
    targets={int(x[4]):dict(name=x[0],w=int(x[1]),h=int(x[2]),fourcc=x[3],file=x[5])
        for x in re.findall(r'\{ "([^"]+)", (\d+)u, (\d+)u, D3DFMT_(DXT[15]), (\d+)ULL, "([^"]+)",',table)}
    manifests=[]
    for ref,root in [(REFS['prototype'],'material-remaster'),(REFS['batch'],'material-remaster-batch2'),(REFS['batch'],'material-remaster-batch3')]:
        paths=subprocess.run(['git','ls-tree','-r','--name-only',ref],cwd=repo,capture_output=True,text=True,check=True).stdout.splitlines()
        for path in paths:
            if path.startswith(root+'/') and path.endswith('.json') and 'summary' not in path:
                doc=json.loads(git(ref,path));
                if 'maps' in doc:manifests.append((ref,root,path,doc))
    a.out.mkdir(parents=True);files=[];excluded=[]
    for ref,root,path,doc in manifests:
        for kind,m in doc['maps'].items():
            if not m.get('substituted'):continue
            require(kind in ('diffuse','normal') and m['is_true_mip0'] and m['mip0_gate_enforced'],'review/low mip must not deploy')
            target=targets.get(m['source_mip0_fnv1a64'])
            if target is None:
                require(m.get('injector_target',{}).get('present') is False,'unexpected missing exact target')
                excluded.append(dict(manifest=path,reason='SIDE declared no injector identity; review-only, never staged'));continue
            require((target['w'],target['h'],target['fourcc'])==(m['source_width'],m['source_height'],m['source_fourcc']),'target format/dimension identity')
            original=Path(m['source_file'].replace('<assets-local>',str(a.assets_local)));originalBytes=original.read_bytes();require(sha(originalBytes)==m['source_sha256'],'source DDS changed')
            block=8 if target['fourcc']=='DXT1' else 16
            mip_bytes=max(1,(target['w']+3)//4)*max(1,(target['h']+3)//4)*block
            fnv=0xcbf29ce484222325
            for byte in originalBytes[128:128+mip_bytes]:fnv=((fnv^byte)*0x100000001b3)&0xffffffffffffffff
            require(fnv==m['source_mip0_fnv1a64'],'actual source mip0 FNV identity changed')
            if root=='material-remaster':
                source=root+'/'+Path(path).parent.name+'/'+Path(m['output_file']).name;payload=git(ref,source)
            else:
                source=a.assets_local/root/Path(path).stem/Path(m['output_file']).name;payload=source.read_bytes()
            require(sha(payload)==m['output_sha256'],'SIDE remaster payload changed')
            desc=dds(payload);header=m.get('output_header',m['validation'])
            require((desc['w'],desc['h'],desc['mips'],desc['fourcc'])==(header['width'],header['height'],header['mip_count'],header['fourcc']),'manifest output header mismatch')
            require(target['file'] not in {f['filename'] for f in files},'duplicate replacement')
            dest=a.out/target['file'];dest.write_bytes(payload)
            installed=a.game/'Binaries/texture-override-remaster'/target['file']
            accepted=a.game/'Binaries/texture-override'/target['file']
            files.append(dict(filename=target['file'],source=str(source),source_commit=ref,manifest=path,
                sha256=sha(payload),baseline_sha256=sha(installed.read_bytes()) if installed.exists() else None,
                accepted_baseline_sha256=sha(accepted.read_bytes()) if accepted.exists() else None,
                original_mip0_fnv=m['source_mip0_fnv1a64'],descriptor=desc,status='OFFLINE_EXISTING_ASSET_NOT_PHYSICAL'))
    require(len(files)==24,'expected6prototype+18batch maps')
    (a.out/'material-files.json').write_text(json.dumps(files,indent=2),encoding='utf-8')
    (a.out/'excluded.json').write_text(json.dumps(excluded,indent=2),encoding='utf-8')
    print('STAGED24 EXISTING MAPS; no Steam changes; payload bytes',sum(f['descriptor']['bytes'] for f in files))
if __name__=='__main__':main()
