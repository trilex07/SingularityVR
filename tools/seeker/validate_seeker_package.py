"""Offline structural gates for private Seeker art; no game process access."""
import argparse
import json
from pathlib import Path
import struct

import numpy as np
from PIL import Image
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'textures'))
from dds_io import read_dds_header


def glb(path):
    data = path.read_bytes()
    magic, version, length = struct.unpack_from('<4sII', data)
    if magic != b'glTF' or version != 2 or length != len(data):
        raise ValueError('bad GLB header')
    size, tag = struct.unpack_from('<I4s', data, 12)
    if tag != b'JSON':
        raise ValueError('missing GLB JSON')
    return json.loads(data[20:20+size])


def validate(root):
    rig = json.loads((root/'source_glb/source_rig_manifest.json').read_text())
    weapon = glb(root/'source_glb/seeker_source_animated.glb')
    magazine = glb(root/'source_glb/seeker_rear_magazine.glb')
    hd_weapon = glb(root/'source_glb/seeker_hd_animated.glb')
    hd_magazine = glb(root/'source_glb/seeker_hd_rear_magazine.glb')
    assert len(weapon['nodes']) >= 85 and [n['name'] for n in weapon['nodes'][:85]] == [n['name'] for n in magazine['nodes'][:85]]
    assert weapon['nodes'][53]['name'] == 'cog_e99_steerable'
    assert weapon['nodes'][57]['name'] == 'clip' and magazine['scenes'][0]['nodes'] == [57]
    assert weapon['animations'][0]['name'] == 'source_Reload_reference'
    assert all(c['target']['node'] != 53 for c in weapon['animations'][0]['channels'])
    assert rig['frames'] == 54 and rig['fps'] == 24 and rig['magazine_triangles'] == 410
    assert sum(x['triangles'] for x in rig['parts'].values()) == 14996
    assert len(hd_weapon['meshes']) == len(weapon['meshes']) == 10
    assert len(hd_magazine['meshes']) == len(magazine['meshes']) == 4
    assert len(hd_weapon['images']) == len(hd_magazine['images']) == 3
    assert hd_weapon['materials'][0]['pbrMetallicRoughness']['metallicRoughnessTexture']['index'] == 2
    pbr = json.loads((root/'textures/pbr_manifest.json').read_text())
    for kind in ('albedo', 'normal_dx', 'roughness', 'metallic'):
        path = root/'textures'/('seeker_'+kind+'_4k.png')
        assert Image.open(path).size == (4096, 2048)
    normal = np.asarray(Image.open(root/'textures/seeker_normal_dx_4k.png'))[::8, ::8]
    vector = (normal.astype(float)/255)*2-1
    assert np.max(np.abs(np.linalg.norm(vector,axis=2)-1)) < .025
    assert np.percentile(normal[:,:,2],1) > 170
    atlas = Image.open(root/'icon/katorga-hud-atlas.png').convert('RGBA')
    source = Image.open(root/'icon/source-katorga-hud-atlas.png').convert('RGBA')
    a, b = np.asarray(atlas), np.asarray(source)
    mask = np.ones((256,512),bool);mask[128:192,256:320]=False
    assert np.array_equal(a[mask],b[mask])
    icon = Image.open(root/'icon/weapon_icon_seeker.png').convert('RGBA')
    assert icon.size == (64,64) and np.count_nonzero(np.asarray(icon)[:,:,3]>32)>250
    assert atlas.getpixel((256,128)) == icon.getpixel((0,0))
    cache = json.loads((root/'runtime/animation-report.json').read_text())
    for name in ('seeker_reload.svwa', 'seeker_clip.svrk', 'seeker_clip.svrt'):
        assert (root/'runtime'/name).is_file()
    assert cache['frames'] == 54 if 'frames' in cache else cache['source_frames'] == 54
    blender = json.loads((root/'blender_validation.json').read_text())
    for stem in ('seeker_hd_animated', 'seeker_hd_rear_magazine'):
        assert blender[stem]['import']['triangles'] == blender[stem]['roundtrip']['triangles']
        assert (root/'clean'/(stem+'.blend')).is_file()
        assert (root/'clean'/(stem+'_roundtrip.glb')).is_file()
    for stem in ('original_side', 'hd_side', 'original_perspective', 'hd_perspective',
                 'original_detached_magazine', 'hd_detached_magazine'):
        assert Image.open(root/'previews'/(stem+'_blender.png')).size == (1600, 900)
    for name in ('AR_Steerable_d.dds', 'AR_Steerable_n.dds'):
        header = read_dds_header(root/'native_textures'/name)
        assert header['width'] == 4096 and header['height'] == 2048
        assert header['fourcc'] == 'DXT5' and header['mip_count'] > 1
    return {'rig': 'PASS', 'source_animation': 'PASS', 'detached_magazine': 'PASS',
            'pbr_maps': 'PASS', 'icon_atlas': 'PASS', 'native_ammo': 'NOT VERIFIED',
            'blender_roundtrip': 'PASS', 'hd_visual_acceptance': 'OFFLINE PASS',
            'native_dds': 'OFFLINE PASS', 'native_hd_material_binding': 'NOT VERIFIED',
            'Steam_PSVR2': 'NOT RUN'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True)
    a = parser.parse_args()
    print(json.dumps(validate(a.package), indent=2))
