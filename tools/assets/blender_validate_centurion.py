"""Independent Blender import check of final Centurion animated and detached GLBs."""
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
    bpy.ops.import_scene.gltf(filepath=str(root/'centurion_hd_animated.glb'))
    for b in rig['bones']:
        require(b['name'] in bpy.data.objects,'missing original bone '+b['name'])
    meshes=[o for o in bpy.data.objects if o.type=='MESH']
    require(len(meshes)==7,'expected seven final meshes')
    for o in meshes:
        require(len(o.data.uv_layers)>0 or o.name in {
            'cylinder_hd_hubs','weapon_body_hd_fasteners'},'missing UV0 '+o.name)
        require(len(o.data.polygons)>0,'empty mesh '+o.name)
    for name,bone in (('cylinder_hd_hubs','clip'),('weapon_body_hd_fasteners','cog_alt_pistol')):
        require(bpy.data.objects[name].parent==bpy.data.objects[bone],
                'HD detail wrong parent '+name)
    actions=[a for a in bpy.data.actions if a.name=='Reload_source_53f']
    require(len(actions)==1,'missing source reload action')
    action=actions[0]
    activate_action(action,1)
    body0=loc(bpy.data.objects['cog_alt_pistol']);clip0=loc(bpy.data.objects['clip'])
    cyl0=loc(bpy.data.objects['cylinder']);hub0=loc(bpy.data.objects['cylinder_hd_hubs'])
    activate_action(action,22)
    body22=loc(bpy.data.objects['cog_alt_pistol']);clip22=loc(bpy.data.objects['clip'])
    cyl22=loc(bpy.data.objects['cylinder']);hub22=loc(bpy.data.objects['cylinder_hd_hubs'])
    require((body22-body0).length<1e-6,'controller weapon root moved')
    require((clip22-clip0).length>.02,'cylinder source motion missing')
    require((cyl22-cyl0).length>.02 and (hub22-hub0).length>.02,
            'cylinder or HD hub did not follow clip')
    # World-preserving detach then 0.1 m translation; parent root stays fixed.
    activate_action(action,1)
    original_body=loc(bpy.data.objects['cog_alt_pistol'])
    cylinder=bpy.data.objects['cylinder'];world=cylinder.matrix_world.copy()
    cylinder.parent=None;cylinder.matrix_world=world
    cylinder.location.x+=.1;bpy.context.view_layer.update()
    require((loc(bpy.data.objects['cog_alt_pistol'])-original_body).length<1e-6,
            'detached cylinder moved weapon root')
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.gltf(filepath=str(root/'centurion_hd_cylinder.glb'))
    detached=[o for o in bpy.data.objects if o.type=='MESH']
    require(len(detached)==2,'detached cylinder missing base or HD hubs')
    report={'status':'OFFLINE_BLENDER_PASS','blender_version':bpy.app.version_string,
            'animated_glb_sha256':sha(root/'centurion_hd_animated.glb'),
            'cylinder_glb_sha256':sha(root/'centurion_hd_cylinder.glb'),
            'source_bones':len(rig['bones']),'final_meshes':7,'detached_cylinder_meshes':2,
            'action':'Reload_source_53f','root_stable':True,
            'clip_displacement_frame22_m':float((clip22-clip0).length),
            'hd_hubs_follow_clip':True,'detached_cylinder_independent_0p1m':True,
            'runtime':'NOT_RUN'}
    (root/'blender_import_validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('CENTURION_FINAL_IMPORT_PASS',json.dumps(report))


if __name__=='__main__':main()
