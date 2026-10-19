import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parents[1]))
from prepare_npc_name import patch_movie, repack_one_block
from prepare_context_cursor import pack_whole
from swf_title_patch import write_rect, split_tags
from test_swf_title_patch import make_movie


def fixture(variable=b'_root.name_NPC', font=340, character=184):
    field = (struct.pack('<H', character)+write_rect([-40, 1940, -40, 474], 12)+
             b'\x0d\x31'+struct.pack('<HH', 102, font)+bytes(4)+bytes(11)+variable+b'\0')
    # Only the ownership signature is needed by the patcher's conservative
    # check; synthetic sprite data carries no game geometry/text/scripts.
    owner = struct.pack('<HH', 185, 1)+b'\x26\x01\x01\x00\xb8\x00'+b'nameTxt\0'
    return make_movie([(37, field), (39, owner)])


class Tests(unittest.TestCase):
    def test_only_two_height_bytes_can_change(self):
        original = fixture()
        result, report = patch_movie(original)
        at = report['font_height_offset']
        self.assertEqual(len(original), len(result))
        self.assertEqual(result[:at], original[:at])
        self.assertEqual(result[at+2:], original[at+2:])
        self.assertEqual(struct.unpack_from('<H', result, at)[0], 204)

    def test_unknown_or_repeated_field_refuses(self):
        for movie in (fixture(font=204), fixture(character=185), fixture(variable=b'subtitles')):
            with self.assertRaises(ValueError):
                patch_movie(movie)
        movie = fixture()
        _, tags = split_tags(movie)
        fields = [(t['code'], movie[t['pay_off']:t['pay_off']+t['pay_len']])
                  for t in tags if t['code'] in (37, 39)]
        with self.assertRaises(ValueError):
            patch_movie(make_movie(fields+fields))

    def test_repack_keeps_unaffected_compressed_block_exact(self):
        logical = bytes(0x20000)+b'unchanged compressed second block'
        changed = bytearray(logical); changed[123:125] = b'ab'
        original = pack_whole(logical)
        output = repack_one_block(original, logical, bytes(changed))
        old_first, old_second = struct.unpack_from('<I', original, 16)[0], struct.unpack_from('<I', original, 24)[0]
        new_first, new_second = struct.unpack_from('<I', output, 16)[0], struct.unpack_from('<I', output, 24)[0]
        self.assertEqual(old_second, new_second)
        self.assertEqual(original[32+old_first:32+old_first+old_second],
                         output[32+new_first:32+new_first+new_second])
        with self.assertRaises(ValueError):
            repack_one_block(original, logical, logical)


if __name__ == '__main__':
    unittest.main()
