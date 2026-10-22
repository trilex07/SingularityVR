"""
Singularity VR - UI Art Direction Correction (v2)
Generates authentic Soviet industrial retrofuturistic concept mockups:
1. Weapon Wheel — Soviet mechanical selector
2. VR Settings — Worn laboratory control console
3. Pause Menu — Aged industrial terminal
Also generates side-by-side comparison images with the rejected e83eb8b cyberpunk baseline.
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
# Soviet Industrial Material Palette (Color Specs)
# ---------------------------------------------------------------------------
COL_GRAPHITE_BASE = (38, 42, 38)        # #262A26 - Cold rolled cast iron
COL_GRAPHITE_DARK = (24, 27, 24)        # #181B18 - Recessed dark chassis
COL_MILITARY_OLIVE = (62, 68, 54)       # #3E4436 - Soviet military olive paint
COL_MILITARY_OLIVE_DARK = (45, 50, 40)  # #2D3228 - Aged olive
COL_WEATHERED_BRASS = (142, 131, 105)   # #8E8369 - Worn brass/bronze knobs & plates
COL_AGED_IVORY = (211, 198, 162)        # #D3C6A2 - Faded ivory stenciled typography
COL_OXIDIZED_RUST = (145, 85, 45)       # #91552D - Corner rust & patina
COL_RUST_DARK = (95, 50, 25)            # #5F3219 - Deep corrosion
COL_SELECTOR_AMBER = (215, 125, 35)     # #D77D23 - Faded amber selector indicator
COL_AMBER_BRIGHT = (245, 165, 65)       # #F5A541 - High amber bulb
COL_E99_RESTRAINED = (65, 175, 170)     # #41AFAA - Rare, restrained E99 phosphor cyan
COL_E99_CORE = (140, 240, 235)          # #8CF0EB - Core E99 focus point
COL_CAUTION_YELLOW = (205, 165, 45)     # #CDA52D - Faded industrial hazard paint
COL_BARE_STEEL_EDGE = (115, 125, 120)   # #737D78 - Chipped paint bare metal edge

def get_font(size, bold=False):
    font_path = r"C:\Windows\Fonts\bahnschrift.ttf"
    if not os.path.exists(font_path):
        font_path = r"C:\Windows\Fonts\arial.ttf"
    try:
        return ImageFont.truetype(font_path, size)
    except Exception:
        return ImageFont.load_default()

# ---------------------------------------------------------------------------
# Procedural Industrial Texture Helpers
# ---------------------------------------------------------------------------
def create_metal_texture(w, h, base_rgb, noise_amt=18, brush_horizontal=True, rust_spots=15):
    """Creates a weathered industrial metallic surface with micro-grain and scratches."""
    np.random.seed(42) # Reproducible patina
    base = np.full((h, w, 3), base_rgb, dtype=np.float32)

    # Micro noise
    noise = np.random.uniform(-noise_amt, noise_amt, (h, w, 1))
    base += noise

    # Brushed lines
    if brush_horizontal:
        lines = np.random.uniform(-6, 6, (h, 1, 1))
        base += lines

    # Rust/wear patches
    for _ in range(rust_spots):
        rx = np.random.randint(0, w)
        ry = np.random.randint(0, h)
        rw = np.random.randint(15, 60)
        rh = np.random.randint(15, 60)
        x0, x1 = max(0, rx - rw), min(w, rx + rw)
        y0, y1 = max(0, ry - rh), min(h, ry + rh)
        if x1 > x0 and y1 > y0:
            patch = np.zeros((y1 - y0, x1 - x0, 3), dtype=np.float32)
            patch[:, :] = [COL_OXIDIZED_RUST[0] - base_rgb[0],
                           COL_OXIDIZED_RUST[1] - base_rgb[1],
                           COL_OXIDIZED_RUST[2] - base_rgb[2]]
            alpha_mask = np.random.uniform(0, 0.25, (y1 - y0, x1 - x0, 1))
            base[y0:y1, x0:x1] += patch * alpha_mask

    base = np.clip(base, 0, 255).astype(np.uint8)
    im = Image.fromarray(base).convert("RGBA")

    # Add subtle scratch lines
    draw = ImageDraw.Draw(im)
    for _ in range(25):
        sx1 = np.random.randint(0, w)
        sy1 = np.random.randint(0, h)
        sx2 = sx1 + np.random.randint(-40, 40)
        sy2 = sy1 + np.random.randint(-10, 10)
        draw.line([sx1, sy1, sx2, sy2], fill=(COL_BARE_STEEL_EDGE[0], COL_BARE_STEEL_EDGE[1], COL_BARE_STEEL_EDGE[2], 120), width=1)

    return im

def draw_industrial_rivet(draw, cx, cy, r=6, is_screw=False):
    """Draws a physical industrial steel rivet or slotted brass screw."""
    # Shadow/hole
    draw.ellipse([cx - r - 1, cy - r - 1, cx + r + 1, cy + r + 1], fill=(15, 18, 15, 240))
    # Beveled head
    draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=COL_WEATHERED_BRASS if is_screw else (55, 60, 55, 255),
                 outline=(90, 98, 90, 255), width=1)
    if is_screw:
        # Screw slot
        draw.line([cx - r + 2, cy - r + 2, cx + r - 2, cy + r - 2], fill=(20, 22, 20, 240), width=2)
    else:
        # Dome highlight
        draw.ellipse([cx - r//2, cy - r//2, cx + r//2, cy + r//2], fill=(85, 95, 88, 255))

def draw_chipped_border(draw, x0, y0, x1, y1, border_w=3):
    """Draws a weathered border with subtle paint wear and exposed steel."""
    # Outer dark shadow
    draw.rectangle([x0 - 1, y0 - 1, x1 + 1, y1 + 1], outline=(18, 22, 18, 200), width=1)
    # Main steel border
    draw.rectangle([x0, y0, x1, y1], outline=(80, 88, 78, 240), width=border_w)
    # Paint wear notches
    for bx in range(x0 + 10, x1 - 10, 40):
        if (bx % 3) == 0:
            draw.line([bx, y0 - 1, bx + 6, y0 - 1], fill=COL_BARE_STEEL_EDGE, width=2)
            draw.line([bx, y1 + 1, bx + 6, y1 + 1], fill=COL_BARE_STEEL_EDGE, width=2)

def load_weapon_silhouette(name, target_w, target_h, tint=COL_AGED_IVORY):
    """Loads authentic silhouette from references/ and re-tints to aged ivory enamel."""
    fn_map = {
        "centurion": "icon-pistol-final.png",
        "valkyrie": "icon-assault-final.png",
        "spikeshot": "icon-shotgun-final.png",
        "kasimov": "icon-sniper-final.png",
        "tmd": "icon-tmd-final.png"
    }
    src_path = os.path.join(REF_DIR, fn_map.get(name, "icon-pistol-final.png"))
    if not os.path.exists(src_path):
        # Fallback to white canvas if not found
        fallback = Image.new("RGBA", (target_w, target_h), (0, 0, 0, 0))
        return fallback

    raw = Image.open(src_path).convert("RGBA")
    # Resize preserving aspect ratio
    raw.thumbnail((target_w, target_h), Image.Resampling.LANCZOS)

    # Tint white silhouette to aged ivory with subtle alpha
    r, g, b, a = raw.split()
    tint_im = Image.new("RGBA", raw.size, (tint[0], tint[1], tint[2], 255))
    tint_im.putalpha(a)
    return tint_im

# ---------------------------------------------------------------------------
# Background Scene (Authentic Katorga-12 Laboratory Setting)
# ---------------------------------------------------------------------------
def create_lab_background(w=1920, h=1080):
    """Creates a dark, decaying industrial facility backdrop with concrete, pipes, and cold atmosphere."""
    bg = Image.new("RGBA", (w, h), (18, 22, 20, 255))
    d = ImageDraw.Draw(bg)

    # Heavy concrete wall shading
    wall = create_metal_texture(w, h, (24, 28, 25), noise_amt=22, rust_spots=30)
    bg = Image.alpha_composite(bg, wall)
    d = ImageDraw.Draw(bg)

    # Decaying vertical steel pillars & conduits
    for px in [180, 520, 1400, 1740]:
        d.rectangle([px, 0, px + 40, h], fill=(16, 19, 17, 240), outline=(28, 34, 30, 255), width=2)
        # Conduit rivets
        for py in range(40, h, 80):
            draw_industrial_rivet(d, px + 20, py, r=4)

    # Horizontal rusty steam pipes at top & bottom
    for py in [60, 980]:
        d.rectangle([0, py, w, py + 36], fill=(32, 26, 22, 250), outline=(50, 42, 34, 255), width=2)
        d.line([0, py + 8, w, py + 8], fill=(65, 52, 40, 180), width=2)

    # Subtle vignette shadow
    vignette = Image.new("L", (w, h), 0)
    vd = ImageDraw.Draw(vignette)
    vd.ellipse([w // 2 - 800, h // 2 - 500, w // 2 + 800, h // 2 + 500], fill=255)
    vignette = vignette.filter(ImageFilter.GaussianBlur(180))
    vignette = ImageOps.invert(vignette)
    bg_shadow = Image.new("RGBA", (w, h), (8, 10, 9, 210))
    bg_shadow.putalpha(vignette)
    bg = Image.alpha_composite(bg, bg_shadow)

    return bg

# ---------------------------------------------------------------------------
# 1. Weapon Wheel Mockup (v2) — Soviet Mechanical Selector
# ---------------------------------------------------------------------------
def generate_weapon_wheel_v2():
    w, h = 1920, 1080
    bg = create_lab_background(w, h)
    cx, cy = w // 2, h // 2

    # Dial Dimensions
    r_outer = 380
    r_inner = 170

    # 1. Heavy Outer Circular Instrument Housing (Oxidized Graphite Iron)
    housing_im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    hd = ImageDraw.Draw(housing_im)

    # Outer cast iron ring
    hd.ellipse([cx - r_outer - 35, cy - r_outer - 35, cx + r_outer + 35, cy + r_outer + 35],
               fill=(28, 32, 28, 250), outline=(50, 56, 48, 255), width=4)
    # Beveled rim
    hd.ellipse([cx - r_outer - 15, cy - r_outer - 15, cx + r_outer + 15, cy + r_outer + 15],
               outline=(75, 84, 72, 255), width=3)

    # 16 Heavy perimeter hex bolts
    for i in range(16):
        ang = math.radians(i * (360 / 16))
        bx = cx + (r_outer + 20) * math.cos(ang)
        by = cy + (r_outer + 20) * math.sin(ang)
        draw_industrial_rivet(hd, int(bx), int(by), r=7, is_screw=False)

    # 360-degree engraved graduation ticks (faded ivory & amber)
    for a in range(0, 360, 5):
        rad = math.radians(a)
        is_major = (a % 30 == 0)
        is_cardinal = (a % 90 == 0)
        t_len = 18 if is_cardinal else (12 if is_major else 6)
        t_col = (COL_SELECTOR_AMBER[0], COL_SELECTOR_AMBER[1], COL_SELECTOR_AMBER[2], 240) if is_cardinal else ((COL_AGED_IVORY[0], COL_AGED_IVORY[1], COL_AGED_IVORY[2], 160) if is_major else (85, 95, 85, 120))
        t_w = 2 if is_major else 1
        x1 = cx + (r_outer - 6) * math.cos(rad)
        y1 = cy + (r_outer - 6) * math.sin(rad)
        x2 = cx + (r_outer - 6 - t_len) * math.cos(rad)
        y2 = cy + (r_outer - 6 - t_len) * math.sin(rad)
        hd.line([x1, y1, x2, y2], fill=t_col, width=t_w)

    # Stamped Soviet Factory Markings along rim
    f_stamp = get_font(13, bold=True)
    hd.text((cx, cy - r_outer - 12), "КАТОРГА-12 // ЗАВ. 1955 // СЕКТОРНЫЙ ПЕРЕКЛЮЧАТЕЛЬ",
            fill=(COL_WEATHERED_BRASS[0], COL_WEATHERED_BRASS[1], COL_WEATHERED_BRASS[2], 220), font=f_stamp, anchor="mm")
    hd.text((cx, cy + r_outer + 12), "ГОСТ 1955 // МЕХАНИЗМ ВЫБОРА ОРУЖИЯ // СЕРИЯ 4-С",
            fill=(COL_WEATHERED_BRASS[0], COL_WEATHERED_BRASS[1], COL_WEATHERED_BRASS[2], 220), font=f_stamp, anchor="mm")

    bg = Image.alpha_composite(bg, housing_im)

    # 2. Four Mechanical Sector Wedges (Audited Order from catalog: Centurion, Valkyrie, Spikeshot, Kasimov)
    sectors = [
        # (Name, Polish Label, Angle Start, Angle End, Icon Name, State)
        ("centurion", "CENTURION .44", 225 + 2, 315 - 2, "centurion", "idle"),
        ("valkyrie", "AR9 WALKIRIA", 315 + 2, 405 - 2, "valkyrie", "selected"),  # Selected!
        ("spikeshot", "SPIKESHOT", 45 + 2, 135 - 2, "spikeshot", "held"),        # Held in hands!
        ("kasimov", "KASIMOV SNV-E99", 135 + 2, 225 - 2, "kasimov", "idle")
    ]

    wedge_im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    wd = ImageDraw.Draw(wedge_im)

    for sec_id, label_pl, a_start, a_end, icon_name, state in sectors:
        # Build polygon arc
        points = []
        steps = 30
        for i in range(steps + 1):
            ang = math.radians(a_start + (a_end - a_start) * (i / steps))
            points.append((cx + (r_outer - 28) * math.cos(ang), cy + (r_outer - 28) * math.sin(ang)))
        for i in range(steps, -1, -1):
            ang = math.radians(a_start + (a_end - a_start) * (i / steps))
            points.append((cx + (r_inner + 8) * math.cos(ang), cy + (r_inner + 8) * math.sin(ang)))

        if state == "selected":
            # Warm Soviet amber highlight for active selection (committed on release)
            fill_col = (COL_SELECTOR_AMBER[0], COL_SELECTOR_AMBER[1], COL_SELECTOR_AMBER[2], 180)
            border_col = (COL_AMBER_BRIGHT[0], COL_AMBER_BRIGHT[1], COL_AMBER_BRIGHT[2], 255)
            border_w = 4
        elif state == "held":
            # Brighter steel fill for currently carried weapon
            fill_col = (48, 56, 48, 220)
            border_col = (110, 125, 110, 240)
            border_w = 2
        else:
            # Idle dark military olive/graphite plate
            fill_col = (30, 35, 30, 210)
            border_col = (55, 65, 55, 200)
            border_w = 2

        wd.polygon(points, fill=fill_col, outline=border_col)

        # Thick boundary lines for mechanical engraved look
        for i in range(steps):
            ang1 = math.radians(a_start + (a_end - a_start) * (i / steps))
            ang2 = math.radians(a_start + (a_end - a_start) * ((i + 1) / steps))
            wd.line([(cx + (r_outer - 28) * math.cos(ang1), cy + (r_outer - 28) * math.sin(ang1)),
                     (cx + (r_outer - 28) * math.cos(ang2), cy + (r_outer - 28) * math.sin(ang2))],
                    fill=border_col, width=border_w)

        # Sector Label & Icon Position
        mid_ang = math.radians((a_start + a_end) / 2)
        icon_r = (r_outer + r_inner) / 2 - 10
        ix = int(cx + icon_r * math.cos(mid_ang))
        iy = int(cy + icon_r * math.sin(mid_ang))

        # Paste Weapon Silhouette
        icon_tint = (255, 255, 255) if state == "selected" else COL_AGED_IVORY
        w_icon = load_weapon_silhouette(icon_name, 140, 60, tint=icon_tint)
        wedge_im.paste(w_icon, (ix - w_icon.width // 2, iy - w_icon.height // 2 - 12), w_icon)

        # Stenciled Polish Label
        f_lbl = get_font(15, bold=True)
        lbl_col = (255, 255, 255) if state == "selected" else COL_AGED_IVORY
        wd.text((ix, iy + 26), label_pl, fill=lbl_col, font=f_lbl, anchor="mm")

        # Held tag if held
        if state == "held":
            f_held = get_font(11, bold=True)
            wd.text((ix, iy + 44), "[W DŁONIACH]", fill=(COL_WEATHERED_BRASS[0], COL_WEATHERED_BRASS[1], COL_WEATHERED_BRASS[2], 240), font=f_held, anchor="mm")
        elif state == "selected":
            f_sel = get_font(11, bold=True)
            wd.text((ix, iy + 44), "[WYBIERZ]", fill=(255, 240, 200, 255), font=f_sel, anchor="mm")

    bg = Image.alpha_composite(bg, wedge_im)

    # 3. Center Mechanical Selector Knob & Indicator Needle
    hub_im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    hud_d = ImageDraw.Draw(hub_im)

    # Milled weathered brass knob
    r_hub = r_inner - 10
    hud_d.ellipse([cx - r_hub, cy - r_hub, cx + r_hub, cy + r_hub],
                  fill=COL_GRAPHITE_DARK, outline=(70, 78, 68, 255), width=3)
    hud_d.ellipse([cx - r_hub + 12, cy - r_hub + 12, cx + r_hub - 12, cy + r_hub - 12],
                  fill=(COL_MILITARY_OLIVE_DARK[0], COL_MILITARY_OLIVE_DARK[1], COL_MILITARY_OLIVE_DARK[2], 250),
                  outline=COL_WEATHERED_BRASS, width=2)

    # Knurled grip grooves around perimeter
    for ga in range(0, 360, 15):
        grad = math.radians(ga)
        gx1 = cx + (r_hub - 10) * math.cos(grad)
        gy1 = cy + (r_hub - 10) * math.sin(grad)
        gx2 = cx + r_hub * math.cos(grad)
        gy2 = cy + r_hub * math.sin(grad)
        hud_d.line([gx1, gy1, gx2, gy2], fill=(50, 56, 48, 255), width=2)

    # Selector needle pointing toward Sector 2 (AR9 Valkyrie at angle 0° / East)
    needle_ang = 0.0 # East
    nx = cx + (r_hub + 15) * math.cos(needle_ang)
    ny = cy + (r_hub + 15) * math.sin(needle_ang)
    hud_d.polygon([(nx, ny),
                   (cx + (r_hub - 25) * math.cos(needle_ang - 0.25), cy + (r_hub - 25) * math.sin(needle_ang - 0.25)),
                   (cx + (r_hub - 25) * math.cos(needle_ang + 0.25), cy + (r_hub - 25) * math.sin(needle_ang + 0.25))],
                  fill=COL_SELECTOR_AMBER, outline=(255, 200, 120, 255))

    # Center Hub Screws & Stencil
    for a in [45, 135, 225, 315]:
        rad = math.radians(a)
        draw_industrial_rivet(hud_d, int(cx + 42 * math.cos(rad)), int(cy + 42 * math.sin(rad)), r=5, is_screw=True)

    f_hub_title = get_font(18, bold=True)
    f_hub_sub = get_font(11, bold=True)
    hud_d.text((cx, cy - 8), "WYBÓR", fill=COL_AGED_IVORY, font=f_hub_title, anchor="mm")
    hud_d.text((cx, cy + 12), "BRONI", fill=COL_AGED_IVORY, font=f_hub_title, anchor="mm")
    hud_d.text((cx, cy + 32), "4 POZYCJE", fill=(COL_WEATHERED_BRASS[0], COL_WEATHERED_BRASS[1], COL_WEATHERED_BRASS[2], 220), font=f_hub_sub, anchor="mm")

    bg = Image.alpha_composite(bg, hub_im)

    # 4. Realistic Weapon Data Clipboard Plate (Right Side)
    card_x = cx + 450
    card_y = cy - 140
    card_w = 400
    card_h = 280

    card_plate = create_metal_texture(card_w, card_h, COL_MILITARY_OLIVE_DARK, noise_amt=16, rust_spots=10)
    card_d = ImageDraw.Draw(card_plate)
    draw_chipped_border(card_d, 2, 2, card_w - 3, card_h - 3, border_w=2)
    # 4 Corner screws
    draw_industrial_rivet(card_d, 14, 14, r=4, is_screw=True)
    draw_industrial_rivet(card_d, card_w - 14, 14, r=4, is_screw=True)
    draw_industrial_rivet(card_d, 14, card_h - 14, r=4, is_screw=True)
    draw_industrial_rivet(card_d, card_w - 14, card_h - 14, r=4, is_screw=True)

    # Stenciled Content
    f_c_head = get_font(17, bold=True)
    f_c_sub = get_font(13)
    f_c_val = get_font(14, bold=True)

    card_d.rectangle([10, 10, card_w - 10, 44], fill=(24, 28, 24, 240), outline=(50, 58, 48, 255))
    card_d.text((20, 22), "SEKTOR 2 // AR9 WALKIRIA", fill=COL_AMBER_BRIGHT, font=f_c_head)

    card_d.text((20, 60), "TYP UZBROJENIA:", fill=COL_WEATHERED_BRASS, font=f_c_sub)
    card_d.text((160, 60), "KARABIN SZTURMOWY", fill=COL_AGED_IVORY, font=f_c_val)

    card_d.text((20, 90), "AMUNICJA:", fill=COL_WEATHERED_BRASS, font=f_c_sub)
    card_d.text((160, 90), "7.62 x 39mm (WZ. 1943)", fill=COL_AGED_IVORY, font=f_c_val)

    card_d.text((20, 120), "POJEMNOŚĆ:", fill=COL_WEATHERED_BRASS, font=f_c_sub)
    card_d.text((160, 120), "30 / 180 NABOJÓW", fill=COL_AGED_IVORY, font=f_c_val)

    card_d.text((20, 150), "STATUS BRONI:", fill=COL_WEATHERED_BRASS, font=f_c_sub)
    card_d.text((160, 150), "GOTOWA DO UŻYCIA", fill=(120, 195, 120, 255), font=f_c_val)

    # Action prompt inside card
    card_d.rectangle([15, 195, card_w - 15, 255], fill=(20, 24, 20, 255), outline=COL_SELECTOR_AMBER, width=2)
    card_d.text((card_w // 2, 215), "ZWOLNIJ R3 ABY WYBRAĆ", fill=COL_AMBER_BRIGHT, font=f_c_head, anchor="mm")
    card_d.text((card_w // 2, 238), "[RUCH DŁONIĄ LUB PRAWY STIK]", fill=COL_AGED_IVORY, font=f_c_sub, anchor="mm")

    bg.paste(card_plate, (card_x, card_y), card_plate)

    # 5. Bottom PSVR2 Legend Plate
    f_leg = get_font(15, bold=True)
    leg_w = 900
    leg_h = 50
    leg_x = (w - leg_w) // 2
    leg_y = h - 75

    leg_plate = create_metal_texture(leg_w, leg_h, (24, 28, 24), noise_amt=14, rust_spots=4)
    ld = ImageDraw.Draw(leg_plate)
    draw_chipped_border(ld, 1, 1, leg_w - 2, leg_h - 2, border_w=1)
    ld.text((leg_w // 2, leg_h // 2),
            "[ZWOLNIJ R3] WYBIERZ BROŃ      [L3] ANULUJ      [PRAWY DRĄŻEK / RUCH DŁONIĄ] KIERUNEK",
            fill=COL_AGED_IVORY, font=f_leg, anchor="mm")

    bg.paste(leg_plate, (leg_x, leg_y), leg_plate)

    out_path = os.path.join(MOCKUP_DIR, "weapon_wheel_mockup_v2.png")
    bg.save(out_path, "PNG")
    print(f"Generated Soviet Mechanical Weapon Wheel mockup: {out_path}")
    return out_path

# ---------------------------------------------------------------------------
# 2. VR Settings Mockup (v2) — Worn Laboratory Control Console
# ---------------------------------------------------------------------------
def generate_vr_settings_v2():
    w, h = 1920, 1080
    bg = create_lab_background(w, h)

    panel_w, panel_h = 1200, 840
    panel_x = (w - panel_w) // 2
    panel_y = (h - panel_h) // 2 - 20

    # 1. Main Heavy Soviet Console Frame (Military Olive Enamel on Oxidized Steel)
    console_tex = create_metal_texture(panel_w, panel_h, COL_MILITARY_OLIVE_DARK, noise_amt=20, rust_spots=24)
    cd = ImageDraw.Draw(console_tex)
    draw_chipped_border(cd, 4, 4, panel_w - 5, panel_h - 5, border_w=3)

    # 4 Heavy Reinforced Corner Plates with Hex Bolts
    c_size = 90
    for cx, cy in [(0, 0), (panel_w - c_size, 0), (0, panel_h - c_size), (panel_w - c_size, panel_h - c_size)]:
        cd.rectangle([cx + 6, cy + 6, cx + c_size - 6, cy + c_size - 6], fill=(30, 35, 30, 240), outline=(65, 75, 65, 255), width=2)
        draw_industrial_rivet(cd, cx + 25, cy + 25, r=7, is_screw=False)
        draw_industrial_rivet(cd, cx + c_size - 25, cy + 25, r=5, is_screw=True)
        draw_industrial_rivet(cd, cx + 25, cy + c_size - 25, r=5, is_screw=True)

    # Top Stamped Identification Tag
    cd.rectangle([80, 20, panel_w - 80, 75], fill=(22, 26, 22, 255), outline=(55, 65, 55, 255), width=2)
    # Caution hazard stripes on left of header plate
    for z in range(80, 160, 16):
        cd.polygon([(z, 20), (z + 10, 20), (z - 2, 75), (z - 12, 75)], fill=COL_CAUTION_YELLOW)

    f_title = get_font(22, bold=True)
    f_sub = get_font(12, bold=True)
    cd.text((180, 36), "STANOWISKO KONTROLNE VR // PROTOKÓŁ KATORGA-12", fill=COL_AGED_IVORY, font=f_title)
    cd.text((180, 60), "RADZIECKA STACJA BADAWCZA E99 // APARATURA DOŚWIADCZALNA 1955", fill=COL_WEATHERED_BRASS, font=f_sub)
    cd.text((panel_w - 100, 48), "SERIA: 4-VR", fill=COL_WEATHERED_BRASS, font=f_sub, anchor="ra")

    # 2. Left Column: Category Tabs (Weathered Metal Nameplates in Polish)
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
        t_fill = (45, 52, 42, 255) if is_active else (25, 29, 25, 230)
        t_border = COL_WEATHERED_BRASS if is_active else (48, 56, 48, 220)
        t_w = 2 if is_active else 1
        cd.rectangle([80, tab_y, 250, tab_y + 44], fill=t_fill, outline=t_border, width=t_w)
        # Left brass notch for active tab
        if is_active:
            cd.rectangle([80, tab_y, 88, tab_y + 44], fill=COL_WEATHERED_BRASS)
        txt_col = COL_AGED_IVORY if is_active else (140, 150, 138, 200)
        cd.text((105, tab_y + 14), t_title, fill=txt_col, font=f_tab)
        tab_y += 52

    # 3. Center/Right Column: Calibration Console Rows
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
        # Row Backing Plate
        if is_hov:
            # Subtle warm amber/brass mechanical focus (NOT electric neon cyan!)
            r_fill = (45, 42, 34, 255)
            r_border = COL_SELECTOR_AMBER
            r_bw = 2
        else:
            r_fill = (22, 26, 22, 240)
            r_border = (44, 52, 44, 220)
            r_bw = 1

        cd.rectangle([row_x, cur_y, row_x + row_w, cur_y + row_h], fill=r_fill, outline=r_border, width=r_bw)

        # Tactical corner registration notches for hovered row
        if is_hov:
            cd.line([row_x + 4, cur_y + 4, row_x + 14, cur_y + 4], fill=COL_AMBER_BRIGHT, width=2)
            cd.line([row_x + 4, cur_y + 4, row_x + 4, cur_y + 14], fill=COL_AMBER_BRIGHT, width=2)
            cd.line([row_x + row_w - 4, cur_y + 4, row_x + row_w - 14, cur_y + 4], fill=COL_AMBER_BRIGHT, width=2)
            cd.line([row_x + row_w - 4, cur_y + 4, row_x + row_w - 4, cur_y + 14], fill=COL_AMBER_BRIGHT, width=2)

        # Labels
        t_col = (255, 255, 255) if is_hov else COL_AGED_IVORY
        cd.text((row_x + 18, cur_y + 8), title, fill=t_col, font=f_opt)
        cd.text((row_x + 18, cur_y + 28), desc, fill=COL_WEATHERED_BRASS, font=f_desc)

        # Widgets
        if opt_type == "toggle":
            # Physical two-position toggle switch
            tog_x = row_x + row_w - 110
            tog_y = cur_y + 9
            # Recessed steel switch well
            cd.rounded_rectangle([tog_x, tog_y, tog_x + 85, tog_y + 32], radius=6, fill=(14, 17, 14, 255), outline=(50, 58, 48, 255))
            if val: # ON
                # Lever thrown right
                cd.rounded_rectangle([tog_x + 42, tog_y + 3, tog_x + 80, tog_y + 29], radius=4, fill=COL_WEATHERED_BRASS, outline=(95, 105, 90, 255))
                # Small amber indicator bulb glowing
                cd.ellipse([tog_x + 14, tog_y + 10, tog_x + 26, tog_y + 22], fill=COL_AMBER_BRIGHT, outline=COL_SELECTOR_AMBER)
                cd.text((tog_x + 32, tog_y + 16), "WŁ", fill=COL_AGED_IVORY, font=get_font(10, bold=True), anchor="lm")
            else: # OFF
                cd.rounded_rectangle([tog_x + 5, tog_y + 3, tog_x + 43, tog_y + 29], radius=4, fill=(40, 46, 38, 255), outline=(65, 75, 62, 255))
                cd.ellipse([tog_x + 60, tog_y + 10, tog_x + 72, tog_y + 22], fill=(30, 25, 20, 255))
                cd.text((tog_x + 50, tog_y + 16), "WYŁ", fill=(110, 120, 110, 200), font=get_font(10, bold=True), anchor="rm")

        elif opt_type == "choice":
            # Stamped mechanical selector box
            ch_x = row_x + row_w - 240
            ch_y = cur_y + 9
            cd.rectangle([ch_x, ch_y, ch_x + 220, ch_y + 32], fill=(16, 19, 16, 255), outline=COL_WEATHERED_BRASS if is_hov else (45, 54, 44, 255), width=1)
            v_col = COL_AMBER_BRIGHT if is_hov else COL_AGED_IVORY
            cd.text((ch_x + 110, ch_y + 16), str(val), fill=v_col, font=f_val, anchor="mm")
            # Mechanical brackets
            cd.text((ch_x + 14, ch_y + 16), "◄", fill=COL_WEATHERED_BRASS, font=f_desc, anchor="mm")
            cd.text((ch_x + 206, ch_y + 16), "►", fill=COL_WEATHERED_BRASS, font=f_desc, anchor="mm")

            if is_hov:
                hover_laser_x = panel_x + ch_x + 110
                hover_laser_y = panel_y + ch_y + 16

        elif opt_type == "slider":
            # Recessed mechanical calibration track
            sl_x = row_x + row_w - 240
            sl_y = cur_y + 18
            sl_w = 220
            # Track slot
            cd.rounded_rectangle([sl_x, sl_y, sl_x + sl_w, sl_y + 12], radius=3, fill=(12, 15, 12, 255), outline=(45, 54, 44, 255))
            # Engraved ticks
            for tx in range(sl_x + 10, sl_x + sl_w, 20):
                cd.line([tx, sl_y - 4, tx, sl_y - 1], fill=COL_WEATHERED_BRASS, width=1)
            # Knurled brass knob
            knob_pos_x = int(sl_x + sl_w * float(val))
            cd.rectangle([knob_pos_x - 8, sl_y - 8, knob_pos_x + 8, sl_y + 20], fill=COL_WEATHERED_BRASS, outline=(40, 48, 38, 255), width=1)
            cd.line([knob_pos_x, sl_y - 6, knob_pos_x, sl_y + 18], fill=(20, 24, 20, 255), width=2)

    # 4. Mechanical Scroll Bar on Far Right
    scr_x = row_x + row_w + 12
    cd.rectangle([scr_x, row_y, scr_x + 18, row_y + 440], fill=(16, 20, 16, 255), outline=(42, 50, 40, 255))
    # Slider block with grip notches
    cd.rectangle([scr_x + 2, row_y + 40, scr_x + 16, row_y + 120], fill=COL_WEATHERED_BRASS, outline=(50, 60, 48, 255))
    for gy in range(row_y + 55, row_y + 105, 10):
        cd.line([scr_x + 4, gy, scr_x + 14, gy], fill=(30, 36, 28, 255), width=2)

    # 5. Stamped Instruction Footplate
    cd.rectangle([80, panel_h - 60, panel_w - 80, panel_h - 20], fill=(22, 26, 22, 255), outline=(50, 60, 48, 255))
    cd.text((panel_w // 2, panel_h - 40),
            "[R2 / SPUST] WYBIERZ      [○ / KOŁO] WSTECZ      [LEWY DRĄŻEK] NAWIGACJA      [PRZYTRZYMAJ △] ZAMKNIJ",
            fill=COL_AGED_IVORY, font=f_sub, anchor="mm")

    bg.paste(console_tex, (panel_x, panel_y), console_tex)

    # 6. Realistic Optical Laboratory Aiming Laser (Collimated narrow beam & precision red/amber reticle dot)
    laser_im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    ld = ImageDraw.Draw(laser_im)

    start_x = w - 160
    start_y = h - 60
    # Fine collimated laser trace
    ld.line([start_x, start_y, hover_laser_x, hover_laser_y], fill=(215, 65, 45, 160), width=2)
    ld.line([start_x, start_y, hover_laser_x, hover_laser_y], fill=(255, 180, 120, 240), width=1)
    # Precision target dot with subtle halo (NOT giant glowing cyan flare)
    ld.ellipse([hover_laser_x - 6, hover_laser_y - 6, hover_laser_x + 6, hover_laser_y + 6], fill=(255, 230, 180, 255), outline=(215, 65, 45, 255), width=2)

    bg = Image.alpha_composite(bg, laser_im)

    out_path = os.path.join(MOCKUP_DIR, "vr_settings_screen_mockup_v2.png")
    bg.save(out_path, "PNG")
    print(f"Generated Worn Laboratory VR Settings mockup: {out_path}")
    return out_path

# ---------------------------------------------------------------------------
# 3. Pause Menu Mockup (v2) — Aged Industrial Terminal
# ---------------------------------------------------------------------------
def generate_pause_menu_v2():
    w, h = 1920, 1080
    bg = create_lab_background(w, h)

    panel_w, panel_h = 680, 840
    panel_x = (w - panel_w) // 2
    panel_y = (h - panel_h) // 2 - 20

    # 1. Physical Terminal Slate / Military Clipboard Chassis
    slate_tex = create_metal_texture(panel_w, panel_h, COL_GRAPHITE_BASE, noise_amt=22, rust_spots=18)
    sd = ImageDraw.Draw(slate_tex)
    draw_chipped_border(sd, 4, 4, panel_w - 5, panel_h - 5, border_w=3)

    # Corner brackets with flathead screws
    for cx, cy in [(14, 14), (panel_w - 14, 14), (14, panel_h - 14), (panel_w - 14, panel_h - 14)]:
        draw_industrial_rivet(sd, cx, cy, r=6, is_screw=True)

    # 2. Top Header Identification Plate
    sd.rectangle([30, 25, panel_w - 30, 110], fill=(20, 24, 20, 255), outline=(55, 65, 55, 255), width=2)
    # Soviet seal/stamp on left
    sd.ellipse([45, 38, 105, 98], fill=(35, 28, 25, 255), outline=COL_OXIDIZED_RUST, width=2)
    f_seal = get_font(12, bold=True)
    sd.text((75, 62), "СССР", fill=COL_OXIDIZED_RUST, font=f_seal, anchor="mm")
    sd.text((75, 78), "1955", fill=COL_OXIDIZED_RUST, font=f_seal, anchor="mm")

    f_p_head = get_font(24, bold=True)
    f_p_sub = get_font(12, bold=True)
    sd.text((125, 48), "PAUZA // KONTROLA OPERACJI", fill=COL_AGED_IVORY, font=f_p_head)
    sd.text((125, 76), "STACJA BADAWCZA KATORGA-12 // PROTOKÓŁ ZATRZYMANIA", fill=COL_WEATHERED_BRASS, font=f_p_sub)
    sd.text((125, 94), "STAN: AKTYWNY CHRONO-BUFOR E99", fill=COL_E99_RESTRAINED, font=f_p_sub)

    # 3. Tactical Menu Button Rows
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
            # Dedicated synthetic VR settings row: aged brass casing with warm illumination
            b_fill = (45, 40, 30, 255)
            b_border = COL_AMBER_BRIGHT
            b_bw = 2
            # Left accent tab
            sd.rectangle([btn_x, by, btn_x + 12, by + btn_h], fill=COL_SELECTOR_AMBER)
            # Corner notches
            sd.line([btn_x + 4, by + 4, btn_x + 16, by + 4], fill=COL_AMBER_BRIGHT, width=2)
            sd.line([btn_x + 4, by + 4, btn_x + 4, by + 16], fill=COL_AMBER_BRIGHT, width=2)
            sd.line([btn_x + btn_w - 4, by + 4, btn_x + btn_w - 16, by + 4], fill=COL_AMBER_BRIGHT, width=2)
            sd.line([btn_x + btn_w - 4, by + 4, btn_x + btn_w - 4, by + 16], fill=COL_AMBER_BRIGHT, width=2)
            t_col = (255, 255, 255)
            hover_laser_x = panel_x + btn_x + btn_w // 2 + 60
            hover_laser_y = panel_y + by + btn_h // 2
        else:
            b_fill = (22, 26, 22, 240)
            b_border = (48, 56, 48, 220)
            b_bw = 1
            t_col = COL_AGED_IVORY

        sd.rectangle([btn_x, by, btn_x + btn_w, by + btn_h], fill=b_fill, outline=b_border, width=b_bw)
        sd.text((btn_x + btn_w // 2, by + btn_h // 2 - (6 if is_vr_row else 0)), b_title, fill=t_col, font=f_b_title, anchor="mm")
        if is_vr_row:
            sd.text((btn_x + btn_w // 2, by + btn_h // 2 + 14), "KONFIGURACJA ŚLEDZENIA I PARAMETRÓW HEŁMU", fill=COL_WEATHERED_BRASS, font=f_b_sub, anchor="mm")

    # 4. Bottom Stamped Footplate
    sd.rectangle([30, panel_h - 75, panel_w - 30, panel_h - 25], fill=(20, 24, 20, 255), outline=(48, 56, 48, 255))
    sd.text((panel_w // 2, panel_h - 50),
            "[R2] WYBIERZ      [○] WSTECZ      [LEWY DRĄŻEK] NAWIGACJA      [△] WYJŚCIE",
            fill=COL_AGED_IVORY, font=get_font(13, bold=True), anchor="mm")

    bg.paste(slate_tex, (panel_x, panel_y), slate_tex)

    # 5. Laser Pointer Targeting "OPCJE VR"
    laser_im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    ld = ImageDraw.Draw(laser_im)

    start_x = w - 180
    start_y = h - 60
    ld.line([start_x, start_y, hover_laser_x, hover_laser_y], fill=(215, 65, 45, 160), width=2)
    ld.line([start_x, start_y, hover_laser_x, hover_laser_y], fill=(255, 180, 120, 240), width=1)
    ld.ellipse([hover_laser_x - 6, hover_laser_y - 6, hover_laser_x + 6, hover_laser_y + 6],
               fill=(255, 230, 180, 255), outline=(215, 65, 45, 255), width=2)

    bg = Image.alpha_composite(bg, laser_im)

    out_path = os.path.join(MOCKUP_DIR, "pause_menu_mockup_v2.png")
    bg.save(out_path, "PNG")
    print(f"Generated Aged Industrial Pause Menu mockup: {out_path}")
    return out_path

# ---------------------------------------------------------------------------
# 4. Side-by-Side Comparison Generator
# ---------------------------------------------------------------------------
def generate_side_by_side(v1_filename, v2_filename, out_filename, title):
    """Combines previous v1 mockup and corrected v2 mockup into a 2560x1440 comparison image."""
    p_v1 = os.path.join(MOCKUP_DIR, v1_filename)
    p_v2 = os.path.join(MOCKUP_DIR, v2_filename)
    if not os.path.exists(p_v1) or not os.path.exists(p_v2):
        print(f"Skipping comparison {out_filename}: missing source files.")
        return

    im1 = Image.open(p_v1).convert("RGBA").resize((1240, 698), Image.Resampling.LANCZOS)
    im2 = Image.open(p_v2).convert("RGBA").resize((1240, 698), Image.Resampling.LANCZOS)

    cw, ch = 2560, 840
    comp = Image.new("RGBA", (cw, ch), (14, 17, 15, 255))
    d = ImageDraw.Draw(comp)

    # Headers
    f_main = get_font(26, bold=True)
    f_tag = get_font(18, bold=True)
    f_expl = get_font(13)

    d.text((cw // 2, 35), title, fill=COL_AGED_IVORY, font=f_main, anchor="mm")

    # Paste Images
    comp.paste(im1, (30, 80))
    comp.paste(im2, (1290, 80))

    # Left Card Border & Label (V1 - Rejected Cyberpunk)
    d.rectangle([28, 78, 1272, 780], outline=(180, 50, 50, 255), width=3)
    d.rectangle([30, 80, 380, 120], fill=(180, 40, 40, 240))
    d.text((45, 98), "POPRZEDNI (ODRZUCONY: CYBERPUNK)", fill=(255, 255, 255), font=f_tag, anchor="lm")
    d.text((45, 795), "WADY: Neonowe ramki cyan, nieautentyczne uzbrojenie, czysty płaski styl, brak radzieckiej patyny",
           fill=(220, 140, 140), font=f_expl)

    # Right Card Border & Label (V2 - Corrected Soviet Industrial)
    d.rectangle([1288, 78, 2532, 780], outline=(85, 175, 85, 255), width=3)
    d.rectangle([1290, 80, 1720, 120], fill=(45, 130, 45, 240))
    d.text((1305, 98), "POPRAWIONY (RADZIECKI RETROFUTURYZM)", fill=(255, 255, 255), font=f_tag, anchor="lm")
    d.text((1305, 795), "ZALETY: Mosiądz, oksydowana stal, autentyczny katalog broni, polskie etykiety, bursztynowy wybór",
           fill=(160, 220, 160), font=f_expl)

    out_path = os.path.join(MOCKUP_DIR, out_filename)
    comp.save(out_path, "PNG")
    print(f"Generated comparison: {out_path}")

if __name__ == "__main__":
    generate_weapon_wheel_v2()
    generate_vr_settings_v2()
    generate_pause_menu_v2()

    # Generate comparisons
    generate_side_by_side("weapon_wheel_mockup.png", "weapon_wheel_mockup_v2.png",
                          "compare_weapon_wheel.png", "PORÓWNANIE: KOŁO WYBORU BRONI (WEAPON WHEEL)")
    generate_side_by_side("vr_settings_screen_mockup.png", "vr_settings_screen_mockup_v2.png",
                          "compare_vr_settings.png", "PORÓWNANIE: USTAWIENIA VR (VR SETTINGS CONSOLE)")
    generate_side_by_side("pause_menu_mockup.png", "pause_menu_mockup_v2.png",
                          "compare_pause_menu.png", "PORÓWNANIE: MENU PAUZY (PAUSE OPERATIONS TERMINAL)")
