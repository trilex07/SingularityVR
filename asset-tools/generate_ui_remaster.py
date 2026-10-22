"""
Singularity VR - UI Utility Graphics Remaster Asset Generator
Generates SVG vector sources, high-resolution transparent PNG exports,
and concept presentation mockups for the Singularity VR UI remaster package.
"""

import os
import math
import json
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageOps

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SRC_DIR = os.path.join(BASE_DIR, "assets", "source", "ui_remaster")
PROC_DIR = os.path.join(BASE_DIR, "assets", "processed", "ui_remaster")
DOCS_DIR = os.path.join(BASE_DIR, "docs", "previews", "ui_remaster")
MOCKUP_DIR = os.path.join(DOCS_DIR, "mockups")

# Ensure all directories exist
for d in [SRC_DIR, PROC_DIR, DOCS_DIR, MOCKUP_DIR]:
    os.makedirs(d, exist_ok=True)
for sub in ["weapon_wheel", "vr_settings", "pause_menu", "interaction_icons", "laser_pointer", "utility_hud"]:
    os.makedirs(os.path.join(SRC_DIR, sub), exist_ok=True)
    os.makedirs(os.path.join(PROC_DIR, sub), exist_ok=True)

# Colors
C_BG_DARK = (11, 15, 21)          # #0B0F15 - Cold dark iron/slate
C_BG_PANEL = (16, 22, 30)         # #10161E - Stamped metal backing
C_BG_ROW_HOVER = (18, 55, 70)     # #123746 - Phosphor tint hover
C_E99_CYAN = (44, 232, 228)       # #2CE8E4 - Core E99 glow
C_E99_CYAN_BRIGHT = (120, 255, 250) # #78FFFA - Supercharged cyan
C_E99_CYAN_DIM = (20, 110, 120)   # #146E78 - Ambient cyan
C_AMBER_GLOW = (255, 169, 39)     # #FFA927 - Soviet vacuum tube amber
C_AMBER_BRIGHT = (255, 210, 110)  # #FFD26E - High amber
C_HAZARD_YELLOW = (245, 195, 35)  # #F5C323 - Industrial caution stripe
C_HAZARD_RED = (235, 45, 55)      # #EB2D37 - Warning/locked crimson
C_TEXT_WHITE = (235, 244, 250)    # #EBF4FA - High-contrast readable off-white
C_TEXT_DIM = (125, 145, 158)      # #7D919E - Technical annotation grey
C_BORDER_STEEL = (65, 85, 102)    # #415566 - Milled steel bevel

# Fonts
def get_font(size, bold=False, mono=False):
    font_path = r"C:\Windows\Fonts\bahnschrift.ttf"
    if mono:
        font_path = r"C:\Windows\Fonts\consola.ttf"
    if not os.path.exists(font_path):
        font_path = r"C:\Windows\Fonts\arial.ttf"
    try:
        return ImageFont.truetype(font_path, size)
    except Exception:
        return ImageFont.load_default()

print("UI Remaster Generator initialised.")
