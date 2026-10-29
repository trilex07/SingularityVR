"""Copy private Centurion outputs into one offline handoff and hash every file."""
import argparse,hashlib,json,shutil
from pathlib import Path
from PIL import Image

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--private-root',type=Path,required=True);a=ap.parse_args()
    base=a.private_root;final=base/'final_handoff';visual=base/'visual';mech=base/'mechanical'
    if json.loads((final/'blender_import_validation.json').read_text())['status']!='OFFLINE_BLENDER_PASS':
        raise ValueError('Blender reimport gate missing')
    for src,dest in ((visual/'source',final/'source'),(visual/'maps',final/'textures'),
                     (visual/'clean',final/'clean'),(mech,final/'mechanical')):
        dest.mkdir(exist_ok=True)
        for f in src.iterdir():
            if f.is_file():shutil.copy2(f,dest/f.name)
    previews=final/'previews';previews.mkdir(exist_ok=True)
    for p in final.glob('*.png'):shutil.move(str(p),str(previews/p.name))
    for name in ('centurion_hd_albedo_4k.png','centurion_hd_normal_gltf_4k.png',
                 'centurion_hd_roughness_4k.png','centurion_hd_metallic_4k.png'):
        with Image.open(final/'textures'/name) as im:
            if im.size!=(4096,4096):raise ValueError('map resolution '+name)
    if len(list(previews.glob('*.png')))<7:raise ValueError('previews missing')
    files={str(p.relative_to(final)).replace('\\','/'):{'sha256':sha(p),'bytes':p.stat().st_size}
           for p in sorted(final.rglob('*')) if p.is_file() and p.name!='SHA256_manifest.json'}
    (final/'SHA256_manifest.json').write_text(json.dumps({'status':'OFFLINE_BLENDER_PASS',
        'private_game_assets':True,'files':files},indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'path':str(final),'file_count':len(files),'bytes':sum(v['bytes'] for v in files.values()),
                      'status':'OFFLINE_BLENDER_PASS'},indent=2))
if __name__=='__main__':main()
