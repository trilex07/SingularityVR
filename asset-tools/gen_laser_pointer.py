"""
Singularity VR - UI Remaster: Laser & Pointer Visual Feedback Kit Generator
Generates SVG sources, transparent PNG exports, composite sheet, and concept mockup.
"""

import os
import math
from PIL import Image, ImageDraw, ImageFont, ImageFilter

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SRC_DIR = os.path.join(BASE_DIR, "assets", "source", "ui_remaster", "laser_pointer")
PROC_DIR = os.path.join(BASE_DIR, "assets", "processed", "ui_remaster", "laser_pointer")
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
# 1. Generate Pointer Components (Dots, Ring, Burst, Brackets)
# -------------------------------------------------------------
def generate_pointer_components():
    scale = 4

    # 1. Laser Dots (64x64 & 128x128)
    for mode in ["idle", "active"]:
        s = 128
        im = Image.new("RGBA", (s * scale, s * scale), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        cx, cy = (s * scale) // 2, (s * scale) // 2

        if mode == "idle":
            # Soft ambient cyan halo
            for r in range(40 * scale, 10 * scale, -2 * scale):
                alpha = int(80 * (1 - r / (40 * scale)))
                d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(44, 232, 228, alpha))
            # Core dot
            d.ellipse([cx - 10 * scale, cy - 10 * scale, cx + 10 * scale, cy + 10 * scale],
                      fill=(245, 255, 255, 255), outline=(44, 232, 228, 255), width=2 * scale)
        else: # active
            for r in range(55 * scale, 14 * scale, -2 * scale):
                alpha = int(140 * (1 - r / (55 * scale)))
                d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(120, 255, 250, alpha))
            d.ellipse([cx - 14 * scale, cy - 14 * scale, cx + 14 * scale, cy + 14 * scale],
                      fill=(255, 255, 255, 255), outline=(120, 255, 250, 255), width=3 * scale)

        final_128 = im.resize((128, 128), Image.Resampling.LANCZOS)
        final_64 = im.resize((64, 64), Image.Resampling.LANCZOS)
        final_128.save(os.path.join(PROC_DIR, f"laser_dot_{mode}_128px.png"), "PNG")
        final_64.save(os.path.join(PROC_DIR, f"laser_dot_{mode}_64px.png"), "PNG")

    # 2. Contextual Hover Ring (128x128)
    s = 128
    im_ring = Image.new("RGBA", (s * scale, s * scale), (0, 0, 0, 0))
    d_ring = ImageDraw.Draw(im_ring)
    cx, cy = (s * scale) // 2, (s * scale) // 2
    r_ring = 46 * scale

    # 4 Cardinal Arc Segments (gap notches at cardinals)
    d_ring.arc([cx - r_ring, cy - r_ring, cx + r_ring, cy + r_ring], 15, 75, fill=(44, 232, 228, 255), width=4 * scale)
    d_ring.arc([cx - r_ring, cy - r_ring, cx + r_ring, cy + r_ring], 105, 165, fill=(44, 232, 228, 255), width=4 * scale)
    d_ring.arc([cx - r_ring, cy - r_ring, cx + r_ring, cy + r_ring], 195, 255, fill=(44, 232, 228, 255), width=4 * scale)
    d_ring.arc([cx - r_ring, cy - r_ring, cx + r_ring, cy + r_ring], 285, 345, fill=(44, 232, 228, 255), width=4 * scale)

    # Tactical cross-ticks
    tick_len = 8 * scale
    d_ring.line([cx - r_ring - tick_len, cy, cx - r_ring + tick_len, cy], fill=(120, 255, 250, 255), width=2 * scale)
    d_ring.line([cx + r_ring - tick_len, cy, cx + r_ring + tick_len, cy], fill=(120, 255, 250, 255), width=2 * scale)
    d_ring.line([cx, cy - r_ring - tick_len, cx, cy - r_ring + tick_len], fill=(120, 255, 250, 255), width=2 * scale)
    d_ring.line([cx, cy + r_ring - tick_len, cx, cy + r_ring + tick_len], fill=(120, 255, 250, 255), width=2 * scale)

    im_ring.resize((128, 128), Image.Resampling.LANCZOS).save(os.path.join(PROC_DIR, "laser_hover_ring.png"), "PNG")

    # 3. Click Confirmation Burst (128x128)
    im_burst = Image.new("RGBA", (s * scale, s * scale), (0, 0, 0, 0))
    d_burst = ImageDraw.Draw(im_burst)
    r_burst = 50 * scale

    # Expanding shockwave ring
    d_burst.ellipse([cx - r_burst, cy - r_burst, cx + r_burst, cy + r_burst],
                    outline=(120, 255, 250, 240), width=3 * scale)
    # 8 Radial spark rays
    for a in range(0, 360, 45):
        rad = math.radians(a)
        r1 = 20 * scale
        r2 = 56 * scale
        d_burst.line([cx + r1 * math.cos(rad), cy + r1 * math.sin(rad),
                      cx + r2 * math.cos(rad), cy + r2 * math.sin(rad)], fill=(255, 255, 255, 255), width=2 * scale)
        d_burst.ellipse([cx + r2 * math.cos(rad) - 3 * scale, cy + r2 * math.sin(rad) - 3 * scale,
                         cx + r2 * math.cos(rad) + 3 * scale, cy + r2 * math.sin(rad) + 3 * scale],
                        fill=(44, 232, 228, 255))

    im_burst.resize((128, 128), Image.Resampling.LANCZOS).save(os.path.join(PROC_DIR, "laser_click_burst.png"), "PNG")

    # 4. Subtle Focus Brackets (64x64 each: TL, TR, BL, BR)
    bs = 64
    bracket_dirs = {
        "tl": (1, 1),
        "tr": (-1, 1),
        "bl": (1, -1),
        "br": (-1, -1)
    }
    for b_name, (dx, dy) in bracket_dirs.items():
        im_brk = Image.new("RGBA", (bs * scale, bs * scale), (0, 0, 0, 0))
        d_brk = ImageDraw.Draw(im_brk)

        ox = 10 * scale if dx == 1 else (bs - 10) * scale
        oy = 10 * scale if dy == 1 else (bs - 10) * scale
        arm_len = 36 * scale

        d_brk.line([ox, oy, ox + dx * arm_len, oy], fill=(44, 232, 228, 255), width=4 * scale)
        d_brk.line([ox, oy, ox, oy + dy * arm_len], fill=(44, 232, 228, 255), width=4 * scale)
        # Corner pip
        d_brk.rectangle([ox - 4 * scale, oy - 4 * scale, ox + 4 * scale, oy + 4 * scale], fill=(120, 255, 250, 255))

        im_brk.resize((bs, bs), Image.Resampling.LANCZOS).save(os.path.join(PROC_DIR, f"laser_focus_bracket_{b_name}.png"), "PNG")

    # 5. Disabled State Marker (128x128)
    im_dis = Image.new("RGBA", (s * scale, s * scale), (0, 0, 0, 0))
    d_dis = ImageDraw.Draw(im_dis)
    r_dis = 44 * scale
    # Red/amber circle with diagonal slash
    d_dis.ellipse([cx - r_dis, cy - r_dis, cx + r_dis, cy + r_dis], outline=(235, 45, 55, 240), width=4 * scale)
    d_dis.line([cx - r_dis * 0.7, cy - r_dis * 0.7, cx + r_dis * 0.7, cy + r_dis * 0.7], fill=(235, 45, 55, 240), width=4 * scale)
    im_dis.resize((128, 128), Image.Resampling.LANCZOS).save(os.path.join(PROC_DIR, "laser_disabled_marker.png"), "PNG")

    # 6. Composite Sheet (512x256)
    sheet_w, sheet_h = 512, 256
    sheet = Image.new("RGBA", (sheet_w, sheet_h), (0, 0, 0, 0))
    # Row 0 (128x128): dot_idle, dot_active, hover_ring, click_burst
    sheet.paste(final_128, (0, 0), final_128)
    sheet.paste(Image.open(os.path.join(PROC_DIR, "laser_dot_active_128px.png")), (128, 0))
    sheet.paste(Image.open(os.path.join(PROC_DIR, "laser_hover_ring.png")), (256, 0))
    sheet.paste(Image.open(os.path.join(PROC_DIR, "laser_click_burst.png")), (384, 0))
    # Row 1 (128x128): disabled marker, 4 brackets
    sheet.paste(Image.open(os.path.join(PROC_DIR, "laser_disabled_marker.png")), (0, 128))
    sheet.paste(Image.open(os.path.join(PROC_DIR, "laser_focus_bracket_tl.png")), (128, 128))
    sheet.paste(Image.open(os.path.join(PROC_DIR, "laser_focus_bracket_tr.png")), (192, 128))
    sheet.paste(Image.open(os.path.join(PROC_DIR, "laser_focus_bracket_bl.png")), (128, 192))
    sheet.paste(Image.open(os.path.join(PROC_DIR, "laser_focus_bracket_br.png")), (192, 192))
    sheet.save(os.path.join(PROC_DIR, "laser_pointer_sheet.png"), "PNG")

    print("Generated all laser pointer components & composite sheet.")

# -------------------------------------------------------------
# 2. Concept Mockup (1920x1080)
# -------------------------------------------------------------
def generate_pointer_mockup():
    w, h = 1920, 1080
    im = Image.new("RGBA", (w, h), (10, 14, 18, 255))
    d = ImageDraw.Draw(im)

    # Ambient lab background
    for r in range(h // 2, 0, -25):
        alpha = int(35 * (1 - r / (h // 2)))
        d.ellipse([w // 2 - r * 1.6, h // 2 - r, w // 2 + r * 1.6, h // 2 + r], fill=(16, 26, 36, alpha))

    # Grid overlay
    for gx in range(0, w, 80):
        d.line([gx, 0, gx, h], fill=(20, 30, 42, 60), width=1)
    for gy in range(0, h, 80):
        d.line([0, gy, w, gy], fill=(20, 30, 42, 60), width=1)

    f_head = get_font(26, bold=True)
    f_sub = get_font(14)
    f_card = get_font(18, bold=True)

    # Header
    d.rectangle([w // 2 - 380, 30, w // 2 + 380, 90], fill=(14, 20, 28, 230), outline=(65, 85, 102, 255), width=2)
    d.rectangle([w // 2 - 376, 34, w // 2 - 360, 86], fill=(44, 232, 228, 255))
    d.text((w // 2 - 340, 44), "НАБОР ОБРАТНОЙ СВЯЗИ УКАЗАТЕЛЯ // LASER POINTER FEEDBACK KIT", fill=(245, 250, 255, 255), font=f_head)
    d.text((w // 2 - 340, 70), "ДЕТЕКЦИЯ, ФОКУСИРОВКА, НАВЕДЕНИЕ И ПОДТВЕРЖДЕНИЕ НАЖАТИЯ", fill=(130, 160, 180, 220), font=f_sub)

    # Showcase 4 Stages across the screen:
    stages = [
        # Stage 1: Idle Ray & Dot
        (160, 320, "1. ТОЧКА НАВЕДЕНИЯ // IDLE RAY", "Свободное движение луча в меню", "laser_dot_idle_128px.png", None),
        # Stage 2: Contextual Hover & Brackets
        (580, 320, "2. ФОКУСИРОВКА // CONTEXTUAL HOVER", "Захват границы элемента интерфейса", "laser_hover_ring.png", "laser_dot_active_128px.png"),
        # Stage 3: Click Confirmation Pulse
        (1000, 320, "3. КЛИК // CONFIRMATION PULSE", "Регистрация нажатия курка R2", "laser_click_burst.png", "laser_dot_active_128px.png"),
        # Stage 4: Disabled / Invalid Target
        (1420, 320, "4. БЛОКИРОВКА // DISABLED TARGET", "Неактивная область или заблокировано", "laser_disabled_marker.png", None)
    ]

    for sx, sy, st_title, st_desc, comp_file, overlay_file in stages:
        # Card backing (360x480)
        d.rectangle([sx, sy, sx + 360, sy + 480], fill=(14, 20, 28, 240), outline=(50, 70, 90, 255), width=2)
        d.rectangle([sx + 8, sy + 8, sx + 352, sy + 50], fill=(20, 28, 38, 255))
        d.text((sx + 16, sy + 20), st_title, fill=(245, 250, 255, 255), font=f_card)

        # Inner display well (280x280)
        wx = sx + 40
        wy = sy + 80
        d.rectangle([wx, wy, wx + 280, wy + 280], fill=(8, 12, 16, 255), outline=(30, 42, 54, 255), width=2)

        # Place component image
        c_path = os.path.join(PROC_DIR, comp_file)
        if os.path.exists(c_path):
            c_im = Image.open(c_path).resize((180, 180), Image.Resampling.LANCZOS)
            im.paste(c_im, (wx + 50, wy + 50), c_im)

        if overlay_file:
            ov_path = os.path.join(PROC_DIR, overlay_file)
            if os.path.exists(ov_path):
                ov_im = Image.open(ov_path).resize((120, 120), Image.Resampling.LANCZOS)
                im.paste(ov_im, (wx + 80, wy + 80), ov_im)

        # Description
        d.text((sx + 20, sy + 390), st_desc, fill=(140, 170, 190, 220), font=f_sub)
        d.rectangle([sx + 20, sy + 430, sx + 340, sy + 460], fill=(18, 25, 34, 255), outline=(44, 232, 228, 200))
        d.text((sx + 180, sy + 445), "ОПТИМИЗИРОВАНО ДЛЯ PSVR2", fill=(44, 232, 228, 255), font=f_sub, anchor="mm")

    # Bottom PSVR2 Legend
    f_hint = get_font(16)
    d.rectangle([w // 2 - 400, h - 75, w // 2 + 400, h - 25], fill=(12, 17, 24, 240), outline=(45, 60, 75, 255), width=2)
    d.text((w // 2, h - 50),
           "ПЛАВНОЕ СГЛАЖИВАНИЕ И ОТСУТСТВИЕ МЕРЦАНИЯ В ЛИНЗАХ // ZERO JITTER VISUALS",
           fill=(225, 240, 250, 240), font=f_hint, anchor="mm")

    mockup_path = os.path.join(MOCKUP_DIR, "laser_pointer_feedback_mockup.png")
    im.save(mockup_path, "PNG")
    print(f"Generated Laser Pointer feedback mockup: {mockup_path}")

if __name__ == "__main__":
    generate_pointer_components()
    generate_pointer_mockup()
