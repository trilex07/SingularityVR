"""
Singularity VR - UI Remaster: Interaction Prompts & Utility Icons Generator
Generates SVG sources, transparent PNG exports at 64/128/256px, dark/light variants,
remastered BMP/PNG atlas with JSON metadata, and in-game prompt concept mockup.
"""

import os
import math
import json
from PIL import Image, ImageDraw, ImageFont, ImageFilter

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SRC_DIR = os.path.join(BASE_DIR, "assets", "source", "ui_remaster", "interaction_icons")
PROC_DIR = os.path.join(BASE_DIR, "assets", "processed", "ui_remaster", "interaction_icons")
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
# 1. Generate Individual Prompts & Controller Glyphs
# -------------------------------------------------------------
def draw_icon_geometry(d, name, cx, cy, s, color):
    """Draws crisp vector icon geometry centered at (cx, cy) with base scale s."""
    if name == "interact":
        # Hand reaching / Gear interlock
        # Outer gear notches
        d.ellipse([cx - s * 0.7, cy - s * 0.7, cx + s * 0.7, cy + s * 0.7], outline=color, width=int(s * 0.12))
        for a in range(0, 360, 45):
            rad = math.radians(a)
            ox = cx + s * 0.7 * math.cos(rad)
            oy = cy + s * 0.7 * math.sin(rad)
            d.line([ox, oy, ox + s * 0.2 * math.cos(rad), oy + s * 0.2 * math.sin(rad)], fill=color, width=int(s * 0.16))
        # Center hand / pointer pip
        d.ellipse([cx - s * 0.25, cy - s * 0.25, cx + s * 0.25, cy + s * 0.25], fill=color)

    elif name == "pickup":
        # Upward chevron in cradle bracket
        d.polygon([(cx, cy - s * 0.6), (cx - s * 0.45, cy), (cx - s * 0.2, cy),
                   (cx - s * 0.2, cy + s * 0.3), (cx + s * 0.2, cy + s * 0.3),
                   (cx + s * 0.2, cy), (cx + s * 0.45, cy)], fill=color)
        # Base cradle bracket
        d.line([cx - s * 0.6, cy + s * 0.35, cx - s * 0.6, cy + s * 0.6,
                cx + s * 0.6, cy + s * 0.6, cx + s * 0.6, cy + s * 0.35], fill=color, width=int(s * 0.12))

    elif name == "holster":
        # Crossed straps / holster silhouette
        d.rectangle([cx - s * 0.35, cy - s * 0.5, cx + s * 0.35, cy + s * 0.5], outline=color, width=int(s * 0.12))
        d.line([cx - s * 0.5, cy - s * 0.6, cx + s * 0.5, cy + s * 0.6], fill=color, width=int(s * 0.1))
        d.line([cx + s * 0.5, cy - s * 0.6, cx - s * 0.5, cy + s * 0.6], fill=color, width=int(s * 0.1))
        d.ellipse([cx - s * 0.15, cy - s * 0.15, cx + s * 0.15, cy + s * 0.15], fill=color)

    elif name == "grip":
        # Controller handle clench / squeeze lines
        d.rounded_rectangle([cx - s * 0.3, cy - s * 0.6, cx + s * 0.3, cy + s * 0.6], radius=int(s * 0.15),
                            outline=color, width=int(s * 0.12))
        # Inward squeeze chevrons
        d.polygon([(cx - s * 0.65, cy), (cx - s * 0.45, cy - s * 0.2), (cx - s * 0.45, cy + s * 0.2)], fill=color)
        d.polygon([(cx + s * 0.65, cy), (cx + s * 0.45, cy - s * 0.2), (cx + s * 0.45, cy + s * 0.2)], fill=color)

    elif name == "reload":
        # Circular reload arrows with center bullet cartridge
        d.arc([cx - s * 0.6, cy - s * 0.6, cx + s * 0.6, cy + s * 0.6], 30, 150, fill=color, width=int(s * 0.12))
        d.arc([cx - s * 0.6, cy - s * 0.6, cx + s * 0.6, cy + s * 0.6], 210, 330, fill=color, width=int(s * 0.12))
        # Arrow heads
        d.polygon([(cx + s * 0.52, cy + s * 0.3), (cx + s * 0.35, cy + s * 0.5), (cx + s * 0.65, cy + s * 0.55)], fill=color)
        d.polygon([(cx - s * 0.52, cy - s * 0.3), (cx - s * 0.35, cy - s * 0.5), (cx - s * 0.65, cy - s * 0.55)], fill=color)
        # Bullet in center
        d.rounded_rectangle([cx - s * 0.1, cy - s * 0.35, cx + s * 0.1, cy + s * 0.35], radius=int(s * 0.08), fill=color)

    elif name == "melee":
        # Combat knife diagonal strike
        d.polygon([(cx - s * 0.5, cy + s * 0.5), (cx - s * 0.2, cy + s * 0.5),
                   (cx + s * 0.6, cy - s * 0.3), (cx + s * 0.3, cy - s * 0.6)], fill=color)
        # Kinetic slash arcs
        d.arc([cx - s * 0.7, cy - s * 0.7, cx + s * 0.7, cy + s * 0.7], 280, 350, fill=color, width=int(s * 0.1))

    elif name == "tmd":
        # TMD Chronal Loop with 3 focus needles
        d.arc([cx - s * 0.6, cy - s * 0.6, cx + s * 0.6, cy + s * 0.6], 0, 360, fill=color, width=int(s * 0.1))
        # 3 Articulated Needles
        for a in [0, 120, 240]:
            rad = math.radians(a)
            px = cx + s * 0.6 * math.cos(rad)
            py = cy + s * 0.6 * math.sin(rad)
            d.polygon([(px, py), (px + s * 0.25 * math.cos(rad - 0.5), py + s * 0.25 * math.sin(rad - 0.5)),
                       (px + s * 0.25 * math.cos(rad + 0.5), py + s * 0.25 * math.sin(rad + 0.5))], fill=color)
        # Inner core
        d.ellipse([cx - s * 0.2, cy - s * 0.2, cx + s * 0.2, cy + s * 0.2], fill=color)

    elif name == "locked":
        # Padlock with diagonal warning strike
        d.arc([cx - s * 0.3, cy - s * 0.65, cx + s * 0.3, cy - s * 0.05], 180, 360, fill=color, width=int(s * 0.12))
        d.rectangle([cx - s * 0.45, cy - s * 0.1, cx + s * 0.45, cy + s * 0.55], fill=color)
        d.ellipse([cx - s * 0.1, cy + s * 0.1, cx + s * 0.1, cy + s * 0.3], fill=(16, 22, 30, 255))

    elif name == "nav_arrows":
        # 4 Cardinal Chevrons (Expand / Navigation)
        d.polygon([(cx, cy - s * 0.6), (cx - s * 0.2, cy - s * 0.35), (cx + s * 0.2, cy - s * 0.35)], fill=color)
        d.polygon([(cx, cy + s * 0.6), (cx - s * 0.2, cy + s * 0.35), (cx + s * 0.2, cy + s * 0.35)], fill=color)
        d.polygon([(cx - s * 0.6, cy), (cx - s * 0.35, cy - s * 0.2), (cx - s * 0.35, cy + s * 0.2)], fill=color)
        d.polygon([(cx + s * 0.6, cy), (cx + s * 0.35, cy - s * 0.2), (cx + s * 0.35, cy + s * 0.2)], fill=color)

    elif name.startswith("psvr2_"):
        # PSVR2 Controller Buttons
        btn = name.replace("psvr2_", "")
        # Rounded capsule or circle
        if btn in ["r1", "l1"]:
            d.rounded_rectangle([cx - s * 0.7, cy - s * 0.35, cx + s * 0.7, cy + s * 0.35],
                                radius=int(s * 0.15), outline=color, width=int(s * 0.12))
            f = get_font(int(s * 0.45), bold=True)
            d.text((cx, cy), btn.upper(), fill=color, font=f, anchor="mm")
        elif btn in ["r2", "l2"]:
            # Trigger wedge silhouette
            d.polygon([(cx - s * 0.5, cy - s * 0.45), (cx + s * 0.5, cy - s * 0.45),
                       (cx + s * 0.35, cy + s * 0.55), (cx - s * 0.35, cy + s * 0.55)],
                      outline=color, fill=None)
            f = get_font(int(s * 0.45), bold=True)
            d.text((cx, cy), btn.upper(), fill=color, font=f, anchor="mm")
        elif btn == "cross":
            d.ellipse([cx - s * 0.65, cy - s * 0.65, cx + s * 0.65, cy + s * 0.65], outline=color, width=int(s * 0.1))
            # Cross lines
            d.line([cx - s * 0.3, cy - s * 0.3, cx + s * 0.3, cy + s * 0.3], fill=color, width=int(s * 0.14))
            d.line([cx + s * 0.3, cy - s * 0.3, cx - s * 0.3, cy + s * 0.3], fill=color, width=int(s * 0.14))
        elif btn == "circle":
            d.ellipse([cx - s * 0.65, cy - s * 0.65, cx + s * 0.65, cy + s * 0.65], outline=color, width=int(s * 0.1))
            d.ellipse([cx - s * 0.32, cy - s * 0.32, cx + s * 0.32, cy + s * 0.32], outline=color, width=int(s * 0.14))
        elif btn == "square":
            d.ellipse([cx - s * 0.65, cy - s * 0.65, cx + s * 0.65, cy + s * 0.65], outline=color, width=int(s * 0.1))
            d.rectangle([cx - s * 0.28, cy - s * 0.28, cx + s * 0.28, cy + s * 0.28], outline=color, width=int(s * 0.14))
        elif btn == "triangle":
            d.ellipse([cx - s * 0.65, cy - s * 0.65, cx + s * 0.65, cy + s * 0.65], outline=color, width=int(s * 0.1))
            d.polygon([(cx, cy - s * 0.32), (cx - s * 0.3, cy + s * 0.25), (cx + s * 0.3, cy + s * 0.25)],
                      outline=color, fill=None)
        elif btn in ["l3", "r3"]:
            d.ellipse([cx - s * 0.65, cy - s * 0.65, cx + s * 0.65, cy + s * 0.65], outline=color, width=int(s * 0.1))
            f = get_font(int(s * 0.45), bold=True)
            d.text((cx, cy), btn.upper(), fill=color, font=f, anchor="mm")
        elif btn in ["stick_l", "stick_r"]:
            d.ellipse([cx - s * 0.65, cy - s * 0.65, cx + s * 0.65, cy + s * 0.65], outline=color, width=int(s * 0.1))
            d.polygon([(cx, cy - s * 0.45), (cx - s * 0.15, cy - s * 0.2), (cx + s * 0.15, cy - s * 0.2)], fill=color)
            d.polygon([(cx, cy + s * 0.45), (cx - s * 0.15, cy + s * 0.2), (cx + s * 0.15, cy + s * 0.2)], fill=color)
            d.polygon([(cx - s * 0.45, cy), (cx - s * 0.2, cy - s * 0.15), (cx - s * 0.2, cy + s * 0.15)], fill=color)
            d.polygon([(cx + s * 0.45, cy), (cx + s * 0.2, cy - s * 0.15), (cx + s * 0.2, cy + s * 0.15)], fill=color)

def generate_all_icons():
    icon_names = [
        "interact", "pickup", "holster", "grip", "reload", "melee", "tmd", "locked", "nav_arrows",
        "psvr2_r1", "psvr2_r2", "psvr2_l1", "psvr2_l2", "psvr2_cross", "psvr2_circle",
        "psvr2_square", "psvr2_triangle", "psvr2_l3", "psvr2_r3", "psvr2_stick_l", "psvr2_stick_r"
    ]

    variants = {
        "cyan_dark": ((44, 232, 228, 255), (14, 20, 28, 220)),   # E99 Phosphor Cyan on dark
        "amber_dark": ((255, 169, 39, 255), (16, 20, 26, 220)),  # Vacuum tube amber on dark
        "white_trans": ((245, 250, 255, 255), (0, 0, 0, 0))      # Crisp white on transparent
    }

    resolutions = [64, 128, 256]

    for name in icon_names:
        for v_name, (fg_col, bg_col) in variants.items():
            # Render at 256px with 4x supersampling
            high_size = 1024
            im = Image.new("RGBA", (high_size, high_size), (0, 0, 0, 0))
            d = ImageDraw.Draw(im)

            # Circular backing if not transparent
            if bg_col[3] > 0:
                d.ellipse([40, 40, high_size - 40, high_size - 40], fill=bg_col, outline=fg_col, width=16)

            draw_icon_geometry(d, name, high_size // 2, high_size // 2, high_size // 2 - 120, fg_col)

            # Save in requested sizes
            for r in resolutions:
                final_im = im.resize((r, r), Image.Resampling.LANCZOS)
                fn = f"{name}_{v_name}_{r}px.png"
                final_im.save(os.path.join(PROC_DIR, fn), "PNG")

    print(f"Generated all icons across {len(resolutions)} resolutions and {len(variants)} variants.")

# -------------------------------------------------------------
# 2. Master Atlas Candidate (vr_prompt_atlas_remastered.bmp/.png & JSON)
# -------------------------------------------------------------
def generate_remastered_atlas():
    # Atlas layout: 512x128 grid (8 icons wide x 2 icons tall = 16 slots, 64x64 each)
    icon_w = 64
    icon_h = 64
    cols = 8
    rows = 2
    atlas_w = cols * icon_w   # 512
    atlas_h = rows * icon_h   # 128

    scale = 4
    high_w = atlas_w * scale
    high_h = atlas_h * scale
    high_icon = icon_w * scale

    im = Image.new("RGBA", (high_w, high_h), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)

    atlas_slots = [
        # Row 0: Controller primary buttons & gameplay
        ("psvr2_r2", "R2", "gameplay_fire"),
        ("psvr2_r1", "R1", "gameplay_interact"),
        ("psvr2_cross", "Cross", "gameplay_jump"),
        ("psvr2_circle", "Circle", "gameplay_crouch"),
        ("psvr2_l2", "L2", "gameplay_aim"),
        ("psvr2_l1", "L1", "gameplay_offhand_grip"),
        ("psvr2_square", "Square", "gameplay_reload"),
        ("psvr2_triangle", "Triangle", "menu_pause"),
        # Row 1: Actions & System
        ("interact", "Interact", "prompt_interact"),
        ("pickup", "Pickup", "prompt_pickup"),
        ("tmd", "TMD", "prompt_tmd"),
        ("reload", "Reload", "prompt_reload"),
        ("melee", "Melee", "prompt_melee"),
        ("locked", "Locked", "prompt_locked"),
        ("psvr2_r3", "R3", "weapon_wheel"),
        ("psvr2_l3", "L3", "recenter_chord")
    ]

    json_data = {
        "source": "Singularity VR UI Remaster Pack",
        "author": "Side VR UI Team",
        "atlas_png": "runtime/vr_prompt_atlas_remastered.png",
        "atlas_bmp": "runtime/vr_prompt_atlas_remastered.bmp",
        "width": atlas_w,
        "height": atlas_h,
        "format": "BMP/32bpp ARGB & PNG",
        "icon_size": icon_w,
        "icons": {},
        "runtime_roles": {}
    }

    fg_col = (44, 232, 228, 255)
    bg_col = (14, 20, 28, 220)

    for idx, (icon_geom, icon_key, role) in enumerate(atlas_slots):
        c = idx % cols
        r = idx // cols

        ix0 = c * high_icon
        iy0 = r * high_icon
        cx = ix0 + high_icon // 2
        cy = iy0 + high_icon // 2

        # Draw dark backing
        d.ellipse([cx - high_icon * 0.42, cy - high_icon * 0.42,
                   cx + high_icon * 0.42, cy + high_icon * 0.42], fill=bg_col, outline=fg_col, width=6)

        draw_icon_geometry(d, icon_geom, cx, cy, high_icon * 0.32, fg_col)

        # JSON UV Mapping
        u0 = (c * icon_w) / atlas_w
        v0 = (r * icon_h) / atlas_h
        u1 = ((c + 1) * icon_w) / atlas_w
        v1 = ((r + 1) * icon_h) / atlas_h

        json_data["icons"][icon_key] = {
            "x": c * icon_w,
            "y": r * icon_h,
            "width": icon_w,
            "height": icon_h,
            "u0": round(u0, 4),
            "v0": round(v0, 4),
            "u1": round(u1, 4),
            "v1": round(v1, 4)
        }
        json_data["runtime_roles"][role] = icon_key

    # Downsample atlas
    final_atlas = im.resize((atlas_w, atlas_h), Image.Resampling.LANCZOS)
    png_path = os.path.join(PROC_DIR, "vr_prompt_atlas_remastered.png")
    final_atlas.save(png_path, "PNG")

    # Save as 32bpp BMP (matching D3D9 LoadVrPromptAtlas requirement)
    bmp_path = os.path.join(PROC_DIR, "vr_prompt_atlas_remastered.bmp")
    final_atlas.save(bmp_path, "BMP")

    # Save JSON metadata
    json_path = os.path.join(PROC_DIR, "vr_prompt_atlas_remastered.json")
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(json_data, f, indent=2)

    print(f"Generated Remastered Atlas: {png_path}, {bmp_path}, {json_path}")

# -------------------------------------------------------------
# 3. In-Game Interaction Prompts Concept Mockup (1920x1080)
# -------------------------------------------------------------
def generate_interaction_mockup():
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
    f_card_head = get_font(18, bold=True)
    f_card_sub = get_font(13)

    # Header
    d.rectangle([w // 2 - 360, 30, w // 2 + 360, 90], fill=(14, 20, 28, 230), outline=(65, 85, 102, 255), width=2)
    d.rectangle([w // 2 - 356, 34, w // 2 - 340, 86], fill=(44, 232, 228, 255))
    d.text((w // 2 - 320, 44), "КОНТЕКСТНЫЕ ПОДСКАЗКИ // VR INTERACTION PROMPTS", fill=(245, 250, 255, 255), font=f_title)
    d.text((w // 2 - 320, 70), "ПРОСТРАНСТВЕННОЕ ПОЗИЦИОНИРОВАНИЕ В 3D МИРЕ", fill=(130, 160, 180, 220), font=f_card_sub)

    # Sample In-World Floating HUD Cards
    cards = [
        # Card 1: Weapon Pickup
        (280, 360, "psvr2_r1_cyan_dark_128px.png", "ПОДОБРАТЬ ОРУЖИЕ // PICKUP", "ВОЛК S-4 (7.62x39mm)", "[R1 / GRIP] ЗАХВАТ В РУКУ", (44, 232, 228)),
        # Card 2: TMD Chrono-Action
        (1050, 360, "tmd_cyan_dark_128px.png", "TMD ХРОНО-ВОЗДЕЙСТВИЕ // AGE", "СЛОМАННЫЙ МЕХАНИЗМ КАТОРГА-12", "[L2] СОСТАРИТЬ / ОМОЛОДИТЬ", (120, 255, 250)),
        # Card 3: Security Lock
        (280, 680, "locked_amber_dark_128px.png", "БЛОКИРОВКА СИСТЕМЫ // LOCKED", "ДВЕРЬ ЛАБОРАТОРИИ №4", "ТРЕБУЕТСЯ КАРТА ДОСТУПА E99", (255, 169, 39)),
        # Card 4: Quick Reload Prompt
        (1050, 680, "reload_cyan_dark_128px.png", "ПЕРЕЗАРЯДКА // MANUAL RELOAD", "ЦЕНТУРИОН .44 РЕВОЛЬВЕР", "[КНОПКА □] ОТКИНУТЬ БАРАБАН", (44, 232, 228))
    ]

    for px, py, icon_file, head_txt, sub_txt, hint_txt, col in cards:
        # Card Backing (580x180)
        d.rectangle([px, py, px + 580, py + 180], fill=(13, 18, 26, 245), outline=col, width=2)
        # Inner corner ticks
        d.line([px + 4, py + 4, px + 16, py + 4], fill=col, width=2)
        d.line([px + 4, py + 4, px + 4, py + 16], fill=col, width=2)

        # Icon paste
        icon_path = os.path.join(PROC_DIR, icon_file)
        if os.path.exists(icon_path):
            icon_im = Image.open(icon_path).resize((110, 110), Image.Resampling.LANCZOS)
            im.paste(icon_im, (px + 20, py + 35), icon_im)

        # Typography
        d.text((px + 150, py + 35), head_txt, fill=col, font=f_card_head)
        d.text((px + 150, py + 70), sub_txt, fill=(240, 248, 255, 255), font=f_card_head)
        d.rectangle([px + 150, py + 105, px + 540, py + 145], fill=(20, 28, 38, 255), outline=(50, 70, 90, 255))
        d.text((px + 165, py + 115), hint_txt, fill=(210, 230, 245, 240), font=f_card_sub)

    # PSVR2 Bottom Legend
    f_hint = get_font(16)
    d.rectangle([w // 2 - 420, h - 75, w // 2 + 420, h - 25], fill=(12, 17, 24, 240), outline=(45, 60, 75, 255), width=2)
    d.text((w // 2, h - 50),
           "ВЫСОКИЙ КОНТРАСТ В ГАРНИТУРЕ // ANTI-ALIASED PHOSPHOR AT 2684px SBS TARGET",
           fill=(44, 232, 228, 240), font=f_hint, anchor="mm")

    mockup_path = os.path.join(MOCKUP_DIR, "interaction_prompts_mockup.png")
    im.save(mockup_path, "PNG")
    print(f"Generated Interaction Prompts mockup: {mockup_path}")

if __name__ == "__main__":
    generate_all_icons()
    generate_remastered_atlas()
    generate_interaction_mockup()
