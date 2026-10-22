"""
Singularity VR - UI Remaster: Weapon Wheel Generator
Generates SVG sources, transparent PNG exports, and concept mockup for Weapon Wheel.
"""

import os
import math
from PIL import Image, ImageDraw, ImageFont, ImageFilter

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SRC_DIR = os.path.join(BASE_DIR, "assets", "source", "ui_remaster", "weapon_wheel")
PROC_DIR = os.path.join(BASE_DIR, "assets", "processed", "ui_remaster", "weapon_wheel")
MOCKUP_DIR = os.path.join(BASE_DIR, "docs", "previews", "ui_remaster", "mockups")

for d in [SRC_DIR, PROC_DIR, MOCKUP_DIR]:
    os.makedirs(d, exist_ok=True)

def get_font(size, bold=False):
    font_path = r"C:\Windows\Fonts\bahnschrift.ttf"
    if not os.path.exists(font_path):
        font_path = r"C:\Windows\Fonts\arial.ttf"
    try:
        return ImageFont.truetype(font_path, size)
    except Exception:
        return ImageFont.load_default()

# -------------------------------------------------------------
# 1. Background Ring (1024x1024)
# -------------------------------------------------------------
def generate_wheel_background():
    size = 1024
    scale = 2
    w_size = size * scale
    im = Image.new("RGBA", (w_size, w_size), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    cx, cy = w_size // 2, w_size // 2

    r_outer = 480 * scale
    r_outer_bevel = 465 * scale
    r_inner_gauge = 440 * scale
    r_inner_edge = 380 * scale
    r_center_hole = 170 * scale

    # Outer dark metal ring backing
    d.ellipse([cx - r_outer, cy - r_outer, cx + r_outer, cy + r_outer],
              fill=(16, 22, 30, 220), outline=(65, 85, 102, 255), width=4 * scale)
    # Bevel groove
    d.ellipse([cx - r_outer_bevel, cy - r_outer_bevel, cx + r_outer_bevel, cy + r_outer_bevel],
              outline=(35, 48, 60, 255), width=2 * scale)

    # 360-degree dial ticks
    for angle in range(0, 360, 5):
        rad = math.radians(angle)
        cos_a = math.cos(rad)
        sin_a = math.sin(rad)
        is_major = (angle % 30 == 0)
        is_cardinal = (angle % 90 == 0)

        length = (24 if is_cardinal else (16 if is_major else 8)) * scale
        t_color = ((44, 232, 228, 255) if is_cardinal else
                   ((255, 169, 39, 220) if is_major else (80, 110, 130, 160)))
        w = (3 if is_cardinal else (2 if is_major else 1)) * scale

        x1 = cx + (r_inner_gauge - length) * cos_a
        y1 = cy + (r_inner_gauge - length) * sin_a
        x2 = cx + r_inner_gauge * cos_a
        y2 = cy + r_inner_gauge * sin_a
        d.line([x1, y1, x2, y2], fill=t_color, width=w)

    # Perimeter hex rivets (16 rivets around rim)
    for i in range(16):
        rad = math.radians(i * (360 / 16))
        rx = cx + (r_outer - 12 * scale) * math.cos(rad)
        ry = cy + (r_outer - 12 * scale) * math.sin(rad)
        rr = 6 * scale
        # Hex bolt representation
        d.ellipse([rx - rr, ry - rr, rx + rr, ry + rr], fill=(45, 60, 75, 255), outline=(90, 115, 135, 255), width=scale)
        d.ellipse([rx - rr//2, ry - rr//2, rx + rr//2, ry + rr//2], fill=(25, 35, 45, 255))

    # Soviet technical markings along ring
    font_serial = get_font(14 * scale)
    text_top = "КАТОРГА-12 // СЕКТОР ВЫБОРА ВООРУЖЕНИЯ"
    text_bot = "ГОСТ 1955 // ЭЛЕМЕНТ-99 // СИНГУЛЯРНОСТЬ"
    d.text((cx, cy - r_outer_bevel + 18 * scale), text_top, fill=(130, 165, 185, 200), font=font_serial, anchor="mm")
    d.text((cx, cy + r_outer_bevel - 18 * scale), text_bot, fill=(130, 165, 185, 200), font=font_serial, anchor="mm")

    # Center hole cutout (make transparent)
    d.ellipse([cx - r_center_hole, cy - r_center_hole, cx + r_center_hole, cy + r_center_hole],
              fill=(0, 0, 0, 0), outline=(50, 70, 85, 255), width=2 * scale)

    # Downsample
    final_im = im.resize((size, size), Image.Resampling.LANCZOS)
    png_path = os.path.join(PROC_DIR, "wheel_bg_ring.png")
    final_im.save(png_path, "PNG")
    print(f"Generated: {png_path}")

# -------------------------------------------------------------
# 2. Wheel Center Hub (Idle & Active, 512x512)
# -------------------------------------------------------------
def generate_wheel_center_hub():
    size = 512
    scale = 2
    w_size = size * scale

    for mode in ["idle", "active"]:
        im = Image.new("RGBA", (w_size, w_size), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        cx, cy = w_size // 2, w_size // 2

        r_base = 220 * scale
        r_bevel = 200 * scale
        r_chamber = 150 * scale
        r_core = 90 * scale

        # Outer heavy steel containment flange
        d.ellipse([cx - r_base, cy - r_base, cx + r_base, cy + r_base],
                  fill=(18, 24, 32, 240), outline=(75, 95, 115, 255), width=4 * scale)
        d.ellipse([cx - r_bevel, cy - r_bevel, cx + r_bevel, cy + r_bevel],
                  fill=(12, 16, 22, 255), outline=(40, 52, 65, 255), width=2 * scale)

        # 8 heavy hex bolts around rim
        for i in range(8):
            rad = math.radians(i * 45)
            bx = cx + (r_base - 14 * scale) * math.cos(rad)
            by = cy + (r_base - 14 * scale) * math.sin(rad)
            br = 7 * scale
            d.ellipse([bx - br, by - br, bx + br, by + br], fill=(50, 65, 80, 255), outline=(100, 130, 150, 255), width=scale)
            d.ellipse([bx - br//2, by - br//2, bx + br//2, by + br//2], fill=(20, 28, 36, 255))

        # E99 Chamber vacuum well
        d.ellipse([cx - r_chamber, cy - r_chamber, cx + r_chamber, cy + r_chamber],
                  fill=(8, 12, 16, 255), outline=(25, 40, 50, 255), width=2 * scale)

        # E99 Cathode core glow & filaments
        if mode == "active":
            glow_col = (44, 232, 228, 160)
            core_col = (120, 255, 250, 255)
            d.ellipse([cx - r_core - 20 * scale, cy - r_core - 20 * scale,
                       cx + r_core + 20 * scale, cy + r_core + 20 * scale],
                      fill=(14, 70, 80, 140))
        else:
            glow_col = (20, 90, 100, 100)
            core_col = (44, 232, 228, 200)

        # Iris blades / vacuum aperture
        for a in range(0, 360, 60):
            rad = math.radians(a)
            ix = cx + r_chamber * math.cos(rad)
            iy = cy + r_chamber * math.sin(rad)
            jx = cx + r_core * math.cos(rad + 0.6)
            jy = cy + r_core * math.sin(rad + 0.6)
            d.line([ix, iy, jx, jy], fill=(50, 75, 95, 200), width=2 * scale)

        # Central glowing chronal core
        d.ellipse([cx - r_core, cy - r_core, cx + r_core, cy + r_core],
                  fill=glow_col, outline=core_col, width=3 * scale)
        d.ellipse([cx - r_core//2, cy - r_core//2, cx + r_core//2, cy + r_core//2],
                  fill=core_col)

        # Stencil label in hub
        f_hub = get_font(13 * scale, bold=True)
        lbl = "TMD-E99" if mode == "idle" else "СИСТЕМА АКТИВНА"
        lbl_col = (180, 220, 240, 230) if mode == "idle" else (255, 255, 255, 255)
        d.text((cx, cy + r_base - 32 * scale), lbl, fill=lbl_col, font=f_hub, anchor="mm")

        # Downsample & save
        final_im = im.resize((size, size), Image.Resampling.LANCZOS)
        png_path = os.path.join(PROC_DIR, f"wheel_center_hub_{mode}.png")
        final_im.save(png_path, "PNG")
        print(f"Generated: {png_path}")

# -------------------------------------------------------------
# 3. Sector Geometry (North, East, South, West x 4 states)
# -------------------------------------------------------------
def generate_sectors():
    size = 1024
    scale = 2
    w_size = size * scale

    # Angles for 4 quadrants:
    # 0 = North (225° to 315° in standard polar where 270° is straight up)
    # 1 = East (315° to 45°)
    # 2 = South (45° to 135°)
    # 3 = West (135° to 225°)
    sectors = {
        "north": (225 + 3, 315 - 3, "СЕКТОР 1 // ВЕРХ"),
        "east":  (315 + 3, 405 - 3, "СЕКТОР 2 // ПРАВО"),
        "south": (45 + 3,  135 - 3, "СЕКТОР 3 // НИЗ"),
        "west":  (135 + 3, 225 - 3, "СЕКТОР 4 // ЛЕВО")
    }

    r_in = 185 * scale
    r_out = 435 * scale

    states = {
        "idle": {
            "fill": (18, 26, 36, 190),
            "border": (60, 85, 105, 200),
            "border_w": 2 * scale,
            "glow": None
        },
        "hover": {
            "fill": (24, 60, 78, 230),
            "border": (44, 232, 228, 255),
            "border_w": 4 * scale,
            "glow": (44, 232, 228, 80)
        },
        "selected": {
            "fill": (20, 75, 95, 245),
            "border": (120, 255, 250, 255),
            "border_w": 5 * scale,
            "glow": (44, 232, 228, 140)
        },
        "disabled": {
            "fill": (12, 15, 20, 140),
            "border": (40, 50, 60, 160),
            "border_w": 2 * scale,
            "glow": None
        }
    }

    for s_name, (a_start, a_end, label) in sectors.items():
        for st_name, st_cfg in states.items():
            im = Image.new("RGBA", (w_size, w_size), (0, 0, 0, 0))
            d = ImageDraw.Draw(im)
            cx, cy = w_size // 2, w_size // 2

            # Build polygon points along arc
            points = []
            # Outer arc
            steps = 40
            for i in range(steps + 1):
                ang = math.radians(a_start + (a_end - a_start) * (i / steps))
                points.append((cx + r_out * math.cos(ang), cy + r_out * math.sin(ang)))
            # Inner arc (reverse)
            for i in range(steps, -1, -1):
                ang = math.radians(a_start + (a_end - a_start) * (i / steps))
                points.append((cx + r_in * math.cos(ang), cy + r_in * math.sin(ang)))

            # If glow
            if st_cfg["glow"]:
                glow_im = Image.new("RGBA", (w_size, w_size), (0, 0, 0, 0))
                gd = ImageDraw.Draw(glow_im)
                gd.polygon(points, fill=st_cfg["glow"], outline=st_cfg["border"])
                glow_im = glow_im.filter(ImageFilter.GaussianBlur(8 * scale))
                im = Image.alpha_composite(im, glow_im)
                d = ImageDraw.Draw(im)

            # Draw sector polygon
            d.polygon(points, fill=st_cfg["fill"], outline=st_cfg["border"])

            # Outline stroke
            if st_cfg["border_w"] > 1:
                # Pillow draw polygon outline is thin; draw line segments for outer & inner arcs
                for i in range(steps):
                    a1 = math.radians(a_start + (a_end - a_start) * (i / steps))
                    a2 = math.radians(a_start + (a_end - a_start) * ((i + 1) / steps))
                    d.line([(cx + r_out * math.cos(a1), cy + r_out * math.sin(a1)),
                            (cx + r_out * math.cos(a2), cy + r_out * math.sin(a2))],
                           fill=st_cfg["border"], width=st_cfg["border_w"])
                    d.line([(cx + r_in * math.cos(a1), cy + r_in * math.sin(a1)),
                            (cx + r_in * math.cos(a2), cy + r_in * math.sin(a2))],
                           fill=st_cfg["border"], width=st_cfg["border_w"])

            # Diagonal caution lines for disabled
            if st_name == "disabled":
                mid_r = (r_in + r_out) / 2
                mid_a = math.radians((a_start + a_end) / 2)
                d.line([cx + (mid_r - 40 * scale) * math.cos(mid_a),
                        cy + (mid_r - 40 * scale) * math.sin(mid_a),
                        cx + (mid_r + 40 * scale) * math.cos(mid_a),
                        cy + (mid_r + 40 * scale) * math.sin(mid_a)],
                       fill=(120, 40, 45, 180), width=4 * scale)

            # High-visibility corner registration brackets for hover & selected
            if st_name in ["hover", "selected"]:
                c_col = (120, 255, 250, 255) if st_name == "selected" else (44, 232, 228, 255)
                for corner_a in [a_start, a_end]:
                    ca = math.radians(corner_a)
                    px = cx + (r_out - 10 * scale) * math.cos(ca)
                    py = cy + (r_out - 10 * scale) * math.sin(ca)
                    d.rectangle([px - 4 * scale, py - 4 * scale, px + 4 * scale, py + 4 * scale], fill=c_col)

            # Downsample and save
            final_im = im.resize((size, size), Image.Resampling.LANCZOS)
            out_file = os.path.join(PROC_DIR, f"sector_{s_name}_{st_name}.png")
            final_im.save(out_file, "PNG")

    print("Generated all 16 sector states.")

# -------------------------------------------------------------
# 4. Weapon Icons (256x256 each + Composite Sheet)
# -------------------------------------------------------------
def generate_weapon_icons():
    size = 256
    scale = 4
    w_size = size * scale

    icons_data = [
        ("centurion", "ЦЕНТУРИОН", ".44 РЕВОЛЬВЕР", (44, 232, 228)),
        ("volk", "ВОЛК S-4", "7.62mm АВТОМАТ", (44, 232, 228)),
        ("kasimov", "КАСИМОВ SNV", "12.7mm ВИНТОВКА", (44, 232, 228)),
        ("arclight", "АРК-ЛАЙТ", "E99 ДУГОВОЙ ИЗЛУЧАТЕЛЬ", (120, 255, 250)),
        ("spikeshot", "СПАЙКШОТ", "ИГОЛЬЧАТЫЙ ПАТРОН", (255, 169, 39)),
        ("autocannon", "АВТОПУШКА", "МНОГОСТВОЛЬНЫЙ", (255, 169, 39)),
        ("tmd", "TMD-E99", "ХРОНО-МАНИПУЛЯТОР", (44, 232, 228))
    ]

    sheet_w = 4 * size
    sheet_h = 2 * size
    sheet_im = Image.new("RGBA", (sheet_w, sheet_h), (0, 0, 0, 0))

    col_white = (240, 248, 255, 255)

    for idx, (name, title_cyr, sub_en, accent_col) in enumerate(icons_data):
        im = Image.new("RGBA", (w_size, w_size), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        cx, cy = w_size // 2, w_size // 2 - 20 * scale

        # Draw silhouette/vector per weapon
        if name == "centurion":
            # Heavy Soviet Revolver
            # Barrel & top strap
            d.rectangle([cx - 90 * scale, cy - 25 * scale, cx + 50 * scale, cy - 5 * scale], fill=col_white)
            # Front sight
            d.polygon([(cx + 40 * scale, cy - 25 * scale), (cx + 50 * scale, cy - 25 * scale), (cx + 50 * scale, cy - 35 * scale)], fill=col_white)
            # Heavy under-barrel lug
            d.rectangle([cx - 40 * scale, cy - 5 * scale, cx + 45 * scale, cy + 10 * scale], fill=col_white)
            # Cylinder
            d.rectangle([cx - 85 * scale, cy - 25 * scale, cx - 25 * scale, cy + 20 * scale], fill=col_white)
            d.line([cx - 55 * scale, cy - 25 * scale, cx - 55 * scale, cy + 20 * scale], fill=(30, 40, 50, 255), width=3 * scale)
            # Frame & grip
            d.polygon([(cx - 85 * scale, cy - 20 * scale), (cx - 120 * scale, cy + 10 * scale),
                       (cx - 100 * scale, cy + 70 * scale), (cx - 70 * scale, cy + 65 * scale),
                       (cx - 60 * scale, cy + 15 * scale)], fill=col_white)
            # Hammer
            d.polygon([(cx - 100 * scale, cy - 25 * scale), (cx - 115 * scale, cy - 40 * scale), (cx - 95 * scale, cy - 35 * scale)], fill=col_white)
            # Trigger guard
            d.arc([cx - 60 * scale, cy + 10 * scale, cx - 20 * scale, cy + 45 * scale], 0, 180, fill=col_white, width=4 * scale)

        elif name == "volk":
            # Volk S4 Assault Rifle
            # Receiver
            d.rectangle([cx - 90 * scale, cy - 18 * scale, cx + 20 * scale, cy + 10 * scale], fill=col_white)
            # Barrel & Gas Tube
            d.rectangle([cx + 20 * scale, cy - 15 * scale, cx + 110 * scale, cy - 5 * scale], fill=col_white)
            d.rectangle([cx + 20 * scale, cy - 5 * scale, cx + 125 * scale, cy + 5 * scale], fill=col_white)
            # Front Sight & Muzzle brake
            d.polygon([(cx + 110 * scale, cy - 5 * scale), (cx + 115 * scale, cy - 25 * scale), (cx + 120 * scale, cy - 5 * scale)], fill=col_white)
            d.rectangle([cx + 125 * scale, cy - 8 * scale, cx + 135 * scale, cy + 8 * scale], fill=col_white)
            # Handguard
            d.rectangle([cx + 20 * scale, cy - 8 * scale, cx + 80 * scale, cy + 15 * scale], fill=col_white)
            # Curved Banana Magazine
            d.polygon([(cx - 20 * scale, cy + 10 * scale), (cx + 10 * scale, cy + 10 * scale),
                       (cx + 25 * scale, cy + 60 * scale), (cx - 5 * scale, cy + 65 * scale)], fill=col_white)
            # Pistol Grip
            d.polygon([(cx - 80 * scale, cy + 10 * scale), (cx - 65 * scale, cy + 10 * scale),
                       (cx - 50 * scale, cy + 50 * scale), (cx - 70 * scale, cy + 52 * scale)], fill=col_white)
            # Stock
            d.polygon([(cx - 90 * scale, cy - 15 * scale), (cx - 135 * scale, cy - 10 * scale),
                       (cx - 140 * scale, cy + 30 * scale), (cx - 120 * scale, cy + 30 * scale),
                       (cx - 90 * scale, cy + 5 * scale)], fill=col_white)

        elif name == "kasimov":
            # Kasimov SNV-E99 Sniper Rifle
            # Long barrel
            d.rectangle([cx - 60 * scale, cy - 8 * scale, cx + 130 * scale, cy + 4 * scale], fill=col_white)
            # Massive Muzzle brake
            d.rectangle([cx + 130 * scale, cy - 14 * scale, cx + 145 * scale, cy + 10 * scale], fill=col_white)
            # Receiver
            d.rectangle([cx - 80 * scale, cy - 15 * scale, cx - 10 * scale, cy + 12 * scale], fill=col_white)
            # Optical Scope on high riser
            d.rectangle([cx - 65 * scale, cy - 35 * scale, cx + 35 * scale, cy - 20 * scale], fill=col_white)
            d.polygon([(cx + 35 * scale, cy - 40 * scale), (cx + 50 * scale, cy - 42 * scale),
                       (cx + 50 * scale, cy - 13 * scale), (cx + 35 * scale, cy - 15 * scale)], fill=col_white)
            d.polygon([(cx - 65 * scale, cy - 38 * scale), (cx - 75 * scale, cy - 40 * scale),
                       (cx - 75 * scale, cy - 15 * scale), (cx - 65 * scale, cy - 17 * scale)], fill=col_white)
            # Scope mounts
            d.rectangle([cx - 45 * scale, cy - 20 * scale, cx - 35 * scale, cy - 15 * scale], fill=col_white)
            d.rectangle([cx + 5 * scale, cy - 20 * scale, cx + 15 * scale, cy - 15 * scale], fill=col_white)
            # Skeleton Stock
            d.polygon([(cx - 80 * scale, cy - 12 * scale), (cx - 135 * scale, cy - 5 * scale),
                       (cx - 140 * scale, cy + 35 * scale), (cx - 125 * scale, cy + 35 * scale),
                       (cx - 80 * scale, cy + 10 * scale)], fill=col_white)
            # Magazine
            d.rectangle([cx - 35 * scale, cy + 12 * scale, cx - 10 * scale, cy + 40 * scale], fill=col_white)

        elif name == "arclight":
            # ARCLight Heavy Energy Projector
            # Central generator canister
            d.rectangle([cx - 70 * scale, cy - 25 * scale, cx + 15 * scale, cy + 25 * scale], fill=col_white)
            # Cathode coils
            d.line([cx - 45 * scale, cy - 25 * scale, cx - 45 * scale, cy + 25 * scale], fill=(30, 45, 60, 255), width=4 * scale)
            d.line([cx - 20 * scale, cy - 25 * scale, cx - 20 * scale, cy + 25 * scale], fill=(30, 45, 60, 255), width=4 * scale)
            # Dual forward arc prongs
            d.polygon([(cx + 15 * scale, cy - 35 * scale), (cx + 80 * scale, cy - 40 * scale),
                       (cx + 100 * scale, cy - 20 * scale), (cx + 80 * scale, cy - 18 * scale),
                       (cx + 15 * scale, cy - 15 * scale)], fill=col_white)
            d.polygon([(cx + 15 * scale, cy + 35 * scale), (cx + 80 * scale, cy + 40 * scale),
                       (cx + 100 * scale, cy + 20 * scale), (cx + 80 * scale, cy + 18 * scale),
                       (cx + 15 * scale, cy + 15 * scale)], fill=col_white)
            # Lightning arc between prongs (cyan)
            d.line([(cx + 90 * scale, cy - 20 * scale), (cx + 105 * scale, cy), (cx + 90 * scale, cy + 20 * scale)],
                   fill=accent_col, width=4 * scale)
            # Rear stock & grip
            d.polygon([(cx - 70 * scale, cy - 15 * scale), (cx - 120 * scale, cy - 10 * scale),
                       (cx - 125 * scale, cy + 35 * scale), (cx - 95 * scale, cy + 30 * scale),
                       (cx - 70 * scale, cy + 15 * scale)], fill=col_white)

        elif name == "spikeshot":
            # Spikeshot Pneumatic Flechette Railgun
            # Heavy dual guide rails
            d.rectangle([cx - 60 * scale, cy - 25 * scale, cx + 115 * scale, cy - 12 * scale], fill=col_white)
            d.rectangle([cx - 60 * scale, cy + 5 * scale, cx + 115 * scale, cy + 18 * scale], fill=col_white)
            # Pneumatic pressure reservoir tank (cylinder under rails)
            d.rectangle([cx - 30 * scale, cy + 18 * scale, cx + 60 * scale, cy + 38 * scale], fill=col_white)
            # Top flechette hopper
            d.polygon([(cx - 40 * scale, cy - 25 * scale), (cx + 10 * scale, cy - 25 * scale),
                       (cx + 20 * scale, cy - 45 * scale), (cx - 50 * scale, cy - 45 * scale)], fill=col_white)
            # Heavy rear receiver & grip
            d.rectangle([cx - 110 * scale, cy - 20 * scale, cx - 60 * scale, cy + 25 * scale], fill=col_white)
            d.polygon([(cx - 90 * scale, cy + 25 * scale), (cx - 75 * scale, cy + 25 * scale),
                       (cx - 60 * scale, cy + 60 * scale), (cx - 80 * scale, cy + 62 * scale)], fill=col_white)

        elif name == "autocannon":
            # Multi-barrel Rotary Gatling Autocannon
            # Triple barrels
            d.rectangle([cx - 10 * scale, cy - 22 * scale, cx + 115 * scale, cy - 14 * scale], fill=col_white)
            d.rectangle([cx - 10 * scale, cy - 4 * scale, cx + 115 * scale, cy + 4 * scale], fill=col_white)
            d.rectangle([cx - 10 * scale, cy + 14 * scale, cx + 115 * scale, cy + 22 * scale], fill=col_white)
            # Barrel clamp rings
            d.rectangle([cx + 30 * scale, cy - 26 * scale, cx + 45 * scale, cy + 26 * scale], fill=col_white)
            d.rectangle([cx + 90 * scale, cy - 26 * scale, cx + 105 * scale, cy + 26 * scale], fill=col_white)
            # Rotor housing
            d.rectangle([cx - 80 * scale, cy - 35 * scale, cx - 10 * scale, cy + 35 * scale], fill=col_white)
            # Top carry handle
            d.line([cx - 60 * scale, cy - 35 * scale, cx - 60 * scale, cy - 55 * scale,
                    cx + 10 * scale, cy - 55 * scale, cx + 10 * scale, cy - 35 * scale], fill=col_white, width=6 * scale)
            # Rear spade grip
            d.rectangle([cx - 120 * scale, cy - 25 * scale, cx - 80 * scale, cy + 25 * scale], fill=col_white)

        elif name == "tmd":
            # TMD - Time Manipulation Gauntlet
            # Gauntlet body base
            d.polygon([(cx - 85 * scale, cy - 40 * scale), (cx + 10 * scale, cy - 30 * scale),
                       (cx + 30 * scale, cy + 30 * scale), (cx - 85 * scale, cy + 40 * scale)], fill=col_white)
            # Central chronal core ring
            d.ellipse([cx - 20 * scale, cy - 25 * scale, cx + 30 * scale, cy + 25 * scale], fill=accent_col)
            d.ellipse([cx - 10 * scale, cy - 15 * scale, cx + 20 * scale, cy + 15 * scale], fill=(16, 22, 30, 255))
            # Three articulated focus needles
            d.polygon([(cx + 25 * scale, cy - 30 * scale), (cx + 95 * scale, cy - 45 * scale), (cx + 40 * scale, cy - 15 * scale)], fill=col_white)
            d.polygon([(cx + 35 * scale, cy - 8 * scale), (cx + 110 * scale, cy), (cx + 35 * scale, cy + 8 * scale)], fill=col_white)
            d.polygon([(cx + 25 * scale, cy + 30 * scale), (cx + 95 * scale, cy + 45 * scale), (cx + 40 * scale, cy + 15 * scale)], fill=col_white)

        # Stencil Labels
        f_title = get_font(15 * scale, bold=True)
        f_sub = get_font(10 * scale)
        d.text((cx, cy + 80 * scale), title_cyr, fill=(240, 245, 250, 240), font=f_title, anchor="mm")
        d.text((cx, cy + 98 * scale), sub_en, fill=(130, 160, 180, 200), font=f_sub, anchor="mm")

        # Downsample
        final_im = im.resize((size, size), Image.Resampling.LANCZOS)
        png_path = os.path.join(PROC_DIR, f"weapon_icon_{name}.png")
        final_im.save(png_path, "PNG")

        # Composite into sheet
        col_idx = idx % 4
        row_idx = idx // 4
        sheet_im.paste(final_im, (col_idx * size, row_idx * size), final_im)

    sheet_path = os.path.join(PROC_DIR, "weapon_icons_sheet.png")
    sheet_im.save(sheet_path, "PNG")
    print(f"Generated weapon icons and composite sheet: {sheet_path}")

# -------------------------------------------------------------
# 5. Full Weapon Wheel Concept Mockup (1920x1080)
# -------------------------------------------------------------
def generate_weapon_wheel_mockup():
    w, h = 1920, 1080
    im = Image.new("RGBA", (w, h), (10, 14, 18, 255))
    d = ImageDraw.Draw(im)

    # Ambient lab background vignette
    for r in range(h // 2, 0, -20):
        alpha = int(40 * (1 - r / (h // 2)))
        d.ellipse([w // 2 - r * 1.6, h // 2 - r, w // 2 + r * 1.6, h // 2 + r], fill=(16, 26, 36, alpha))

    # Grid lines (technical tactical overlay)
    for gx in range(0, w, 80):
        d.line([gx, 0, gx, h], fill=(20, 30, 42, 60), width=1)
    for gy in range(0, h, 80):
        d.line([0, gy, w, gy], fill=(20, 30, 42, 60), width=1)

    cx, cy = w // 2, h // 2

    # Load components
    bg_ring = Image.open(os.path.join(PROC_DIR, "wheel_bg_ring.png"))
    sec_n = Image.open(os.path.join(PROC_DIR, "sector_north_idle.png"))
    sec_e = Image.open(os.path.join(PROC_DIR, "sector_east_hover.png"))      # Active/Hovered sector
    sec_s = Image.open(os.path.join(PROC_DIR, "sector_south_idle.png"))
    sec_w = Image.open(os.path.join(PROC_DIR, "sector_west_idle.png"))
    hub = Image.open(os.path.join(PROC_DIR, "wheel_center_hub_active.png"))

    icon_cent = Image.open(os.path.join(PROC_DIR, "weapon_icon_centurion.png"))
    icon_volk = Image.open(os.path.join(PROC_DIR, "weapon_icon_volk.png"))
    icon_kas = Image.open(os.path.join(PROC_DIR, "weapon_icon_kasimov.png"))
    icon_arc = Image.open(os.path.join(PROC_DIR, "weapon_icon_arclight.png"))

    # Paste Wheel Ring
    im.paste(bg_ring, (cx - 512, cy - 512), bg_ring)

    # Paste Sectors
    im.paste(sec_n, (cx - 512, cy - 512), sec_n)
    im.paste(sec_e, (cx - 512, cy - 512), sec_e)
    im.paste(sec_s, (cx - 512, cy - 512), sec_s)
    im.paste(sec_w, (cx - 512, cy - 512), sec_w)

    # Paste Weapon Icons on Sectors
    # North: Centurion (y - 300)
    im.paste(icon_cent, (cx - 128, cy - 420), icon_cent)
    # East: Volk S-4 (x + 300)
    im.paste(icon_volk, (cx + 170, cy - 128), icon_volk)
    # South: Kasimov (y + 300)
    im.paste(icon_kas, (cx - 128, cy + 170), icon_kas)
    # West: ARCLight (x - 300)
    im.paste(icon_arc, (cx - 420, cy - 128), icon_arc)

    # Paste Center Hub
    im.paste(hub, (cx - 256, cy - 256), hub)

    # Draw Controller Thumbstick Pointer Vector (pointing towards East / Volk S-4)
    stick_dx = 120
    stick_dy = -10
    d.line([cx, cy, cx + stick_dx, cy + stick_dy], fill=(44, 232, 228, 255), width=4)
    d.ellipse([cx + stick_dx - 6, cy + stick_dy - 6, cx + stick_dx + 6, cy + stick_dy + 6],
              fill=(120, 255, 250, 255), outline=(44, 232, 228, 255))

    # Tactical HUD Info Overlays
    f_header = get_font(28, bold=True)
    f_sub = get_font(18)
    f_hint = get_font(16)

    # Header Box
    d.rectangle([cx - 300, 30, cx + 300, 90], fill=(16, 22, 30, 230), outline=(65, 85, 102, 255), width=2)
    d.rectangle([cx - 296, 34, cx - 280, 86], fill=(44, 232, 228, 255))
    d.text((cx - 260, 42), "ВЫБОР ОРУЖИЯ // WEAPON SELECTION", fill=(240, 248, 255, 255), font=f_header)
    d.text((cx - 260, 68), "УДЕРЖИВАЙТЕ R3 + ПРАВЫЙ СТИК", fill=(130, 165, 185, 220), font=f_sub)

    # Selected weapon callout card (Right side)
    card_x = cx + 460
    card_y = cy - 140
    d.rectangle([card_x, card_y, card_x + 360, card_y + 260], fill=(16, 25, 35, 240), outline=(44, 232, 228, 255), width=2)
    d.rectangle([card_x + 8, card_y + 8, card_x + 352, card_y + 44], fill=(24, 60, 78, 255))
    d.text((card_x + 16, card_y + 14), "АКТИВНЫЙ ВЫБОР [СЕКТОР 2]", fill=(120, 255, 250, 255), font=f_sub)
    d.text((card_x + 16, card_y + 56), "ВОЛК S-4 (VOLK S-4)", fill=(245, 250, 255, 255), font=f_header)
    d.text((card_x + 16, card_y + 94), "ТИП: ШТУРМОВАЯ ВИНТОВКА", fill=(160, 190, 210, 230), font=f_sub)
    d.text((card_x + 16, card_y + 124), "КАЛИБР: 7.62x39mm ОБРАЗЦА 1943г.", fill=(160, 190, 210, 230), font=f_sub)
    d.text((card_x + 16, card_y + 154), "ЕМКОСТЬ МАГАЗИНА: 30 / 180", fill=(44, 232, 228, 255), font=f_sub)
    d.text((card_x + 16, card_y + 184), "МОДИФИКАЦИЯ: УВЕЛИЧЕННЫЙ СТВОЛ", fill=(255, 169, 39, 255), font=f_sub)
    d.text((card_x + 16, card_y + 224), "ОТПУСТИТЕ R3 ДЛЯ ПОДТВЕРЖДЕНИЯ", fill=(130, 165, 185, 220), font=f_hint)

    # Bottom PSVR2 prompt legend
    d.rectangle([cx - 400, h - 70, cx + 400, h - 20], fill=(12, 16, 22, 230), outline=(45, 60, 75, 255), width=2)
    d.text((cx, h - 45), "[R3 RELEASE] ПОДТВЕРДИТЬ     [L3 CLICK] ОТМЕНА     [R-STICK] ВЫБОР СЕКТОРА",
           fill=(220, 235, 245, 230), font=f_hint, anchor="mm")

    mockup_path = os.path.join(MOCKUP_DIR, "weapon_wheel_mockup.png")
    im.save(mockup_path, "PNG")
    print(f"Generated weapon wheel concept mockup: {mockup_path}")

# Run all weapon wheel generators
if __name__ == "__main__":
    generate_wheel_background()
    generate_wheel_center_hub()
    generate_sectors()
    generate_weapon_icons()
    generate_weapon_wheel_mockup()
