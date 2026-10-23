"""Read-only log analysis. JSON on stdout; line numbers refer to full archive."""
import json,re,sys,hashlib
from pathlib import Path
p=Path(sys.argv[1]);raw=p.read_bytes();lines=raw.decode('utf-8',errors='replace').splitlines()
def kv(line):return dict(re.findall(r'([A-Za-z]+)=([^ ]+)',line))
draws=[];events=[];cur=None
for n,line in enumerate(lines,1):
    if line.startswith('volk-skin event:'):events.append(dict(line=n,**kv(line)))
    if line.startswith('volk-skin draw: frame='):
        cur=dict(line=n,**kv(line),matrices=[],segments=[]);draws.append(cur)
    if cur is None:continue
    if line.startswith('volk-skin upload:'):cur['upload']=dict(line=n,**kv(line))
    if line.startswith('volk-skin bone:'):
        v=dict(line=n,**kv(line));cur['matrices'].append(v)
    if line.startswith('volk-skin segment:'):cur['segments'].append(dict(line=n,**kv(line)))
    if line.startswith('volk-skin submitted:'):cur['completion']=dict(line=n,**kv(line))
print(json.dumps(dict(sha256=hashlib.sha256(raw).hexdigest(),bytes=len(raw),lines=len(lines),events=events,draws=draws),indent=2))
