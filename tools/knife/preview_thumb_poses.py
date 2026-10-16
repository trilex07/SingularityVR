"""Private CPU depth/UV thumb previews, not a native lighting render."""
import argparse
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw
from validate_hand_grip import raster
from fit_thumb_poses import Rig, read_psk, unit


def preview(prepared,psk_path,hand_texture,knife_texture):
    rig=Rig(read_psk(psk_path));tex=np.array(Image.open(hand_texture).convert('RGB'))
    ktex=np.array(Image.open(knife_texture).convert('RGB'));atlas=Image.new('RGB',(1680,1680))
    for row,(mode,side) in enumerate((('fist',0),('fist',1),('knife',0),('knife',1))):
        prefix=('l_','r_')[side];data=np.load(prepared/f'{mode}_{prefix}assembly.npz')
        hand=data['hand'];knife=data['knife'];hf=data['hand_faces']
        wrist=rig.bind[9 if side==0 else 51,3,:3]
        forward=unit(rig.bind[rig.names.index(prefix+'middle_1'),3,:3]-wrist)
        across=unit(rig.bind[rig.names.index(prefix+'index_1'),3,:3]-rig.bind[rig.names.index(prefix+'pinky_1'),3,:3])
        dorsal=unit(np.cross(across,forward))*(1 if side==0 else -1)
        hp=(hand-wrist)/70;kp=(knife-wrist)/70;centre=hp[np.unique(hf)].mean(0)
        uv=np.array([w[1:] for w in rig.psk[6]])
        for col,axes in enumerate((np.array([across,-forward,dorsal]),np.array([across,forward,-dorsal]),np.array([forward,dorsal,across]),np.array([-forward,dorsal,-across]))):
            rgb,z=raster(hp-centre,hf,uv,tex,axes)
            if mode=='knife':
                krgb,kz=raster(kp-centre,data['knife_faces'],data['knife_uv'],ktex,axes)
                mask=kz>z;rgb[mask]=krgb[mask]
            im=Image.fromarray(rgb);ImageDraw.Draw(im).text((8,8),f'{mode} {prefix} view {col} / CPU ALBEDO',fill='white')
            atlas.paste(im,(420*col,420*row))
    atlas.save(prepared/'thumb-preview.png')


if __name__=='__main__':
    p=argparse.ArgumentParser()
    for name in ('prepared','psk','hand-texture','knife-texture'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();preview(a.prepared,a.psk,a.hand_texture,a.knife_texture)
