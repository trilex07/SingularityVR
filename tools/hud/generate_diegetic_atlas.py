"""Generates the ultra-crisp 1024x512 Soviet E99 phosphor texture atlas for Singularity VR Diegetic HUD.
Contains:
1. Milled Soviet gunmetal / graphite bezel with chamfered rim and corner mounting rivets
2. Recessed vacuum-fluorescent CRT / nixie screen background with phosphor lattice
3. Digits 0-9 in E99 Electric Cyan, Amber Warning, and Crimson Alert
4. Ammo magazine / cartridge icon and Soviet medical cross icon
5. Discrete 10-segment vacuum-tube health cells in Cyan, Amber, and Crimson
"""
import json
import math
import os
from PIL import Image, ImageDraw, ImageFilter

def create_diegetic_atlas(output_dir):
    os.makedirs(output_dir, exist_ok=True)
    width, height = 1024, 512
    img = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    rects = {}

    # Helper to register and return rect
    def reg(name, x, y, w, h):
        rects[name] = [x, y, w, h]
        return x, y, w, h

    # 1. Bezel Background (Dark Soviet Milled Steel / Graphite with chamfered edges)
    # (0, 0, 256, 128)
    bx, by, bw, bh = reg("bezel_bg", 0, 0, 256, 128)
    # Outer dark gunmetal plate
    draw.rounded_rectangle([bx+4, by+4, bx+bw-5, by+bh-5], radius=12, fill=(28, 32, 38, 255), outline=(70, 80, 95, 255), width=4)
    # Inner chamfered lip
    draw.rounded_rectangle([bx+16, by+14, bx+bw-17, by+bh-15], radius=8, fill=(18, 22, 26, 255), outline=(45, 52, 62, 255), width=3)
    # Corner mounting hex bolts / rivets
    rivet_pos = [(bx+14, by+14), (bx+bw-15, by+14), (bx+14, by+bh-15), (bx+bw-15, by+bh-15)]
    for rx, ry in rivet_pos:
        draw.ellipse([rx-6, ry-6, rx+6, ry+6], fill=(90, 102, 118, 255), outline=(20, 24, 28, 255), width=2)
        # Screw slot
        draw.line([rx-4, ry, rx+4, ry], fill=(30, 36, 42, 255), width=2)

    # 2. Display Screen Face (Recessed E99 cathode/phosphor glass)
    # (264, 0, 256, 128)
    sx, sy, sw, sh = reg("screen_bg", 264, 0, 256, 128)
    draw.rounded_rectangle([sx+4, sy+4, sx+sw-5, sy+sh-5], radius=8, fill=(8, 14, 20, 255), outline=(22, 38, 52, 255), width=3)
    # Subtle cathode scanlines
    for ly in range(sy+8, sy+sh-8, 4):
        draw.line([sx+8, ly, sx+sw-9, ly], fill=(12, 22, 32, 120), width=1)

    # 3. Health Gauge Elements (Cyan, Amber, Crimson, and Empty)
    # Segment Full - Cyan: (528, 0, 64, 96)
    reg("health_seg_cyan", 528, 0, 64, 96)
    draw.rounded_rectangle([536, 10, 584, 86], radius=6, fill=(0, 229, 255, 255), outline=(180, 245, 255, 255), width=2)
    draw.line([542, 18, 542, 78], fill=(255, 255, 255, 255), width=2) # core filament

    # Segment Full - Amber: (596, 0, 64, 96)
    reg("health_seg_amber", 596, 0, 64, 96)
    draw.rounded_rectangle([604, 10, 652, 86], radius=6, fill=(255, 145, 0, 255), outline=(255, 210, 140, 255), width=2)
    draw.line([610, 18, 610, 78], fill=(255, 255, 255, 255), width=2)

    # Segment Full - Crimson: (664, 0, 64, 96)
    reg("health_seg_crimson", 664, 0, 64, 96)
    draw.rounded_rectangle([672, 10, 720, 86], radius=6, fill=(255, 23, 68, 255), outline=(255, 160, 180, 255), width=2)
    draw.line([678, 18, 678, 78], fill=(255, 255, 255, 255), width=2)

    # Segment Empty: (732, 0, 64, 96)
    reg("health_seg_empty", 732, 0, 64, 96)
    draw.rounded_rectangle([740, 10, 788, 86], radius=6, fill=(12, 20, 26, 180), outline=(28, 44, 56, 220), width=2)

    # 4. Icons:
    # Bio / Medical Cross (Cyan): (804, 0, 96, 96)
    reg("icon_bio_cyan", 804, 0, 96, 96)
    draw.rounded_rectangle([810, 6, 894, 90], radius=8, fill=(10, 18, 26, 220), outline=(35, 60, 80, 255), width=2)
    # Cross shape
    draw.rectangle([844, 20, 860, 76], fill=(0, 229, 255, 255))
    draw.rectangle([826, 38, 878, 58], fill=(0, 229, 255, 255))
    # Inner white core
    draw.line([852, 24, 852, 72], fill=(255, 255, 255, 255), width=3)
    draw.line([830, 48, 874, 48], fill=(255, 255, 255, 255), width=3)

    # Bio / Medical Cross (Crimson Alert): (908, 0, 96, 96)
    reg("icon_bio_crimson", 908, 0, 96, 96)
    draw.rounded_rectangle([914, 6, 998, 90], radius=8, fill=(24, 10, 14, 220), outline=(80, 30, 40, 255), width=2)
    draw.rectangle([948, 20, 964, 76], fill=(255, 23, 68, 255))
    draw.rectangle([930, 38, 982, 58], fill=(255, 23, 68, 255))
    draw.line([956, 24, 956, 72], fill=(255, 255, 255, 255), width=3)
    draw.line([934, 48, 978, 48], fill=(255, 255, 255, 255), width=3)

    # Cartridge / Mag Icon (Cyan): (0, 140, 96, 96)
    reg("icon_mag_cyan", 0, 140, 96, 96)
    draw.rounded_rectangle([6, 146, 90, 230], radius=8, fill=(10, 18, 26, 220), outline=(35, 60, 80, 255), width=2)
    # Bullet shape
    draw.rectangle([34, 172, 62, 214], fill=(0, 229, 255, 255))
    draw.polygon([(34, 172), (48, 154), (62, 172)], fill=(255, 255, 255, 255))
    draw.rectangle([30, 214, 66, 222], fill=(0, 180, 210, 255))

    # 5. Numerals 0 through 9 in 7-Segment Nixie Style
    def draw_segment_digit(base_x, base_y, char, primary_color, dim_color):
        w, h = 64, 112
        t = 10 # thickness
        segs = {
            'a': [base_x + 8, base_y, base_x + w - 8, base_y + t],
            'b': [base_x + w - t, base_y + 6, base_x + w, base_y + h//2],
            'c': [base_x + w - t, base_y + h//2, base_x + w, base_y + h - 6],
            'd': [base_x + 8, base_y + h - t, base_x + w - 8, base_y + h],
            'e': [base_x, base_y + h//2, base_x + t, base_y + h - 6],
            'f': [base_x, base_y + 6, base_x + t, base_y + h//2],
            'g': [base_x + 8, base_y + h//2 - t//2, base_x + w - 8, base_y + h//2 + t//2]
        }
        digit_map = {
            '0': 'abcdef', '1': 'bc', '2': 'abged', '3': 'abgcd',
            '4': 'fgbc', '5': 'afgcd', '6': 'afedcg', '7': 'abc',
            '8': 'abcdefg', '9': 'abcdfg', '-': 'g'
        }
        active = digit_map.get(char, '')
        for s_name in 'abcdefg':
            box = segs[s_name]
            if s_name in active:
                draw.rounded_rectangle(box, radius=4, fill=primary_color)
                # bright core filament
                if s_name in 'adg':
                    draw.line([box[0]+4, (box[1]+box[3])//2, box[2]-4, (box[1]+box[3])//2], fill=(255, 255, 255, 255), width=2)
                else:
                    draw.line([(box[0]+box[2])//2, box[1]+4, (box[0]+box[2])//2, box[3]-4], fill=(255, 255, 255, 255), width=2)
            else:
                draw.rounded_rectangle(box, radius=4, fill=dim_color)

    cyan_col = (0, 229, 255, 255)
    dim_cyan = (15, 35, 48, 80)
    
    # Row 1 (Cyan Digits 0 to 9 + Slash): Y = 140 to 252
    chars = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9']
    for idx, ch in enumerate(chars):
        dx = 110 + idx * 82
        dy = 140
        draw_segment_digit(dx, dy, ch, cyan_col, dim_cyan)
        reg(f"digit_cyan_{ch}", dx-4, dy-4, 72, 120)

    # Slash separator
    sx = 110 + 10 * 82
    sy = 140
    reg("slash_cyan", sx-4, sy-4, 52, 120)
    draw.line([sx + 8, sy + 106, sx + 40, sy + 6], fill=cyan_col, width=10)
    draw.line([sx + 10, sy + 104, sx + 38, sy + 8], fill=(255, 255, 255, 255), width=3)

    # Row 2 (Amber Digits 0-9): Y = 270 to 382
    amber_col = (255, 145, 0, 255)
    dim_amber = (45, 25, 10, 80)
    for idx, ch in enumerate(chars):
        dx = 110 + idx * 82
        dy = 270
        draw_segment_digit(dx, dy, ch, amber_col, dim_amber)
        reg(f"digit_amber_{ch}", dx-4, dy-4, 72, 120)

    # Row 3 (Crimson Digits 0-9): Y = 395 to 507
    crimson_col = (255, 23, 68, 255)
    dim_crimson = (45, 12, 18, 80)
    for idx, ch in enumerate(chars):
        dx = 110 + idx * 82
        dy = 395
        draw_segment_digit(dx, dy, ch, crimson_col, dim_crimson)
        reg(f"digit_crimson_{ch}", dx-4, dy-4, 72, 120)

    # Exclamation mark
    ex_x = 110 + 10 * 82
    ex_y = 270
    reg("exclaim_crimson", ex_x-4, ex_y-4, 52, 120)
    draw.rounded_rectangle([ex_x + 16, ex_y + 8, ex_x + 28, ex_y + 76], radius=4, fill=crimson_col)
    draw.rounded_rectangle([ex_x + 16, ex_y + 88, ex_x + 28, ex_y + 102], radius=4, fill=crimson_col)

    png_path = os.path.join(output_dir, "diegetic_hud_atlas.png")
    json_path = os.path.join(output_dir, "diegetic_hud_atlas.json")

    img.save(png_path)
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(rects, f, indent=2)

    print(f"Generated enhanced atlas {png_path} ({width}x{height}) with {len(rects)} sprites")
    return png_path, json_path

if __name__ == "__main__":
    out_dir = r"c:\Users\Trilex\vr-mods\SingularityVR-diegetic-hud\assets\processed\hud\diegetic"
    create_diegetic_atlas(out_dir)
