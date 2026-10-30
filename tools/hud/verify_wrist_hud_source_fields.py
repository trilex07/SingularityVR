"""Verify property ownership against owned decoded packages, never runtime offsets."""
import argparse, sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'title'))
from ue3_layout import inspect
p=argparse.ArgumentParser();p.add_argument('--game',required=True);p.add_argument('--engine',required=True);a=p.parse_args()
def owners(path,name):
    rows=inspect(path,include_all=True)['records']
    return {rows[e['outer']-1]['name'] for e in rows if e['name']==name and e['outer']>0}
assert owners(a.game,'sharedHUD')=={'ClientSetCinematicMode'}
assert 'PlayerController' in owners(a.engine,'myHUD')
assert 'RvGameSharedHUD' in owners(a.game,'mE99TechCount')
source=(Path(__file__).resolve().parents[2]/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')
assert 'offShared=PropOffsetInChain(cc,"myHUD")' in source
assert '{ "myHUD", -1 }' in source
print('PASS actual package ownership: sharedHUD is function-local; myHUD belongs to PlayerController; E99 count belongs to shared HUD')
