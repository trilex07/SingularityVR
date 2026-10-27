"""Generates the minimalist, authentic 1950s Soviet E99 phosphor texture atlas for Singularity VR Diegetic HUD.
Creates diegetic_hud_atlas.png and diegetic_hud_atlas.json.
"""
import json
import math
import os
from PIL import Image, ImageDraw, ImageFilter, ImageFont

def create_diegetic_atlas(output_dir):
    width, height = 512, 256
    # Create RGBA image with transparent background
    img = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Dictionary to store UV rects: name -> [x, y, w, h]
    rects = {}

    # 1. Bezel Background (Dark Soviet Milled Steel / Graphite)
    # Position: (0, 0, 128, 64)
    bezel_rect = [0, 0, 128, 64]
    rects["bezel_bg"] = bezel_rect
    # Draw dark metallic rounded box with inner border
    draw.rounded_rectangle([2, 2, 125, 61], radius=6, fill=(24, 28, 34, 250), outline=(65, 75, 88, 255), width=2)
    # Corner screws / rivets (classic Soviet rugged hardware)
    for cx, cy in [(8, 8), (119, 8), (8, 55), (119, 55)]:
        draw.ellipse([cx-2, cy-2, cx+2, cy+2], fill=(85, 95, 110, 255), outline=(35, 40, 48, 255))

    # 2. Display Screen Face (Recessed E99 cathode/phosphor glass)
    # Position: (132, 0, 128, 64)
    screen_rect = [132, 0, 128, 64]
    rects["screen_bg"] = screen_rect
    draw.rounded_rectangle([134, 2, 257, 61], radius=4, fill=(8, 14, 20, 240), outline=(20, 35, 48, 255), width=1)

    # 3. Health Gauge Elements
    # Segment Full (Glowing E99 cathode cell): (264, 0, 32, 48)
    rects["health_seg_full"] = [264, 0, 32, 48]
    # Draw glowing pill segment
    draw.rounded_rectangle([268, 6, 291, 41], radius=3, fill=(240, 255, 255, 255), outline=(180, 240, 255, 255), width=1)
    draw.line([271, 10, 271, 37], fill=(255, 255, 255, 255), width=1) # core highlight

    # Segment Empty (Dim glass cell): (300, 0, 32, 48)
    rects["health_seg_empty"] = [300, 0, 32, 48]
    draw.rounded_rectangle([304, 6, 327, 41], radius=3, fill=(15, 24, 32, 160), outline=(30, 48, 62, 200), width=1)

    # Health Bio/Cross Icon: (336, 0, 48, 48)
    rects["icon_bio"] = [336, 0, 48, 48]
    draw.rounded_rectangle([340, 4, 379, 43], radius=4, fill=(12, 20, 28, 200), outline=(40, 60, 80, 255), width=1)
    # Soviet medical / E99 pulse cross
    draw.rectangle([356, 12, 363, 35], fill=(240, 255, 255, 255))
    draw.rectangle([348, 20, 371, 27], fill=(240, 255, 255, 255))

    # Ammo Magazine Icon: (388, 0, 48, 48)
    rects["icon_mag"] = [388, 0, 48, 48]
    draw.rounded_rectangle([392, 4, 431, 43], radius=4, fill=(12, 20, 28, 200), outline=(40, 60, 80, 255), width=1)
    # Minimalist Soviet bullet / cartridge icon
    draw.rectangle([405, 18, 417, 36], fill=(220, 240, 250, 255))
    draw.polygon([(405, 18), (411, 10), (417, 18)], fill=(255, 255, 255, 255))
    draw.rectangle([404, 36, 418, 39], fill=(160, 180, 200, 255))

    # 4. Digits: 0 through 9, plus '/' and '!' and '-'
    # We lay them out in two rows:
    # Row 1 (y = 70 to 155): Digits 0, 1, 2, 3, 4, 5, 6
    # Row 2 (y = 160 to 245): Digits 7, 8, 9, '/', '!', '-'
    
    # 7-Segment coordinates helper (compact, crisp Soviet Nixie style)
    # Digital segment width & height: 36 x 64
    def draw_7segment(base_x, base_y, digit_char):
        w, h = 34, 60
        t = 5 # thickness
        # Segment definitions: (x0, y0, x1, y1)
        # a: top horizontal
        # b: top-right vertical
        # c: bottom-right vertical
        # d: bottom horizontal
        # e: bottom-left vertical
        # f: top-left vertical
        # g: middle horizontal
        segs = {
            'a': [base_x + 4, base_y, base_x + w - 4, base_y + t],
            'b': [base_x + w - t, base_y + 3, base_x + w, base_y + h//2],
            'c': [base_x + w - t, base_y + h//2, base_x + w, base_y + h - 3],
            'd': [base_x + 4, base_y + h - t, base_x + w - 4, base_y + h],
            'e': [base_x, base_y + h//2, base_x + t, base_y + h - 3],
            'f': [base_x, base_y + 3, base_x + t, base_y + h//2],
            'g': [base_x + 4, base_y + h//2 - t//2, base_x + w - 4, base_y + h//2 + t//2 + 1]
        }
        
        # Mapping digit -> active segments
        digit_map = {
            '0': 'abcdef',
            '1': 'bc',
            '2': 'abged',
            '3': 'abgcd',
            '4': 'fgbc',
            '5': 'afgcd',
            '6': 'afedcg',
            '7': 'abc',
            '8': 'abcdefg',
            '9': 'abcdfg',
            '-': 'g',
        }
        
        if digit_char in digit_map:
            active = digit_map[digit_char]
            for s_name in 'abcdefg':
                box = segs[s_name]
                if s_name in active:
                    # Glowing white phosphor core
                    draw.rounded_rectangle(box, radius=2, fill=(255, 255, 255, 255))
                else:
                    # Very subtle unlit filament trace
                    draw.rounded_rectangle(box, radius=2, fill=(35, 45, 55, 60))
        elif digit_char == '/':
            # Slash separator
            draw.line([base_x + 6, base_y + h - 4, base_x + w - 6, base_y + 4], fill=(255, 255, 255, 255), width=t)
        elif digit_char == '!':
            draw.rounded_rectangle([base_x + w//2 - t//2, base_y + 4, base_x + w//2 + t//2 + 1, base_y + h - 14], radius=2, fill=(255, 255, 255, 255))
            draw.rounded_rectangle([base_x + w//2 - t//2, base_y + h - 8, base_x + w//2 + t//2 + 1, base_y + h - 2], radius=2, fill=(255, 255, 255, 255))

    row1_chars = ['0', '1', '2', '3', '4', '5', '6']
    for idx, ch in enumerate(row1_chars):
        bx = 10 + idx * 46
        by = 80
        draw_7segment(bx, by, ch)
        rects[f"digit_{ch}"] = [bx - 4, by - 4, 42, 68]

    row2_chars = ['7', '8', '9', '/', '!', '-']
    for idx, ch in enumerate(row2_chars):
        bx = 10 + idx * 46
        by = 170
        name = "slash" if ch == '/' else ("exclaim" if ch == '!' else ("dash" if ch == '-' else ch))
        draw_7segment(bx, by, ch)
        rects[f"digit_{name}"] = [bx - 4, by - 4, 42, 68]

    # Now create a soft bloom glow pass:
    # Extract alpha channel, blur it, and blend back to give authentic phosphor glow
    alpha = img.split()[3]
    glow = alpha.filter(ImageFilter.GaussianBlur(radius=3))
    
    # Create glow image (white with blurred alpha)
    glow_img = Image.new("RGBA", (width, height), (255, 255, 255, 0))
    glow_img.putalpha(glow)
    
    # Composite glow behind crisp foreground
    final_img = Image.alpha_composite(glow_img, img)

    # Save PNG and JSON
    png_path = os.path.join(output_dir, "diegetic_hud_atlas.png")
    json_path = os.path.join(output_dir, "diegetic_hud_atlas.json")
    
    final_img.save(png_path, "PNG")
    
    manifest = {
        "width": width,
        "height": height,
        "rects": rects
    }
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
        
    print(f"Generated {png_path} ({width}x{height}) and {json_path}")
    return png_path, json_path

if __name__ == "__main__":
    import sys
    out = sys.argv[1] if len(sys.argv) > 1 else r"C:\Users\Trilex\vr-mods\SingularityVR-settings-rifle-reload\assets\processed\hud\diegetic"
    create_diegetic_atlas(out)
