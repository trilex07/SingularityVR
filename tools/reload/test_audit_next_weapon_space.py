"""Synthetic rejection tests; real cooked geometry is a separate private fixture."""
import copy
import struct
import unittest
from unittest.mock import patch
import numpy as np
import audit_next_weapon_space as a


def fixture():
    points = np.array([[1., -2., 3.], [4., -5., 6.], [7., -8., 9.]])
    bind = np.eye(4)[None].copy(); bind[0, 3, 1] = -5
    psk = (['clip'], [0], bind, bind, points, {i: [(1., 0)] for i in range(3)},
           [(0, 0., 0.), (1, 1., 0.), (2, 0., 1.)], [(0, 1, 2, 0, 0, 0)])
    data = bytearray(128+3*32)
    struct.pack_into('<3H', data, 22, 1, 0, 2)
    for i, p in enumerate(points):
        struct.pack_into('<3f', data, 128+i*32, *(p*[1, -1, 1]))
        data[128+i*32+24] = 255
    spec = dict(vertices=3, triangles=[1])
    layout = dict(offset=0, index_count=3, index_fnv=1,
                  chunks=[dict(first=0, vertices=3, palette=[0])])
    return psk, data, spec, layout


class Tests(unittest.TestCase):
    def run_audit(self, psk, data, spec, layout):
        with patch.dict(a.SPECS, {'valkyrie': spec}), patch.object(a, 'cooked_layout', return_value=layout):
            return a.audit('valkyrie', psk, data, 0, len(data))

    def test_complete_stream_and_original_winding(self):
        psk, data, spec, layout = fixture()
        binary, report = self.run_audit(psk, data, spec, layout)
        self.assertEqual(binary, b'SVNV'+struct.pack('<3I', 1, 3, 3)+data[128:]+data[22:28])
        self.assertEqual(report['rigid_clip_wedges'], 3)
        self.assertEqual(report['wrong_bind_origin_distance_uu'], 10)

    def test_corrupt_position_weight_bone_or_winding_rejected(self):
        psk, data, spec, layout = fixture()
        for offset, value in ((128+32+2, 0), (128+24, 254), (128+20, 1), (22, 2)):
            bad = bytearray(data); bad[offset] = value
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                self.run_audit(psk, bad, spec, layout)

    def test_ambiguous_stream_and_topology_rejected(self):
        psk, data, spec, layout = fixture()
        for bad, shape in ((data+data[128:], spec), (data, dict(spec, vertices=4))):
            with self.assertRaises(ValueError): self.run_audit(psk, bad, shape, layout)

    def test_source_weight_disagreement_rejected(self):
        psk, data, spec, layout = fixture(); psk = copy.deepcopy(psk)
        psk[5][1] = [(1., 1)]
        with self.assertRaises(ValueError): self.run_audit(psk, data, spec, layout)


if __name__ == '__main__': unittest.main()
