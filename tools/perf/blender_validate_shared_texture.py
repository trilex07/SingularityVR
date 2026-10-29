"""Blender 5.2 reimport gate for all lossless shared-texture sidecars."""
import json,sys
from pathlib import Path
import bpy

def capture(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(path))
    meshes=[o for o in bpy.data.objects if o.type=='MESH']
    images={node.image.name for obj in meshes for mat in obj.data.materials if mat and mat.use_nodes
            for node in mat.node_tree.nodes if node.type=='TEX_IMAGE' and node.image}
    return {'meshes':len(meshes),'triangles':sum(len(o.data.polygons) for o in meshes),
            'uv_meshes':sum(bool(o.data.uv_layers) for o in meshes),
            'actions':sorted(a.name for a in bpy.data.actions),
            'image_count':len(images)}

def main():
    if '--' not in sys.argv:raise SystemExit('expected PRIVATE_ROOT CANDIDATE')
    base,candidate=map(Path,sys.argv[sys.argv.index('--')+1:]);pairs={
      'volk_s4':('volk_s4_hd_animated.glb','volk_s4_hd_magazine.glb'),
      'valkyrie_ar9':('valkyrie_ar9_hd_animated.glb','valkyrie_ar9_hd_magazine.glb'),
      'kasimov_snv_e99':('kasimov_hd_animated.glb','kasimov_hd_magazine.glb'),
      'centurion':('centurion_hd_animated.glb','centurion_hd_cylinder.glb')}
    result=[]
    for weapon,files in pairs.items():
        for file in files:
            src=capture(base/weapon/'final_handoff'/file)
            dst=capture(candidate/weapon/(Path(file).stem+'.gltf'))
            if src!=dst:raise ValueError(f'Blender import mismatch {weapon}/{file}: {src} != {dst}')
            result.append({'weapon':weapon,'file':file,'source':src,'candidate':dst,'pass':True})
    report={'status':'OFFLINE_BLENDER_PASS','blender_version':bpy.app.version_string,'files':result}
    (candidate/'blender_validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('SHARED_TEXTURE_BLENDER_PASS',json.dumps({'files':len(result)}))
if __name__=='__main__':main()
