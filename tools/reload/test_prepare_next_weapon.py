import struct
import unittest

import numpy as np

from prepare_next_weapon import SPECS, prepare


def fixture(kind, move=5, sequence='Reload'):
    spec = SPECS[kind]
    names = ['r_arm_wrist', spec['parent'], 'clip']
    bind = np.repeat(np.eye(4)[None], 3, axis=0)
    psk = (names, [0, 0, 1], bind, bind,
           np.array([[0,0,0],[1,0,0],[0,1,0]], dtype=float),
           {i:[(1.,2)] for i in range(3)},
           [(0,0.,0.),(1,1.,0.),(2,0.,1.)] + [(0,0.,0.)]*(spec['wedges']-3),
           [(0,1,2,0,0,0)])
    def chunk(name, size, count, raw=b''):
        return struct.pack('<20siii', name.encode(), 20100422, size, count)+raw
    # PSA order != PSK order; names, not indices, identify the clip.
    bones = b''.join(n.encode().ljust(120,b'\0') for n in [spec['parent'], 'clip'])
    info = sequence.encode().ljust(64,b'\0')+b'None'.ljust(64,b'\0')
    info += struct.pack('<4i3f3i',2,0,0,6,0,3,30,0,0,3)
    keys = np.zeros((3,2,8),dtype='<f4');keys[:,:,6:]=1;keys[1,1,0]=move
    psa = (chunk('ANIMHEAD',0,0)+chunk('BONENAMES',120,2,bones)+
           chunk('ANIMINFO',168,1,info)+chunk('ANIMKEYS',32,6,keys.tobytes()))
    return psk, psa


class Tests(unittest.TestCase):
    def test_three_profiles_reuse_validators_deterministically(self):
        for kind in SPECS:
            psk, psa = fixture(kind)
            cache, report = prepare(kind, psk, psa)
            self.assertEqual((cache,report), prepare(kind,psk,psa))
            self.assertEqual(struct.unpack_from('<4sIII',cache),(b'SVRK',2,3,3))
            self.assertEqual(report['part']['crossing_triangles'],0)
            self.assertEqual(report['animation']['clip_psa_track'],1)
            self.assertEqual(report['animation']['clip_psk_bone'],2)
            self.assertIn('NOT WIRED',report['runtime'])
            self.assertEqual(report['animation']['sequences'][0]['max_clip_local_travel_uu'],5)

    def test_profile_and_geometry_mismatch_rejected(self):
        psk, psa = fixture('valkyrie')
        for kind in ('centurion','unknown','volk','kasimov'):
            with self.assertRaises(ValueError):
                prepare(kind,psk,psa)

    def test_parent_identity_and_duplicate_clip_rejected(self):
        psk, psa = fixture('volk')
        for names in (['r_arm_wrist','wrong','clip'],['clip','cog_shotgun','clip']):
            changed = list(psk);changed[0]=names
            with self.assertRaises(ValueError):
                prepare('volk',changed,psa)

    def test_reload_absent_or_immobile_rejected(self):
        for args in (dict(sequence='idle'),dict(move=0)):
            with self.assertRaises(ValueError):
                prepare('kasimov',*fixture('kasimov',**args))

    def test_mixed_skin_and_animation_truncation_rejected(self):
        psk, psa = fixture('valkyrie')
        changed = list(psk);changed[5]=dict(psk[5]);changed[5][0]=[(.5,0),(.5,2)]
        with self.assertRaises(ValueError):
            prepare('valkyrie',changed,psa)
        with self.assertRaises(ValueError):
            prepare('valkyrie',psk,psa[:-1])

    def test_nonfinite_and_bad_units_rejected(self):
        for units in (0,float('nan'),float('inf'),10001):
            with self.assertRaises(ValueError):
                prepare('volk',*fixture('volk'),units=units)
        with self.assertRaises(ValueError):
            prepare('volk',*fixture('volk',move=float('nan')))


if __name__ == '__main__':
    unittest.main()
