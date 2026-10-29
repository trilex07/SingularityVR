"""Repeatable offline Blender import timing; not a game loading benchmark."""
import json,statistics,sys,time
from pathlib import Path
import bpy

PAIRS={
 'volk_s4':('volk_s4_hd_animated.glb','volk_s4_hd_magazine.glb'),
 'valkyrie_ar9':('valkyrie_ar9_hd_animated.glb','valkyrie_ar9_hd_magazine.glb'),
 'kasimov_snv_e99':('kasimov_hd_animated.glb','kasimov_hd_magazine.glb'),
 'centurion':('centurion_hd_animated.glb','centurion_hd_cylinder.glb')}

def load(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    start=time.perf_counter();bpy.ops.import_scene.gltf(filepath=str(path))
    return (time.perf_counter()-start)*1000

def main():
    if '--' not in sys.argv:raise SystemExit('expected PRIVATE_ROOT CANDIDATE')
    root,candidate=map(Path,sys.argv[sys.argv.index('--')+1:]);rows=[]
    for weapon,files in PAIRS.items():
        for filename in files:
            before=root/weapon/'final_handoff'/filename
            after=candidate/weapon/(Path(filename).stem+'.gltf')
            # Warm both paths; alternating order reduces systematic cache bias.
            load(before);load(after);bt=[];at=[]
            for i in range(5):
                order=((before,bt),(after,at)) if i%2==0 else ((after,at),(before,bt))
                for path,bucket in order:bucket.append(load(path))
            rows.append({'weapon':weapon,'file':filename,'source_ms':bt,'candidate_ms':at,
                         'source_median_ms':statistics.median(bt),
                         'candidate_median_ms':statistics.median(at)})
    report={'scope':'Blender 5.2.2 import, warm filesystem cache, five alternating repetitions per file; not runtime',
            'blender_version':bpy.app.version_string,'rows':rows}
    (candidate/'blender_import_benchmark.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('BLENDER_IMPORT_BENCHMARK',json.dumps([{'weapon':r['weapon'],'file':r['file'],
       'source_ms':round(r['source_median_ms'],2),'candidate_ms':round(r['candidate_median_ms'],2)} for r in rows]))
if __name__=='__main__':main()
