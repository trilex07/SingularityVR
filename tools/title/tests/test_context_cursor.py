import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1]))
sys.path.insert(0, str(Path(__file__).parents[2]/'fonts'))
from prepare_context_cursor import core_shape, patch_movie, sha
from swf_shape import decode_shape
from swf_title_patch import read_rect, split_tags
from test_swf_ui_polish import fake_cursor


class Tests(unittest.TestCase):
    def test_fixed_length_core_is_closed_centred_and_parseable(self):
        for size in (32, 248):
            result = core_shape(17, size)
            self.assertEqual(len(result), size)
            self.assertEqual(struct.unpack_from('<H', result)[0], 17)
            _, bounds, after = read_rect(result, 2)
            self.assertEqual(bounds, [-48, 48, -48, 48])
            self.assertEqual(result[after:after+6], bytes((1, 0, 245, 245, 245, 0)))
            data = result[after+6:]; parsed = decode_shape(data)
            x = y = 0; edges = []
            for kind, *detail in parsed['records']:
                if kind == 'style' and 'move' in detail[0]:
                    _, x, y = detail[0]['move']
                elif kind == 'line':
                    _, dx, dy = detail[0]; edges.append(((x, y), (x+dx, y+dy))); x += dx; y += dy
                elif kind not in ('end', 'style'):
                    self.fail('unexpected edge record')
            self.assertEqual(len(edges), 4)
            self.assertEqual(edges[0][0], edges[-1][1])
            self.assertEqual({p for edge in edges for p in edge}, {(40, 0), (0, 40), (-40, 0), (0, -40)})
            self.assertLess(len(data)*8-parsed['end_bit'], 8)
    def test_movie_actions_sprite_and_offsets_preserved(self):
        before = fake_cursor(b'never edit actions')
        allowed = {sha(bytes(30))}
        after, report = patch_movie(before, allowed)
        self.assertEqual(len(before), len(after))
        self.assertEqual(split_tags(before), split_tags(after))
        start = report['offset']; end = start+report['size']
        self.assertEqual(before[:start], after[:start])
        self.assertEqual(before[end:], after[end:])
    def test_unknown_and_shared_geometry_refuse(self):
        with self.assertRaises(ValueError):
            patch_movie(fake_cursor())
        with self.assertRaises(ValueError):
            patch_movie(fake_cursor(bytes.fromhex('060100010000')), {sha(bytes(30))})


if __name__ == '__main__':
    unittest.main()
