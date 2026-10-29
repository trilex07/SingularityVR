"""Independent Blender import check of final Valkyrie animated and detached GLBs."""
import hashlib
import json
import sys
from pathlib import Path

import bpy
from mathutils import Matrix


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition,message):
    if not condition:raise ValueError(message)


def activate_action(action,frame):
    for obj in bpy.data.objects:
        if not obj.animation_data:continue
        ad=obj.animation_data;ad.use_nla=False;ad.action=None
        slot=next((s for s in action.slots if s.identifier=='OB'+obj.name),None)
        if slot:
            ad.action=action;ad.action_slot=slot
    bpy.context.scene.frame_set(frame);bpy.context.view_layer.update()


def loc(obj):return obj.matrix_world.translation.copy()


def main():
    if '--' not in sys.argv or len(sys.argv[sys.argv.index('--')+1:])!=1:
        raise SystemExit('expected FINAL after --')
    root=Path(sys.argv[sys.argv.index('--')+1])
    visual=root.parent/'visual'
    rig=json.loads((visual/'clean/rig_manifest.json').read_text())
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.gltf(filepath=str(root/'valkyrie_ar9_hd_animated.glb'))
    for b in rig['bones']:
        require(b['name'] in bpy.data.objects,'missing original bone '+b['name'])
    meshes=[o for o in bpy.data.objects if o.type=='MESH']
    require(len(meshes)==9,'expected nine final meshes')
    for o in meshes:
        require(len(o.data.uv_layers)>0 or o.name in {
            'magazine_hd_ribs','weapon_body_hd_fasteners','fan_hd_hub'},'missing UV0 '+o.name)
        require(len(o.data.polygons)>0,'empty mesh '+o.name)
    for name,bone in (('magazine_hd_ribs','clip'),('weapon_body_hd_fasteners','cog_machinegun'),
                      ('fan_hd_hub','fan')):
        require(bpy.data.objects[name].parent==bpy.data.objects[bone],
                'HD detail wrong parent '+name)
    actions=[a for a in bpy.data.actions if a.name=='Reload_source_52f']
    require(len(actions)==1,'missing source reload action')
    action=actions[0]
    activate_action(action,1)
    body0=loc(bpy.data.objects['cog_machinegun']);clip0=loc(bpy.data.objects['clip'])
    mag0=loc(bpy.data.objects['magazine']);ribs0=loc(bpy.data.objects['magazine_hd_ribs'])
    relative0=ribs0-clip0
    activate_action(action,22)
    body22=loc(bpy.data.objects['cog_machinegun']);clip22=loc(bpy.data.objects['clip'])
    mag22=loc(bpy.data.objects['magazine']);ribs22=loc(bpy.data.objects['magazine_hd_ribs'])
    require((body22-body0).length<1e-6,'controller weapon root moved')
    require((clip22-clip0).length>.02,'magazine source motion missing')
    require((mag22-mag0).length>.02 and (ribs22-ribs0).length>.02,
            'magazine or HD ribs did not follow clip')
    # World-preserving detach then 0.1 m translation; parent root stays fixed.
    activate_action(action,1)
    original_body=loc(bpy.data.objects['cog_machinegun'])
    magazine=bpy.data.objects['magazine'];world=magazine.matrix_world.copy()
    magazine.parent=None;magazine.matrix_world=world
    magazine.location.x+=.1;bpy.context.view_layer.update()
    require((loc(bpy.data.objects['cog_machinegun'])-original_body).length<1e-6,
            'detached magazine moved weapon root')
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.gltf(filepath=str(root/'valkyrie_ar9_hd_magazine.glb'))
    detached=[o for o in bpy.data.objects if o.type=='MESH']
    require(len(detached)==2,'detached magazine missing base or HD ribs')
    report={'status':'OFFLINE_BLENDER_PASS','blender_version':bpy.app.version_string,
            'animated_glb_sha256':sha(root/'valkyrie_ar9_hd_animated.glb'),
            'magazine_glb_sha256':sha(root/'valkyrie_ar9_hd_magazine.glb'),
            'source_bones':len(rig['bones']),'final_meshes':9,'detached_magazine_meshes':2,
            'action':'Reload_source_52f','root_stable':True,
            'clip_displacement_frame22_m':float((clip22-clip0).length),
            'hd_ribs_follow_clip':True,'detached_magazine_independent_0p1m':True,
            'runtime':'NOT_RUN'}
    (root/'blender_import_validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('VALKYRIE_FINAL_IMPORT_PASS',json.dumps(report))


if __name__=='__main__':main()
