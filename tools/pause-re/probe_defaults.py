"""Targeted static GFx/Raven class, CDO and property ownership inspection."""
import argparse
import json
from inspect_package import inspect

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('package');ap.add_argument('query')
    ap.add_argument('--exact',action='store_true',help='match one leaf name, not descendants')
    a=ap.parse_args()
    r=inspect(a.package,include_all=True)
    records={e['index']:e for e in r['records']};imports=r['imports'];names=r['names']
    def ref(i):return names[imports[-i-1][5]] if i<0 else records[i]['name'] if i else 'None'
    results=[]
    for e in r['exports']:
        ancestry=[];c=e['index']
        for _ in range(16):
            if not c:break
            ancestry.append(ref(c))
            c=records[c]['outer'] if c>0 else imports[-c-1][4]
        if a.exact:
            if e['name']!=a.query:continue
        elif not any(a.query.lower() in name.lower() for name in ancestry):continue
        results.append(dict(name='.'.join(reversed(ancestry)),cls=e['class_name'],
            super=ref(e['super_index']),archetype=ref(e['archetype']),offset=hex(e['offset']),size=e['size'],
            properties=e['properties'],property_end=e.get('property_end'),parse_error=e.get('parse_error')))
    print(json.dumps(results,indent=2))
