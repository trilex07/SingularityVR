"""Validate pinned private package delta and render synthetic PL/EN comparisons."""
import argparse
import hashlib
import struct
import sys
import unittest
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from ue3_layout import inspect
from remaster_native_buttons import movie_records
from swf_title_patch import read_rect
from polish_main_menu_buttons import (
    BITMAP_IDS, TEXT_IDS, LABELS, button_art, text_field, sha, build,
)

BEFORE = AFTER = OUTPUT = None


def movie(logical, name):
    export = next(e for e in inspect('check', logical, include_all=True)['exports'] if e['name'] == name)
    prop = next(p for p in export['properties'] if p['name'] == 'RawData')
    at = prop['tag_offset']+24
    return logical[at+4:at+prop['value']['bytes']]


class MainMenuPolish(unittest.TestCase):
    def test_all_label_widths_and_diacritics(self):
        font = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 21)
        longest = max((font.getlength(label), label) for labels in LABELS.values() for label in labels)
        self.assertLessEqual(longest[0], 320-2*15)
        self.assertIn('ę', ''.join(LABELS['PL']))
        self.assertIn('ź', ''.join(LABELS['PL']))
        print('longest label:', longest[1], 'Arial proxy width:', round(longest[0], 1), 'px; available 290px')

    def test_states_distinct_and_safe(self):
        images = [button_art(s) for s in ('up', 'over', 'down', 'disabled')]
        self.assertEqual({im.size for im in images}, {(480, 54)})
        self.assertEqual(len({sha(im.tobytes()) for im in images}), 4)
        for im in images:
            self.assertEqual(im.getpixel((0, 0))[3], 0)
            self.assertEqual(im.getpixel((240, 27))[3], 255)

    def test_private_package_delta(self):
        if not BEFORE or not AFTER:
            self.skipTest('private packages not supplied')
        original, logical = BEFORE
        output = AFTER
        expected, report = build(original, logical)
        self.assertEqual(expected, output)
        self.assertEqual(movie(logical, 'iis'), movie(output, 'iis'))
        before, after = movie_records(movie(logical, 'SPMenus'))[1], movie_records(movie(output, 'SPMenus'))[1]
        self.assertEqual(len(before), len(after))
        changed = []
        for (c0, p0), (c1, p1) in zip(before, after):
            self.assertEqual(c0, c1)
            if p0 != p1:
                char = struct.unpack_from('<H', p0)[0]
                self.assertEqual(char, struct.unpack_from('<H', p1)[0])
                self.assertIn((c0, char), {(37, i) for i in TEXT_IDS} | {(36, i) for i in BITMAP_IDS})
                changed.append((c0, char))
                if c0 == 37:
                    self.assertEqual(p1, text_field(p0))
                    self.assertEqual(read_rect(p0, 2)[1], read_rect(p1, 2)[1])
            else:
                self.assertNotIn((c0, struct.unpack_from('<H', p0)[0] if len(p0) >= 2 else -1),
                                 {(37, i) for i in TEXT_IDS} | {(36, i) for i in BITMAP_IDS})
        self.assertEqual(set(changed), {(37, i) for i in TEXT_IDS} | {(36, i) for i in BITMAP_IDS})
        # Every original timeline and AVM1/event payload remains byte-identical.
        self.assertEqual([p for c, p in before if c in (39, 12)], [p for c, p in after if c in (39, 12)])
        self.assertEqual(report['exports_verified'], 45)


def preview(path):
    font_before = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 36)
    font_after = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 31)
    root = Path(__file__).resolve().parents[3]
    source = root/'assets/processed/ui_remaster/pause_menu'
    states = ('up', 'over', 'down', 'disabled')
    oldnames = ('idle', 'hover', 'pressed', 'disabled')
    sheet = Image.new('RGB', (1040, 840), (10, 16, 20))
    draw = ImageDraw.Draw(sheet)
    for i, (state, old) in enumerate(zip(states, oldnames)):
        before = Image.open(source/f'btn_plate_{old}_neutral.png').convert('RGBA')
        after = button_art(state)
        for j, label in enumerate((LABELS['EN'][0], LABELS['EN'][1], LABELS['PL'][0], LABELS['PL'][1])):
            y = 20+i*200+j*43
            for x, plate, font, title in ((20, before, font_before, 'V2.1'), (540, after, font_after, 'Polished')):
                pic = plate.copy()
                d = ImageDraw.Draw(pic)
                box = d.textbbox((0, 0), label, font=font)
                d.text(((480-(box[2]-box[0]))/2-box[0], (54-(box[3]-box[1]))/2-box[1]),
                       label, font=font, fill=(240, 244, 243, 255))
                sheet.paste(pic.convert('RGB'), (x, y))
                if j == 0:
                    draw.text((x, y-15), f'{title} / {state}', fill=(190, 206, 208))
    path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(path)


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('--original', type=Path)
    ap.add_argument('--logical', type=Path)
    ap.add_argument('--after', type=Path)
    ap.add_argument('--preview', type=Path)
    args, rest = ap.parse_known_args()
    if args.original and args.logical and args.after:
        BEFORE = (args.original.read_bytes(), args.logical.read_bytes())
        AFTER = args.after.read_bytes()
    if args.preview:
        preview(args.preview)
    unittest.main(argv=[sys.argv[0]]+rest)
