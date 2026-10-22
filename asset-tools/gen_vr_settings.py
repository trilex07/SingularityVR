"""
Singularity VR - UI Remaster: VR Settings Visual Refresh Generator
Generates SVG sources, transparent PNG exports, and full screen mockup for VR Settings.
"""

import os
import math
from PIL import Image, ImageDraw, ImageFont, ImageFilter

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SRC_DIR = os.path.join(BASE_DIR, "assets", "source", "ui_remaster", "vr_settings")
PROC_DIR = os.path.join(BASE_DIR, "assets", "processed", "ui_remaster", "vr_settings")
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
# 1. Panel Background Plate & 9-Slice Corners (1200x900)
# -------------------------------------------------------------
def generate_panel_background():
    w, h = 1200, 900
    scale = 2
    sw, sh = w * scale, h * scale
    im = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)

    pad = 20 * scale
    # Main dark steel plate
    d.rectangle([pad, pad, sw - pad, sh - pad], fill=(12, 17, 24, 240), outline=(65, 85, 102, 255), width=3 * scale)
    # Inner bevel
    d.rectangle([pad + 8 * scale, pad + 8 * scale, sw - pad - 8 * scale, sh - pad - 8 * scale],
                outline=(30, 42, 54, 255), width=2 * scale)

    # 4 Corner reinforcement brackets with hex bolts
    corn_size = 80 * scale
    bolt_r = 8 * scale
    # Top-Left
    d.polygon([(pad, pad), (pad + corn_size, pad), (pad + corn_size, pad + 30 * scale),
               (pad + 30 * scale, pad + corn_size), (pad, pad + corn_size)], fill=(32, 44, 58, 255))
    d.ellipse([pad + 18 * scale - bolt_r, pad + 18 * scale - bolt_r, pad + 18 * scale + bolt_r, pad + 18 * scale + bolt_r],
              fill=(55, 75, 95, 255), outline=(110, 140, 165, 255), width=scale)
    # Top-Right
    d.polygon([(sw - pad, pad), (sw - pad - corn_size, pad), (sw - pad - corn_size, pad + 30 * scale),
               (sw - pad - 30 * scale, pad + corn_size), (sw - pad, pad + corn_size)], fill=(32, 44, 58, 255))
    d.ellipse([sw - pad - 18 * scale - bolt_r, pad + 18 * scale - bolt_r, sw - pad - 18 * scale + bolt_r, pad + 18 * scale + bolt_r],
              fill=(55, 75, 95, 255), outline=(110, 140, 165, 255), width=scale)
    # Bottom-Left
    d.polygon([(pad, sh - pad), (pad + corn_size, sh - pad), (pad + corn_size, sh - pad - 30 * scale),
               (pad + 30 * scale, sh - pad - corn_size), (pad, sh - pad - corn_size)], fill=(32, 44, 58, 255))
    d.ellipse([pad + 18 * scale - bolt_r, sh - pad - 18 * scale - bolt_r, pad + 18 * scale + bolt_r, sh - pad - 18 * scale + bolt_r],
              fill=(55, 75, 95, 255), outline=(110, 140, 165, 255), width=scale)
    # Bottom-Right
    d.polygon([(sw - pad, sh - pad), (sw - pad - corn_size, sh - pad), (sw - pad - corn_size, sh - pad - 30 * scale),
               (sw - pad - 30 * scale, sh - pad - corn_size), (sw - pad, sh - pad - corn_size)], fill=(32, 44, 58, 255))
    d.ellipse([sw - pad - 18 * scale - bolt_r, sh - pad - 18 * scale - bolt_r, sw - pad - 18 * scale + bolt_r, sh - pad - 18 * scale + bolt_r],
              fill=(55, 75, 95, 255), outline=(110, 140, 165, 255), width=scale)

    # Top header bar backing
    d.rectangle([pad + 20 * scale, pad + 20 * scale, sw - pad - 20 * scale, pad + 75 * scale],
                fill=(18, 26, 36, 255), outline=(45, 60, 75, 255), width=2 * scale)
    # Warning chevron hazard strip on left header
    for z in range(0, 100 * scale, 16 * scale):
        d.polygon([(pad + 20 * scale + z, pad + 20 * scale),
                   (pad + 32 * scale + z, pad + 20 * scale),
                   (pad + 24 * scale + z, pad + 75 * scale),
                   (pad + 12 * scale + z, pad + 75 * scale)], fill=(245, 195, 35, 220))

    # Stenciled Titles
    f_title = get_font(24 * scale, bold=True)
    f_ser = get_font(12 * scale)
    d.text((pad + 130 * scale, pad + 34 * scale), "ПАРАМЕТРЫ VR // VR SYSTEM CONFIGURATION", fill=(240, 248, 255, 255), font=f_title)
    d.text((sw - pad - 30 * scale, pad + 40 * scale), "КАТОРГА-12 // ОБЪЕКТ-99", fill=(44, 232, 228, 240), font=f_ser, anchor="ra")

    # Bottom footer line
    d.text((sw // 2, sh - pad - 16 * scale), "СИСТЕМА КАЛИБРОВКИ // ГОСТ-1955 // ПОЛНЫЙ КОНТРОЛЬ ПАРАМЕТРОВ",
           fill=(110, 135, 155, 200), font=f_ser, anchor="mm")

    # Downsample
    final_im = im.resize((w, h), Image.Resampling.LANCZOS)
    png_path = os.path.join(PROC_DIR, "panel_background_plate.png")
    final_im.save(png_path, "PNG")

    # Generate 9-slice modular corners (128x128 each)
    c_size = 128
    corners = {
        "panel_corner_tl.png": (0, 0),
        "panel_corner_tr.png": (w - c_size, 0),
        "panel_corner_bl.png": (0, h - c_size),
        "panel_corner_br.png": (w - c_size, h - c_size)
    }
    for c_file, (cx, cy) in corners.items():
        crop_im = final_im.crop((cx, cy, cx + c_size, cy + c_size))
        crop_path = os.path.join(PROC_DIR, c_file)
        crop_im.save(crop_path, "PNG")

    # Modular borders (128x32 horizontal, 32x128 vertical)
    border_h = final_im.crop((w // 2 - 64, 0, w // 2 + 64, 32))
    border_h.save(os.path.join(PROC_DIR, "panel_border_h.png"), "PNG")
    border_v = final_im.crop((0, h // 2 - 64, 32, h // 2 + 64))
    border_v.save(os.path.join(PROC_DIR, "panel_border_v.png"), "PNG")

    print(f"Generated: {png_path} and 9-slice components")

# -------------------------------------------------------------
# 2. Section Header Bar (800x64)
# -------------------------------------------------------------
def generate_section_header():
    w, h = 800, 64
    scale = 2
    sw, sh = w * scale, h * scale
    im = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)

    # Base heavy metal casing
    d.rectangle([0, 0, sw, sh], fill=(18, 25, 35, 250), outline=(55, 75, 95, 255), width=2 * scale)
    # Hazard tab on left
    for z in range(0, 48 * scale, 12 * scale):
        d.polygon([(z, 0), (z + 8 * scale, 0), (z, sh), (max(0, z - 8 * scale), sh)], fill=(245, 195, 35, 240))

    # Cyan accent underline
    d.line([0, sh - 3 * scale, sw, sh - 3 * scale], fill=(44, 232, 228, 255), width=3 * scale)

    # Title text
    f_head = get_font(20 * scale, bold=True)
    f_sub = get_font(12 * scale)
    d.text((64 * scale, 14 * scale), "КОНТРОЛЛЕР И УПРАВЛЕНИЕ // CONTROLLER & MOTION", fill=(240, 248, 255, 255), font=f_head)
    d.text((sw - 20 * scale, 20 * scale), "РАЗДЕЛ [01 / 10]", fill=(44, 232, 228, 230), font=f_sub, anchor="ra")

    final_im = im.resize((w, h), Image.Resampling.LANCZOS)
    png_path = os.path.join(PROC_DIR, "section_header_bar.png")
    final_im.save(png_path, "PNG")
    print(f"Generated: {png_path}")

# -------------------------------------------------------------
# 3. Option Rows (800x56 x 4 states)
# -------------------------------------------------------------
def generate_option_rows():
    w, h = 800, 56
    scale = 2
    sw, sh = w * scale, h * scale

    states = {
        "default": {
            "fill": (15, 21, 29, 210),
            "border": (45, 60, 75, 220),
            "border_w": scale,
            "glow": False
        },
        "hover": {
            "fill": (20, 50, 68, 240),
            "border": (44, 232, 228, 255),
            "border_w": 2 * scale,
            "glow": True
        },
        "selected": {
            "fill": (18, 70, 90, 250),
            "border": (120, 255, 250, 255),
            "border_w": 3 * scale,
            "glow": True
        },
        "disabled": {
            "fill": (12, 15, 20, 140),
            "border": (35, 45, 55, 160),
            "border_w": scale,
            "glow": False
        }
    }

    for s_name, cfg in states.items():
        im = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)

        # Base rect
        d.rectangle([2 * scale, 2 * scale, sw - 2 * scale, sh - 2 * scale],
                    fill=cfg["fill"], outline=cfg["border"], width=cfg["border_w"])

        # Inner subtle divider
        d.line([2 * scale, sh - 2 * scale, sw - 2 * scale, sh - 2 * scale], fill=(30, 42, 54, 200), width=scale)

        # Tactical corner registration pips for hover & selected
        if cfg["glow"]:
            pip_col = (120, 255, 250, 255) if s_name == "selected" else (44, 232, 228, 255)
            # Left bracket
            d.line([4 * scale, 6 * scale, 14 * scale, 6 * scale], fill=pip_col, width=2 * scale)
            d.line([4 * scale, 6 * scale, 4 * scale, 16 * scale], fill=pip_col, width=2 * scale)
            d.line([4 * scale, sh - 6 * scale, 14 * scale, sh - 6 * scale], fill=pip_col, width=2 * scale)
            d.line([4 * scale, sh - 6 * scale, 4 * scale, sh - 16 * scale], fill=pip_col, width=2 * scale)
            # Right bracket
            d.line([sw - 4 * scale, 6 * scale, sw - 14 * scale, 6 * scale], fill=pip_col, width=2 * scale)
            d.line([sw - 4 * scale, 6 * scale, sw - 4 * scale, 16 * scale], fill=pip_col, width=2 * scale)
            d.line([sw - 4 * scale, sh - 6 * scale, sw - 14 * scale, sh - 6 * scale], fill=pip_col, width=2 * scale)
            d.line([sw - 4 * scale, sh - 6 * scale, sw - 4 * scale, sh - 16 * scale], fill=pip_col, width=2 * scale)

        final_im = im.resize((w, h), Image.Resampling.LANCZOS)
        png_path = os.path.join(PROC_DIR, f"row_bg_{s_name}.png")
        final_im.save(png_path, "PNG")

    print("Generated all 4 option row states.")

# -------------------------------------------------------------
# 4. Toggle Switch (120x48 x OFF & ON)
# -------------------------------------------------------------
def generate_toggle_switch():
    w, h = 120, 48
    scale = 3
    sw, sh = w * scale, h * scale

    for mode in ["off", "on"]:
        im = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)

        # Recessed industrial housing socket
        pad = 4 * scale
        d.rounded_rectangle([pad, pad, sw - pad, sh - pad], radius=10 * scale,
                            fill=(14, 18, 24, 255), outline=(55, 75, 95, 255), width=2 * scale)

        # Track bevel
        d.rounded_rectangle([pad + 4 * scale, pad + 4 * scale, sw - pad - 4 * scale, sh - pad - 4 * scale],
                            radius=8 * scale, fill=(8, 12, 16, 255), outline=(25, 35, 45, 255), width=scale)

        f_txt = get_font(10 * scale, bold=True)

        if mode == "off":
            # Toggle lever thrown left
            thumb_w = 48 * scale
            tx = pad + 6 * scale
            d.rounded_rectangle([tx, pad + 6 * scale, tx + thumb_w, sh - pad - 6 * scale],
                                radius=6 * scale, fill=(35, 48, 62, 255), outline=(75, 95, 115, 255), width=2 * scale)
            # Lever knurling
            d.line([tx + thumb_w // 2, pad + 10 * scale, tx + thumb_w // 2, sh - pad - 10 * scale], fill=(60, 80, 100, 255), width=2 * scale)
            # Amber inactive pip
            d.ellipse([sw - 28 * scale, sh // 2 - 5 * scale, sw - 18 * scale, sh // 2 + 5 * scale], fill=(60, 40, 20, 255))
            d.text((sw - 40 * scale, sh // 2), "ВЫКЛ", fill=(100, 120, 140, 200), font=f_txt, anchor="rm")
        else:
            # Toggle lever thrown right
            thumb_w = 48 * scale
            tx = sw - pad - 6 * scale - thumb_w
            d.rounded_rectangle([tx, pad + 6 * scale, tx + thumb_w, sh - pad - 6 * scale],
                                radius=6 * scale, fill=(45, 65, 85, 255), outline=(44, 232, 228, 255), width=2 * scale)
            d.line([tx + thumb_w // 2, pad + 10 * scale, tx + thumb_w // 2, sh - pad - 10 * scale], fill=(120, 255, 250, 255), width=2 * scale)
            # Glowing E99 cyan indicator
            d.ellipse([18 * scale, sh // 2 - 6 * scale, 30 * scale, sh // 2 + 6 * scale],
                      fill=(120, 255, 250, 255), outline=(44, 232, 228, 255), width=scale)
            d.text((40 * scale, sh // 2), "ВКЛ", fill=(44, 232, 228, 255), font=f_txt, anchor="lm")

        final_im = im.resize((w, h), Image.Resampling.LANCZOS)
        png_path = os.path.join(PROC_DIR, f"toggle_switch_{mode}.png")
        final_im.save(png_path, "PNG")

    print("Generated toggle switches (off, on).")

# -------------------------------------------------------------
# 5. Sliders (Track Empty, Track Fill, Knob Normal, Knob Hover)
# -------------------------------------------------------------
def generate_sliders():
    # 1. Slider Track (320x32)
    tw, th = 320, 32
    scale = 3
    stw, sth = tw * scale, th * scale

    # Empty Track
    im_empty = Image.new("RGBA", (stw, sth), (0, 0, 0, 0))
    d_empty = ImageDraw.Draw(im_empty)
    # Recessed track groove
    groove_h = 10 * scale
    gy = (sth - groove_h) // 2
    d_empty.rounded_rectangle([4 * scale, gy, stw - 4 * scale, gy + groove_h],
                              radius=4 * scale, fill=(10, 14, 20, 255), outline=(45, 60, 75, 255), width=scale)
    # Millimeter graduation ticks
    for tick_x in range(12 * scale, stw - 12 * scale, 16 * scale):
        d_empty.line([tick_x, gy - 4 * scale, tick_x, gy], fill=(70, 95, 115, 200), width=scale)
    final_empty = im_empty.resize((tw, th), Image.Resampling.LANCZOS)
    final_empty.save(os.path.join(PROC_DIR, "slider_track_empty.png"), "PNG")

    # Filled Track
    im_fill = Image.new("RGBA", (stw, sth), (0, 0, 0, 0))
    d_fill = ImageDraw.Draw(im_fill)
    d_fill.rounded_rectangle([4 * scale, gy, stw - 4 * scale, gy + groove_h],
                             radius=4 * scale, fill=(10, 14, 20, 255), outline=(45, 60, 75, 255), width=scale)
    # Segmented E99 cyan phosphor fill
    fill_end = int(stw * 0.65)
    d_fill.rounded_rectangle([4 * scale, gy + scale, fill_end, gy + groove_h - scale],
                             radius=3 * scale, fill=(44, 232, 228, 240), outline=(120, 255, 250, 255), width=scale)
    for tick_x in range(12 * scale, stw - 12 * scale, 16 * scale):
        d_fill.line([tick_x, gy - 4 * scale, tick_x, gy], fill=(70, 95, 115, 200), width=scale)
    final_fill = im_fill.resize((tw, th), Image.Resampling.LANCZOS)
    final_fill.save(os.path.join(PROC_DIR, "slider_track_fill.png"), "PNG")

    # 2. Slider Knobs (32x48)
    kw, kh = 32, 48
    skw, skh = kw * scale, kh * scale

    for k_mode in ["normal", "hover"]:
        im_k = Image.new("RGBA", (skw, skh), (0, 0, 0, 0))
        d_k = ImageDraw.Draw(im_k)

        pad = 3 * scale
        fill_col = (35, 48, 62, 255) if k_mode == "normal" else (24, 65, 82, 255)
        out_col = (75, 95, 115, 255) if k_mode == "normal" else (44, 232, 228, 255)
        w_stroke = 2 * scale if k_mode == "normal" else 3 * scale

        # Milled knurled handle shape
        d_k.rounded_rectangle([pad, pad, skw - pad, skh - pad], radius=6 * scale,
                              fill=fill_col, outline=out_col, width=w_stroke)

        # Center index line
        line_col = (120, 255, 250, 255) if k_mode == "hover" else (140, 175, 200, 255)
        d_k.line([skw // 2, pad + 6 * scale, skw // 2, skh - pad - 6 * scale], fill=line_col, width=2 * scale)

        # Tactile ridges
        d_k.line([skw // 2 - 6 * scale, pad + 10 * scale, skw // 2 - 6 * scale, skh - pad - 10 * scale], fill=(50, 70, 90, 200), width=scale)
        d_k.line([skw // 2 + 6 * scale, pad + 10 * scale, skw // 2 + 6 * scale, skh - pad - 10 * scale], fill=(50, 70, 90, 200), width=scale)

        final_k = im_k.resize((kw, kh), Image.Resampling.LANCZOS)
        final_k.save(os.path.join(PROC_DIR, f"slider_knob_{k_mode}.png"), "PNG")

    print("Generated slider assets.")

# -------------------------------------------------------------
# 6. Scroll Indicators & Industrial Divider
# -------------------------------------------------------------
def generate_scroll_and_dividers():
    scale = 3

    # Up / Down Chevron Arrows (40x40)
    for arr in ["up", "down"]:
        w, h = 40, 40
        im = Image.new("RGBA", (w * scale, h * scale), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        cx, cy = (w * scale) // 2, (h * scale) // 2

        # Housing box
        d.rectangle([2 * scale, 2 * scale, w * scale - 2 * scale, h * scale - 2 * scale],
                    fill=(18, 25, 34, 250), outline=(55, 75, 95, 255), width=scale)
        # Chevron points
        if arr == "up":
            d.polygon([(cx, cy - 8 * scale), (cx - 10 * scale, cy + 6 * scale),
                       (cx - 6 * scale, cy + 6 * scale), (cx, cy - 2 * scale),
                       (cx + 6 * scale, cy + 6 * scale), (cx + 10 * scale, cy + 6 * scale)],
                      fill=(44, 232, 228, 255))
        else:
            d.polygon([(cx, cy + 8 * scale), (cx - 10 * scale, cy - 6 * scale),
                       (cx - 6 * scale, cy - 6 * scale), (cx, cy + 2 * scale),
                       (cx + 6 * scale, cy - 6 * scale), (cx + 10 * scale, cy - 6 * scale)],
                      fill=(44, 232, 228, 255))

        final_im = im.resize((w, h), Image.Resampling.LANCZOS)
        final_im.save(os.path.join(PROC_DIR, f"scroll_arrow_{arr}.png"), "PNG")

    # Scroll Track (24x400) & Thumb (24x80)
    tw, th = 24, 400
    im_trk = Image.new("RGBA", (tw * scale, th * scale), (0, 0, 0, 0))
    d_trk = ImageDraw.Draw(im_trk)
    d_trk.rectangle([4 * scale, 0, (tw - 4) * scale, th * scale],
                    fill=(10, 14, 20, 255), outline=(35, 48, 60, 255), width=scale)
    im_trk.resize((tw, th), Image.Resampling.LANCZOS).save(os.path.join(PROC_DIR, "scroll_track.png"), "PNG")

    bw, bh = 24, 80
    im_thb = Image.new("RGBA", (bw * scale, bh * scale), (0, 0, 0, 0))
    d_thb = ImageDraw.Draw(im_thb)
    d_thb.rounded_rectangle([2 * scale, 2 * scale, (bw - 2) * scale, (bh - 2) * scale],
                            radius=4 * scale, fill=(35, 48, 62, 255), outline=(75, 95, 115, 255), width=scale)
    for ry in range(25 * scale, 55 * scale, 8 * scale):
        d_thb.line([6 * scale, ry, (bw - 6) * scale, ry], fill=(120, 255, 250, 255), width=scale)
    im_thb.resize((bw, bh), Image.Resampling.LANCZOS).save(os.path.join(PROC_DIR, "scroll_thumb.png"), "PNG")

    # Industrial Divider (800x16)
    dw, dh = 800, 16
    im_div = Image.new("RGBA", (dw * scale, dh * scale), (0, 0, 0, 0))
    d_div = ImageDraw.Draw(im_div)
    mid_y = (dh * scale) // 2
    # Perforated line
    for px in range(0, dw * scale, 20 * scale):
        d_div.line([px, mid_y, px + 12 * scale, mid_y], fill=(65, 85, 105, 200), width=scale)
    # Center diamond
    dcx = (dw * scale) // 2
    d_div.polygon([(dcx, mid_y - 5 * scale), (dcx + 8 * scale, mid_y), (dcx, mid_y + 5 * scale), (dcx - 8 * scale, mid_y)],
                  fill=(44, 232, 228, 255))
    im_div.resize((dw, dh), Image.Resampling.LANCZOS).save(os.path.join(PROC_DIR, "divider_industrial.png"), "PNG")

    print("Generated scroll and divider assets.")

# -------------------------------------------------------------
# 7. VR Settings Full Screen Concept Mockup (1920x1080)
# -------------------------------------------------------------
def generate_vr_settings_mockup():
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

    # Place Main VR Settings Plate (1200x820)
    panel_w, panel_h = 1200, 820
    panel_x = (w - panel_w) // 2
    panel_y = (h - panel_h) // 2 - 20

    bg_plate = Image.open(os.path.join(PROC_DIR, "panel_background_plate.png")).resize((panel_w, panel_h), Image.Resampling.LANCZOS)
    im.paste(bg_plate, (panel_x, panel_y), bg_plate)

    # Place Section Header
    header_im = Image.open(os.path.join(PROC_DIR, "section_header_bar.png")).resize((840, 56), Image.Resampling.LANCZOS)
    im.paste(header_im, (panel_x + 60, panel_y + 90), header_im)

    # Place Tab Bar on Left Column (Submenu categories)
    tabs = [
        ("VR SETTINGS", True),
        ("CONTROLLER", False),
        ("COMFORT", False),
        ("DISPLAY", False),
        ("ADVANCED", False),
        ("WEAPON ALIGN", False),
        ("TMD ALIGN", False),
        ("HANDS", False)
    ]
    f_tab = get_font(15, bold=True)
    tab_y = panel_y + 160
    for t_name, is_act in tabs:
        t_col = (20, 75, 95, 255) if is_act else (14, 20, 28, 200)
        t_border = (44, 232, 228, 255) if is_act else (45, 60, 75, 200)
        t_text = (245, 250, 255, 255) if is_act else (130, 155, 175, 220)
        d.rectangle([panel_x + 60, tab_y, panel_x + 240, tab_y + 44], fill=t_col, outline=t_border, width=2 if is_act else 1)
        if is_act:
            d.rectangle([panel_x + 60, tab_y, panel_x + 66, tab_y + 44], fill=(120, 255, 250, 255))
        d.text((panel_x + 80, tab_y + 12), t_name, fill=t_text, font=f_tab)
        tab_y += 52

    # Place Option Rows in Right Column
    row_x = panel_x + 260
    row_y = panel_y + 160
    row_w = 840
    row_h = 50

    row_default = Image.open(os.path.join(PROC_DIR, "row_bg_default.png")).resize((row_w, row_h), Image.Resampling.LANCZOS)
    row_hover = Image.open(os.path.join(PROC_DIR, "row_bg_hover.png")).resize((row_w, row_h), Image.Resampling.LANCZOS)

    toggle_on = Image.open(os.path.join(PROC_DIR, "toggle_switch_on.png")).resize((90, 36), Image.Resampling.LANCZOS)
    toggle_off = Image.open(os.path.join(PROC_DIR, "toggle_switch_off.png")).resize((90, 36), Image.Resampling.LANCZOS)

    track_fill = Image.open(os.path.join(PROC_DIR, "slider_track_fill.png")).resize((220, 24), Image.Resampling.LANCZOS)
    track_empty = Image.open(os.path.join(PROC_DIR, "slider_track_empty.png")).resize((220, 24), Image.Resampling.LANCZOS)
    knob_normal = Image.open(os.path.join(PROC_DIR, "slider_knob_normal.png")).resize((22, 34), Image.Resampling.LANCZOS)
    knob_hover = Image.open(os.path.join(PROC_DIR, "slider_knob_hover.png")).resize((22, 34), Image.Resampling.LANCZOS)

    options = [
        ("ENABLE VR MODE", "ВКЛЮЧИТЬ РЕЖИМ VR", "toggle", True, False),
        ("AIM METHOD", "СПОСОБ ПРИЦЕЛИВАНИЯ", "choice", "MOTION CONTROLLER", True),  # Hovered by laser!
        ("TURN MODE", "РЕЖИМ ПОВОРОТА", "choice", "SNAP TURN (30°)", False),
        ("TURN SPEED", "СКОРОСТЬ ПОВОРОТА", "slider", 0.65, False),
        ("WORLD SCALE", "МАСШТАБ ОКРУЖЕНИЯ", "slider", 0.50, False),
        ("LASER POINTER", "ЛАЗЕРНЫЙ УКАЗАТЕЛЬ В МЕНЮ", "toggle", True, False),
        ("CUTSCENE PRESENTATION", "ОТОБРАЖЕНИЕ КАТСЦЕН", "choice", "FLAT CINEMA HUD", False),
        ("HUD NOTE SIZE", "РАЗМЕР ЗАПИСОК И ТЕКСТА", "slider", 0.80, False)
    ]

    f_opt = get_font(16, bold=True)
    f_cyr = get_font(11)
    f_val = get_font(15, bold=True)

    for idx, (title, cyr_desc, opt_type, val, is_hov) in enumerate(options):
        cur_y = row_y + idx * 56
        # Paste row background
        r_img = row_hover if is_hov else row_default
        im.paste(r_img, (row_x, cur_y), r_img)

        # Title & Cyrillic description
        t_col = (120, 255, 250, 255) if is_hov else (235, 244, 250, 240)
        d.text((row_x + 20, cur_y + 8), title, fill=t_col, font=f_opt)
        d.text((row_x + 20, cur_y + 28), cyr_desc, fill=(125, 145, 160, 200), font=f_cyr)

        # Widget on right side
        if opt_type == "toggle":
            tog_img = toggle_on if val else toggle_off
            im.paste(tog_img, (row_x + row_w - 110, cur_y + 7), tog_img)
        elif opt_type == "choice":
            d.rectangle([row_x + row_w - 240, cur_y + 8, row_x + row_w - 20, cur_y + 42],
                        fill=(12, 18, 25, 240), outline=(44, 232, 228, 255) if is_hov else (50, 70, 90, 220), width=2 if is_hov else 1)
            d.text((row_x + row_w - 130, cur_y + 15), str(val),
                   fill=(120, 255, 250, 255) if is_hov else (220, 235, 245, 240), font=f_val, anchor="mm")
            # Arrows
            d.text((row_x + row_w - 225, cur_y + 15), "<", fill=(44, 232, 228, 255), font=f_val, anchor="mm")
            d.text((row_x + row_w - 35, cur_y + 15), ">", fill=(44, 232, 228, 255), font=f_val, anchor="mm")
        elif opt_type == "slider":
            im.paste(track_fill, (row_x + row_w - 240, cur_y + 13), track_fill)
            knob_x = int(row_x + row_w - 240 + 220 * float(val)) - 11
            im.paste(knob_normal, (knob_x, cur_y + 8), knob_normal)

    # Scroll bar on far right
    scroll_x = row_x + row_w + 12
    scr_up = Image.open(os.path.join(PROC_DIR, "scroll_arrow_up.png")).resize((24, 24), Image.Resampling.LANCZOS)
    scr_dn = Image.open(os.path.join(PROC_DIR, "scroll_arrow_down.png")).resize((24, 24), Image.Resampling.LANCZOS)
    scr_trk = Image.open(os.path.join(PROC_DIR, "scroll_track.png")).resize((16, 380), Image.Resampling.LANCZOS)
    scr_thb = Image.open(os.path.join(PROC_DIR, "scroll_thumb.png")).resize((16, 70), Image.Resampling.LANCZOS)

    im.paste(scr_up, (scroll_x, row_y), scr_up)
    im.paste(scr_trk, (scroll_x + 4, row_y + 30), scr_trk)
    im.paste(scr_thb, (scroll_x + 4, row_y + 50), scr_thb)
    im.paste(scr_dn, (scroll_x, row_y + 416), scr_dn)

    # Draw Laser Pointer Aiming at the Hovered Row
    laser_start_x = w - 180
    laser_start_y = h - 60
    laser_hit_x = row_x + row_w - 130
    laser_hit_y = row_y + 56 * 1 + 25

    # Glowing Laser Beam
    d.line([laser_start_x, laser_start_y, laser_hit_x, laser_hit_y], fill=(44, 232, 228, 160), width=3)
    d.line([laser_start_x, laser_start_y, laser_hit_x, laser_hit_y], fill=(120, 255, 250, 240), width=1)
    # Laser Reticle Dot at hit point
    d.ellipse([laser_hit_x - 8, laser_hit_y - 8, laser_hit_x + 8, laser_hit_y + 8],
              fill=(120, 255, 250, 255), outline=(44, 232, 228, 255), width=2)

    # Bottom PSVR2 Legend Bar
    f_hint = get_font(16)
    d.rectangle([w // 2 - 440, h - 75, w // 2 + 440, h - 25], fill=(12, 17, 24, 240), outline=(45, 60, 75, 255), width=2)
    d.text((w // 2, h - 50),
           "[R2 / TRIGGER] ВЫБРАТЬ     [○ / CIRCLE] НАЗАД     [L-STICK] НАВИГАЦИЯ     [УДЕРЖАНИЕ △] ЗАКРЫТЬ",
           fill=(225, 240, 250, 240), font=f_hint, anchor="mm")

    mockup_path = os.path.join(MOCKUP_DIR, "vr_settings_screen_mockup.png")
    im.save(mockup_path, "PNG")
    print(f"Generated VR Settings concept mockup: {mockup_path}")

# Run all VR Settings generators
if __name__ == "__main__":
    generate_panel_background()
    generate_section_header()
    generate_option_rows()
    generate_toggle_switch()
    generate_sliders()
    generate_scroll_and_dividers()
    generate_vr_settings_mockup()
