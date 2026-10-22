"""
Singularity VR - UI Art Direction V2.1 Color Direction Correction
Rebalances materials to establish weathered gray steel, graphite, ash and concrete
as the primary 70% visual identity, with 15% aged ivory, 8% secondary faded olive,
5% rust/brass/amber, and 2% E99 cyan.
Generates:
1. weapon_wheel_mockup_v2_1.png
2. vr_settings_screen_mockup_v2_1.png
3. pause_menu_mockup_v2_1.png
And side-by-side V2 vs V2.1 comparison images.
"""

import os
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageOps

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
REF_DIR = os.path.abspath(os.path.join(BASE_DIR, "..", "references"))
PROC_DIR = os.path.join(BASE_DIR, "assets", "processed", "ui_remaster")
MOCKUP_DIR = os.path.join(BASE_DIR, "docs", "previews", "ui_remaster", "mockups")

os.makedirs(MOCKUP_DIR, exist_ok=True)

# ---------------------------------------------------------------------------
# V2.1 Target Proportions & Material Palette
# 70% Weathered Graphite & Steel / 15% Aged Ivory / 8% Secondary Olive / 5% Rust & Amber / 2% E99 Cyan
# ---------------------------------------------------------------------------
COL_STEEL_DARK = (28, 32, 36)            # #1C2024 - Deep graphite chassis
COL_STEEL_BASE = (36, 41, 46)            # #24292E - Weathered dark gray steel
COL_STEEL_LIGHT = (52, 59, 66)           # #343B42 - Cold-rolled steel plate
COL_STEEL_RECESSED = (18, 21, 24)        # #121518 - Sockets, slots, grooves
COL_STEEL_EDGE = (108, 120, 130)         # #6C7882 - Scratched bare steel highlights
COL_CONCRETE_ASH = (32, 36, 40)          # #202428 - Concrete and soot background

COL_AGED_IVORY = (222, 214, 192)         # #DDD6C0 - Primary readable stenciled typography (15%)
COL_IVORY_BRIGHT = (244, 238, 222)       # #F4EEDE - High-contrast active typography

COL_OLIVE_SECONDARY = (60, 66, 52)       # #3C4234 - Secondary accent band only (8%)
COL_OLIVE_MUTED = (46, 52, 40)           # #2E3428 - Faded inspection tag

COL_WEATHERED_BRASS = (140, 129, 102)    # #8C8166 - Milled knobs, screws, hardware (5% tier)
COL_OXIDIZED_RUST = (136, 78, 38)        # #884E26 - Corner corrosion and seams
COL_SELECTOR_AMBER = (217, 126, 38)      # #D97E26 - Vacuum tube amber selection
COL_AMBER_BRIGHT = (248, 168, 62)        # #F8A83E - High amber indicator light

COL_E99_CYAN = (65, 175, 170)            # #41AFAA - Concentrated active E99 (2% tier)
COL_E99_CORE = (130, 240, 235)           # #82F0EB - Core filament focus point
COL_CAUTION_YELLOW = (205, 165, 45)      # #CDA52D - Faded hazard striping

def get_font(size, bold=False):
    font_path = r"C:\Windows\Fonts\bahnschrift.ttf"
    if not os.path.exists(font_path):
        font_path = r"C:\Windows\Fonts\arial.ttf"
    try:
        return ImageFont.truetype(font_path, size)
    except Exception:
        return ImageFont.load_default()

# ---------------------------------------------------------------------------
# Weathered Gray Steel & Graphite Texture Generator
# ---------------------------------------------------------------------------
def create_gray_steel_texture(w, h, base_rgb=COL_STEEL_BASE, noise_amt=16, rust_spots=14):
    """Creates an authentic Cold War Soviet gray steel surface with scratches and subtle oxidation."""
    np.random.seed(1955) # Katorga-12 seed
    base = np.full((h, w, 3), base_rgb, dtype=np.float32)

    # Micro noise grain
    noise = np.random.uniform(-noise_amt, noise_amt, (h, w, 1))
    base += noise

    # Fine horizontal brushed metal striations
    lines = np.random.uniform(-5, 5, (h, 1, 1))
    base += lines

    # Subtle localized oxidation in crevices (rust spots)
    for _ in range(rust_spots):
        rx = np.random.randint(0, w)
        ry = np.random.randint(0, h)
        rw = np.random.randint(12, 45)
        rh = np.random.randint(12, 45)
        x0, x1 = max(0, rx - rw), min(w, rx + rw)
        y0, y1 = max(0, ry - rh), min(h, ry + rh)
        if x1 > x0 and y1 > y0:
            patch = np.zeros((y1 - y0, x1 - x0, 3), dtype=np.float32)
            patch[:, :] = [COL_OXIDIZED_RUST[0] - base_rgb[0],
                           COL_OXIDIZED_RUST[1] - base_rgb[1],
                           COL_OXIDIZED_RUST[2] - base_rgb[2]]
            alpha_mask = np.random.uniform(0, 0.22, (y1 - y0, x1 - x0, 1))
            base[y0:y1, x0:x1] += patch * alpha_mask

    base = np.clip(base, 0, 255).astype(np.uint8)
    im = Image.fromarray(base).convert("RGBA")

    # Scratches revealing bare steel
    draw = ImageDraw.Draw(im)
    for _ in range(30):
        sx1 = np.random.randint(0, w)
        sy1 = np.random.randint(0, h)
        sx2 = sx1 + np.random.randint(-45, 45)
        sy2 = sy1 + np.random.randint(-12, 12)
        draw.line([sx1, sy1, sx2, sy2], fill=(COL_STEEL_EDGE[0], COL_STEEL_EDGE[1], COL_STEEL_EDGE[2], 130), width=1)

    return im

def draw_steel_rivet(draw, cx, cy, r=6, is_screw=False):
    """Draws a physical industrial steel rivet or slotted brass screw with dirt in recess."""
    # Dark shadow/recess with slight dirt
    draw.ellipse([cx - r - 2, cy - r - 2, cx + r + 2, cy + r + 2], fill=(12, 14, 16, 240))
    # Beveled fastener head
    fill_c = COL_WEATHERED_BRASS if is_screw else (58, 64, 72, 255)
    out_c = (88, 98, 108, 255) if not is_screw else (110, 102, 80, 255)
    draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=fill_c, outline=out_c, width=1)
    if is_screw:
        draw.line([cx - r + 2, cy - r + 2, cx + r - 2, cy + r - 2], fill=(18, 20, 22, 255), width=2)
    else:
        # Steel dome reflection
        draw.ellipse([cx - r//2, cy - r//2, cx + r//2, cy + r//2], fill=(92, 104, 116, 255))

def draw_steel_border(draw, x0, y0, x1, y1, border_w=3):
    """Draws a physical milled steel border with chipped edges and bare steel highlights."""
    draw.rectangle([x0 - 1, y0 - 1, x1 + 1, y1 + 1], outline=(14, 17, 20, 220), width=1)
    draw.rectangle([x0, y0, x1, y1], outline=(68, 77, 86, 255), width=border_w)
    # Highlights on upper-left edges
    draw.line([x0, y0, x1, y0], fill=COL_STEEL_EDGE, width=1)
    draw.line([x0, y0, x0, y1], fill=COL_STEEL_EDGE, width=1)

def load_weapon_silhouette(name, target_w, target_h, tint=COL_AGED_IVORY):
    """Loads authentic silhouette from references/ and re-tints to aged ivory enamel."""
    fn_map = {
        "centurion": "icon-pistol-final.png",
        "valkyrie": "icon-assault-final.png",
        "spikeshot": "icon-shotgun-final.png",
        "kasimov": "icon-sniper-final.png"
    }
    src_path = os.path.join(REF_DIR, fn_map.get(name, "icon-pistol-final.png"))
    if not os.path.exists(src_path):
        return Image.new("RGBA", (target_w, target_h), (0, 0, 0, 0))

    raw = Image.open(src_path).convert("RGBA")
    raw.thumbnail((target_w, target_h), Image.Resampling.LANCZOS)
    r, g, b, a = raw.split()
    tint_im = Image.new("RGBA", raw.size, (tint[0], tint[1], tint[2], 255))
    tint_im.putalpha(a)
    return tint_im

# ---------------------------------------------------------------------------
# Background: Abandoned Soviet Research Facility (Cold Concrete, Steel & Ash)
# ---------------------------------------------------------------------------
def create_facility_background(w=1920, h=1080):
    """Creates a cold, dark Soviet laboratory backdrop dominated by concrete ash and graphite steel."""
    bg = Image.new("RGBA", (w, h), (18, 20, 22, 255))

    # Concrete ash texture (70% gray/graphite identity)
    wall = create_gray_steel_texture(w, h, (24, 27, 30), noise_amt=24, rust_spots=28)
    bg = Image.alpha_composite(bg, wall)
    d = ImageDraw.Draw(bg)

    # Heavy structural I-beams in cold dark graphite
    for px in [160, 500, 1420, 1760]:
        d.rectangle([px, 0, px + 44, h], fill=(16, 18, 20, 240), outline=(32, 36, 42, 255), width=2)
        for py in range(50, h, 90):
            draw_steel_rivet(d, px + 22, py, r=5)

    # Horizontal steel steam pipes (graphite/steel with rust seams)
    for py in [60, 980]:
        d.rectangle([0, py, w, py + 34], fill=(26, 29, 32, 250), outline=(44, 49, 56, 255), width=2)
        d.line([0, py + 8, w, py + 8], fill=(58, 64, 72, 180), width=2)

    # Vignette shadow
    vignette = Image.new("L", (w, h), 0)
    vd = ImageDraw.Draw(vignette)
    vd.ellipse([w // 2 - 820, h // 2 - 500, w // 2 + 820, h // 2 + 500], fill=255)
    vignette = vignette.filter(ImageFilter.GaussianBlur(180))
    vignette = ImageOps.invert(vignette)
    bg_shadow = Image.new("RGBA", (w, h), (10, 11, 13, 215))
    bg_shadow.putalpha(vignette)
    bg = Image.alpha_composite(bg, bg_shadow)

    return bg

# ---------------------------------------------------------------------------
# 1. Weapon Wheel Mockup (V2.1) — Weathered Gray Steel & Graphite Selector
# ---------------------------------------------------------------------------
def generate_weapon_wheel_v2_1():
    w, h = 1920, 1080
    bg = create_facility_background(w, h)
    cx, cy = w // 2, h // 2

    r_outer = 380
    r_inner = 170

    # 1. Outer Circular Housing (Weathered Gray Steel & Cold Rolled Iron — 70% Identity)
    housing_im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    hd = ImageDraw.Draw(housing_im)

    # Outer cast iron / gray steel rim
    hd.ellipse([cx - r_outer - 35, cy - r_outer - 35, cx + r_outer + 35, cy + r_outer + 35],
               fill=(24, 27, 30, 250), outline=(48, 54, 62, 255), width=4)
    # Milled steel bevel groove
    hd.ellipse([cx - r_outer - 15, cy - r_outer - 15, cx + r_outer + 15, cy + r_outer + 15],
               outline=(68, 77, 86, 255), width=3)

    # 16 Cold steel perimeter hex bolts
    for i in range(16):
        ang = math.radians(i * (360 / 16))
        bx = cx + (r_outer + 20) * math.cos(ang)
        by = cy + (r_outer + 20) * math.sin(ang)
        draw_steel_rivet(hd, int(bx), int(by), r=7, is_screw=False)

    # 360-degree engraved graduation ticks (faded ivory & amber cardinal ticks)
    for a in range(0, 360, 5):
        rad = math.radians(a)
        is_major = (a % 30 == 0)
        is_cardinal = (a % 90 == 0)
        t_len = 18 if is_cardinal else (12 if is_major else 6)
        t_col = (COL_SELECTOR_AMBER[0], COL_SELECTOR_AMBER[1], COL_SELECTOR_AMBER[2], 240) if is_cardinal else \
                ((COL_AGED_IVORY[0], COL_AGED_IVORY[1], COL_AGED_IVORY[2], 180) if is_major else (82, 92, 102, 140))
        t_w = 2 if is_major else 1
        x1 = cx + (r_outer - 6) * math.cos(rad)
        y1 = cy + (r_outer - 6) * math.sin(rad)
        x2 = cx + (r_outer - 6 - t_len) * math.cos(rad)
        y2 = cy + (r_outer - 6 - t_len) * math.sin(rad)
        hd.line([x1, y1, x2, y2], fill=t_col, width=t_w)

    # Stamped Soviet Factory Markings in Aged Ivory on Gray Steel
    f_stamp = get_font(13, bold=True)
    hd.text((cx, cy - r_outer - 12), "КАТОРГА-12 // ЗАВ. 1955 // СЕКТОРНЫЙ ПЕРЕКЛЮЧАТЕЛЬ",
            fill=(COL_AGED_IVORY[0], COL_AGED_IVORY[1], COL_AGED_IVORY[2], 210), font=f_stamp, anchor="mm")
    hd.text((cx, cy + r_outer + 12), "ГОСТ 1955 // МЕХАНИЗМ ВЫБОРА ОРУЖИЯ // СЕРИЯ 4-С",
            fill=(COL_AGED_IVORY[0], COL_AGED_IVORY[1], COL_AGED_IVORY[2], 210), font=f_stamp, anchor="mm")

    bg = Image.alpha_composite(bg, housing_im)

    # 2. Four Mechanical Sector Wedges (Graphite / Gray Steel Fills)
    sectors = [
        ("centurion", "CENTURION .44", 225 + 2, 315 - 2, "centurion", "idle"),
        ("valkyrie", "AR9 WALKIRIA", 315 + 2, 405 - 2, "valkyrie", "selected"),  # Selected: amber accent
        ("spikeshot", "SPIKESHOT", 45 + 2, 135 - 2, "spikeshot", "held"),        # Held: brighter gray steel
        ("kasimov", "KASIMOV SNV-E99", 135 + 2, 225 - 2, "kasimov", "idle")
    ]

    wedge_im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    wd = ImageDraw.Draw(wedge_im)

    for sec_id, label_pl, a_start, a_end, icon_name, state in sectors:
        points = []
        steps = 30
        for i in range(steps + 1):
            ang = math.radians(a_start + (a_end - a_start) * (i / steps))
            points.append((cx + (r_outer - 28) * math.cos(ang), cy + (r_outer - 28) * math.sin(ang)))
        for i in range(steps, -1, -1):
            ang = math.radians(a_start + (a_end - a_start) * (i / steps))
            points.append((cx + (r_inner + 8) * math.cos(ang), cy + (r_inner + 8) * math.sin(ang)))

        if state == "selected":
            # Warm Soviet amber highlight (5% accent tier)
            fill_col = (COL_SELECTOR_AMBER[0], COL_SELECTOR_AMBER[1], COL_SELECTOR_AMBER[2], 170)
            border_col = (COL_AMBER_BRIGHT[0], COL_AMBER_BRIGHT[1], COL_AMBER_BRIGHT[2], 255)
            border_w = 4
        elif state == "held":
            # Slightly lighter cold-rolled steel for carried weapon (70% gray tier)
            fill_col = (44, 50, 58, 230)
            border_col = (112, 126, 140, 255)
            border_w = 2
        else:
            # Weathered graphite iron (70% gray tier)
            fill_col = (26, 30, 34, 220)
            border_col = (54, 62, 70, 220)
            border_w = 2

        wd.polygon(points, fill=fill_col, outline=border_col)

        # Thick boundary lines
        for i in range(steps):
            ang1 = math.radians(a_start + (a_end - a_start) * (i / steps))
            ang2 = math.radians(a_start + (a_end - a_start) * ((i + 1) / steps))
            wd.line([(cx + (r_outer - 28) * math.cos(ang1), cy + (r_outer - 28) * math.sin(ang1)),
                     (cx + (r_outer - 28) * math.cos(ang2), cy + (r_outer - 28) * math.sin(ang2))],
                    fill=border_col, width=border_w)

        mid_ang = math.radians((a_start + a_end) / 2)
        icon_r = (r_outer + r_inner) / 2 - 10
        ix = int(cx + icon_r * math.cos(mid_ang))
        iy = int(cy + icon_r * math.sin(mid_ang))

        # Silhouette: Aged ivory enamel (15% tier)
        icon_tint = COL_IVORY_BRIGHT if state == "selected" else COL_AGED_IVORY
        w_icon = load_weapon_silhouette(icon_name, 140, 60, tint=icon_tint)
        wedge_im.paste(w_icon, (ix - w_icon.width // 2, iy - w_icon.height // 2 - 12), w_icon)

        # Label
        f_lbl = get_font(15, bold=True)
        lbl_col = COL_IVORY_BRIGHT if state == "selected" else COL_AGED_IVORY
        wd.text((ix, iy + 26), label_pl, fill=lbl_col, font=f_lbl, anchor="mm")

        if state == "held":
            f_held = get_font(11, bold=True)
            wd.text((ix, iy + 44), "[W DŁONIACH]", fill=COL_STEEL_EDGE, font=f_held, anchor="mm")
        elif state == "selected":
            f_sel = get_font(11, bold=True)
            wd.text((ix, iy + 44), "[WYBIERZ]", fill=(255, 240, 205, 255), font=f_sel, anchor="mm")

    bg = Image.alpha_composite(bg, wedge_im)

    # 3. Center Selector Knob (Milled Steel with Weathered Brass Bezel — 5% Brass)
    hub_im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    hud_d = ImageDraw.Draw(hub_im)

    r_hub = r_inner - 10
    # Outer dark steel bezel
    hud_d.ellipse([cx - r_hub, cy - r_hub, cx + r_hub, cy + r_hub],
                  fill=COL_STEEL_DARK, outline=(64, 72, 82, 255), width=3)
    # Inner knurled brass/steel rim
    hud_d.ellipse([cx - r_hub + 12, cy - r_hub + 12, cx + r_hub - 12, cy + r_hub - 12],
                  fill=(32, 36, 42, 250), outline=COL_WEATHERED_BRASS, width=2)

    # Knurled grip ridges
    for ga in range(0, 360, 15):
        grad = math.radians(ga)
        gx1 = cx + (r_hub - 10) * math.cos(grad)
        gy1 = cy + (r_hub - 10) * math.sin(grad)
        gx2 = cx + r_hub * math.cos(grad)
        gy2 = cy + r_hub * math.sin(grad)
        hud_d.line([gx1, gy1, gx2, gy2], fill=(50, 56, 64, 255), width=2)

    # Selector needle pointing toward AR9 Walkiria (East)
    needle_ang = 0.0
    nx = cx + (r_hub + 15) * math.cos(needle_ang)
    ny = cy + (r_hub + 15) * math.sin(needle_ang)
    hud_d.polygon([(nx, ny),
                   (cx + (r_hub - 25) * math.cos(needle_ang - 0.25), cy + (r_hub - 25) * math.sin(needle_ang - 0.25)),
                   (cx + (r_hub - 25) * math.cos(needle_ang + 0.25), cy + (r_hub - 25) * math.sin(needle_ang + 0.25))],
                  fill=COL_SELECTOR_AMBER, outline=(255, 205, 130, 255))

    # Fasteners & Ivory Stencils
    for a in [45, 135, 225, 315]:
        rad = math.radians(a)
        draw_steel_rivet(hud_d, int(cx + 42 * math.cos(rad)), int(cy + 42 * math.sin(rad)), r=5, is_screw=True)

    f_hub_title = get_font(18, bold=True)
    f_hub_sub = get_font(11, bold=True)
    hud_d.text((cx, cy - 8), "WYBÓR", fill=COL_AGED_IVORY, font=f_hub_title, anchor="mm")
    hud_d.text((cx, cy + 12), "BRONI", fill=COL_AGED_IVORY, font=f_hub_title, anchor="mm")
    hud_d.text((cx, cy + 32), "4 POZYCJE", fill=COL_STEEL_EDGE, font=f_hub_sub, anchor="mm")

    bg = Image.alpha_composite(bg, hub_im)

    # 4. Weapon Specification Clipboard Plate (Right Side — 70% Steel with 8% Olive Accent Band)
    card_x = cx + 450
    card_y = cy - 140
    card_w = 400
    card_h = 280

    # Base: Weathered dark steel plate (NOT olive!)
    card_plate = create_gray_steel_texture(card_w, card_h, COL_STEEL_BASE, noise_amt=16, rust_spots=8)
    card_d = ImageDraw.Draw(card_plate)
    draw_steel_border(card_d, 2, 2, card_w - 3, card_h - 3, border_w=2)
    for fx, fy in [(14, 14), (card_w - 14, 14), (14, card_h - 14), (card_w - 14, card_h - 14)]:
        draw_steel_rivet(card_d, fx, fy, r=4, is_screw=True)

    # Faded Olive Header Band (8% secondary tier)
    card_d.rectangle([10, 10, card_w - 10, 44], fill=COL_OLIVE_SECONDARY, outline=(80, 90, 72, 255))
    f_c_head = get_font(17, bold=True)
    f_c_sub = get_font(13)
    f_c_val = get_font(14, bold=True)
    card_d.text((20, 22), "SEKTOR 2 // AR9 WALKIRIA", fill=COL_IVORY_BRIGHT, font=f_c_head)

    # Specification Rows on Gray Steel
    card_d.text((20, 60), "TYP UZBROJENIA:", fill=COL_STEEL_EDGE, font=f_c_sub)
    card_d.text((160, 60), "KARABIN SZTURMOWY", fill=COL_AGED_IVORY, font=f_c_val)

    card_d.text((20, 90), "AMUNICJA:", fill=COL_STEEL_EDGE, font=f_c_sub)
    card_d.text((160, 90), "7.62 x 39mm (WZ. 1943)", fill=COL_AGED_IVORY, font=f_c_val)

    card_d.text((20, 120), "POJEMNOŚĆ:", fill=COL_STEEL_EDGE, font=f_c_sub)
    card_d.text((160, 120), "30 / 180 NABOJÓW", fill=COL_AGED_IVORY, font=f_c_val)

    card_d.text((20, 150), "STATUS BRONI:", fill=COL_STEEL_EDGE, font=f_c_sub)
    card_d.text((160, 150), "GOTOWA DO UŻYCIA", fill=(130, 205, 130, 255), font=f_c_val)

    # Action prompt inside card
    card_d.rectangle([15, 195, card_w - 15, 255], fill=COL_STEEL_RECESSED, outline=COL_SELECTOR_AMBER, width=2)
    card_d.text((card_w // 2, 215), "ZWOLNIJ R3 ABY WYBRAĆ", fill=COL_AMBER_BRIGHT, font=f_c_head, anchor="mm")
    card_d.text((card_w // 2, 238), "[RUCH DŁONIĄ LUB PRAWY STIK]", fill=COL_AGED_IVORY, font=f_c_sub, anchor="mm")

    bg.paste(card_plate, (card_x, card_y), card_plate)

    # 5. Bottom Stamped Gray Steel Footplate
    f_leg = get_font(15, bold=True)
    leg_w = 900
    leg_h = 50
    leg_x = (w - leg_w) // 2
    leg_y = h - 75

    leg_plate = create_gray_steel_texture(leg_w, leg_h, (22, 25, 28), noise_amt=14, rust_spots=4)
    ld = ImageDraw.Draw(leg_plate)
    draw_steel_border(ld, 1, 1, leg_w - 2, leg_h - 2, border_w=1)
    ld.text((leg_w // 2, leg_h // 2),
            "[ZWOLNIJ R3] WYBIERZ BROŃ      [L3] ANULUJ      [PRAWY DRĄŻEK / RUCH DŁONIĄ] KIERUNEK",
            fill=COL_AGED_IVORY, font=f_leg, anchor="mm")

    bg.paste(leg_plate, (leg_x, leg_y), leg_plate)

    out_path = os.path.join(MOCKUP_DIR, "weapon_wheel_mockup_v2_1.png")
    bg.save(out_path, "PNG")
    print(f"Generated Gray Steel Weapon Wheel V2.1 mockup: {out_path}")
    return out_path

# ---------------------------------------------------------------------------
# 2. VR Settings Mockup (V2.1) — Weathered Steel & Ash Laboratory Console
# ---------------------------------------------------------------------------
def generate_vr_settings_v2_1():
    w, h = 1920, 1080
    bg = create_facility_background(w, h)

    panel_w, panel_h = 1200, 840
    panel_x = (w - panel_w) // 2
    panel_y = (h - panel_h) // 2 - 20

    # 1. Main Instrument Console Body (Weathered Dark Gray Steel — 70% Identity)
    console_tex = create_gray_steel_texture(panel_w, panel_h, COL_STEEL_BASE, noise_amt=18, rust_spots=18)
    cd = ImageDraw.Draw(console_tex)
    draw_steel_border(cd, 4, 4, panel_w - 5, panel_h - 5, border_w=3)

    # 4 Heavy Reinforced Corner Plates with Cold Steel Fasteners
    c_size = 90
    for cx, cy in [(0, 0), (panel_w - c_size, 0), (0, panel_h - c_size), (panel_w - c_size, panel_h - c_size)]:
        cd.rectangle([cx + 6, cy + 6, cx + c_size - 6, cy + c_size - 6], fill=(26, 30, 34, 240), outline=(62, 70, 80, 255), width=2)
        draw_steel_rivet(cd, cx + 25, cy + 25, r=7, is_screw=False)
        draw_steel_rivet(cd, cx + c_size - 25, cy + 25, r=5, is_screw=True)
        draw_steel_rivet(cd, cx + 25, cy + c_size - 25, r=5, is_screw=True)

    # Top Header Tag: Faded Olive Strip (8% secondary tier) on Steel Console
    cd.rectangle([80, 20, panel_w - 80, 75], fill=(20, 23, 26, 255), outline=(52, 60, 68, 255), width=2)
    # Olive accent bar on left of header
    cd.rectangle([80, 20, 180, 75], fill=COL_OLIVE_SECONDARY)
    for z in range(80, 180, 16):
        cd.polygon([(z, 20), (z + 10, 20), (z - 2, 75), (z - 12, 75)], fill=COL_CAUTION_YELLOW)

    f_title = get_font(22, bold=True)
    f_sub = get_font(12, bold=True)
    cd.text((200, 36), "STANOWISKO KONTROLNE VR // PROTOKÓŁ KATORGA-12", fill=COL_IVORY_BRIGHT, font=f_title)
    cd.text((200, 60), "RADZIECKA STACJA BADAWCZA E99 // APARATURA DOŚWIADCZALNA 1955", fill=COL_STEEL_EDGE, font=f_sub)
    cd.text((panel_w - 100, 48), "SERIA: 4-VR", fill=COL_WEATHERED_BRASS, font=f_sub, anchor="ra")

    # 2. Left Column: Category Tabs (Weathered Graphite Steel Plates)
    tabs = [
        ("OGÓLNE VR", True),
        ("KONTROLER", False),
        ("KOMFORT", False),
        ("OBRAZ", False),
        ("ZAAWANSOWANE", False),
        ("KALIBRACJA BRONI", False),
        ("TMD", False),
        ("DŁONIE", False)
    ]
    tab_y = 100
    f_tab = get_font(14, bold=True)

    for t_title, is_active in tabs:
        t_fill = (44, 50, 58, 255) if is_active else (22, 25, 29, 230)
        t_border = COL_STEEL_EDGE if is_active else (44, 50, 58, 220)
        t_w = 2 if is_active else 1
        cd.rectangle([80, tab_y, 250, tab_y + 44], fill=t_fill, outline=t_border, width=t_w)
        if is_active:
            # Subtle brass registration notch
            cd.rectangle([80, tab_y, 88, tab_y + 44], fill=COL_WEATHERED_BRASS)
        txt_col = COL_IVORY_BRIGHT if is_active else COL_STEEL_EDGE
        cd.text((105, tab_y + 14), t_title, fill=txt_col, font=f_tab)
        tab_y += 52

    # 3. Center/Right Column: Calibration Console Rows (Weathered Graphite Steel)
    row_x = 270
    row_y = 100
    row_w = 810
    row_h = 50

    options = [
        ("WŁĄCZ TRYB VR", "Główny przełącznik systemu VR", "toggle", True, False),
        ("METODA CELOWANIA", "Wybór śledzenia lufy broni", "choice", "KONTROLER RUCHU", True),  # Laser Hovered!
        ("TRYB OBROTU", "Płynny lub skokowy obrót kamery", "choice", "SKOKOWY (30°)", False),
        ("PRĘDKOŚĆ OBROTU", "Czułość drążka obrotu", "slider", 0.65, False),
        ("SKALA ŚWIATA", "Skala stereoskopowa w hełmie", "slider", 0.50, False),
        ("WSKAŹNIK LASEROWY", "Promień celowania w menu", "toggle", True, False),
        ("KIERUNEK RUCHU", "Wektor chodu względem dłoni", "choice", "WG DŁONI", False),
        ("ROZMIAR NOTATEK", "Skala czytelności dokumentów HUD", "slider", 0.80, False)
    ]

    f_opt = get_font(15, bold=True)
    f_desc = get_font(11)
    f_val = get_font(14, bold=True)

    hover_laser_x = 0
    hover_laser_y = 0

    for idx, (title, desc, opt_type, val, is_hov) in enumerate(options):
        cur_y = row_y + idx * 56
        if is_hov:
            # Subtle amber/brass mechanical focus
            r_fill = (44, 42, 36, 255)
            r_border = COL_SELECTOR_AMBER
            r_bw = 2
        else:
            r_fill = (22, 25, 29, 240)
            r_border = (44, 50, 58, 220)
            r_bw = 1

        cd.rectangle([row_x, cur_y, row_x + row_w, cur_y + row_h], fill=r_fill, outline=r_border, width=r_bw)

        if is_hov:
            cd.line([row_x + 4, cur_y + 4, row_x + 14, cur_y + 4], fill=COL_AMBER_BRIGHT, width=2)
            cd.line([row_x + 4, cur_y + 4, row_x + 4, cur_y + 14], fill=COL_AMBER_BRIGHT, width=2)
            cd.line([row_x + row_w - 4, cur_y + 4, row_x + row_w - 14, cur_y + 4], fill=COL_AMBER_BRIGHT, width=2)
            cd.line([row_x + row_w - 4, cur_y + 4, row_x + row_w - 4, cur_y + 14], fill=COL_AMBER_BRIGHT, width=2)

        t_col = COL_IVORY_BRIGHT if is_hov else COL_AGED_IVORY
        cd.text((row_x + 18, cur_y + 8), title, fill=t_col, font=f_opt)
        cd.text((row_x + 18, cur_y + 28), desc, fill=COL_STEEL_EDGE, font=f_desc)

        if opt_type == "toggle":
            tog_x = row_x + row_w - 110
            tog_y = cur_y + 9
            cd.rounded_rectangle([tog_x, tog_y, tog_x + 85, tog_y + 32], radius=6, fill=COL_STEEL_RECESSED, outline=(48, 54, 62, 255))
            if val: # ON
                cd.rounded_rectangle([tog_x + 42, tog_y + 3, tog_x + 80, tog_y + 29], radius=4, fill=(58, 66, 76, 255), outline=COL_STEEL_EDGE)
                cd.ellipse([tog_x + 14, tog_y + 10, tog_x + 26, tog_y + 22], fill=COL_AMBER_BRIGHT, outline=COL_SELECTOR_AMBER)
                cd.text((tog_x + 32, tog_y + 16), "WŁ", fill=COL_IVORY_BRIGHT, font=get_font(10, bold=True), anchor="lm")
            else: # OFF
                cd.rounded_rectangle([tog_x + 5, tog_y + 3, tog_x + 43, tog_y + 29], radius=4, fill=(36, 40, 46, 255), outline=(56, 64, 72, 255))
                cd.ellipse([tog_x + 60, tog_y + 10, tog_x + 72, tog_y + 22], fill=(24, 28, 32, 255))
                cd.text((tog_x + 50, tog_y + 16), "WYŁ", fill=COL_STEEL_EDGE, font=get_font(10, bold=True), anchor="rm")

        elif opt_type == "choice":
            ch_x = row_x + row_w - 240
            ch_y = cur_y + 9
            cd.rectangle([ch_x, ch_y, ch_x + 220, ch_y + 32], fill=COL_STEEL_RECESSED, outline=COL_SELECTOR_AMBER if is_hov else (44, 50, 58, 255), width=1)
            v_col = COL_AMBER_BRIGHT if is_hov else COL_AGED_IVORY
            cd.text((ch_x + 110, ch_y + 16), str(val), fill=v_col, font=f_val, anchor="mm")
            cd.text((ch_x + 14, ch_y + 16), "◄", fill=COL_STEEL_EDGE, font=f_desc, anchor="mm")
            cd.text((ch_x + 206, ch_y + 16), "►", fill=COL_STEEL_EDGE, font=f_desc, anchor="mm")

            if is_hov:
                hover_laser_x = panel_x + ch_x + 110
                hover_laser_y = panel_y + ch_y + 16

        elif opt_type == "slider":
            sl_x = row_x + row_w - 240
            sl_y = cur_y + 18
            sl_w = 220
            cd.rounded_rectangle([sl_x, sl_y, sl_x + sl_w, sl_y + 12], radius=3, fill=COL_STEEL_RECESSED, outline=(44, 50, 58, 255))
            for tx in range(sl_x + 10, sl_x + sl_w, 20):
                cd.line([tx, sl_y - 4, tx, sl_y - 1], fill=COL_STEEL_EDGE, width=1)
            knob_pos_x = int(sl_x + sl_w * float(val))
            # Milled steel knob with brass index
            cd.rectangle([knob_pos_x - 8, sl_y - 8, knob_pos_x + 8, sl_y + 20], fill=(58, 66, 76, 255), outline=(36, 41, 48, 255), width=1)
            cd.line([knob_pos_x, sl_y - 6, knob_pos_x, sl_y + 18], fill=COL_WEATHERED_BRASS, width=2)

    # 4. Mechanical Scroll Bar in Cold Steel
    scr_x = row_x + row_w + 12
    cd.rectangle([scr_x, row_y, scr_x + 18, row_y + 440], fill=COL_STEEL_RECESSED, outline=(40, 46, 54, 255))
    cd.rectangle([scr_x + 2, row_y + 40, scr_x + 16, row_y + 120], fill=(54, 62, 72, 255), outline=(72, 82, 92, 255))
    for gy in range(row_y + 55, row_y + 105, 10):
        cd.line([scr_x + 4, gy, scr_x + 14, gy], fill=(26, 30, 36, 255), width=2)

    # 5. Stamped Gray Steel Footplate
    cd.rectangle([80, panel_h - 60, panel_w - 80, panel_h - 20], fill=(20, 23, 26, 255), outline=(48, 54, 62, 255))
    cd.text((panel_w // 2, panel_h - 40),
            "[R2 / SPUST] WYBIERZ      [○ / KOŁO] WSTECZ      [LEWY DRĄŻEK] NAWIGACJA      [PRZYTRZYMAJ △] ZAMKNIJ",
            fill=COL_AGED_IVORY, font=f_sub, anchor="mm")

    bg.paste(console_tex, (panel_x, panel_y), console_tex)

    # 6. Optical Laboratory Laser Pointer
    laser_im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    ld = ImageDraw.Draw(laser_im)
    start_x = w - 160
    start_y = h - 60
    ld.line([start_x, start_y, hover_laser_x, hover_laser_y], fill=(215, 65, 45, 160), width=2)
    ld.line([start_x, start_y, hover_laser_x, hover_laser_y], fill=(255, 180, 120, 240), width=1)
    ld.ellipse([hover_laser_x - 6, hover_laser_y - 6, hover_laser_x + 6, hover_laser_y + 6],
               fill=(255, 230, 180, 255), outline=(215, 65, 45, 255), width=2)

    bg = Image.alpha_composite(bg, laser_im)

    out_path = os.path.join(MOCKUP_DIR, "vr_settings_screen_mockup_v2_1.png")
    bg.save(out_path, "PNG")
    print(f"Generated Gray Steel VR Settings V2.1 mockup: {out_path}")
    return out_path

# ---------------------------------------------------------------------------
# 3. Pause Menu Mockup (V2.1) — Weathered Steel & Ash Operations Terminal
# ---------------------------------------------------------------------------
def generate_pause_menu_v2_1():
    w, h = 1920, 1080
    bg = create_facility_background(w, h)

    panel_w, panel_h = 680, 840
    panel_x = (w - panel_w) // 2
    panel_y = (h - panel_h) // 2 - 20

    # 1. Main Terminal Body (Cast Graphite Iron / Cold Steel — 70% Identity)
    slate_tex = create_gray_steel_texture(panel_w, panel_h, COL_STEEL_BASE, noise_amt=20, rust_spots=14)
    sd = ImageDraw.Draw(slate_tex)
    draw_steel_border(sd, 4, 4, panel_w - 5, panel_h - 5, border_w=3)

    for cx, cy in [(14, 14), (panel_w - 14, 14), (14, panel_h - 14), (panel_w - 14, panel_h - 14)]:
        draw_steel_rivet(sd, cx, cy, r=6, is_screw=True)

    # 2. Header Tag (Dark Steel with Secondary Faded Olive Stamp — 8% Olive)
    sd.rectangle([30, 25, panel_w - 30, 110], fill=(20, 23, 26, 255), outline=(50, 58, 66, 255), width=2)
    # Faded olive secondary backing for Soviet seal
    sd.rectangle([34, 29, 115, 106], fill=COL_OLIVE_MUTED)
    sd.ellipse([45, 38, 105, 98], fill=(32, 28, 26, 255), outline=COL_OXIDIZED_RUST, width=2)
    f_seal = get_font(12, bold=True)
    sd.text((75, 62), "СССР", fill=COL_OXIDIZED_RUST, font=f_seal, anchor="mm")
    sd.text((75, 78), "1955", fill=COL_OXIDIZED_RUST, font=f_seal, anchor="mm")

    f_p_head = get_font(24, bold=True)
    f_p_sub = get_font(12, bold=True)
    sd.text((130, 48), "PAUZA // KONTROLA OPERACJI", fill=COL_IVORY_BRIGHT, font=f_p_head)
    sd.text((130, 76), "STACJA BADAWCZA KATORGA-12 // PROTOKÓŁ ZATRZYMANIA", fill=COL_STEEL_EDGE, font=f_p_sub)
    # Restrained E99 Cyan indicator (2% tier)
    sd.text((130, 94), "STAN: AKTYWNY CHRONO-BUFOR E99", fill=COL_E99_CYAN, font=f_p_sub)

    # 3. Tactical Menu Button Rows (Weathered Graphite Steel Plates)
    buttons = [
        ("WZNOWIENIE GRY", False, False),
        ("WCZYTAJ STAN", False, False),
        ("ZAPISZ STAN", False, False),
        ("OPCJE VR // VR SETTINGS", True, True),   # Dedicated synthetic row, Laser Hovered!
        ("USTAWIENIA GRY", False, False),
        ("MENU GŁÓWNE", False, False),
        ("WYJŚCIE DO PULPITU", False, False)
    ]

    btn_x = 60
    btn_start_y = 140
    btn_w = panel_w - 120
    btn_h = 58
    gap = 68

    f_b_title = get_font(17, bold=True)
    f_b_sub = get_font(10, bold=True)

    hover_laser_x = 0
    hover_laser_y = 0

    for i, (b_title, is_vr_row, is_hov) in enumerate(buttons):
        by = btn_start_y + i * gap

        if is_vr_row and is_hov:
            # Synthetic VR row: Weathered dark steel with brass border and warm amber focus
            b_fill = (44, 40, 32, 255)
            b_border = COL_AMBER_BRIGHT
            b_bw = 2
            # Left amber tab
            sd.rectangle([btn_x, by, btn_x + 12, by + btn_h], fill=COL_SELECTOR_AMBER)
            sd.line([btn_x + 4, by + 4, btn_x + 16, by + 4], fill=COL_AMBER_BRIGHT, width=2)
            sd.line([btn_x + 4, by + 4, btn_x + 4, by + 16], fill=COL_AMBER_BRIGHT, width=2)
            sd.line([btn_x + btn_w - 4, by + 4, btn_x + btn_w - 16, by + 4], fill=COL_AMBER_BRIGHT, width=2)
            sd.line([btn_x + btn_w - 4, by + 4, btn_x + btn_w - 4, by + 16], fill=COL_AMBER_BRIGHT, width=2)
            t_col = (255, 255, 255)
            hover_laser_x = panel_x + btn_x + btn_w // 2 + 60
            hover_laser_y = panel_y + by + btn_h // 2
        else:
            b_fill = (22, 25, 29, 240)
            b_border = (44, 50, 58, 220)
            b_bw = 1
            t_col = COL_AGED_IVORY

        sd.rectangle([btn_x, by, btn_x + btn_w, by + btn_h], fill=b_fill, outline=b_border, width=b_bw)
        sd.text((btn_x + btn_w // 2, by + btn_h // 2 - (6 if is_vr_row else 0)), b_title, fill=t_col, font=f_b_title, anchor="mm")
        if is_vr_row:
            sd.text((btn_x + btn_w // 2, by + btn_h // 2 + 14), "KONFIGURACJA ŚLEDZENIA I PARAMETRÓW HEŁMU", fill=COL_STEEL_EDGE, font=f_b_sub, anchor="mm")

    # 4. Stamped Steel Footplate
    sd.rectangle([30, panel_h - 75, panel_w - 30, panel_h - 25], fill=(20, 23, 26, 255), outline=(44, 50, 58, 255))
    sd.text((panel_w // 2, panel_h - 50),
            "[R2] WYBIERZ      [○] WSTECZ      [LEWY DRĄŻEK] NAWIGACJA      [△] WYJŚCIE",
            fill=COL_AGED_IVORY, font=get_font(13, bold=True), anchor="mm")

    bg.paste(slate_tex, (panel_x, panel_y), slate_tex)

    # 5. Laser Pointer
    laser_im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    ld = ImageDraw.Draw(laser_im)
    start_x = w - 180
    start_y = h - 60
    ld.line([start_x, start_y, hover_laser_x, hover_laser_y], fill=(215, 65, 45, 160), width=2)
    ld.line([start_x, start_y, hover_laser_x, hover_laser_y], fill=(255, 180, 120, 240), width=1)
    ld.ellipse([hover_laser_x - 6, hover_laser_y - 6, hover_laser_x + 6, hover_laser_y + 6],
               fill=(255, 230, 180, 255), outline=(215, 65, 45, 255), width=2)

    bg = Image.alpha_composite(bg, laser_im)

    out_path = os.path.join(MOCKUP_DIR, "pause_menu_mockup_v2_1.png")
    bg.save(out_path, "PNG")
    print(f"Generated Gray Steel Pause Menu V2.1 mockup: {out_path}")
    return out_path

# ---------------------------------------------------------------------------
# 4. Side-by-Side Comparison Generator (V2 Olive vs V2.1 Gray Steel)
# ---------------------------------------------------------------------------
def generate_v2_vs_v2_1_comparison(v2_file, v2_1_file, out_file, title):
    p_v2 = os.path.join(MOCKUP_DIR, v2_file)
    p_v2_1 = os.path.join(MOCKUP_DIR, v2_1_file)
    if not os.path.exists(p_v2) or not os.path.exists(p_v2_1):
        print(f"Missing {v2_file} or {v2_1_file}")
        return

    im1 = Image.open(p_v2).convert("RGBA").resize((1240, 698), Image.Resampling.LANCZOS)
    im2 = Image.open(p_v2_1).convert("RGBA").resize((1240, 698), Image.Resampling.LANCZOS)

    cw, ch = 2560, 840
    comp = Image.new("RGBA", (cw, ch), (16, 18, 20, 255))
    d = ImageDraw.Draw(comp)

    f_main = get_font(26, bold=True)
    f_tag = get_font(18, bold=True)
    f_expl = get_font(13)

    d.text((cw // 2, 35), title, fill=COL_AGED_IVORY, font=f_main, anchor="mm")

    comp.paste(im1, (30, 80))
    comp.paste(im2, (1290, 80))

    # Left: V2 (Olive dominant)
    d.rectangle([28, 78, 1272, 780], outline=(150, 130, 70, 255), width=3)
    d.rectangle([30, 80, 420, 120], fill=(70, 78, 55, 240))
    d.text((45, 98), "WERSJA V2 (DOMINACJA OLIWKOWA)", fill=(255, 255, 255), font=f_tag, anchor="lm")
    d.text((45, 795), "CECHA: Wojskowa zieleń oliwkowa dominuje tło i panele (zbyt militarna, zbyt mało laboratoryjna)",
           fill=(210, 200, 160), font=f_expl)

    # Right: V2.1 (Weathered Gray Steel & Graphite 70%)
    d.rectangle([1288, 78, 2532, 780], outline=(85, 175, 85, 255), width=3)
    d.rectangle([1290, 80, 1790, 120], fill=(45, 115, 60, 240))
    d.text((1305, 98), "WERSJA V2.1 (70% STAL / GRAFIT / BETON)", fill=(255, 255, 255), font=f_tag, anchor="lm")
    d.text((1305, 795), "CECHA: 70% stal/grafit, 15% kość słoniowa, 8% zieleń wtórna, 5% mosiądz/bursztyn, 2% E99 cyan",
           fill=(160, 230, 160), font=f_expl)

    out_path = os.path.join(MOCKUP_DIR, out_file)
    comp.save(out_path, "PNG")
    print(f"Generated V2 vs V2.1 comparison: {out_path}")

if __name__ == "__main__":
    generate_weapon_wheel_v2_1()
    generate_vr_settings_v2_1()
    generate_pause_menu_v2_1()

    generate_v2_vs_v2_1_comparison("weapon_wheel_mockup_v2.png", "weapon_wheel_mockup_v2_1.png",
                                   "compare_weapon_wheel_v2_1.png",
                                   "PORÓWNANIE V2 vs V2.1: KOŁO BRONI (WEAPON WHEEL)")
    generate_v2_vs_v2_1_comparison("vr_settings_screen_mockup_v2.png", "vr_settings_screen_mockup_v2_1.png",
                                   "compare_vr_settings_v2_1.png",
                                   "PORÓWNANIE V2 vs V2.1: USTAWIENIA VR (VR SETTINGS CONSOLE)")
    generate_v2_vs_v2_1_comparison("pause_menu_mockup_v2.png", "pause_menu_mockup_v2_1.png",
                                   "compare_pause_menu_v2_1.png",
                                   "PORÓWNANIE V2 vs V2.1: MENU PAUZY (PAUSE TERMINAL)")
