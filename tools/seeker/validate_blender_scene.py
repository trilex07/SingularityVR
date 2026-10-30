"""Run in Blender background mode against the private Seeker handoff."""
import json
import sys
from pathlib import Path

import bpy


def inspect(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(path))
    meshes = [o for o in bpy.data.objects if o.type == 'MESH']
    names = {o.name for o in bpy.data.objects}
    return {
        'meshes': len(meshes),
        'triangles': sum(len(o.data.polygons) for o in meshes),
        'clip': 'clip' in names,
        'root': 'cog_e99_steerable' in names,
        'actions': sorted(a.name for a in bpy.data.actions),
        'images': {i.name: list(i.size) for i in bpy.data.images if i.size[0]},
    }


def main(root):
    source = root / 'source_glb'
    clean = root / 'clean'
    clean.mkdir(exist_ok=True)
    result = {}
    for stem in ('seeker_hd_animated', 'seeker_hd_rear_magazine'):
        original = inspect(source / (stem + '.glb'))
        if not original['clip'] or original['triangles'] <= 0:
            raise ValueError(stem + ': missing magazine or mesh')
        if stem == 'seeker_hd_animated' and not original['actions']:
            raise ValueError('source reload animation absent')
        if any(size != [4096, 2048] for size in original['images'].values()):
            raise ValueError(stem + ': non-4K embedded image')
        bpy.ops.wm.save_as_mainfile(filepath=str(clean / (stem + '.blend')))
        exported = clean / (stem + '_roundtrip.glb')
        bpy.ops.export_scene.gltf(filepath=str(exported), export_format='GLB', export_animations=True)
        imported = inspect(exported)
        if (imported['triangles'] != original['triangles'] or
                imported['clip'] != original['clip'] or
                bool(imported['actions']) != bool(original['actions'])):
            raise ValueError(stem + ': export/reimport mismatch')
        result[stem] = {'import': original, 'roundtrip': imported}
    (root / 'blender_validation.json').write_text(json.dumps(result, indent=2) + '\n')
    print('OFFLINE_BLENDER_PASS', json.dumps(result))


if __name__ == '__main__':
    argv = sys.argv[sys.argv.index('--') + 1:]
    main(Path(argv[0]).resolve())
