"""Headless Blender import/export and source motion validation for Valkyrie."""
import json
import sys
from pathlib import Path

import bpy


def pose(action_name, frame):
    chosen=bpy.data.actions.get(action_name)
    for obj in bpy.data.objects:
        if obj.animation_data:
            ad=obj.animation_data
            ad.use_nla=False
            ad.action=None
            if chosen:
                slot=next((s for s in chosen.slots if s.identifier=='OB'+obj.name),None)
                if slot:
                    ad.action=chosen
                    ad.action_slot=slot
    bpy.context.scene.frame_set(frame)
    bpy.context.view_layer.update()


def main(source,output):
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.gltf(filepath=str(source))
    expected={'weapon_body':'cog_machinegun','magazine':'clip','trigger':'Trigger',
              'moving_reload_part':'Slide','fan':'fan','clip_release':'clip_release'}
    mesh_objects={}
    for mesh,bone in expected.items():
        hits=[o for o in bpy.data.objects if o.type=='MESH' and o.parent and
              o.parent.name==bone and o.name.startswith(mesh)]
        if len(hits)!=1 or not hits[0].data.uv_layers:
            raise ValueError(f'bad mesh binding/UV: {mesh}, hits={[(o.name,len(o.data.uv_layers)) for o in hits]}')
        mesh_objects[mesh]=hits[0]
    action='Reload_source_52f'
    if action not in bpy.data.actions: raise ValueError('missing source Reload action')
    pose(action,0)
    nodes=['cog_machinegun','clip','Trigger','Slide','fan','clip_release']
    initial={n:bpy.data.objects[n].matrix_world.copy() for n in nodes}
    translation={n:0.0 for n in nodes};rotation={n:0.0 for n in nodes}
    for frame in range(52):
        pose(action,frame)
        for n in nodes:
            now=bpy.data.objects[n].matrix_world
            translation[n]=max(translation[n],float((now.translation-initial[n].translation).length))
            rotation[n]=max(rotation[n],float(now.to_quaternion().rotation_difference(initial[n].to_quaternion()).angle))
    if translation['cog_machinegun']>1e-7 or rotation['cog_machinegun']>1e-7:
        raise ValueError('controller root moved')
    if max(translation['clip'],rotation['clip'])<.001:
        raise ValueError('clip did not animate')
    if rotation['clip_release']<.001:
        raise ValueError('clip_release did not animate')
    for n in ('Trigger','Slide','fan'):
        if translation[n]>1e-6 or rotation[n]>1e-6:
            raise ValueError(f'static source part unexpectedly animated: {n}')
    pose(action,0)
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.export_scene.gltf(filepath=str(output/'valkyrie_mechanical_blender_roundtrip.glb'),
                              export_format='GLB',use_selection=False,export_animations=True)
    report={'status':'BLENDER_IMPORT_EXPORT_PASS','blender_version':bpy.app.version_string,
            'objects':{n:{'blender_name':mesh_objects[n].name,'parent':p,'vertices':len(mesh_objects[n].data.vertices)}
                       for n,p in expected.items()},
            'action':action,'source_frames':52,'fps':24,
            'max_frame0_relative_translation_m':translation,'max_frame0_relative_rotation_rad':rotation,
            'root_stable':True,'static_source_parts_unchanged':True}
    (output/'blender_mechanical_validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print('VALKYRIE_BLENDER_PASS',json.dumps(report))


if __name__=='__main__':
    if '--' not in sys.argv: raise SystemExit('expected GLB and output paths after --')
    p=sys.argv.index('--')
    main(Path(sys.argv[p+1]),Path(sys.argv[p+2]))
