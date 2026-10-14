"""Export bounded copied constants for private offline replay; never touches Steam."""
import argparse
import json
import re
from pathlib import Path

def extract(text):
    records=[]
    mode='UNKNOWN'
    lines=text.splitlines()
    names=['local230','controllerAim','controllerOff','nativeLeft9','nativeRight51','posedLeft9','posedRight51']
    widths=[16,16,16,12,12,12,12]
    for i,line in enumerate(lines):
        transition=re.search(r'SceneMode: \w+ -> (\w+)',line)
        if transition:mode=transition.group(1)
        match=re.search(r'hand-anchor sample=(\d+) frame=(\d+) moved=(\d+) targetL=\(([^)]+)\) targetR=\(([^)]+)\)',line)
        if not match:continue
        stage=re.search(r'stage=(\w+)',line)
        record=dict(sample=int(match[1]),frame=int(match[2]),moved=int(match[3]),mode=stage[1] if stage else mode,
                    target=[list(map(float,match[4].split(','))),list(map(float,match[5].split(',')))])
        fields=list(zip(names,widths))
        if i+4<len(lines) and lines[i+4].startswith('hand-anchor cameraRide:'):
            fields.insert(3,('cameraRide',16))
        for n,(name,width) in enumerate(fields,1):
            prefix='hand-anchor '+name+':'
            if i+n>=len(lines) or not lines[i+n].startswith(prefix):raise ValueError('incomplete record')
            values=list(map(float,lines[i+n][len(prefix):].split()))
            if len(values)!=width:raise ValueError('wrong matrix length')
            record[name]=values
        records.append(record)
    if len(records)>8:raise ValueError('bounded probe exceeded')
    return records

if __name__=='__main__':
    ap=argparse.ArgumentParser()
    ap.add_argument('log',type=Path);ap.add_argument('--out',type=Path,required=True)
    args=ap.parse_args()
    if args.out.exists():raise ValueError('refuse existing output')
    records=extract(args.log.read_text(encoding='utf-8',errors='replace'))
    args.out.mkdir(parents=True)
    (args.out/'records.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
    # No live shader/camera claim: the replay explicitly labels its ride assumption.
    with (args.out/'replay.txt').open('w',encoding='utf-8') as stream:
        for r in records:
            if r['mode']!='GameplayStereo':continue
            values=[r['frame'],r['moved'],*r['target'][0],*r['target'][1]]
            for name in ['local230','controllerAim','controllerOff','nativeLeft9','nativeRight51','posedLeft9','posedRight51']:
                values.extend(r[name])
            if 'cameraRide' in r:values.extend(r['cameraRide'])
            stream.write(' '.join(map(str,values))+'\n')
    print(json.dumps(dict(total=len(records),gameplay=sum(r['mode']=='GameplayStereo' for r in records),
                         other=sum(r['mode']!='GameplayStereo' for r in records))))
