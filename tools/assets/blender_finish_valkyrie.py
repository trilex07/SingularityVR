"""Run in Blender 5.2: finish Valkyrie HD scene, animated GLB and detached mag.

Usage: blender --background --factory-startup --python this.py -- VISUAL FINAL
"""
import hashlib
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def image(path,space):
    im=bpy.data.images.load(str(path),check_existing=True)
    im.colorspace_settings.name=space;im.pack();return im


def pbr(name,albedo,roughness,metallic,normal):
    mat=bpy.data.materials.new(name);mat.use_nodes=True
    nodes=mat.node_tree.nodes;nodes.clear()
    out=nodes.new('ShaderNodeOutputMaterial');bsdf=nodes.new('ShaderNodeBsdfPrincipled')
    mat.node_tree.links.new(bsdf.outputs['BSDF'],out.inputs['Surface'])
    for path,space,socket in ((albedo,'sRGB','Base Color'),
                              (roughness,'Non-Color','Roughness'),
                              (metallic,'Non-Color','Metallic')):
        tex=nodes.new('ShaderNodeTexImage');tex.image=image(path,space)
        mat.node_tree.links.new(tex.outputs['Color'],bsdf.inputs[socket])
    tex=nodes.new('ShaderNodeTexImage');tex.image=image(normal,'Non-Color')
    normal_node=nodes.new('ShaderNodeNormalMap');normal_node.space='TANGENT'
    mat.node_tree.links.new(tex.outputs['Color'],normal_node.inputs['Color'])
    mat.node_tree.links.new(normal_node.outputs['Normal'],bsdf.inputs['Normal'])
    return mat


def detail_material():
    m=bpy.data.materials.new('valkyrie_machined_dark_metal');m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value=(.085,.09,.085,1)
    p.inputs['Metallic'].default_value=.58
    p.inputs['Roughness'].default_value=.41
    return m


def camera_lights(meshes):
    bounds=[o.matrix_world@Vector(c) for o in meshes for c in o.bound_box]
    lo=Vector(tuple(min(p[i] for p in bounds) for i in range(3)))
    hi=Vector(tuple(max(p[i] for p in bounds) for i in range(3)))
    center=(lo+hi)/2;extent=max((hi-lo).length,.25)
    cam_data=bpy.data.cameras.new('Valkyrie inspection camera')
    cam=bpy.data.objects.new('Valkyrie inspection camera',cam_data)
    bpy.context.scene.collection.objects.link(cam)
    cam.location=center+Vector((extent*.8,-extent*1.25,extent*.5))
    cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
    cam_data.type='ORTHO';cam_data.ortho_scale=extent*1.25
    bpy.context.scene.camera=cam
    for name,offset,power in [('key',(1,-1,1),900),('fill',(-1,-.4,.4),550)]:
        light_data=bpy.data.lights.new(name,'AREA');light=bpy.data.objects.new(name,light_data)
        bpy.context.scene.collection.objects.link(light)
        light.location=center+Vector(offset)*extent
        light.rotation_euler=(center-light.location).to_track_quat('-Z','Y').to_euler()
        light_data.energy=power;light_data.shape='DISK';light_data.size=extent*1.1
    scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=32
    scene.render.resolution_x=960;scene.render.resolution_y=720
    scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG'
    scene.world.color=(.12,.12,.12);scene.render.film_transparent=False
    return center,extent


def render(path):
    bpy.context.scene.render.filepath=str(path);bpy.ops.render.render(write_still=True)


def export_magazine(output):
    clip=bpy.data.objects['clip']
    source=[o for o in bpy.data.objects if o.type=='MESH' and o.parent==clip]
    if len(source)!=2 or {o.name for o in source}!={'magazine','magazine_hd_ribs'}:
        raise ValueError('magazine source/detail binding mismatch')
    copies=[]
    for obj in source:
        copy=obj.copy();copy.data=obj.data.copy()
        bpy.context.scene.collection.objects.link(copy)
        copy.parent=None;copy.matrix_world=clip.matrix_world.inverted()@obj.matrix_world
        copy.name=obj.name+'_detached';copies.append(copy)
    bpy.ops.object.select_all(action='DESELECT')
    for obj in copies:obj.select_set(True)
    bpy.context.view_layer.objects.active=copies[0]
    bpy.ops.export_scene.gltf(filepath=str(output/'valkyrie_ar9_hd_magazine.glb'),
                              export_format='GLB',use_selection=True,
                              export_animations=False,export_tangents=True)
    for obj in copies:
        mesh=obj.data;bpy.data.objects.remove(obj,do_unlink=True);bpy.data.meshes.remove(mesh)


def main():
    if '--' not in sys.argv or len(sys.argv[sys.argv.index('--')+1:])!=2:
        raise SystemExit('expected VISUAL FINAL after --')
    visual,output=map(Path,sys.argv[sys.argv.index('--')+1:])
    assembled=output/'valkyrie_hd_mechanical_untextured.glb'
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.gltf(filepath=str(assembled))
    rig=json.loads((visual/'clean/rig_manifest.json').read_text())
    assembly=json.loads((output/'visual_assembly_manifest.json').read_text())
    if rig['source_psk_sha256']!=assembly['source_psk_sha256']:
        raise ValueError('PSK mismatch')
    for bone in rig['bones']:
        if bpy.data.objects.get(bone['name']) is None:raise ValueError('missing bone '+bone['name'])
    meshes=[o for o in bpy.data.objects if o.type=='MESH']
    if len(meshes)!=9:raise ValueError('expected 6 source and 3 additive meshes')
    expected={'weapon_body','magazine','trigger','moving_reload_part',
              'fan.001','clip_release.001','magazine_hd_ribs',
              'weapon_body_hd_fasteners','fan_hd_hub'}
    if {o.name for o in meshes}!=expected:raise ValueError('mesh labels changed')
    maps=visual/'maps'
    hd=pbr('valkyrie_ar9_hd_pbr',maps/'valkyrie_hd_albedo_4k.png',
           maps/'valkyrie_hd_roughness_4k.png',maps/'valkyrie_hd_metallic_4k.png',
           maps/'valkyrie_hd_normal_gltf_4k.png')
    baseline=pbr('valkyrie_ar9_baseline_pbr',maps/'valkyrie_baseline_albedo_4k.png',
                 maps/'valkyrie_hd_roughness_4k.png',maps/'valkyrie_hd_metallic_4k.png',
                 maps/'valkyrie_hd_normal_gltf_4k.png')
    metal=detail_material();detail_names={'magazine_hd_ribs','weapon_body_hd_fasteners','fan_hd_hub'}
    for obj in meshes:
        obj.data.materials.clear();obj.data.materials.append(metal if obj.name in detail_names else hd)
    bpy.context.scene.render.fps=24
    bpy.context.scene.frame_set(1)
    center,extent=camera_lights(meshes)
    # Fixed camera/lights/exposure. Baseline differs only by existing albedo and
    # visibility of the additive HD details.
    for obj in meshes:
        if obj.name in detail_names:obj.hide_render=True
        else:obj.data.materials[0]=baseline
    render(output/'original_bind.png')
    for obj in meshes:
        if obj.name in detail_names:obj.hide_render=False
        else:obj.data.materials[0]=hd
    render(output/'hd_bind.png')
    bpy.context.scene.frame_set(22);render(output/'hd_reload_frame22.png')
    bpy.context.scene.frame_set(1)
    bpy.context.preferences.filepaths.save_version=0
    bpy.ops.wm.save_as_mainfile(filepath=str(output/'valkyrie_ar9_hd_animated.blend'))
    bpy.ops.export_scene.gltf(filepath=str(output/'valkyrie_ar9_hd_animated.glb'),
                              export_format='GLB',export_animations=True,
                              export_animation_mode='ACTIONS',export_tangents=True,
                              export_extras=True)
    export_magazine(output)
    magazine_parts={'magazine','magazine_hd_ribs'}
    for obj in meshes:obj.hide_render=obj.name not in magazine_parts
    mag_bounds=[o.matrix_world@Vector(c) for o in meshes if o.name in magazine_parts for c in o.bound_box]
    mag_lo=Vector(tuple(min(p[i] for p in mag_bounds) for i in range(3)))
    mag_hi=Vector(tuple(max(p[i] for p in mag_bounds) for i in range(3)))
    mag_center=(mag_lo+mag_hi)/2
    cam=bpy.context.scene.camera
    cam.location=mag_center+Vector((.35,-.48,.25))
    cam.rotation_euler=(mag_center-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.ortho_scale=max((mag_hi-mag_lo).length*1.5,.25)
    render(output/'hd_magazine_inspection.png')
    report={'blender_version':bpy.app.version_string,'source_glb_sha256':digest(assembled),
            'blend_sha256':digest(output/'valkyrie_ar9_hd_animated.blend'),
            'animated_glb_sha256':digest(output/'valkyrie_ar9_hd_animated.glb'),
            'magazine_glb_sha256':digest(output/'valkyrie_ar9_hd_magazine.glb'),
            'mesh_objects':sorted(o.name for o in meshes),
            'actions':sorted(a.name for a in bpy.data.actions),
            'rendered_previews':['original_bind.png','hd_bind.png','hd_reload_frame22.png',
                                 'hd_magazine_inspection.png']}
    (output/'blender_visual_validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('VALKYRIE_BLENDER_PASS',json.dumps(report))


if __name__=='__main__':main()
