"""Identically lit source/HD Seeker Blender renders for private visual review."""
import sys
from pathlib import Path

import bpy
from mathutils import Vector


def setup(path, diffuse=None):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(path))
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE'
    scene.render.resolution_x = 1600
    scene.render.resolution_y = 900
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.film_transparent = False
    scene.world = bpy.data.worlds.new('studio_world')
    scene.world.color = (.12, .15, .17)
    if diffuse:
        image = bpy.data.images.load(str(diffuse), check_existing=True)
        for material in bpy.data.materials:
            if material.name != 'e99_steerable_body':
                continue
            material.use_nodes = True
            tree = material.node_tree
            bsdf = next(n for n in tree.nodes if n.type == 'BSDF_PRINCIPLED')
            tex = tree.nodes.new('ShaderNodeTexImage')
            tex.image = image
            tree.links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
            bsdf.inputs['Metallic'].default_value = .25
            bsdf.inputs['Roughness'].default_value = .58
    bpy.context.view_layer.update()
    corners = [o.matrix_world @ Vector(c) for o in scene.objects if o.type == 'MESH' for c in o.bound_box]
    lo = Vector(tuple(min(v[i] for v in corners) for i in range(3)))
    hi = Vector(tuple(max(v[i] for v in corners) for i in range(3)))
    center = (lo + hi) / 2
    # Identical framing is established by the source rig, then reused by HD.
    return scene, center, (hi - lo)


def camera_light(scene, center, dimensions, view):
    bpy.ops.object.camera_add(location=center + (Vector((3.5, .2, 0)) if view == 'side' else Vector((2.7, -1.9, -1.3))))
    camera = bpy.context.object
    camera.rotation_euler = (center - camera.location).to_track_quat('-Z', 'Y').to_euler()
    camera.data.type = 'ORTHO'
    camera.data.ortho_scale = max(dimensions.z * 1.18, dimensions.y * 2.1, .25)
    scene.camera = camera
    for location, energy, size in ((Vector((1.5, -2, 1.8)), 750, 2.2),
                                   (Vector((-1.1, 1.1, -.8)), 500, 1.5)):
        bpy.ops.object.light_add(type='AREA', location=center + location)
        light = bpy.context.object
        light.data.energy = energy
        light.data.shape = 'DISK'
        light.data.size = size
        light.rotation_euler = (center - light.location).to_track_quat('-Z', 'Y').to_euler()


def main(root):
    source = root / 'source_glb'
    previews = root / 'previews'
    previews.mkdir(exist_ok=True)
    original = source / 'seeker_source_animated.glb'
    hd = source / 'seeker_hd_animated.glb'
    diffuse = root / 'source/ar_steerable_d.png'
    for view in ('side', 'perspective'):
        for label, path, albedo in (('original', original, diffuse), ('hd', hd, None)):
            scene, center, dimensions = setup(path, albedo)
            camera_light(scene, center, dimensions, view)
            scene.render.filepath = str(previews / f'{label}_{view}_blender.png')
            bpy.ops.render.render(write_still=True)
            print('RENDERED', label, view, scene.render.filepath)
    for label, path, albedo in (('original', source/'seeker_rear_magazine.glb', diffuse),
                                ('hd', source/'seeker_hd_rear_magazine.glb', None)):
        scene, center, dimensions = setup(path, albedo)
        camera_light(scene, center, dimensions, 'perspective')
        scene.render.filepath = str(previews / f'{label}_detached_magazine_blender.png')
        bpy.ops.render.render(write_still=True)
        print('RENDERED', label, 'detached magazine', scene.render.filepath)


if __name__ == '__main__':
    argv = sys.argv[sys.argv.index('--') + 1:]
    main(Path(argv[0]).resolve())
