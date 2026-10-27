from pathlib import Path
import mmap,struct,ctypes,io,json
from PIL import Image
import numpy as np
base=Path(r'C:/Users/Trilex/vr-mods/SingularityVR-assets-local')
dll=ctypes.CDLL(str(Path('build/final-visual/lzo_wrap.dll').resolve())); dec=dll.decode_lzo; dec.argtypes=[ctypes.c_void_p,ctypes.c_ulonglong,ctypes.c_void_p,ctypes.c_ulonglong];dec.restype=ctypes.c_int
src=(base/'textures/original/RvGameMPLoader_SF/Texture2D/ar_sniper_rifle_d.dds').read_bytes(); a=np.asarray(Image.open(io.BytesIO(src)).convert('RGB').resize((64,32)),dtype=np.float32).flatten();a-=a.mean(); a/=np.linalg.norm(a)
results=[];count=0
with (base/'cooked/Textures_P0.tfc').open('rb') as f, mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ) as mm:
 p=0
 while True:
  p=mm.find(b'\xc1\x83\x2a\x9e',p)
  if p<0:break
  at=p;p+=4
  tag,block,c,u=struct.unpack_from('<4I',mm,at)
  if block!=131072 or u!=2097152:continue
  blocks=(u+block-1)//block; cursor=at+16+blocks*8;out=bytearray()
  try:
   for k in range(blocks):
    cc,uu=struct.unpack_from('<2I',mm,at+16+k*8); bb=mm[cursor:cursor+cc];cursor+=cc
    target=ctypes.create_string_buffer(uu); used=dec(bb,cc,target,uu)
    if used!=uu:raise ValueError('decode')
    out.extend(target.raw)
   if len(out)!=u:raise ValueError('size')
   hdr=bytearray(src[:128]);struct.pack_into('<I',hdr,12,1024);struct.pack_into('<I',hdr,16,2048);struct.pack_into('<I',hdr,28,1)
   im=Image.open(io.BytesIO(hdr+out)).convert('RGB');b=np.asarray(im.resize((64,32)),dtype=np.float32).flatten();b-=b.mean(); b/=max(np.linalg.norm(b),1e-8)
   corr=float(a@b);count+=1
   results.append({'offset':hex(at),'correlation':round(corr,6),'compressed':c})
  except Exception as e: print('bad',hex(at),str(e))
results.sort(key=lambda z:-z['correlation'])
Path('build/final-visual/kasimov-tfc-scan.json').write_text(json.dumps({'count':count,'best':results[:20]},indent=2));print(count,results[:10])
