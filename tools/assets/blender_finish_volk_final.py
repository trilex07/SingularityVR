"""Run inside official Blender: import, materialize, inspect and export Volk HD.

Usage: blender --background --factory-startup --python this.py -- PACKAGE HANDOFF
"""
import hashlib
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def image(path,space):
    img=bpy.data.images.load(str(path),check_existing=True)
    img.colorspace_settings.name=space
    img.pack()
    return img


def material(name,albedo,roughness,metallic,normal):
    mat=bpy.data.materials.new(name)
    mat.use_nodes=True
    nodes=mat.node_tree.nodes
    nodes.clear()
    out=nodes.new('ShaderNodeOutputMaterial')
    bsdf=nodes.new('ShaderNodeBsdfPrincipled')
    mat.node_tree.links.new(bsdf.outputs['BSDF'],out.inputs['Surface'])
    for path,space,socket in ((albedo,'sRGB','Base Color'),
                              (roughness,'Non-Color','Roughness'),
                              (metallic,'Non-Color','Metallic')):
        tex=nodes.new('ShaderNodeTexImage')
        tex.image=image(path,space)
        mat.node_tree.links.new(tex.outputs['Color'],bsdf.inputs[socket])
    tex=nodes.new('ShaderNodeTexImage')
    tex.image=image(normal,'Non-Color')
    normal_node=nodes.new('ShaderNodeNormalMap')
    normal_node.space='TANGENT'
    mat.node_tree.links.new(tex.outputs['Color'],normal_node.inputs['Color'])
    mat.node_tree.links.new(normal_node.outputs['Normal'],bsdf.inputs['Normal'])
    return mat


def dark_metal():
    mat=bpy.data.materials.new('volk_machined_dark_metal')
    mat.use_nodes=True
    p=mat.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value=(.14,.15,.15,1)
    p.inputs['Metallic'].default_value=.65
    p.inputs['Roughness'].default_value=.34
    return mat


def world_bounds(meshes):
    points=[o.matrix_world@Vector(c) for o in meshes for c in o.bound_box]
    lo=Vector(tuple(min(p[i] for p in points) for i in range(3)))
    hi=Vector(tuple(max(p[i] for p in points) for i in range(3)))
    return lo,hi


def set_action(name,frame):
    action=bpy.data.actions.get(name)
    for obj in bpy.data.objects:
        if obj.animation_data:
            ad=obj.animation_data
            ad.use_nla=False
            ad.action=None
            if action:
                slot=next((s for s in action.slots if s.identifier=='OB'+obj.name),None)
                if slot:
                    ad.action=action
                    ad.action_slot=slot
    bpy.context.scene.frame_set(frame)
    bpy.context.view_layer.update()


def setup_render(meshes,output):
    lo,hi=world_bounds(meshes)
    center=(lo+hi)/2
    extent=max((hi-lo).length,.25)
    camera_data=bpy.data.cameras.new('Volk inspection camera')
    camera=bpy.data.objects.new('Volk inspection camera',camera_data)
    bpy.context.scene.collection.objects.link(camera)
    camera.location=center+Vector((extent*.75,-extent*1.2,extent*.45))
    direction=center-camera.location
    camera.rotation_euler=direction.to_track_quat('-Z','Y').to_euler()
    camera_data.type='ORTHO'
    camera_data.ortho_scale=extent*1.3
    bpy.context.scene.camera=camera
    for name,offset,power in [('key',(1,-1,1),850),('fill',(-1,-.5,.4),500)]:
        light_data=bpy.data.lights.new(name,'AREA')
        light=bpy.data.objects.new(name,light_data)
        bpy.context.scene.collection.objects.link(light)
        light.location=center+Vector(offset)*extent
        light.rotation_euler=(center-light.location).to_track_quat('-Z','Y').to_euler()
        light_data.energy=power
        light_data.shape='DISK'
        light_data.size=extent*1.2
    scene=bpy.context.scene
    scene.render.engine='CYCLES'
    scene.cycles.samples=24
    scene.render.resolution_x=720
    scene.render.resolution_y=720
    scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG'
    scene.world.color=(.12,.12,.12)
    for name,frame in [('bind',0),('slide_move',4),('mag_seat',4)]:
        set_action(name if name!='bind' else '__none__',frame)
        scene.render.filepath=str(output/f'animated_inspection_{name}.png')
        bpy.ops.render.render(write_still=True)
    set_action('__none__',1)


def export_magazine(output):
    clip=bpy.data.objects['clip']
    names=['volk_magazine','magazine_machined_fasteners',
           'magazine_rib_lower','magazine_rib_middle','magazine_rib_upper']
    objects=[bpy.data.objects[n] for n in names]
    copies=[]
    for obj in objects:
        copy=obj.copy()
        copy.data=obj.data.copy()
        bpy.context.scene.collection.objects.link(copy)
        copy.parent=None
        copy.matrix_world=clip.matrix_world.inverted()@obj.matrix_world
        copy.name=obj.name+'_detached'
        copies.append(copy)
    bpy.ops.object.select_all(action='DESELECT')
    for obj in copies:
        obj.select_set(True)
    bpy.context.view_layer.objects.active=copies[0]
    bpy.ops.export_scene.gltf(filepath=str(output/'volk_s4_hd_magazine.glb'),
                              export_format='GLB',use_selection=True,
                              export_animations=False,export_tangents=True)
    for obj in copies:
        data=obj.data
        bpy.data.objects.remove(obj,do_unlink=True)
        bpy.data.meshes.remove(data)


def main():
    if '--' not in sys.argv or len(sys.argv[sys.argv.index('--')+1:])!=2:
        raise SystemExit('expected PACKAGE HANDOFF after --')
    package,output=map(Path,sys.argv[sys.argv.index('--')+1:])
    source=output/'volk_s4_hd_mechanical_untextured.glb'
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.gltf(filepath=str(source))
    rig=json.loads((package/'clean/rig_manifest.json').read_text())
    assembly=json.loads((output/'assembly_manifest.json').read_text())
    if rig['source_psk_sha256']!=assembly['source_psk_sha256']:
        raise ValueError('Blender input/source identity mismatch')
    for i,bone in enumerate(rig['bones']):
        if bpy.data.objects.get(bone['name']) is None:
            raise ValueError('missing imported source bone '+bone['name'])
    for name in ('mag_seat','slide_move','slide_return','mag_remove_reference',
                 'mag_insert_reference','mag_release_state'):
        if name not in bpy.data.actions:
            raise ValueError('missing imported source action '+name)
    maps=output/'maps'
    base=material('volk_s4_hd_pbr',maps/'volk_s4_hd_albedo_4k.png',
                  maps/'volk_s4_hd_roughness_4k.png',
                  maps/'volk_s4_hd_metallic_4k.png',
                  maps/'volk_s4_hd_normal_gltf_4k.png')
    detail=dark_metal()
    base_names={'volk_body','volk_magazine','volk_slide','volk_trigger','volk_reload_part_01'}
    meshes=[o for o in bpy.data.objects if o.type=='MESH']
    if len(meshes)!=11:
        raise ValueError('expected five source parts and six HD detail meshes')
    for obj in meshes:
        obj.data.materials.clear()
        obj.data.materials.append(base if obj.name in base_names else detail)
    scene=bpy.context.scene
    scene.render.film_transparent=False
    scene.render.resolution_percentage=100
    scene.unit_settings.system='METRIC'
    scene.render.fps=36
    # Actions are imported as muted NLA strips. Save them independently.
    set_action('__none__',1)
    bpy.ops.wm.save_as_mainfile(filepath=str(output/'volk_s4_hd_animated.blend'))
    setup_render(meshes,output)
    bpy.ops.export_scene.gltf(filepath=str(output/'volk_s4_hd_animated.glb'),
                              export_format='GLB',export_animations=True,
                              export_animation_mode='ACTIONS',
                              export_tangents=True,export_extras=True)
    export_magazine(output)
    report={'blender_version':bpy.app.version_string,
            'source_glb_sha256':digest(source),
            'blend_sha256':digest(output/'volk_s4_hd_animated.blend'),
            'animated_glb_sha256':digest(output/'volk_s4_hd_animated.glb'),
            'magazine_glb_sha256':digest(output/'volk_s4_hd_magazine.glb'),
            'mesh_objects':[o.name for o in meshes],
            'actions':sorted(a.name for a in bpy.data.actions),
            'material_maps':{p.name:digest(p) for p in sorted(maps.glob('*.png'))},
            'rendered_previews':['animated_inspection_bind.png',
                                 'animated_inspection_slide_move.png',
                                 'animated_inspection_mag_seat.png']}
    (output/'blender_validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print('VOLK_FINAL_PASS',json.dumps(report))


if __name__=='__main__':
    main()
