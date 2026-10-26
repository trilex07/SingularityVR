"""Real GFx timeline/state tests plus embedded bitmap pixel/alpha checks.

--private-dir consumes local extracted iis.gfx/SPMenus.gfx. No game launch.
"""
import argparse
import hashlib
import struct
import sys
import unittest
import zlib
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from PIL import Image, ImageDraw
import remaster_native_buttons as patch
from inspect_gameplay_hud import nested_tags, matrix, skip_cxform
from swf_title_patch import read_rect, _take_bits

ROOT = Path(__file__).resolve().parents[3]
ASSETS = ROOT/'assets/processed/ui_remaster/pause_menu'
PRIVATE = None


def fields(code, data):
    depth_at, child_at, child, cx_at, cx_end = patch.placement_fields(code, data)
    at = depth_at + 2 + (2 if child_at is not None else 0)
    transform = matrix(data, at)[0] if data[0] & 4 else None
    # Exact tail contains ratio, instance name, filters and clip-actions.
    return dict(depth=struct.unpack_from('<H', data, depth_at)[0], child=child,
                matrix=transform, tail=data[cx_end:], cx=data[cx_at:cx_end])


def snapshots(payload):
    live = {}; result = []
    for code, data in nested_tags(payload):
        if code in (26, 70):
            f = fields(code, data); depth = f.pop('depth')
            old = live.get(depth, {}) if data[0] & 1 else {}
            for key in ('child', 'matrix'):
                if f[key] is None:
                    f[key] = old.get(key)
            live[depth] = f
        elif code == 28:
            live.pop(struct.unpack('<H', data)[0], None)
        elif code == 1:
            result.append({d: dict(p) for d, p in live.items()})
    return result


class NativeButtons(unittest.TestCase):
    def test_bitmap_pixels_premultiplied_argb(self):
        image = Image.new('RGBA', (2, 1))
        image.putdata([(100, 200, 50, 128), (255, 15, 99, 0)])
        data = patch.bitmap_payload(19, image)
        self.assertEqual(struct.unpack_from('<HBHH', data), (19, 5, 2, 1))
        self.assertEqual(zlib.decompress(data[7:]), bytes((128, 50, 100, 25, 0, 0, 0, 0)))

    def test_shapes_reference_exact_bitmap_and_bounds(self):
        for targets in patch.TARGETS.values():
            for _, rect in targets.values():
                p = patch.plate_shape(200, 199, rect, (480, 54))
                _, bounds, at = read_rect(p, 2)
                self.assertEqual(bounds, list(rect))
                self.assertEqual(p[at:at+2], b'\x01\x41')
                self.assertEqual(struct.unpack_from('<H', p, at+2)[0], 199)
                m, _ = matrix(p, at+4)
                self.assertAlmostEqual(m['sx']*480, rect[1]-rect[0], delta=.01)
                self.assertAlmostEqual(m['sy']*54, rect[3]-rect[2], delta=.01)
                self.assertEqual((m['x_twips'], m['y_twips']), (rect[0], rect[2]))

    def test_four_distinct_original_asset_states(self):
        hashes = []
        for state in patch.PLATES:
            img = Image.open(ASSETS/f'btn_plate_{state}_neutral.png').convert('RGBA')
            self.assertEqual(img.size, (480, 54))
            hashes.append(hashlib.sha256(img.tobytes()).hexdigest())
            self.assertGreater(sum(p[3] > 0 for p in img.getdata()), 20000)
        self.assertEqual(len(set(hashes)), 4)

    def test_fail_closed_unknown_movie_and_clip_depth(self):
        with self.assertRaises(ValueError):
            patch.patch_movie(b'unknown', 'iis', ASSETS)
        with self.assertRaises(ValueError):
            patch.placement_fields(26, b'\x40\x01\0')

    def test_real_native_timelines_and_bounds(self):
        if not PRIVATE:
            self.skipTest('private GFx fixtures not supplied')
        for name, targets in patch.TARGETS.items():
            original = (PRIVATE/(name+'.gfx')).read_bytes()
            changed, report = patch.patch_movie(original, name, ASSETS)
            before = patch.movie_records(original)[1]; after = patch.movie_records(changed)[1]
            defs_before = {struct.unpack_from('<H', p)[0]: (c, p) for c, p in before if c in (37, 39)}
            defs_after = {struct.unpack_from('<H', p)[0]: (c, p) for c, p in after if c in (37, 39)}
            # All original top-level payloads unchanged except exact buttons.
            added_ids = set(report['bitmap_ids']) | {s for row in report['buttons'] for s in row['shapes']}
            original_after = [(c,p) for c,p in after if not
                              (c in (32,36) and struct.unpack_from('<H',p)[0] in added_ids)]
            self.assertEqual(before[:3], after[:3])  # exporter/attributes/background header
            self.assertEqual(len(before), len(original_after))
            ids = {v[0] for v in targets.values()}
            for a, b in zip(before, original_after):
                if a[0] == 39 and struct.unpack_from('<H', a[1])[0] in ids:
                    continue
                self.assertEqual(a, b)
            for symbol, (character, rect) in targets.items():
                a = defs_before[character][1]; b = defs_after[character][1]
                self.assertEqual(a[:4], b[:4])  # character and total frame count
                for code in (0, 1, 12, 43):
                    self.assertEqual([p for c, p in nested_tags(a) if c == code],
                                     [p for c, p in nested_tags(b) if c == code])
                oldframes, newframes = snapshots(a), snapshots(b)
                self.assertEqual(len(oldframes), 40); self.assertEqual(len(newframes), 40)
                for old, new in zip(oldframes, newframes):
                    self.assertEqual(set(new), {1} | {d+1 for d in old})
                    for depth, state in old.items():
                        newstate = new[depth+1]
                        self.assertEqual(state['child'], newstate['child'])
                        self.assertEqual(state['matrix'], newstate['matrix'])
                        self.assertEqual(state['tail'], newstate['tail'])
                        if state['child'] == 101:
                            self.assertEqual(newstate['cx'], patch.invisible_alpha())
                        else:
                            self.assertEqual(state['cx'], newstate['cx'])
                    text = [s for s in old.values() if defs_before.get(s['child'], (None,))[0] == 37]
                    self.assertEqual(len(text), 1)
                    field = text[0]; bounds = read_rect(defs_before[field['child']][1], 2)[1]
                    m = field['matrix']; self.assertEqual((m['r0'], m['r1']), (0, 0))
                    world = (bounds[0]*m['sx']+m['x_twips'], bounds[1]*m['sx']+m['x_twips'],
                             bounds[2]*m['sy']+m['y_twips'], bounds[3]*m['sy']+m['y_twips'])
                    self.assertGreaterEqual(rect[0], world[0]); self.assertLessEqual(rect[1], world[1])
                    self.assertGreaterEqual(rect[2], world[2]); self.assertLessEqual(rect[3], world[3])
                # Original clip-action records/name/filter bytes stay identical.
                placements_a = [(c, fields(c, p)) for c, p in nested_tags(a) if c in (26, 70)]
                placements_b = [(c, fields(c, p)) for c, p in nested_tags(b)
                                if c in (26, 70) and fields(c, p)['depth'] != 1]
                self.assertEqual(len(placements_a), len(placements_b))
                for (ca, fa), (cb, fb) in zip(placements_a, placements_b):
                    self.assertEqual(ca, cb); self.assertEqual(fa['tail'], fb['tail'])

    def test_real_embedded_plate_pixels(self):
        if not PRIVATE:
            self.skipTest('private GFx fixtures not supplied')
        for name in patch.TARGETS:
            original = (PRIVATE/(name+'.gfx')).read_bytes()
            changed, report = patch.patch_movie(original, name, ASSETS)
            embedded = [p for c, p in patch.movie_records(changed)[1]
                        if c == 36 and struct.unpack_from('<H', p)[0] in report['bitmap_ids']]
            for state, payload in zip(patch.PLATES, embedded):
                reference = Image.open(ASSETS/f'btn_plate_{state}_neutral.png').convert('RGBA')
                # Re-encode independently from source pixels, compare exact GPU
                # bitmap payload after zlib decoding (including transparent corners).
                expected = bytearray()
                for r, g, b, a in reference.getdata():
                    expected += bytes((a, (r*a+127)//255, (g*a+127)//255, (b*a+127)//255))
                self.assertEqual(zlib.decompress(payload[7:]), expected)


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('--private-dir', type=Path)
    args, remaining = ap.parse_known_args()
    PRIVATE = args.private_dir
    unittest.main(argv=[sys.argv[0]]+remaining)
