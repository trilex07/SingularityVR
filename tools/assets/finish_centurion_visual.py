"""Add small rigid machined details, anchored to measured Centurion source surfaces."""
import argparse
import json
from pathlib import Path
from finish_valkyrie_visual import load_obj,side_surface,disk,write,append

def main():
    p=argparse.ArgumentParser();p.add_argument('--visual',type=Path,required=True);a=p.parse_args()
    root=a.visual;rig=json.loads((root/'clean/rig_manifest.json').read_text())
    if rig['source_psk_sha256']!='4e18a25d5241de1b3f18100e3bd9373d464ee16baf3e29d7c60289ae18fe73d4':
        raise ValueError('unrecognized source rig')
    report={};v,f=load_obj(root/'clean/weapon_body.obj');vv=[];ff=[];anchors=[]
    for side in (-1,1):
        for y,z in ((112,55),(114,55),(111,42)):
            x=side_surface(v,f,side,y,z)
            if x is None:raise ValueError(f'no source surface at {side,y,z}')
            center=(x+side*.008,y,z);v1,f1=disk(center,[side,0,0],.16,.055,20)
            append(vv,ff,v1,f1);anchors.append({'side':side,'source_x_uu':x,'y_uu':y,'z_uu':z})
    write(root/'clean/weapon_body_hd_fasteners.obj',vv,ff)
    report['weapon_body_hd_fasteners']={'source_bone':'cog_alt_pistol','vertices':len(vv),'triangles':len(ff),'anchors':anchors}
    # Original six-chamber cylinder remains intact. Two small hub caps move on clip.
    v,f=load_obj(root/'clean/cylinder.obj');vv=[];ff=[]
    center_y=float((v[:,1].min()+v[:,1].max())/2);center_z=float((v[:,2].min()+v[:,2].max())/2)
    for side in (-1,1):
        x=float(v[:,0].max() if side==1 else v[:,0].min())+side*.008
        v1,f1=disk((x,center_y,center_z),[side,0,0],.22,.07,24);append(vv,ff,v1,f1)
    write(root/'clean/cylinder_hd_hubs.obj',vv,ff)
    report['cylinder_hd_hubs']={'source_bone':'clip','vertices':len(vv),'triangles':len(ff),'center_yz_uu':[center_y,center_z]}
    (root/'clean/centurion_detail.mtl').write_text('newmtl machined_dark_metal\nKd .16 .17 .17\nKs .55 .55 .55\nNs 160\n',encoding='utf-8')
    (root/'clean/hd_detail_manifest.json').write_text(json.dumps({'source_psk_sha256':rig['source_psk_sha256'],'details':report,'source_geometry_unchanged':True,'binding':'inverse original source bind per named bone'},indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
