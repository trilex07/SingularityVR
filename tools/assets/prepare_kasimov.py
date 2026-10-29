"""Prepare private original Kasimov weighted mesh and source animation data."""
import json,sys,struct,hashlib
from pathlib import Path
import numpy as np
from PIL import Image
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'reload'))
from prepare_centurion import read_psk
from prepare_weapon_animation import key_local

def main():
 root=Path(sys.argv[1]);root.mkdir(parents=True,exist_ok=True)
 base=Path(__file__).resolve().parents[3]
 psk=base/'research/native-hands-20261007/assets/RvGameMPLoader_SF/SkeletalMesh3/ch_inview_sniper_mesh.psk'
 cache=base/'dependencies/Singularity/Binaries/vr-assets/reload/kasimov/kasimov_reload.svwa'
 names,parents,local,bind,points,weights,wedges,faces=read_psk(psk)
 r=np.diag([1.,-1.,1.,1.]);absolute=r@bind@r
 data=cache.read_bytes();h=struct.unpack_from('<4s10If',data)
 assert h==(b'SVWA',1,3,11389,0,12942,906901468,6,0,5,239,60.)
 tracks=[55,56,57,58,59];keys=np.frombuffer(data,'<f4',239*5*7,112+5*72).reshape(239,5,7)
 poses=[]
 for frame in range(239):
  matrices=absolute.copy()
  for t,b in enumerate(tracks):matrices[b]=key_local(keys[frame,t])@matrices[parents[b]]
  poses.append(matrices[tracks].tolist())
 report={'names':names,'parents':parents,'bind':absolute.tolist(),'points':points.tolist(),
  'weights':{str(k):v for k,v in weights.items()},'wedges':wedges,'faces':faces,
  'tracks':tracks,'poses':poses,'fps':60,'units':52.5,
  'source_hashes':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (psk,cache)},
  'dual_points':[p for p,w in weights.items() if len(w)==2]}
 assert len(report['dual_points'])==143
 (root/'source_data.json').write_text(json.dumps(report))
 for p in (psk,cache):(root/p.name).write_bytes(p.read_bytes())
 maps=root/'maps';maps.mkdir(exist_ok=True)
 texture=(base/'dependencies/Singularity/Binaries/vr-assets/reload/kasimov/kasimov_clip.svrt').read_bytes()
 magic,version,w,h=struct.unpack_from('<4sIII',texture);assert magic==b'SVRT'
 Image.fromarray(np.frombuffer(texture,'u1',w*h*4,16).reshape(h,w,4)[:,:,[2,1,0,3]],'RGBA').save(maps/'kasimov_original_albedo.png')
 tex=base/'dependencies/Singularity/Binaries/texture-override-remaster'
 albedo=Image.open(tex/'ar_sniper_rifle_d.dds').convert('RGB');albedo.save(maps/'kasimov_albedo_4k.png')
 n=np.asarray(Image.open(tex/'ar_sniper_rifle_n.dds').convert('RGB')).copy();n[:,:,1]=255-n[:,:,1]
 v=n.astype(np.float32)/127.5-1;v/=np.maximum(np.linalg.norm(v,axis=2,keepdims=True),1e-6)
 Image.fromarray(np.uint8(np.clip((v*.5+.5)*255,0,255))).save(maps/'kasimov_normal_4k.png')
 rgb=np.asarray(albedo,dtype=np.float32);lum=rgb.mean(2);sat=rgb.max(2)-rgb.min(2)
 metal=np.clip((lum-65)/80,0,1)*np.clip((75-sat)/50,0,1)
 Image.fromarray(np.uint8(metal*210)).save(maps/'kasimov_metallic_4k.png')
 Image.fromarray(np.uint8(np.clip(175-metal*65+sat*.1,80,215))).save(maps/'kasimov_roughness_4k.png')
 print('prepared',len(points),len(wedges),len(faces),'dual',143)
if __name__=='__main__':main()
