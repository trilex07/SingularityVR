"""
Singularity VR - UI Remaster: Pause & Frontend Panel Styling Generator
Generates SVG sources, transparent PNG exports, and concept mockups for Pause Menu & Confirmation Dialog.
"""

import os
import math
from PIL import Image, ImageDraw, ImageFont, ImageFilter

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SRC_DIR = os.path.join(BASE_DIR, "assets", "source", "ui_remaster", "pause_menu")
PROC_DIR = os.path.join(BASE_DIR, "assets", "processed", "ui_remaster", "pause_menu")
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
# 1. Pause Menu Panel Background (680x860) & Submenu Card
# -------------------------------------------------------------
def generate_pause_backgrounds():
    w, h = 680, 860
    scale = 2
    sw, sh = w * scale, h * scale

    # Main Pause Panel
    im = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    pad = 16 * scale

    # Dark Frosted Terminal Backing
    d.rectangle([pad, pad, sw - pad, sh - pad], fill=(13, 17, 24, 240), outline=(65, 85, 102, 255), width=3 * scale)
    d.rectangle([pad + 6 * scale, pad + 6 * scale, sw - pad - 6 * scale, sh - pad - 6 * scale],
                outline=(28, 38, 48, 255), width=2 * scale)

    # 4 Industrial Corner Brackets
    c_len = 60 * scale
    for cx, cy in [(pad, pad), (sw - pad, pad), (pad, sh - pad), (sw - pad, sh - pad)]:
        dx = 1 if cx == pad else -1
        dy = 1 if cy == pad else -1
        d.polygon([(cx, cy), (cx + dx * c_len, cy), (cx + dx * c_len, cy + dy * 20 * scale),
                   (cx + dx * 20 * scale, cy + dy * c_len), (cx, cy + dy * c_len)], fill=(32, 45, 60, 255))
        d.ellipse([cx + dx * 14 * scale - 6 * scale, cy + dy * 14 * scale - 6 * scale,
                   cx + dx * 14 * scale + 6 * scale, cy + dy * 14 * scale + 6 * scale],
                  fill=(55, 75, 95, 255), outline=(100, 130, 155, 255), width=scale)

    # Top Stenciled Title Header
    d.rectangle([pad + 20 * scale, pad + 20 * scale, sw - pad - 20 * scale, pad + 100 * scale],
                fill=(18, 25, 34, 255), outline=(44, 232, 228, 255), width=2 * scale)

    # Glowing E99 Insignia Box on Left Header
    d.rectangle([pad + 24 * scale, pad + 24 * scale, pad + 100 * scale, pad + 96 * scale],
                fill=(20, 65, 80, 255), outline=(120, 255, 250, 255), width=2 * scale)
    f_e99 = get_font(18 * scale, bold=True)
    d.text((pad + 62 * scale, pad + 60 * scale), "E99", fill=(120, 255, 250, 255), font=f_e99, anchor="mm")

    f_title = get_font(24 * scale, bold=True)
    f_sub = get_font(12 * scale)
    d.text((pad + 120 * scale, pad + 44 * scale), "ПАУЗА // PAUSE TERMINAL", fill=(245, 250, 255, 255), font=f_title)
    d.text((pad + 120 * scale, pad + 74 * scale), "НАУЧНО-ИССЛЕДОВАТЕЛЬСКИЙ СЕКТОР КАТОРГА-12", fill=(130, 160, 180, 220), font=f_sub)

    # Bottom Metadata Bar
    d.line([pad + 20 * scale, sh - pad - 45 * scale, sw - pad - 20 * scale, sh - pad - 45 * scale],
           fill=(45, 60, 75, 255), width=scale)
    f_bot = get_font(11 * scale)
    d.text((sw // 2, sh - pad - 25 * scale), "СИНГУЛЯРНОСТЬ VR // ПРОЕКТ-1955 // ОСТАНОВКА ПРОЦЕССА",
           fill=(100, 125, 145, 200), font=f_bot, anchor="mm")

    final_im = im.resize((w, h), Image.Resampling.LANCZOS)
    png_path = os.path.join(PROC_DIR, "pause_panel_bg.png")
    final_im.save(png_path, "PNG")

    # Submenu Card BG (680x600)
    cw, ch = 680, 600
    im_c = Image.new("RGBA", (cw * scale, ch * scale), (0, 0, 0, 0))
    dc = ImageDraw.Draw(im_c)
    dc.rectangle([pad, pad, cw * scale - pad, ch * scale - pad],
                 fill=(13, 17, 24, 240), outline=(65, 85, 102, 255), width=3 * scale)
    im_c.resize((cw, ch), Image.Resampling.LANCZOS).save(os.path.join(PROC_DIR, "submenu_card_bg.png"), "PNG")

    print(f"Generated: {png_path} and submenu card")

# -------------------------------------------------------------
# 2. Button States (520x64)
# -------------------------------------------------------------
def generate_button_states():
    w, h = 520, 64
    scale = 2
    sw, sh = w * scale, h * scale

    # 1. Dedicated "VR SETTINGS" Button (Injected into Pause & Main Menu)
    for b_state in ["default", "hover", "pressed"]:
        im = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)

        if b_state == "default":
            fill_col = (16, 26, 36, 230)
            out_col = (44, 232, 228, 220)
            text_col = (44, 232, 228, 255)
            w_out = 2 * scale
            left_tab = (44, 232, 228, 255)
        elif b_state == "hover":
            fill_col = (20, 65, 82, 250)
            out_col = (120, 255, 250, 255)
            text_col = (255, 255, 255, 255)
            w_out = 3 * scale
            left_tab = (120, 255, 250, 255)
        else: # pressed
            fill_col = (25, 80, 100, 255)
            out_col = (255, 169, 39, 255)
            text_col = (255, 210, 110, 255)
            w_out = 4 * scale
            left_tab = (255, 169, 39, 255)

        # Base Button Body
        d.rectangle([2 * scale, 2 * scale, sw - 2 * scale, sh - 2 * scale],
                    fill=fill_col, outline=out_col, width=w_out)

        # Left Accent Tab (VR Highlight Notch)
        d.rectangle([2 * scale, 2 * scale, 16 * scale, sh - 2 * scale], fill=left_tab)

        # Tactical Corner registration ticks if hover
        if b_state == "hover":
            d.line([4 * scale, 4 * scale, 20 * scale, 4 * scale], fill=(120, 255, 250, 255), width=2 * scale)
            d.line([4 * scale, 4 * scale, 4 * scale, 20 * scale], fill=(120, 255, 250, 255), width=2 * scale)
            d.line([sw - 4 * scale, 4 * scale, sw - 20 * scale, 4 * scale], fill=(120, 255, 250, 255), width=2 * scale)
            d.line([sw - 4 * scale, 4 * scale, sw - 4 * scale, 20 * scale], fill=(120, 255, 250, 255), width=2 * scale)

        # Button Label
        f_btn = get_font(20 * scale, bold=True)
        f_sub = get_font(11 * scale)
        d.text((sw // 2, sh // 2 - 8 * scale), "ПАРАМЕТРЫ VR // VR SETTINGS", fill=text_col, font=f_btn, anchor="mm")
        d.text((sw // 2, sh // 2 + 14 * scale), "КОНФИГУРАЦИЯ СИСТЕМЫ И ПРИЦЕЛА", fill=(140, 180, 200, 220), font=f_sub, anchor="mm")

        final_im = im.resize((w, h), Image.Resampling.LANCZOS)
        final_im.save(os.path.join(PROC_DIR, f"btn_vr_settings_{b_state}.png"), "PNG")

    # 2. Generic Menu Buttons (Resume, Load, Save, Options, Quit)
    btn_types = ["generic", "resume"]
    for b_type in btn_types:
        for b_state in ["default", "hover", "pressed", "disabled"]:
            im = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
            d = ImageDraw.Draw(im)

            if b_state == "default":
                fill_col = (15, 21, 29, 220)
                out_col = (55, 75, 95, 240)
                text_col = (235, 244, 250, 240)
                w_out = 2 * scale
            elif b_state == "hover":
                fill_col = (20, 52, 70, 250)
                out_col = (44, 232, 228, 255)
                text_col = (120, 255, 250, 255)
                w_out = 3 * scale
            elif b_state == "pressed":
                fill_col = (22, 72, 92, 255)
                out_col = (255, 169, 39, 255)
                text_col = (255, 220, 130, 255)
                w_out = 4 * scale
            else: # disabled
                fill_col = (11, 14, 18, 140)
                out_col = (35, 45, 55, 160)
                text_col = (85, 105, 120, 160)
                w_out = scale

            d.rectangle([2 * scale, 2 * scale, sw - 2 * scale, sh - 2 * scale],
                        fill=fill_col, outline=out_col, width=w_out)

            # Title label
            f_btn = get_font(20 * scale, bold=True)
            lbl = "ПРОДОЛЖИТЬ // RESUME" if b_type == "resume" else "ВЫБОР МЕНЮ // MENU OPTION"
            d.text((sw // 2, sh // 2), lbl, fill=text_col, font=f_btn, anchor="mm")

            final_im = im.resize((w, h), Image.Resampling.LANCZOS)
            final_im.save(os.path.join(PROC_DIR, f"btn_{b_type}_{b_state}.png"), "PNG")

    print("Generated all button states.")

# -------------------------------------------------------------
# 3. Confirmation Dialog (700x380) & Dialog Buttons
# -------------------------------------------------------------
def generate_confirmation_dialog():
    w, h = 700, 380
    scale = 2
    sw, sh = w * scale, h * scale

    im = Image.new("RGBA", (sw, sh), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    pad = 12 * scale

    # Heavy Security Modal Box Frame
    d.rectangle([pad, pad, sw - pad, sh - pad], fill=(14, 18, 25, 250), outline=(65, 85, 102, 255), width=3 * scale)
    d.rectangle([pad + 6 * scale, pad + 6 * scale, sw - pad - 6 * scale, sh - pad - 6 * scale],
                outline=(30, 42, 54, 255), width=2 * scale)

    # Caution Hazard Striped Header
    d.rectangle([pad + 10 * scale, pad + 10 * scale, sw - pad - 10 * scale, pad + 65 * scale],
                fill=(20, 26, 36, 255), outline=(45, 60, 75, 255), width=scale)
    for z in range(0, 120 * scale, 18 * scale):
        d.polygon([(pad + 10 * scale + z, pad + 10 * scale),
                   (pad + 24 * scale + z, pad + 10 * scale),
                   (pad + 14 * scale + z, pad + 65 * scale),
                   (pad + 0 * scale + z, pad + 65 * scale)], fill=(245, 195, 35, 240))

    # Header Stencil Title
    f_head = get_font(20 * scale, bold=True)
    d.text((pad + 140 * scale, pad + 38 * scale), "ВНИМАНИЕ // CONFIRMATION REQUIRED", fill=(245, 250, 255, 255), font=f_head, anchor="lm")

    # Dialog Warning Icon & Body Text
    f_body = get_font(18 * scale, bold=True)
    f_desc = get_font(13 * scale)
    d.text((sw // 2, sh // 2 - 30 * scale), "ВЫ ДЕЙСТВИТЕЛЬНО ХОТИТЕ ВЫЙТИ В МЕНЮ?", fill=(235, 244, 250, 255), font=f_body, anchor="mm")
    d.text((sw // 2, sh // 2), "НЕСОХРАНЕННЫЙ ПРОГРЕСС БУДЕТ УТЕРЯН // E99 ХРОНО-ПОТОК ПРЕРВЕТСЯ",
           fill=(255, 169, 39, 240), font=f_desc, anchor="mm")

    final_im = im.resize((w, h), Image.Resampling.LANCZOS)
    png_path = os.path.join(PROC_DIR, "modal_dialog_frame.png")
    final_im.save(png_path, "PNG")

    # Confirm & Cancel Dialog Buttons (240x56)
    bw, bh = 240, 56
    # Confirm (Cyan/Amber Highlight)
    im_conf = Image.new("RGBA", (bw * scale, bh * scale), (0, 0, 0, 0))
    dc = ImageDraw.Draw(im_conf)
    dc.rectangle([2 * scale, 2 * scale, bw * scale - 2 * scale, bh * scale - 2 * scale],
                 fill=(22, 60, 76, 250), outline=(44, 232, 228, 255), width=2 * scale)
    f_dbtn = get_font(16 * scale, bold=True)
    dc.text(((bw * scale) // 2, (bh * scale) // 2), "ПОДТВЕРДИТЬ // YES", fill=(120, 255, 250, 255), font=f_dbtn, anchor="mm")
    im_conf.resize((bw, bh), Image.Resampling.LANCZOS).save(os.path.join(PROC_DIR, "modal_btn_confirm.png"), "PNG")

    # Cancel (Dark Steel)
    im_canc = Image.new("RGBA", (bw * scale, bh * scale), (0, 0, 0, 0))
    dca = ImageDraw.Draw(im_canc)
    dca.rectangle([2 * scale, 2 * scale, bw * scale - 2 * scale, bh * scale - 2 * scale],
                  fill=(16, 22, 30, 240), outline=(65, 85, 102, 255), width=2 * scale)
    dca.text(((bw * scale) // 2, (bh * scale) // 2), "ОТМЕНА // CANCEL", fill=(210, 225, 235, 240), font=f_dbtn, anchor="mm")
    im_canc.resize((bw, bh), Image.Resampling.LANCZOS).save(os.path.join(PROC_DIR, "modal_btn_cancel.png"), "PNG")

    print(f"Generated: {png_path} and dialog buttons")

# -------------------------------------------------------------
# 4. Full Pause Menu Concept Mockup (1920x1080)
# -------------------------------------------------------------
def generate_pause_menu_mockup():
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

    cx, cy = w // 2, h // 2

    # Place Pause Menu Panel (680x860)
    panel_w, panel_h = 680, 840
    panel_x = (w - panel_w) // 2
    panel_y = (h - panel_h) // 2 - 20

    bg_plate = Image.open(os.path.join(PROC_DIR, "pause_panel_bg.png")).resize((panel_w, panel_h), Image.Resampling.LANCZOS)
    im.paste(bg_plate, (panel_x, panel_y), bg_plate)

    # Menu Buttons to Place
    btn_w, btn_h = 540, 60
    btn_x = panel_x + (panel_w - btn_w) // 2
    btn_start_y = panel_y + 140
    gap = 70

    buttons_spec = [
        ("btn_resume_default.png", "ПРОДОЛЖИТЬ // RESUME", False),
        ("btn_generic_default.png", "ЗАГРУЗИТЬ ИГРУ // LOAD GAME", False),
        ("btn_generic_default.png", "СОХРАНИТЬ ИГРУ // SAVE GAME", False),
        ("btn_vr_settings_hover.png", "ПАРАМЕТРЫ VR // VR SETTINGS", True),   # Laser hovered!
        ("btn_generic_default.png", "НАСТРОЙКИ // GAME OPTIONS", False),
        ("btn_generic_default.png", "ГЛАВНОЕ МЕНЮ // MAIN MENU", False),
        ("btn_generic_default.png", "ВЫХОД НА РАБОЧИЙ СТОЛ // QUIT", False)
    ]

    f_btn = get_font(18, bold=True)
    f_sub = get_font(10)

    laser_hit_x = 0
    laser_hit_y = 0

    for i, (btn_img_file, btn_label, is_hov) in enumerate(buttons_spec):
        by = btn_start_y + i * gap
        btn_im = Image.open(os.path.join(PROC_DIR, btn_img_file)).resize((btn_w, btn_h), Image.Resampling.LANCZOS)
        im.paste(btn_im, (btn_x, by), btn_im)

        if is_hov:
            laser_hit_x = btn_x + btn_w // 2 + 80
            laser_hit_y = by + btn_h // 2

    # Laser Pointer aiming at "VR SETTINGS" button
    laser_start_x = w - 240
    laser_start_y = h - 60
    d.line([laser_start_x, laser_start_y, laser_hit_x, laser_hit_y], fill=(44, 232, 228, 160), width=3)
    d.line([laser_start_x, laser_start_y, laser_hit_x, laser_hit_y], fill=(120, 255, 250, 240), width=1)
    # Laser Reticle Dot
    d.ellipse([laser_hit_x - 8, laser_hit_y - 8, laser_hit_x + 8, laser_hit_y + 8],
              fill=(120, 255, 250, 255), outline=(44, 232, 228, 255), width=2)

    # Bottom PSVR2 Legend Bar
    f_hint = get_font(16)
    d.rectangle([w // 2 - 400, h - 75, w // 2 + 400, h - 25], fill=(12, 17, 24, 240), outline=(45, 60, 75, 255), width=2)
    d.text((w // 2, h - 50),
           "[R2 / TRIGGER] ВЫБРАТЬ     [○ / CIRCLE] НАЗАД     [L-STICK] НАВИГАЦИЯ     [△ / TRIANGLE] ВЫХОД",
           fill=(225, 240, 250, 240), font=f_hint, anchor="mm")

    mockup_path = os.path.join(MOCKUP_DIR, "pause_menu_mockup.png")
    im.save(mockup_path, "PNG")
    print(f"Generated Pause Menu concept mockup: {mockup_path}")

# -------------------------------------------------------------
# 5. Full Confirmation Modal Concept Mockup (1920x1080)
# -------------------------------------------------------------
def generate_confirmation_modal_mockup():
    w, h = 1920, 1080
    im = Image.new("RGBA", (w, h), (10, 14, 18, 255))
    d = ImageDraw.Draw(im)

    # Ambient dimmed background
    for r in range(h // 2, 0, -25):
        d.ellipse([w // 2 - r * 1.6, h // 2 - r, w // 2 + r * 1.6, h // 2 + r], fill=(12, 18, 25, 30))

    # Dark overlay backdrop
    d.rectangle([0, 0, w, h], fill=(0, 0, 0, 160))

    # Place Modal Dialog Frame (700x380)
    dw, dh = 700, 380
    dx = (w - dw) // 2
    dy = (h - dh) // 2 - 20

    diag_im = Image.open(os.path.join(PROC_DIR, "modal_dialog_frame.png")).resize((dw, dh), Image.Resampling.LANCZOS)
    im.paste(diag_im, (dx, dy), diag_im)

    # Place Confirm & Cancel Buttons
    bw, bh = 240, 56
    btn_y = dy + dh - 90
    btn_conf_x = dx + 70
    btn_canc_x = dx + dw - 70 - bw

    b_conf = Image.open(os.path.join(PROC_DIR, "modal_btn_confirm.png")).resize((bw, bh), Image.Resampling.LANCZOS)
    b_canc = Image.open(os.path.join(PROC_DIR, "modal_btn_cancel.png")).resize((bw, bh), Image.Resampling.LANCZOS)

    im.paste(b_conf, (btn_conf_x, btn_y), b_conf)
    im.paste(b_canc, (btn_canc_x, btn_y), b_canc)

    # Laser targeting "CONFIRM"
    laser_start_x = w - 240
    laser_start_y = h - 60
    laser_hit_x = btn_conf_x + bw // 2
    laser_hit_y = btn_y + bh // 2
    d.line([laser_start_x, laser_start_y, laser_hit_x, laser_hit_y], fill=(44, 232, 228, 160), width=3)
    d.line([laser_start_x, laser_start_y, laser_hit_x, laser_hit_y], fill=(120, 255, 250, 240), width=1)
    d.ellipse([laser_hit_x - 8, laser_hit_y - 8, laser_hit_x + 8, laser_hit_y + 8],
              fill=(120, 255, 250, 255), outline=(44, 232, 228, 255), width=2)

    mockup_path = os.path.join(MOCKUP_DIR, "confirmation_modal_mockup.png")
    im.save(mockup_path, "PNG")
    print(f"Generated Confirmation Modal mockup: {mockup_path}")

if __name__ == "__main__":
    generate_pause_backgrounds()
    generate_button_states()
    generate_confirmation_dialog()
    generate_pause_menu_mockup()
    generate_confirmation_modal_mockup()
