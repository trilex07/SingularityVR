import struct
import unittest
import numpy as np
from prepare_cylinder_animation import cooked_section, fnv, prepare


def fixture():
    names = ['bone'+str(i) for i in range(59)]
    for i, n in ((53, 'cog_alt_pistol'), (55, 'Trigger'), (56, 'hammer'), (57, 'clip'), (58, 'clip_release')):
        names[i] = n
    parents = [0]*59
    parents[57] = parents[58] = 53
    mats = np.repeat(np.eye(4)[None], 59, axis=0)
    palette = [57, 53, 56, 55, 58]
    psk = (names, parents, mats, mats, np.zeros((5, 3)),
           {i: [(1., b)] for i, b in enumerate(palette)}, [(0, 0, 0)]*5799, [(0, 1, 2, 0)]*6767)
    def array(fmt, data):
        return struct.pack('<i', len(data))+struct.pack('<'+fmt*len(data), *data)
    cooked = struct.pack('<ihhiHi', 1, 0, 0, 0, 6767, 2)+array('H', [0, 1, 2]*6767)
    cooked += array('H', [])+array('H', [])+array('B', [])+struct.pack('<4i', 1, 0, 0, 0)
    cooked += array('H', palette)+struct.pack('<3i', 5799, 0, 1)
    def chunk(name, size, count, raw=b''):
        return struct.pack('<20siii', name.encode(), 20100422, size, count)+raw
    bones = b''.join(n.encode().ljust(120, b'\0') for n in ['clip', 'clip_release'])
    info = b'Reload'.ljust(64, b'\0')+b'None'.ljust(64, b'\0')
    info += struct.pack('<4i3f3i', 2, 0, 0, 106, 0, 53, 24, 0, 0, 53)
    keys = np.zeros((53, 2, 8), dtype='<f4'); keys[:, :, 6:] = 1
    psa = chunk('ANIMHEAD', 0, 0)+chunk('BONENAMES', 120, 2, bones)
    psa += chunk('ANIMINFO', 168, 1, info)+chunk('ANIMKEYS', 32, 106, keys.tobytes())
    return psk, psa, cooked


class Tests(unittest.TestCase):
    def test_determinism_and_slot_identity(self):
        psk, psa, cooked = fixture()
        data, report = prepare(psk, psa, cooked)
        self.assertEqual((data, report), prepare(psk, psa, cooked))
        self.assertEqual(len(data), 3208)
        self.assertEqual((report['clip_slot'], report['parent_slot'], report['release_slot']), (0, 1, 4))
        self.assertEqual(report['palette'][0], 57)  # PSK 57 is NOT GPU slot57

    def test_unknown_palette_and_topology_refused(self):
        psk, psa, cooked = fixture()
        for bad in (cooked+cooked, cooked.replace(struct.pack('<5H', 57, 53, 56, 55, 58), struct.pack('<5H', 53, 57, 56, 55, 58))):
            with self.assertRaises(ValueError):
                prepare(psk, psa, bad)
        bad = list(psk); bad[1] = list(psk[1]); bad[1][57] = 0
        with self.assertRaises(ValueError):
            prepare(bad, psa, cooked)

    def test_index_content_not_dimension_identity(self):
        _, _, cooked = fixture()
        _, a = cooked_section(cooked, 6767)
        changed = bytearray(cooked); changed[22] ^= 1
        _, b = cooked_section(changed, 6767)
        self.assertEqual(len(a), len(b))
        self.assertNotEqual(fnv(a), fnv(b))


if __name__ == '__main__':
    unittest.main()
