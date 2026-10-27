from pathlib import Path
import hashlib, json, subprocess
r=Path(__file__).resolve().parents[2]
for p in Path(__file__).parent.glob('*.log'):
    t=p.read_text(errors='replace')
    print(p.name, t[-500:].replace('\n',' | '))
for f in ['spikes/view_matrix/d3d9.dll','deployment/texture-cache/artifact/dinput8.dll']:
    p=r/f
    print(f,hashlib.sha256(p.read_bytes()).hexdigest())
print('HEAD',subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip())
