"""Offline native-FX candidates from real driver traces, NOT pixel identification.

Compare same scene/weapon baseline + post-shot captures; never use COM addresses
between sessions or automatically choose a shader as the visible muzzle effect.
"""
import argparse
import importlib.util
import json
from pathlib import Path

spec=importlib.util.spec_from_file_location('pass_trace',Path(__file__).resolve().parents[1]/'analyze-shadow-pass-trace.py')
trace=importlib.util.module_from_spec(spec);spec.loader.exec_module(trace)

def inventory(value):
    rows={}
    for group in value['groups'].values():
        vs=value['shaders'].get(group['VS']);ps=value['shaders'].get(group['PS'])
        if not vs or not ps:continue
        key=vs['fnv1a64']+':'+ps['fnv1a64']
        row=rows.setdefault(key,dict(VS=vs,PS=ps,count=0,groups=[]))
        row['count']+=int(group['draws']);row['groups'].append(group)
    return rows

def compare(baseline,shots):
    values=[trace.read_trace(p) for p in [baseline,*shots]]
    if any(not x['complete_budget'] or x.get('shader_budget_overflow') or x.get('shader_unavailable') for x in values):
        raise ValueError('Truncated/unavailable shader trace: resolve evidence budget before drawing conclusions')
    base=inventory(values[0]);results={}
    for index,value in enumerate(values[1:]):
        for key,row in inventory(value).items():
            original=base.get(key,{}).get('count',0)
            if row['count']<=original:continue
            dest=results.setdefault(key,dict(pair=key,baseline_count=original,VS=row['VS'],PS=row['PS'],shots=[]))
            dest['shots'].append(dict(index=index,count=row['count'],groups=row['groups']))
    return dict(status='CANDIDATES_NOT_IDENTIFIED',baseline=str(baseline),shots=[str(p) for p in shots],
                candidates=list(results.values()),warning='Count differences include enemies, shell ejection, impacts and lights. Require exact-pair drop/content evidence before correction.')

def drop_snippet(report,pair):
    rows=[r for r in report['candidates'] if r['pair']==pair.upper()]
    if len(rows)!=1:raise ValueError('Select exactly one pair present in the measured candidate list')
    row=rows[0]
    return ('; Discriminator only, NOT a fix. MAIN backs up/scopes these keys.\n[Diagnostic]\n'
            'NativeMuzzleProbe=0\nNativeMuzzleStereoMode=1\n'
            f"NativeMuzzleVSHash={row['VS']['fnv1a64']}\nNativeMuzzleVSBytes={row['VS']['bytes']}\n"
            f"NativeMuzzlePSHash={row['PS']['fnv1a64']}\nNativeMuzzlePSBytes={row['PS']['bytes']}\n")

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('baseline',type=Path)
    ap.add_argument('shots',type=Path,nargs='+');ap.add_argument('--select',help='Explicit VS_FNV64:PS_FNV64 pair, output drop-only INI snippet')
    a=ap.parse_args();report=compare(a.baseline,a.shots)
    print(drop_snippet(report,a.select) if a.select else json.dumps(report,indent=2))
