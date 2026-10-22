"""
Singularity VR - UI Remaster: Utility HUD Support Graphics Generator
Generates SVG sources, transparent PNG exports, and concept mockup for Utility HUD.
"""

import os
import math
from PIL import Image, ImageDraw, ImageFont, ImageFilter

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SRC_DIR = os.path.join(BASE_DIR, "assets", "source", "ui_remaster", "utility_hud")
PROC_DIR = os.path.join(BASE_DIR, "assets", "processed", "ui_remaster", "utility_hud")
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
# 1. Generate Utility HUD Elements
# -------------------------------------------------------------
def generate_utility_hud_elements():
    scale = 3

    # 1. Ammo Plate (360x120)
    w, h = 360, 120
    im_ammo = Image.new("RGBA", (w * scale, h * scale), (0, 0, 0, 0))
    d_ammo = ImageDraw.Draw(im_ammo)
    pad = 4 * scale

    # Milled dark metal casing plate
    d_ammo.rectangle([pad, pad, (w - pad) * scale, (h - pad) * scale],
                     fill=(14, 18, 25, 240), outline=(65, 85, 102, 255), width=2 * scale)
    # Inner digital display well
    d_ammo.rectangle([pad + 8 * scale, pad + 8 * scale, (w - pad - 8) * scale, (h - pad - 8) * scale],
                     fill=(8, 12, 16, 255), outline=(30, 42, 54, 255), width=scale)

    # Cartridge silhouette & Caliber Tag
    f_cal = get_font(12 * scale)
    f_num = get_font(36 * scale, bold=True)
    f_res = get_font(20 * scale, bold=True)
    d_ammo.text((pad + 18 * scale, pad + 14 * scale), "КАЛИБР: 7.62x39mm // ВОЛК S-4", fill=(130, 160, 180, 220), font=f_cal)

    # Ammo Count Numbers
    d_ammo.text((pad + 20 * scale, pad + 38 * scale), "30", fill=(44, 232, 228, 255), font=f_num)
    d_ammo.text((pad + 110 * scale, pad + 54 * scale), "/", fill=(75, 95, 115, 255), font=f_res)
    d_ammo.text((pad + 130 * scale, pad + 54 * scale), "180", fill=(225, 240, 250, 240), font=f_res)

    # Ammo Gauge Segmented Bar on Right
    bar_x = (w - pad - 140) * scale
    for i in range(10):
        bx = bar_x + i * 12 * scale
        fill_c = (44, 232, 228, 255) if i < 8 else (35, 50, 65, 200)
        d_ammo.rectangle([bx, (h - pad - 45) * scale, bx + 8 * scale, (h - pad - 18) * scale], fill=fill_c)

    im_ammo.resize((w, h), Image.Resampling.LANCZOS).save(os.path.join(PROC_DIR, "ammo_plate.png"), "PNG")

    # 2. Health & Warning Indicator (360x120)
    im_hlth = Image.new("RGBA", (w * scale, h * scale), (0, 0, 0, 0))
    d_hlth = ImageDraw.Draw(im_hlth)
    d_hlth.rectangle([pad, pad, (w - pad) * scale, (h - pad) * scale],
                     fill=(14, 18, 25, 240), outline=(65, 85, 102, 255), width=2 * scale)
    d_hlth.rectangle([pad + 8 * scale, pad + 8 * scale, (w - pad - 8) * scale, (h - pad - 8) * scale],
                     fill=(8, 12, 16, 255), outline=(30, 42, 54, 255), width=scale)

    f_bio = get_font(12 * scale)
    f_stat = get_font(26 * scale, bold=True)
    d_hlth.text((pad + 18 * scale, pad + 14 * scale), "БИО-МОНИТОР // ОБЪЕКТ-99", fill=(130, 160, 180, 220), font=f_bio)

    # ECG Cathode Pulse Wave
    ecg_pts = [
        (pad + 18 * scale, (h // 2 + 10) * scale),
        (pad + 60 * scale, (h // 2 + 10) * scale),
        (pad + 75 * scale, (h // 2 - 25) * scale),
        (pad + 85 * scale, (h // 2 + 30) * scale),
        (pad + 95 * scale, (h // 2 - 15) * scale),
        (pad + 105 * scale, (h // 2 + 10) * scale),
        (pad + 160 * scale, (h // 2 + 10) * scale)
    ]
    d_hlth.line(ecg_pts, fill=(44, 232, 228, 255), width=3 * scale)

    # Bio Health Value
    d_hlth.text(((w - pad - 120) * scale, pad + 40 * scale), "100%", fill=(120, 255, 250, 255), font=f_stat)
    d_hlth.text(((w - pad - 120) * scale, pad + 78 * scale), "СОСТОЯНИЕ СТАБИЛЬНО", fill=(44, 232, 228, 220), font=f_bio)

    im_hlth.resize((w, h), Image.Resampling.LANCZOS).save(os.path.join(PROC_DIR, "health_warning_indicator.png"), "PNG")

    # 3. Objective Cue Marker (128x128)
    ow, oh = 128, 128
    im_obj = Image.new("RGBA", (ow * scale, oh * scale), (0, 0, 0, 0))
    d_obj = ImageDraw.Draw(im_obj)
    ocx, ocy = (ow * scale) // 2, (oh * scale) // 2
    r_obj = 42 * scale

    # Tactical Waypoint Diamond
    d_obj.polygon([(ocx, ocy - r_obj), (ocx + r_obj, ocy), (ocx, ocy + r_obj), (ocx - r_obj, ocy)],
                  outline=(255, 169, 39, 255), fill=(24, 30, 40, 220))
    d_obj.polygon([(ocx, ocy - r_obj + 8 * scale), (ocx + r_obj - 8 * scale, ocy),
                   (ocx, ocy + r_obj - 8 * scale), (ocx - r_obj + 8 * scale, ocy)],
                  outline=(255, 210, 110, 255), fill=(35, 45, 55, 200))
    # Pulsing E99 Core
    d_obj.ellipse([ocx - 10 * scale, ocy - 10 * scale, ocx + 10 * scale, ocy + 10 * scale], fill=(44, 232, 228, 255))
    # Downward indicator arrow
    d_obj.polygon([(ocx, ocy + r_obj + 16 * scale), (ocx - 10 * scale, ocy + r_obj + 4 * scale),
                   (ocx + 10 * scale, ocy + r_obj + 4 * scale)], fill=(255, 169, 39, 255))

    im_obj.resize((ow, oh), Image.Resampling.LANCZOS).save(os.path.join(PROC_DIR, "objective_cue_marker.png"), "PNG")

    # 4. Panel Corner Ornaments (256x256 Sheet)
    cw, ch = 256, 256
    im_corn = Image.new("RGBA", (cw * scale, ch * scale), (0, 0, 0, 0))
    d_corn = ImageDraw.Draw(im_corn)
    # 4 Quadrants: Hex-bolted, Riveted Flange, Chamfered Steel, Serial Stamp
    # Q1: Hex-bolted
    d_corn.polygon([(10 * scale, 10 * scale), (100 * scale, 10 * scale), (100 * scale, 40 * scale),
                    (40 * scale, 100 * scale), (10 * scale, 100 * scale)], fill=(35, 48, 62, 255), outline=(75, 95, 115, 255), width=2 * scale)
    d_corn.ellipse([25 * scale, 25 * scale, 45 * scale, 45 * scale], fill=(55, 75, 95, 255), outline=(120, 150, 180, 255), width=scale)
    # Q2: Riveted
    d_corn.polygon([(150 * scale, 10 * scale), (240 * scale, 10 * scale), (240 * scale, 100 * scale),
                    (210 * scale, 100 * scale), (150 * scale, 40 * scale)], fill=(30, 42, 54, 255), outline=(70, 90, 110, 255), width=2 * scale)
    d_corn.ellipse([220 * scale, 25 * scale, 232 * scale, 37 * scale], fill=(80, 105, 125, 255))
    d_corn.ellipse([220 * scale, 55 * scale, 232 * scale, 67 * scale], fill=(80, 105, 125, 255))
    d_corn.ellipse([220 * scale, 85 * scale, 232 * scale, 97 * scale], fill=(80, 105, 125, 255))
    im_corn.resize((cw, ch), Image.Resampling.LANCZOS).save(os.path.join(PROC_DIR, "panel_corner_ornaments.png"), "PNG")

    # 5. Warning Hazard Stripes (600x32)
    ww, wh = 600, 32
    im_wrn = Image.new("RGBA", (ww * scale, wh * scale), (0, 0, 0, 0))
    d_wrn = ImageDraw.Draw(im_wrn)
    d_wrn.rectangle([0, 0, ww * scale, wh * scale], fill=(16, 22, 30, 255), outline=(50, 70, 90, 255), width=scale)
    for z in range(-wh * scale, ww * scale, 18 * scale):
        d_wrn.polygon([(z, 0), (z + 10 * scale, 0), (z + (wh + 10) * scale, wh * scale), (z + wh * scale, wh * scale)],
                      fill=(245, 195, 35, 240))
    im_wrn.resize((ww, wh), Image.Resampling.LANCZOS).save(os.path.join(PROC_DIR, "warning_stripes.png"), "PNG")

    # 6. E99 Status Frame (240x320)
    ew, eh = 240, 320
    im_e99 = Image.new("RGBA", (ew * scale, eh * scale), (0, 0, 0, 0))
    d_e99 = ImageDraw.Draw(im_e99)
    d_e99.rectangle([pad, pad, (ew - pad) * scale, (eh - pad) * scale],
                    fill=(12, 16, 22, 240), outline=(65, 85, 102, 255), width=2 * scale)
    # Cathode tube chamber
    d_e99.rectangle([pad + 12 * scale, pad + 12 * scale, (ew - pad - 12) * scale, (eh - pad - 12) * scale],
                    fill=(6, 10, 14, 255), outline=(30, 45, 58, 255), width=scale)
    # E99 Energy Core Glow
    ecy = (eh * scale) // 2
    for r in range(60 * scale, 15 * scale, -4 * scale):
        alpha = int(120 * (1 - r / (60 * scale)))
        d_e99.ellipse([(ew * scale) // 2 - r, ecy - r, (ew * scale) // 2 + r, ecy + r], fill=(44, 232, 228, alpha))
    d_e99.ellipse([(ew * scale) // 2 - 15 * scale, ecy - 15 * scale, (ew * scale) // 2 + 15 * scale, ecy + 15 * scale],
                  fill=(120, 255, 250, 255))
    f_e99 = get_font(14 * scale, bold=True)
    d_e99.text(((ew * scale) // 2, (eh - pad - 35) * scale), "ЗАРЯД TMD // 100%", fill=(44, 232, 228, 240), font=f_e99, anchor="mm")
    im_e99.resize((ew, eh), Image.Resampling.LANCZOS).save(os.path.join(PROC_DIR, "e99_status_frame.png"), "PNG")

    print("Generated all Utility HUD elements.")

# -------------------------------------------------------------
# 2. Concept Mockup (1920x1080)
# -------------------------------------------------------------
def generate_utility_hud_mockup():
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

    f_title = get_font(26, bold=True)
    f_sub = get_font(14)
    f_card = get_font(18, bold=True)

    # Header
    d.rectangle([w // 2 - 380, 30, w // 2 + 380, 90], fill=(14, 20, 28, 230), outline=(65, 85, 102, 255), width=2)
    d.rectangle([w // 2 - 376, 34, w // 2 - 360, 86], fill=(44, 232, 228, 255))
    d.text((w // 2 - 340, 44), "СИСТЕМА ВСПОМОГАТЕЛЬНОГО HUD // UTILITY HUD GRAPHICS", fill=(245, 250, 255, 255), font=f_title)
    d.text((w // 2 - 340, 70), "МОДУЛЬНЫЕ ЭЛЕМЕНТЫ: БОЕПРИПАСЫ, ЗДОРОВЬЕ, ЦЕЛИ, РАЗДЕЛИТЕЛИ", fill=(130, 160, 180, 220), font=f_sub)

    # Place Ammo Plate (Bottom Right)
    ammo_im = Image.open(os.path.join(PROC_DIR, "ammo_plate.png"))
    im.paste(ammo_im, (w - 440, h - 220), ammo_im)
    d.text((w - 260, h - 235), "ПЛАШКА БОЕПРИПАСОВ (AMMO PLATE)", fill=(44, 232, 228, 230), font=f_sub, anchor="mm")

    # Place Health Monitor (Bottom Left)
    hlth_im = Image.open(os.path.join(PROC_DIR, "health_warning_indicator.png"))
    im.paste(hlth_im, (80, h - 220), hlth_im)
    d.text((260, h - 235), "БИО-МОНИТОР ЗДОРОВЬЯ (HEALTH MONITOR)", fill=(44, 232, 228, 230), font=f_sub, anchor="mm")

    # Place Objective Marker (Center Floating)
    obj_im = Image.open(os.path.join(PROC_DIR, "objective_cue_marker.png"))
    im.paste(obj_im, (w // 2 - 64, 300), obj_im)
    d.text((w // 2, 440), "ТАКТИЧЕСКИЙ МАРКЕР ЦЕЛИ // WAYPOINT (24m)", fill=(255, 169, 39, 255), font=f_card, anchor="mm")

    # Place E99 Status Frame (Left Center)
    e99_im = Image.open(os.path.join(PROC_DIR, "e99_status_frame.png"))
    im.paste(e99_im, (140, 360), e99_im)
    d.text((260, 340), "TMD ЭНЕРГО-ИНДИКАТОР", fill=(44, 232, 228, 230), font=f_sub, anchor="mm")

    # Place Hazard Stripes (Top & Bottom dividers)
    wrn_im = Image.open(os.path.join(PROC_DIR, "warning_stripes.png")).resize((800, 24), Image.Resampling.LANCZOS)
    im.paste(wrn_im, (w // 2 - 400, 120), wrn_im)
    im.paste(wrn_im, (w // 2 - 400, h - 120), wrn_im)

    # Bottom PSVR2 Legend
    f_hint = get_font(16)
    d.rectangle([w // 2 - 420, h - 75, w // 2 + 420, h - 25], fill=(12, 17, 24, 240), outline=(45, 60, 75, 255), width=2)
    d.text((w // 2, h - 50),
           "НЕИНВАЗИВНЫЙ ДИЕТЕТИЧЕСКИЙ ДИЗАЙН // CLEAN DIEGETIC-COMPATIBLE VR OVERLAYS",
           fill=(225, 240, 250, 240), font=f_hint, anchor="mm")

    mockup_path = os.path.join(MOCKUP_DIR, "utility_hud_support_mockup.png")
    im.save(mockup_path, "PNG")
    print(f"Generated Utility HUD concept mockup: {mockup_path}")

if __name__ == "__main__":
    generate_utility_hud_elements()
    generate_utility_hud_mockup()
