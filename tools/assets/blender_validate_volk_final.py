"""Run inside Blender against the saved private Volk handoff scene."""
import json
import sys
from pathlib import Path

import bpy
from mathutils import Vector


def pose(action,frame):
    chosen=bpy.data.actions.get(action)
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


def distance(a,b):
    return (a-b).length


def run(output):
    names=('mag_release_state','mag_remove_reference','mag_insert_reference',
           'mag_seat','slide_move','slide_return')
    if set(names)-set(bpy.data.actions.keys()):
        raise ValueError('missing modular actions')
    root=bpy.data.objects['cog_shotgun']
    body=bpy.data.objects['volk_body']
    clip=bpy.data.objects['clip']
    mag=bpy.data.objects['volk_magazine']
    slide=bpy.data.objects['Slide']
    slide_detail=bpy.data.objects['moving_reload_part_machined_fasteners']
    mag_details=[bpy.data.objects[n] for n in ('magazine_machined_fasteners',
                'magazine_rib_lower','magazine_rib_middle','magazine_rib_upper')]
    report={'blender_version':bpy.app.version_string,'actions':{},'part_parents':{},
            'root_stable':True}
    for name in names:
        action=bpy.data.actions[name]
        last=max(0,round(action.frame_range[1]))
        pose(name,0)
        start={k:o.matrix_world.copy() for k,o in [('root',root),('body',body),
                 ('clip',clip),('magazine',mag),('slide',slide),('slide_detail',slide_detail)]}
        mag_local=[clip.matrix_world.inverted()@d.matrix_world for d in mag_details]
        slide_local=slide.matrix_world.inverted()@slide_detail.matrix_world
        pose(name,last)
        end={k:o.matrix_world.copy() for k,o in [('root',root),('body',body),
               ('clip',clip),('magazine',mag),('slide',slide),('slide_detail',slide_detail)]}
        motion={k:distance(start[k].translation,end[k].translation)
                for k in start}
        if motion['root']>1e-7 or motion['body']>1e-7:
            raise ValueError(f'root/body changed in {name}')
        if abs(motion['clip']-motion['magazine'])>1e-6:
            raise ValueError(f'magazine diverged from clip in {name}')
        if abs(motion['slide']-motion['slide_detail'])>1e-6:
            raise ValueError(f'slide detail diverged from source bone in {name}')
        if any((clip.matrix_world.inverted()@d.matrix_world-m).translation.length>1e-6
               for d,m in zip(mag_details,mag_local)):
            raise ValueError(f'magazine detail lost bind in {name}')
        if (slide.matrix_world.inverted()@slide_detail.matrix_world-slide_local).translation.length>1e-6:
            raise ValueError(f'slide detail lost bind in {name}')
        if name.startswith('slide_') and motion['slide']<.01:
            raise ValueError(f'no source slide motion in {name}')
        if name=='mag_seat' and motion['clip']<.01:
            raise ValueError('no source clip seating motion')
        if name=='mag_release_state' and any(v>1e-7 for v in motion.values()):
            raise ValueError('static source release unexpectedly moved')
        report['actions'][name]=motion
    pose('__none__',1)
    for name in ('volk_body','volk_reload_part_01','volk_magazine',
                 'volk_slide','volk_trigger','weapon_body_machined_fasteners',
                 'moving_reload_part_machined_fasteners'):
        obj=bpy.data.objects[name]
        report['part_parents'][name]=obj.parent.name
    original_body=body.matrix_world.copy()
    original_mag=mag.matrix_world.copy()
    # World-preserving detach and hand-follow translation, without saving changes.
    mag.parent=None
    mag.matrix_world=original_mag
    mag.location+=Vector((.1,0,0))
    bpy.context.view_layer.update()
    if distance(mag.matrix_world.translation,original_mag.translation)<.099 or \
       distance(body.matrix_world.translation,original_body.translation)>1e-7:
        raise ValueError('detached magazine movement affected weapon body')
    report['detached_magazine_independent_0p1m']=True
    (output/'blender_pose_validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print('VOLK_POSE_PASS',json.dumps(report))


if __name__=='__main__':
    if '--' not in sys.argv:
        raise SystemExit('expected output path after --')
    run(Path(sys.argv[-1]))
