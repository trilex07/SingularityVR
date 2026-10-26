"""Extend existing exact-hash target registry from verified public manifests.

Never changes an existing identity/alias. No extracted game data is emitted.
"""
import argparse,json,re
from pathlib import Path

ROW=re.compile(r'\{ "([A-Za-z0-9_]+)", (\d+)u, (\d+)u, D3DFMT_(DXT1|DXT5), (\d+)ULL, "([A-Za-z0-9_]+)\.dds", "([a-z]+)" \},')

def extend(text,manifest):
    rows=ROW.findall(text);byname={r[0]:r for r in rows}
    if len(rows)!=len(byname):raise ValueError('Duplicate target name')
    keys={tuple(r[1:5]):r[0] for r in rows}
    additions=[]
    for name,s in manifest['maps'].items():
        if s['status']!='staged':continue
        if not re.fullmatch('[A-Za-z0-9_]+',name):raise ValueError('Unsafe target')
        key=(str(s['source_layout'][0]),str(s['source_layout'][1]),s['source_fourcc'],str(s['source_fnv64']))
        if name in byname:
            if tuple(byname[name][1:5])!=key or byname[name][5]!=name:raise ValueError('Existing identity changed: '+name)
        elif key in keys:raise ValueError('Ambiguous alias: '+name+' / '+keys[key])
        else:
            additions.append(f'    {{ "{name}", {key[0]}u, {key[1]}u, D3DFMT_{key[2]}, {key[3]}ULL, "{name}.dds", "weapons" }},')
            keys[key]=name
    if not additions:return text
    marker='static const Target kTargets[] = {'
    if text.count(marker)!=1:raise ValueError('Registry marker not unique')
    return text.replace(marker,marker+'\n'+'\n'.join(additions),1)

if __name__=='__main__':
    root=Path(__file__).resolve().parents[2];p=argparse.ArgumentParser(description=__doc__);p.add_argument('--write',action='store_true');a=p.parse_args()
    target=root/'spikes/texture_cache/targets_generated.h';text=target.read_text();new=extend(text,json.loads((root/'docs/textures/hires-first-person-manifest.json').read_text()))
    if a.write:target.write_text(new)
    print('Registry changed:',new!=text)
