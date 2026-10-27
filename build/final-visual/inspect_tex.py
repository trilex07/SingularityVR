import sys,struct,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/audio'))
from ue3_sound_package import inspect
p=Path(sys.argv[1]);b=p.read_bytes();pkg=inspect(b);names=pkg['names']
for e in pkg['exports']:
 if e['class_name']!='Texture2D' or e['name'] not in sys.argv[2:]:continue
 print('EXPORT',e)
 q=e['offset']+4;end=e['offset']+e['size']
 while q<end:
  ni,number=struct.unpack_from('<ii',b,q);q+=8;nm=names[ni]
  if nm=='None':print('NONE at',hex(q));break
  ti,tn,n,idx=struct.unpack_from('<iiii',b,q);q+=16;typ=names[ti]
  if typ=='BoolProperty':q+=4
  elif typ=='StructProperty':q+=8
  print('PROP',nm,typ,n,idx,'at',hex(q),'bytes',b[q:q+min(n,32)].hex(' '))
  q+=n
 print('FOOTER',hex(q),end-q,b[q:min(end,q+450)].hex(' '))
