"""Source-key and hierarchy checks for the offline Volk mechanical GLB."""
import json
import struct
import tempfile
import unittest
from pathlib import Path

import numpy as np

from build_volk_mechanical_glb import SEGMENTS, build, load_cache
from prepare_centurion import read_psk
from prepare_weapon_animation import key_local


class VolkMechanicalExport(unittest.TestCase):
    def test_source_tracks_and_original_parent_chain(self):
        root = Path(__file__).resolve().parents[2]
        psk = root.parent/'research/native-hands-20261007/assets/RvGameMPLoader_SF/SkeletalMesh3/ch_inview_shotgun_mesh.psk'
        svwa = root.parent/'dependencies/Singularity/Binaries/vr-assets/reload/volk/volk_reload.svwa'
        if not psk.exists() or not svwa.exists():
            self.skipTest('private source PSK/SVWA unavailable')
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp)
            first = build(psk, svwa, out)
            data = (out/'volk_mechanical_rig.glb').read_bytes()
            second = build(psk, svwa, out)
            self.assertEqual(first['glb_sha256'], second['glb_sha256'])
            length = struct.unpack_from('<I', data, 12)[0]
            doc = json.loads(data[20:20+length])
            binary = data[28+length:]
            names, parents = read_psk(psk)[:2]
            self.assertEqual([n['name'] for n in doc['nodes'][:79]], names)
            self.assertEqual(doc['nodes'][53]['children'][:5], [54,55,56,57,58])
            self.assertEqual(doc['nodes'][55]['children'], [81])
            self.assertEqual([m['name'] for m in doc['meshes']],
                             ['volk_body','volk_reload_part_01','volk_magazine','volk_slide','volk_trigger'])
            self.assertEqual(sum(p['triangles'] for p in first['parts'].values()),10835)
            self.assertEqual([a['name'] for a in doc['animations']], list(SEGMENTS))
            _, keys = load_cache(svwa)
            for anim in doc['animations']:
                bone, first_frame, last_frame = SEGMENTS[anim['name']]
                self.assertEqual({c['target']['node'] for c in anim['channels']}, {bone})
                for channel in anim['channels']:
                    if channel['target']['path'] != 'translation':
                        continue
                    accessor = doc['accessors'][anim['samplers'][channel['sampler']]['output']]
                    view = doc['bufferViews'][accessor['bufferView']]
                    actual = np.frombuffer(binary, '<f4', accessor['count']*3,
                                           view['byteOffset']).reshape(-1,3)
                    track = {54:0,55:1,56:2,57:3}[bone]
                    expected = np.array([key_local(keys[f,track])[3,:3]/52.5
                                         for f in range(first_frame,last_frame+1)])
                    np.testing.assert_allclose(actual, expected, atol=1e-6, rtol=0)
            self.assertEqual(parents[55],53)
            self.assertEqual(doc['scenes'][0]['nodes'],[0])


if __name__ == '__main__':
    unittest.main()
