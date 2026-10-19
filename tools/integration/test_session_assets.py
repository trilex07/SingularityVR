import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import wave

from prepare_session_assets import dds_info, leaf, normalized_pcm
from assemble_session_assets import merge_story
from cook_session_audio import canonical_decode
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'audio'))
from patch_note_manifest import patch


class SessionAssets(unittest.TestCase):
    def test_independent_decoder_normalizes_only_summary_compression_fields(self):
        source = bytes(range(256))
        result = canonical_decode(source)
        self.assertEqual(result[:0x61], source[:0x61])
        self.assertEqual(result[0x61:0x69], b'\0'*8)
        self.assertEqual(result[0x69:], source[0x69:])

    def test_native_ownership_only_for_successful_campaign_waves(self):
        original = ''.join(json.dumps(r)+'\n' for r in [
            dict(package='SP_start_LOC_INT', asset='replaced', text_en='Old ASR', text_pl='old', file='old.wav'),
            dict(package='SP_start_LOC_INT', asset='retained', text_en='Retained', text_pl='zostaje', native_replaced='1'),
            dict(package='MP_other_LOC_INT', asset='unchanged', text_pl='MP unchanged')])
        lines = [dict(asset='replaced', english='Actual native', polish='Nowe', pack='story', file='new.wav'),
                 dict(asset='added', english='Native bark', polish='Krótko', pack='barks', file='not-story.wav')]
        result = merge_story(original, [dict(package='sp_start_loc_int', lines=lines)])
        rows = [json.loads(x) for x in result.splitlines()]
        self.assertEqual(rows[0]['text_en'], 'Actual native')
        self.assertEqual(rows[0]['file'], 'new.wav')
        self.assertEqual(rows[0]['native_replaced'], '1')
        self.assertEqual(rows[-1]['file'], '')
        self.assertEqual(result.splitlines()[1:3], original.splitlines()[1:3])
        with self.assertRaises(ValueError):
            merge_story(original, [dict(package='MP_other_LOC_INT', lines=lines)])
        with self.assertRaises(ValueError):
            merge_story(original, [dict(package='sp_start_loc_int', lines=lines+lines)])

    def test_path_guards(self):
        for name in ('../a.wav', 'C:a.wav', 'a/b.wav', 'a\\b.wav', '..', 'a:stream'):
            with self.assertRaises(ValueError):
                leaf(name)
        self.assertEqual(leaf('SD_GRD1_FIRE1.pl.wav'), 'SD_GRD1_FIRE1.pl.wav')

    def test_pcm_float_quantization_no_timing_or_gain_change(self):
        samples = [-.5, 0., .25, .8]
        fmt = struct.pack('<HHIIHH', 3, 1, 24000, 96000, 4, 32)
        raw = struct.pack('<4f', *samples)
        body = b'WAVEfmt '+struct.pack('<I', len(fmt))+fmt+b'data'+struct.pack('<I', len(raw))+raw
        source = b'RIFF'+struct.pack('<I', len(body))+body
        result, info, changed = normalized_pcm(source)
        self.assertTrue(changed)
        self.assertEqual(info['frames'], 4)
        with wave.open(io.BytesIO(result)) as w:
            actual = struct.unpack('<4h', w.readframes(4))
        self.assertLessEqual(max(abs(a/32767-b) for a, b in zip(actual, samples)), 1/32767)
        self.assertEqual(normalized_pcm(result)[0], result)
        for sample in (float('nan'), float('inf'), 1.1):
            corrupt = source[:-16]+struct.pack('<4f', sample, 0, .25, .8)
            with self.assertRaises(ValueError):
                normalized_pcm(corrupt)
        with self.assertRaises(ValueError):
            normalized_pcm(source[:-1])

    def test_notes_and_chrono_same_key_do_not_collide(self):
        source = ''.join(json.dumps(dict(section=s, key='same', en=e, pl=e))+'\n'
                         for s, e in [('Notes', 'Paper'), ('ChronoNote', 'Wall')])
        changes = [dict(section=s, key='same', source_sha256=hashlib.sha256(e.encode()).hexdigest(), pl=p)
                   for s, e, p in [('Notes', 'Paper', 'Papier'), ('ChronoNote', 'Wall', 'Ściana')]]
        result = [json.loads(x) for x in patch(source, changes).splitlines()]
        self.assertEqual([r['pl'] for r in result], ['Papier', 'Ściana'])
        with self.assertRaises(ValueError):
            patch(source, changes+[changes[0]])
        with self.assertRaises(ValueError):
            patch(source, [dict(changes[0], source_sha256='0'*64)])

    def test_dds_exact_payload_and_full_chain(self):
        header = bytearray(128)
        header[:4] = b'DDS '
        struct.pack_into('<I', header, 4, 124)
        struct.pack_into('<2I', header, 12, 8, 8)
        struct.pack_into('<I', header, 28, 2)
        header[84:88] = b'DXT1'
        data = bytes(header)+b'\0'*40  # 8x8 +4x4 BC1
        self.assertEqual(dds_info(data)['width'], 8)
        for bad in (data[:-1], data+b'\0'):
            with self.assertRaises(ValueError):
                dds_info(bad)
        struct.pack_into('<I', header, 28, 1)
        with self.assertRaises(ValueError):
            dds_info(bytes(header)+b'\0'*32)


if __name__ == '__main__':
    unittest.main()
