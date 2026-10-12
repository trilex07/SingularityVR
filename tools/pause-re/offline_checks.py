"""Real-asset structural proof + synthetic state/alpha counterexamples. Never runs game code.
Usage: python offline_checks.py external/UI_SinglePlayer_SF.u external/movies/movie-0000af67.swf
No modified package or proprietary bytes are written by this program.
"""
import sys
import struct
from pathlib import Path
from inspect_package import inspect
from inspect_movie import movie_info
from inspect_actions import run

def props(e): return {p['name']:p['value'] for p in e['properties']}

package,movie=map(Path,sys.argv[1:3])
raw=package.read_bytes(); report=inspect(package); es={e['name']:e for e in report['exports']}
assert report['counts']=={'names':226,'exports':44,'imports':26}
assert all('parse_error' not in e for e in es.values())
assert props(es['SPPause'])['MovieInfo']==props(es['SPMain'])['MovieInfo']=='SPMenus'
assert 'RenderTexture' not in props(es['SPPause'])
assert props(es['SPMain'])['RenderTexture']=='RTT_UI1'
assert props(es['RTT_UI2'])['SizeX']==1280 and props(es['RTT_UI2'])['SizeY']==720
assert movie_info(movie.read_bytes())['stage_pixels']==[1280,720]
print('PASS: complete package tables/properties; shared SPMenus; authored 1280x720 stage/RT')

# Offline property-stream experiment: use engine-authored RTT_UI2/mode tags from
# SPLoadGame. Update every affected serialized export offset, then parse again.
donor=es['SPLoadGame']; pause=es['SPPause']
tags=[p for p in donor['properties'] if p['name'] in ('RenderTexture','RenderTextureMode')]
addition=b''.join(raw[p['tag_offset']:p['tag_offset']+p['tag_bytes']] for p in tags)
assert len(addition)==60 and len(tags)==2
insert=pause['offset']+pause['size']-8
patched=bytearray(raw[:insert]+addition+raw[insert:])
for rec in report['records']:
    if rec['index']==pause['index']:struct.pack_into('<I',patched,rec['record_offset']+32,rec['size']+len(addition))
    elif rec['offset']>=insert:struct.pack_into('<I',patched,rec['record_offset']+36,rec['offset']+len(addition))
again=inspect(package,patched); ps={e['name']:e for e in again['exports']}
assert all('parse_error' not in e for e in ps.values())
expected=props(pause)|{'RenderTexture':'RTT_UI2','RenderTextureMode':'RTM_AlphaComposite'}
assert props(ps['SPPause'])==expected
assert all(props(ps[n])==props(e) for n,e in es.items() if n!='SPPause')
assert patched[es['SPMenus']['offset']:es['SPMenus']['offset']+es['SPMenus']['size']]==raw[es['SPMenus']['offset']:es['SPMenus']['offset']+es['SPMenus']['size']]
print('PASS: +60-byte native RenderTexture property experiment reparses; other movie properties and SPMenus unchanged')

setup=run(movie,'SetupPauseMenu')['matches'][0]['context']
values=[v for r in setup for v in r['values']]
assert all(n in values for n in ('ReturnToGame','ReloadCheckpoint','Options','QuitToMainMenu','PauseMenuObjectives','Content','safeRect','width'))
objectives=run(movie,'GetObjectives')['matches'][0]['context']
assert [r['op'] for r in objectives]==['Push','GetVariable','Push','SetMember']
assert objectives[0]['values']==['Objectives'] and objectives[2]['values'][0]=='htmlText'
scale=run(movie,'scaleMode')['matches']
assert any(any(r['values']==['scaleMode','noscale'] for r in m['context']) for m in scale)
print('PASS: AVM1 bodies prove root SetupPauseMenu creates Objectives; callback assigns Objectives.htmlText; Stage uses noscale')

# D3D9 contract model: a state block restores pipeline state, not RT0.
dev={'rt':'back','vp':'full','shader':'game'}
saved={k:v for k,v in dev.items() if k!='rt'}
dev['rt']='pool';dev['shader']='gfx';dev.update(saved)
assert dev['rt']=='pool'
dev['rt']='back';assert dev=={'rt':'back','vp':'full','shader':'game'}
# Transparent target + SRCALPHA/INVSRCALPHA without separate-alpha yields a*a.
a=.25; stored_alpha=a*a; straight_present=a*stored_alpha
assert abs(straight_present-.015625)<1e-9
print('PASS: contract counterexamples reproduce RT leak and alpha-squared attenuation (models, no GPU claim)')
