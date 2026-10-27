# -*- coding: utf-8 -*-
"""Original Katorga-12 HUD plates, icon atlas, and previews.

The SVG files are the editable art. This script rebuilds them, the atlas,
the preview PNGs, and the unmounted frame. It does not read the game and
does not embed a font. Preview rasters ask Windows for Segoe UI at draw
time; that face is not shipped.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "assets" / "hud" / "katorga"

PAPER = "#e6e1d6"
STEEL = "#5a6f8d"
PLATE = "#101214"
AMBER = "#c4a46a"
HAIR = "#2a3036"
SLOT = "#3d4650"

PAPER_RGB = (230, 225, 214, 255)
STEEL_RGB = (90, 111, 141, 255)
PLATE_RGB = (16, 18, 20, 255)
AMBER_RGB = (196, 164, 106, 255)
CLEAR = (0, 0, 0, 0)

CELL = 64
ATLAS_W = 512
ATLAS_H = 256

# Rectangles are stable. Empty cells stay reserved so a later icon does not
# slide an existing one.
RECTS = [
    ("tick-full", 0, 0),
    ("tick-empty", 64, 0),
    ("tick-worn", 128, 0),
    ("mag-closed", 192, 0),
    ("mag-open", 256, 0),
    ("reload-mark", 320, 0),
    ("warning-notch", 384, 0),
    ("rivet", 448, 0),
    ("sense-circle", 0, 64),
    ("sense-square", 64, 64),
    ("sense-cross", 128, 64),
    ("sense-triangle", 192, 64),
    ("sense-grip", 256, 64),
    ("sense-stick-down", 320, 64),
    ("digit-slot", 384, 64),
    ("name-rule", 448, 64),
    # Row 3 — koło broni (sylwetki oryginalne, nie z gry)
    ("wheel-current", 0, 128),
    ("weapon-centurion", 64, 128),
    ("weapon-valkyrie", 128, 128),
    ("weapon-spikeshot", 192, 128),
    ("weapon-kasimov", 256, 128),
    ("wheel-slot-empty", 320, 128),
]


def _svg_open(width: int, height: int, ident: str, title: str) -> str:
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}" id="{ident}">\n'
        f"<!-- Original Katorga HUD art. MIT, same terms as the repository. "
        f"Not taken from the game, from Kenney, or from a console mark sheet. "
        f"No font is embedded. -->\n"
        f"<title>{title}</title>\n"
    )


def _icons() -> dict[str, str]:
    """One drawing per cell, in a 64x64 view. Paper on transparent."""
    return {
        "tick-full": (
            f'<path d="M22 12 H40 L44 18 V54 H18 V18 Z" fill="{PAPER}" stroke="{STEEL}" stroke-width="2"/>'
        ),
        "tick-empty": (
            f'<path d="M22 12 H40 L44 18 V54 H18 V18 Z" fill="none" stroke="{STEEL}" stroke-width="2"/>'
        ),
        "tick-worn": (
            f'<path d="M22 30 H40 L44 36 V54 H18 V36 Z" fill="{PAPER}" stroke="{STEEL}" stroke-width="2"/>'
        ),
        "mag-closed": (
            f'<rect x="16" y="12" width="32" height="40" fill="none" stroke="{PAPER}" stroke-width="2"/>'
            f'<path d="M24 12 V52 M16 26 H48 M16 38 H48" fill="none" stroke="{STEEL}" stroke-width="2"/>'
        ),
        "mag-open": (
            f'<path d="M18 26 V50 H46 V26" fill="none" stroke="{PAPER}" stroke-width="3"/>'
            f'<path d="M20 14 H44" fill="none" stroke="{PAPER}" stroke-width="3"/>'
        ),
        "reload-mark": (
            f'<rect x="8" y="24" width="16" height="16" fill="{PAPER}"/>'
            f'<rect x="40" y="24" width="16" height="16" fill="{PAPER}"/>'
            f'<path d="M26 32 H38" fill="none" stroke="{STEEL}" stroke-width="2"/>'
        ),
        "warning-notch": (
            f'<rect x="14" y="14" width="36" height="36" fill="none" stroke="{PAPER}" stroke-width="2"/>'
            f'<path d="M20 42 L30 22 M28 44 L40 22 M36 46 L48 24" fill="none" stroke="{PAPER}" stroke-width="2"/>'
        ),
        "rivet": (
            f'<circle cx="32" cy="32" r="6" fill="none" stroke="{STEEL}" stroke-width="2"/>'
            f'<circle cx="32" cy="32" r="2" fill="{PAPER}"/>'
        ),
        "sense-circle": (
            f'<circle cx="32" cy="32" r="16" fill="none" stroke="{PAPER}" stroke-width="4"/>'
        ),
        "sense-square": (
            f'<rect x="16" y="16" width="32" height="32" fill="none" stroke="{PAPER}" stroke-width="4"/>'
        ),
        "sense-cross": (
            f'<path d="M20 20 L44 44 M44 20 L20 44" fill="none" stroke="{PAPER}" stroke-width="4"/>'
        ),
        "sense-triangle": (
            f'<path d="M32 14 L50 48 H14 Z" fill="none" stroke="{PAPER}" stroke-width="4"/>'
        ),
        "sense-grip": (
            f'<rect x="12" y="8" width="14" height="48" fill="{STEEL}"/>'
            f'<path d="M32 14 C54 14 54 50 32 50" fill="none" stroke="{PAPER}" stroke-width="5" stroke-linecap="round"/>'
        ),
        "sense-stick-down": (
            f'<circle cx="32" cy="18" r="8" fill="none" stroke="{PAPER}" stroke-width="3"/>'
            f'<path d="M32 26 V40 M22 36 L32 50 L42 36" fill="none" stroke="{PAPER}" stroke-width="3"/>'
        ),
        "digit-slot": (
            f'<rect x="12" y="16" width="40" height="32" fill="none" stroke="{STEEL}" stroke-width="2" stroke-dasharray="3 3"/>'
        ),
        "name-rule": (
            f'<path d="M10 40 H54" fill="none" stroke="{STEEL}" stroke-width="2"/>'
            f'<path d="M10 40 H18 M46 40 H54" fill="none" stroke="{PAPER}" stroke-width="2"/>'
        ),
        "wheel-current": (
            f'<path d="M10 34 H18 V46 H14 V38 H10 Z" fill="{PAPER}"/>'
            f'<path d="M18 33 H40 L42 36 V38 H46 L48 34 H54" fill="none" stroke="{PAPER}" stroke-width="2" stroke-linejoin="round"/>'
            f'<path d="M48 34 H54 V35" fill="none" stroke="{PAPER}" stroke-width="2"/>'
        ),
        "weapon-centurion": (
            f'<path d="M8 34 H16 V48 H12 V38 H8 Z" fill="{PAPER}"/>'
            f'<path d="M16 32 H38 L40 35 V37 H44 L46 33 H54" fill="none" stroke="{PAPER}" stroke-width="2.5" stroke-linejoin="round"/>'
            f'<path d="M44 33 H54" fill="none" stroke="{PAPER}" stroke-width="2.5"/>'
            f'<path d="M22 37 H26 V42 H22 Z" fill="none" stroke="{STEEL}" stroke-width="1.5"/>'
        ),
        "weapon-valkyrie": (
            f'<path d="M6 36 H12 V44 H8 V38 H6 Z" fill="{PAPER}"/>'
            f'<path d="M12 34 H46 V38 H12 Z" fill="{PAPER}" stroke="{PAPER}" stroke-width="1"/>'
            f'<path d="M46 35 H58 V37 H46 Z" fill="none" stroke="{PAPER}" stroke-width="2.5"/>'
            f'<rect x="28" y="38" width="6" height="10" fill="none" stroke="{STEEL}" stroke-width="1.5"/>'
            f'<path d="M20 32 H34 V34 H20 Z" fill="none" stroke="{STEEL}" stroke-width="1.5"/>'
        ),
        "weapon-spikeshot": (
            f'<path d="M6 38 H10 V44 H6 Z" fill="{PAPER}"/>'
            f'<path d="M10 30 H38 V42 H10 Z" fill="none" stroke="{PAPER}" stroke-width="2.5"/>'
            f'<path d="M38 32 H56 V36 H38 Z" fill="none" stroke="{PAPER}" stroke-width="2.5"/>'
            f'<path d="M18 42 H32 V46 H18 Z" fill="none" stroke="{STEEL}" stroke-width="1.5"/>'
        ),
        "weapon-kasimov": (
            f'<path d="M6 37 H14 V43 H6 Z" fill="{PAPER}"/>'
            f'<path d="M14 35 H58" fill="none" stroke="{PAPER}" stroke-width="2.5"/>'
            f'<path d="M14 35 H58 V36 H14 Z" fill="none" stroke="{PAPER}" stroke-width="2"/>'
            f'<rect x="22" y="28" width="16" height="6" fill="none" stroke="{STEEL}" stroke-width="1.5"/>'
            f'<path d="M50 33 V37" fill="none" stroke="{STEEL}" stroke-width="1.5"/>'
        ),
        "wheel-slot-empty": (
            f'<rect x="18" y="18" width="28" height="28" fill="none" stroke="{STEEL}" stroke-width="1.5" stroke-dasharray="4 3"/>'
            f'<circle cx="32" cy="32" r="3" fill="{STEEL}"/>'
        ),
    }


def _plate_chrome(width: int, height: int, notch: str, stroke: str) -> str:
    """notch is 'left' or 'right': the inner edge when the plate sits on a hand."""
    rivets = [(18, 18), (width - 18, 18), (18, height - 18), (width - 18, height - 18)]
    rivet_svg = "".join(
        f'<circle cx="{x}" cy="{y}" r="3" fill="none" stroke="{STEEL}" stroke-width="1.5"/>'
        f'<circle cx="{x}" cy="{y}" r="1.2" fill="{PAPER}"/>'
        for x, y in rivets
    )
    if notch == "left":
        notch_svg = f'<path d="M3 44 H14 V{height - 44} H3" fill="{STEEL}"/>'
    else:
        notch_svg = f'<path d="M{width - 3} 44 H{width - 14} V{height - 44} H{width - 3}" fill="{STEEL}"/>'
    return (
        f'<g id="plate">'
        f'<rect x="3" y="3" width="{width - 6}" height="{height - 6}" fill="{PLATE}" stroke="{stroke}" stroke-width="2"/>'
        f"{notch_svg}{rivet_svg}"
        f'<path d="M28 8 H{width - 28}" fill="none" stroke="{HAIR}" stroke-width="1"/>'
        f"</g>"
    )


def _ticks(mode: str) -> str:
    """mode: empty, full, low. Low keeps two worn ticks and eight empty ones."""
    parts = ['<g id="health-ticks">']
    for i in range(10):
        x = 20 + i * 26
        if mode == "full":
            kind = "full"
        elif mode == "low" and i < 2:
            kind = "worn"
        else:
            kind = "empty"
        fill = PAPER if kind != "empty" else "none"
        stroke = STEEL
        if kind == "worn":
            body = (
                f'<path d="M{x} 46 H{x + 14} L{x + 18} 52 V76 H{x} Z" fill="{PAPER}" stroke="{stroke}" stroke-width="1.5"/>'
            )
        else:
            body = (
                f'<path d="M{x} 28 H{x + 14} L{x + 18} 34 V76 H{x} Z" fill="{fill}" stroke="{stroke}" stroke-width="1.5"/>'
            )
        parts.append(f'<g id="health-tick-{i}" class="tick {kind}" data-fill="{kind}">{body}</g>')
    parts.append("</g>")
    return "".join(parts)


def health_svg(mode: str = "template", notch: str = "right") -> str:
    stroke = AMBER if mode == "low" else STEEL
    ticks = "empty" if mode == "template" else mode
    word = "inline" if mode == "low" else "none"
    svg = _svg_open(440, 112, "plate-health", "Plyta zdrowia")
    svg += _plate_chrome(440, 112, notch, stroke)
    svg += _ticks(ticks if ticks != "template" else "empty")
    svg += (
        f'<rect id="health-value-slot" x="292" y="30" width="112" height="52" fill="none" '
        f'stroke="{SLOT}" stroke-width="1.5" stroke-dasharray="3 3"/>'
        f'<text id="health-value" class="data" x="300" y="72" fill="{PAPER}" '
        f'font-family="Segoe UI, Arial, sans-serif" font-size="36"></text>'
        f'<text id="state-low" x="300" y="100" fill="{PAPER}" '
        f'font-family="Segoe UI, Arial, sans-serif" font-size="16" '
        f'font-weight="700" display="{word}">MA\u0141O</text>'
    )
    svg += "</svg>\n"
    return svg


def ammo_svg(mode: str = "template", notch: str = "left") -> str:
    stroke = AMBER if mode == "empty" else STEEL
    closed = "none" if mode == "empty" else "inline"
    opened = "inline" if mode == "empty" else "none"
    reload = "inline" if mode == "empty" else "none"
    name = "inline" if mode == "switch" else "none"
    svg = _svg_open(440, 148, "plate-ammo", "Plyta amunicji")
    svg += _plate_chrome(440, 148, notch, stroke)
    svg += (
        f'<g id="chamber-closed" display="{closed}">'
        f'<rect x="22" y="28" width="64" height="72" fill="none" stroke="{PAPER}" stroke-width="2"/>'
        f'<path d="M38 28 V100 M22 52 H86 M22 74 H86" fill="none" stroke="{STEEL}" stroke-width="2"/>'
        f"</g>"
        f'<g id="chamber-open" display="{opened}">'
        f'<path d="M22 44 H40 M64 28 H86 V100 H22 V44" fill="none" stroke="{PAPER}" stroke-width="2"/>'
        f'<path d="M34 56 H74 M34 76 H74" fill="none" stroke="{STEEL}" stroke-width="2"/>'
        f'<path d="M96 96 H132 V70 H156" fill="none" stroke="{PAPER}" stroke-width="3"/>'
        f'<rect x="148" y="60" width="10" height="14" fill="{PAPER}"/>'
        f"</g>"
        f'<rect id="ammo-mag-slot" x="168" y="32" width="78" height="58" fill="none" '
        f'stroke="{SLOT}" stroke-width="1.5" stroke-dasharray="3 3"/>'
        f'<text id="ammo-mag" class="data" x="176" y="78" fill="{PAPER}" '
        f'font-family="Segoe UI, Arial, sans-serif" font-size="44"></text>'
        f'<text id="ammo-sep" x="254" y="78" fill="{STEEL}" '
        f'font-family="Segoe UI, Arial, sans-serif" font-size="28">/</text>'
        f'<rect id="ammo-reserve-slot" x="276" y="40" width="96" height="46" fill="none" '
        f'stroke="{SLOT}" stroke-width="1.5" stroke-dasharray="3 3"/>'
        f'<text id="ammo-reserve" class="data" x="284" y="76" fill="{STEEL}" '
        f'font-family="Segoe UI, Arial, sans-serif" font-size="28"></text>'
        f'<text id="state-reload" x="168" y="128" fill="{PAPER}" '
        f'font-family="Segoe UI, Arial, sans-serif" font-size="20" '
        f'font-weight="700" display="{reload}">PRZE\u0141ADUJ</text>'
        f'<g id="weapon-name" display="{name}">'
        f'<path d="M168 108 H400" fill="none" stroke="{STEEL}" stroke-width="1.5"/>'
        f'<text id="weapon-name-text" class="data" x="168" y="104" fill="{PAPER}" '
        f'font-family="Segoe UI, Arial, sans-serif" font-size="18"></text>'
        f"</g>"
    )
    svg += "</svg>\n"
    return svg


def weapon_name_svg() -> str:
    svg = _svg_open(440, 56, "plate-weapon-name", "Krotka nazwa broni")
    svg += _plate_chrome(440, 56, "left", STEEL)
    svg += (
        f'<path id="name-rule" d="M28 40 H412" fill="none" stroke="{STEEL}" stroke-width="1.5"/>'
        f'<text id="weapon-name-text" class="data" x="28" y="34" fill="{PAPER}" '
        f'font-family="Segoe UI, Arial, sans-serif" font-size="22"></text>'
    )
    svg += "</svg>\n"
    return svg


def _token_table() -> list[dict]:
    """Player-facing controls from g_padButtons and the menu-back path.

    Right-handed defaults. Left-handed is a restart-time mirror, not a
    second art pass. prompt_roles.h and the Pad N table are not sources.
    """
    return [
        {
            "token": "health",
            "icon": "sense-circle",
            "sentence_key": "GameDialogs.HealthPackDesc",
            "sentence_pl": "Naci\u015bnij [health], aby u\u017cy\u0107 znalezionych w \u015bwiecie apteczek.",
            "replace": "icon only, Polish sentence stays",
            "openxr": "/user/hand/right/input/b/click",
            "emits": "DPAD_UP 0x0001 (kCircleHealthMask)",
            "sense_right_handed": "Circle on the right Sense controller",
            "sense_left_handed": "mirrorHand moves right/b to left/y. That path is also the Triangle menu button, which is not mirrored. Do not ship the left-handed health icon until that collision is tested.",
            "not": "D-pad, Pad 11, R1",
        },
        {
            "token": "use",
            "icon": "sense-grip",
            "sentence_key": "Pickup.TakeHealthPack",
            "sentence_pl": "[use] Apteczka",
            "replace": "icon only, Polish sentence stays",
            "openxr": "/user/hand/right/input/squeeze/value",
            "emits": "XI_X 0x4000",
            "sense_right_handed": "Right grip",
            "sense_left_handed": "Left grip",
            "not": "R1, face button X",
        },
        {
            "token": "reload",
            "icon": "sense-grip",
            "same_rect_as": "use",
            "sentence_key": "Tutorial.Reload",
            "sentence_pl": "Naci\u015bnij [reload], aby prze\u0142adowa\u0107",
            "replace": "icon only, Polish sentence stays",
            "openxr": "/user/hand/right/input/squeeze/value",
            "emits": "XI_X 0x4000",
            "sense_right_handed": "Right grip. Same g_padButtons row as use: \"Use / reload\". There is no second reload binding.",
            "sense_left_handed": "Left grip",
            "not": "a different button from use, R1",
        },
        {
            "token": "age",
            "icon": "sense-square",
            "sentence_key": "Tutorial.Medkit",
            "sentence_pl": "Naci\u015bnij [age], aby odnowi\u0107 apteczki i odzyska\u0107 znajduj\u0105ce si\u0119 w nich pakiety zdrowia.",
            "replace": "icon only, Polish sentence stays",
            "openxr": "/user/hand/left/input/x/click",
            "emits": "XI_LSHOULDER 0x0100",
            "sense_right_handed": "Square on the left Sense controller",
            "sense_left_handed": "mirrorHand moves left/x to right/a, the Cross position. Jump moves the other way.",
            "not": "LB bumper, Pad N",
        },
        {
            "token": "cancel",
            "icon": "sense-circle",
            "same_physical_button_as": "health",
            "context": "menu",
            "sentence_key": "",
            "sentence_pl": "",
            "replace": "icon only, and only after a screenshot shows the sentence",
            "openxr": "/user/hand/right/input/b/click",
            "emits": "XI_B 0x2000 while the menu gate is on (g_actUiBack)",
            "sense_right_handed": "Circle. Menu back is bound with handed=false, so it stays on the right controller.",
            "sense_left_handed": "Stays on right Circle. It does not follow the health mirror.",
            "not": "Pad N. Not the weapon-wheel cancel, which is L3 in weapon_wheel.h. In gameplay, Circle emits health (DPAD_UP), and native B is right-stick down (crouch).",
        },
    ]


def atlas_svg(icons: dict[str, str]) -> str:
    svg = _svg_open(ATLAS_W, ATLAS_H, "katorga-hud-atlas", "Atlas HUD Katorga")
    svg += f'<rect width="{ATLAS_W}" height="{ATLAS_H}" fill="none"/>\n'
    for name, x, y in RECTS:
        svg += f'<g id="{name}" transform="translate({x} {y})">\n{icons[name]}\n</g>\n'
    svg += "</svg>\n"
    return svg


def _pair(ident: str, title: str, left: str, right: str, height: int) -> str:
    svg = _svg_open(920, height, ident, title)
    svg += f'<rect width="920" height="{height}" fill="#121416"/>\n'
    svg += f'<g transform="translate(16 16)">{_inner(left)}</g>\n'
    svg += f'<g transform="translate(470 16)">{_inner(right)}</g>\n'
    svg += "</svg>\n"
    return svg


def _state_sheet_svg() -> str:
    """Four states. Data fields stay empty. Words and shapes carry the state."""
    svg = _svg_open(960, 620, "states", "Stany plyt")
    svg += '<rect width="960" height="620" fill="#121416"/>\n'
    svg += (
        '<text x="24" y="28" fill="#8d97a1" font-family="Segoe UI, Arial, sans-serif" font-size="16">'
        "Stany bez liczb. Puste pola s\u0105 miejscem na dane. "
        "Niskie zdrowie i pusty magazynek czyta si\u0119 kszta\u0142tem i s\u0142owem."
        "</text>\n"
    )
    svg += f'<text x="24" y="56" fill="{STEEL}" font-family="Segoe UI, Arial, sans-serif" font-size="14">SPOK\u00d3J</text>\n'
    svg += f'<g transform="translate(24 68)">{_inner(health_svg("full"))}</g>\n'
    svg += f'<g transform="translate(490 68)">{_inner(ammo_svg("template"))}</g>\n'
    svg += f'<text x="24" y="236" fill="{AMBER}" font-family="Segoe UI, Arial, sans-serif" font-size="14">MA\u0141O I PUSTY MAGAZYNEK</text>\n'
    svg += f'<g transform="translate(24 248)">{_inner(health_svg("low"))}</g>\n'
    svg += f'<g transform="translate(490 248)">{_inner(ammo_svg("empty"))}</g>\n'
    svg += f'<text x="24" y="430" fill="{STEEL}" font-family="Segoe UI, Arial, sans-serif" font-size="14">ZMIANA BRONI \u2014 pole nazwy puste</text>\n'
    svg += f'<g transform="translate(24 442)">{_inner(ammo_svg("switch"))}</g>\n'
    svg += f'<g transform="translate(490 490)">{_inner(weapon_name_svg())}</g>\n'
    svg += "</svg>\n"
    return svg


def _inner(svg: str) -> str:
    start = svg.find(">", svg.find("<svg")) + 1
    end = svg.rfind("</svg>")
    return svg[start:end]


def prompt_sheet_svg(icons: dict[str, str]) -> str:
    rows = [
        ("health", "sense-circle", "Naci\u015bnij", "aby u\u017cy\u0107 apteczki."),
        ("use", "sense-grip", "", "Apteczka"),
        ("age", "sense-square", "Naci\u015bnij", "aby odnowi\u0107 apteczki."),
        ("reload", "sense-grip", "Naci\u015bnij", "aby prze\u0142adowa\u0107."),
        ("cancel", "sense-circle", "Naci\u015bnij", "aby wr\u00f3ci\u0107. Tylko menu."),
    ]
    svg = _svg_open(880, 420, "prompt-replace", "Podmiana tokenu")
    svg += f'<rect width="880" height="420" fill="#121416"/>\n'
    svg += (
        '<text x="24" y="32" fill="#8d97a1" font-family="Segoe UI, Arial, sans-serif" font-size="16">'
        "Ikona wchodzi w miejsce tokenu. Druga legenda obok zdania nie powstaje."
        "</text>\n"
    )
    y = 64
    for token, icon, before, after in rows:
        svg += f'<g transform="translate(24 {y})">\n'
        svg += f'<text x="0" y="28" fill="{STEEL}" font-family="Segoe UI, Arial, sans-serif" font-size="14">[{token}]</text>\n'
        svg += f'<text x="120" y="36" fill="{PAPER}" font-family="Segoe UI, Arial, sans-serif" font-size="22">{before}</text>\n'
        svg += f'<g transform="translate(250 0)">{icons[icon]}</g>\n'
        svg += f'<text x="330" y="36" fill="{PAPER}" font-family="Segoe UI, Arial, sans-serif" font-size="22">{after}</text>\n'
        svg += "</g>\n"
        y += 68
    svg += "</svg>\n"
    return svg


def hands_svg() -> str:
    svg = _svg_open(1280, 720, "hands-floor", "Obie rece, siedzenie")
    svg += f'<rect width="1280" height="720" fill="#121416"/>\n'
    svg += (
        '<text x="32" y="36" fill="#8d97a1" font-family="Segoe UI, Arial, sans-serif" font-size="16">'
        "Siedzenie na pod\u0142odze. P\u0142yty s\u0105 nisko, przy d\u0142oniach. \u015arodek zostaje pusty. "
        "Karb na p\u0142ycie patrzy do \u015brodka cia\u0142a."
        "</text>\n"
    )
    # Right-handed: health left, ammo right, low in the circle.
    svg += f'<circle cx="340" cy="400" r="230" fill="none" stroke="{STEEL}" stroke-dasharray="5 7"/>\n'
    svg += f'<text x="220" y="186" fill="{STEEL}" font-family="Segoe UI, Arial, sans-serif" font-size="14">PRAWA R\u0118KA CELUJE</text>\n'
    svg += f'<g transform="translate(150 500) scale(0.40)">{_inner(health_svg("full"))}</g>\n'
    svg += f'<g transform="translate(338 500) scale(0.40)">{_inner(ammo_svg("template"))}</g>\n'
    # Left-handed plates swap sides. Prompt glyphs do not simply mirror; the
    # caption says so. The plate art flips so the notch stays medial.
    svg += f'<circle cx="960" cy="400" r="230" fill="none" stroke="{STEEL}" stroke-dasharray="5 7"/>\n'
    svg += f'<text x="840" y="186" fill="{STEEL}" font-family="Segoe UI, Arial, sans-serif" font-size="14">LEWA R\u0118KA CELUJE</text>\n'
    svg += f'<g transform="translate(770 500) scale(0.40)">{_inner(ammo_svg("template", notch="right"))}</g>\n'
    svg += f'<g transform="translate(958 500) scale(0.40)">{_inner(health_svg("full", notch="left"))}</g>\n'
    svg += (
        '<text x="700" y="680" fill="#8d97a1" font-family="Segoe UI, Arial, sans-serif" font-size="13">'
        "Odbicie p\u0142yt jest rysunkiem. Ikona [age] przy lewej r\u0119ce celuj\u0105cej "
        "staje si\u0119 krzy\u017cykiem, a [health] wpada na tr\u00f3jk\u0105t menu. Tego rysunek nie rozstrzyga."
        "</text>\n"
    )
    svg += "</svg>\n"
    return svg


def _font(size: int, bold: bool = False):
    from PIL import ImageFont

    name = "segoeuib.ttf" if bold else "segoeui.ttf"
    path = Path("C:/Windows/Fonts") / name
    if path.is_file():
        return ImageFont.truetype(str(path), size)
    return ImageFont.load_default()


def _draw_icon_pil(draw, name: str, ox: int, oy: int) -> None:
    paper = PAPER_RGB
    steel = STEEL_RGB

    def p(points, fill=None, outline=None, width=2):
        shifted = [(ox + x, oy + y) for x, y in points]
        draw.polygon(shifted, fill=fill, outline=outline)
        if outline and width > 1:
            draw.line(shifted + [shifted[0]], fill=outline, width=width)

    def line(a, b, fill, width=2):
        draw.line([(ox + a[0], oy + a[1]), (ox + b[0], oy + b[1])], fill=fill, width=width)

    if name == "tick-full":
        p([(22, 12), (40, 12), (44, 18), (44, 54), (18, 54), (18, 18)], paper, steel, 2)
    elif name == "tick-empty":
        p([(22, 12), (40, 12), (44, 18), (44, 54), (18, 54), (18, 18)], None, steel, 2)
    elif name == "tick-worn":
        p([(22, 30), (40, 30), (44, 36), (44, 54), (18, 54), (18, 36)], paper, steel, 2)
    elif name == "mag-closed":
        draw.rectangle([ox + 16, oy + 12, ox + 48, oy + 52], outline=paper, width=2)
        line((24, 12), (24, 52), steel)
        line((16, 26), (48, 26), steel)
        line((16, 38), (48, 38), steel)
    elif name == "mag-open":
        line((18, 26), (18, 50), paper, 3)
        line((18, 50), (46, 50), paper, 3)
        line((46, 50), (46, 26), paper, 3)
        line((20, 14), (44, 14), paper, 3)
    elif name == "reload-mark":
        draw.rectangle([ox + 8, oy + 24, ox + 24, oy + 40], fill=paper)
        draw.rectangle([ox + 40, oy + 24, ox + 56, oy + 40], fill=paper)
        line((26, 32), (38, 32), steel, 2)
    elif name == "warning-notch":
        draw.rectangle([ox + 14, oy + 14, ox + 50, oy + 50], outline=paper, width=2)
        line((20, 42), (30, 22), paper, 2)
        line((28, 44), (40, 22), paper, 2)
        line((36, 46), (48, 24), paper, 2)
    elif name == "rivet":
        draw.ellipse([ox + 26, oy + 26, ox + 38, oy + 38], outline=steel, width=2)
        draw.ellipse([ox + 30, oy + 30, ox + 34, oy + 34], fill=paper)
    elif name == "sense-circle":
        draw.ellipse([ox + 16, oy + 16, ox + 48, oy + 48], outline=paper, width=4)
    elif name == "sense-square":
        draw.rectangle([ox + 16, oy + 16, ox + 48, oy + 48], outline=paper, width=4)
    elif name == "sense-cross":
        line((20, 20), (44, 44), paper, 4)
        line((44, 20), (20, 44), paper, 4)
    elif name == "sense-triangle":
        p([(32, 14), (50, 48), (14, 48)], None, paper, 4)
    elif name == "sense-grip":
        draw.rectangle([ox + 12, oy + 8, ox + 26, oy + 56], fill=steel)
        draw.arc([ox + 28, oy + 12, ox + 56, oy + 52], start=270, end=90, fill=paper, width=5)
    elif name == "sense-stick-down":
        draw.ellipse([ox + 24, oy + 10, ox + 40, oy + 26], outline=paper, width=3)
        line((32, 26), (32, 40), paper, 3)
        line((22, 36), (32, 50), paper, 3)
        line((42, 36), (32, 50), paper, 3)
    elif name == "digit-slot":
        draw.rectangle([ox + 12, oy + 16, ox + 52, oy + 48], outline=steel, width=2)
    elif name == "name-rule":
        line((10, 40), (54, 40), steel, 2)
        line((10, 40), (18, 40), paper, 2)
        line((46, 40), (54, 40), paper, 2)
    elif name == "wheel-current":
        draw.rectangle([ox + 10, oy + 34, ox + 18, oy + 46], fill=paper)
        line((18, 33), (38, 33), paper, 2)
        line((38, 33), (40, 35), paper, 2)
        line((40, 35), (46, 33), paper, 2)
        line((46, 33), (54, 33), paper, 2)
    elif name == "weapon-centurion":
        draw.rectangle([ox + 8, oy + 34, ox + 16, oy + 48], fill=paper)
        line((16, 32), (38, 32), paper, 3)
        line((38, 32), (40, 35), paper, 3)
        line((40, 35), (44, 35), paper, 3)
        line((44, 35), (46, 33), paper, 3)
        line((46, 33), (54, 33), paper, 3)
        draw.rectangle([ox + 22, oy + 37, ox + 26, oy + 42], outline=steel, width=2)
    elif name == "weapon-valkyrie":
        draw.polygon([(ox + 6, oy + 36), (ox + 12, oy + 36), (ox + 12, oy + 44), (ox + 8, oy + 44), (ox + 6, oy + 38)], fill=paper)
        draw.rectangle([ox + 12, oy + 34, ox + 46, oy + 38], fill=paper)
        draw.rectangle([ox + 46, oy + 35, ox + 58, oy + 37], outline=paper, width=2)
        draw.rectangle([ox + 28, oy + 38, ox + 34, oy + 48], outline=steel, width=2)
        draw.rectangle([ox + 20, oy + 32, ox + 34, oy + 34], outline=steel, width=2)
    elif name == "weapon-spikeshot":
        draw.rectangle([ox + 6, oy + 38, ox + 10, oy + 44], fill=paper)
        draw.rectangle([ox + 10, oy + 30, ox + 38, oy + 42], outline=paper, width=3)
        draw.rectangle([ox + 38, oy + 32, ox + 56, oy + 36], outline=paper, width=3)
        draw.rectangle([ox + 18, oy + 42, ox + 32, oy + 46], outline=steel, width=2)
    elif name == "weapon-kasimov":
        draw.rectangle([ox + 6, oy + 37, ox + 14, oy + 43], fill=paper)
        line((14, 35), (58, 35), paper, 3)
        draw.rectangle([ox + 22, oy + 28, ox + 38, oy + 34], outline=steel, width=2)
        line((50, 33), (50, 37), steel, 2)
    elif name == "wheel-slot-empty":
        draw.rectangle([ox + 18, oy + 18, ox + 46, oy + 46], outline=steel, width=2)
        draw.ellipse([ox + 29, oy + 29, ox + 35, oy + 35], fill=steel)


def _plate_pil(image, origin, kind: str, mode: str, scale: float, sample: str | None, name: str | None = None, medial: str | None = None) -> None:
    from PIL import ImageDraw

    draw = ImageDraw.Draw(image)
    ox, oy = origin
    width = int(440 * scale)
    height = int((112 if kind == "health" else 148 if kind == "ammo" else 56) * scale)
    stroke = AMBER_RGB if (kind == "health" and mode == "low") or (kind == "ammo" and mode == "empty") else STEEL_RGB
    draw.rectangle([ox + 2, oy + 2, ox + width - 2, oy + height - 2], fill=PLATE_RGB, outline=stroke, width=max(1, int(2 * scale)))
    # Medial notch.
    if kind == "ammo":
        side = medial or "left"
    elif kind == "health":
        side = medial or "right"
    else:
        side = medial or "left"
    if side == "left":
        draw.rectangle([ox + 2, oy + int(44 * scale), ox + int(12 * scale), oy + height - int(44 * scale)], fill=STEEL_RGB)
    elif side == "right":
        draw.rectangle([ox + width - int(12 * scale), oy + int(44 * scale), ox + width - 2, oy + height - int(44 * scale)], fill=STEEL_RGB)
    rivet = max(2, int(3 * scale))
    for rx, ry in ((18, 18), (440 - 18, 18), (18, (112 if kind == "health" else 148 if kind == "ammo" else 56) - 18), (440 - 18, (112 if kind == "health" else 148 if kind == "ammo" else 56) - 18)):
        cx, cy = ox + int(rx * scale), oy + int(ry * scale)
        draw.ellipse([cx - rivet, cy - rivet, cx + rivet, cy + rivet], outline=STEEL_RGB)

    font_d = _font(max(12, int(36 * scale)), True)
    font_s = _font(max(10, int(16 * scale)), True)
    font_w = _font(max(11, int(20 * scale)), True)

    if kind == "health":
        for i in range(10):
            x = ox + int((20 + i * 26) * scale)
            y0 = oy + int(28 * scale)
            y1 = oy + int(76 * scale)
            w = int(16 * scale)
            filled = mode == "full" or (mode == "low" and i < 2)
            if filled and mode == "low":
                top = oy + int(46 * scale)
                draw.polygon(
                    [(x, top), (x + int(12 * scale), top), (x + w, top + int(6 * scale)), (x + w, y1), (x, y1)],
                    fill=PAPER_RGB,
                )
            elif filled:
                draw.polygon([(x, y0), (x + int(12 * scale), y0), (x + w, y0 + int(6 * scale)), (x + w, y1), (x, y1)], fill=PAPER_RGB)
            else:
                draw.polygon([(x, y0), (x + int(12 * scale), y0), (x + w, y0 + int(6 * scale)), (x + w, y1), (x, y1)], outline=STEEL_RGB)
        if sample:
            draw.text((ox + int(300 * scale), oy + int(36 * scale)), sample, fill=PAPER_RGB, font=font_d)
        if mode == "low":
            draw.text((ox + int(300 * scale), oy + int(82 * scale)), "MA\u0141O", fill=PAPER_RGB, font=font_s)
    elif kind == "ammo":
        bx, by = ox + int(22 * scale), oy + int(28 * scale)
        bw, bh = int(64 * scale), int(72 * scale)
        if mode == "empty":
            draw.line([(bx, by + int(16 * scale)), (bx + int(18 * scale), by + int(16 * scale))], fill=PAPER_RGB, width=2)
            draw.line([(bx + int(42 * scale), by), (bx + bw, by), (bx + bw, by + bh), (bx, by + bh), (bx, by + int(16 * scale))], fill=PAPER_RGB, width=2)
            draw.text((ox + int(168 * scale), oy + int(108 * scale)), "PRZE\u0141ADUJ", fill=PAPER_RGB, font=font_w)
        else:
            draw.rectangle([bx, by, bx + bw, by + bh], outline=PAPER_RGB, width=2)
            draw.line([(bx + int(16 * scale), by), (bx + int(16 * scale), by + bh)], fill=STEEL_RGB, width=2)
        if sample and "/" in sample:
            mag, reserve = sample.split("/")
            draw.text((ox + int(176 * scale), oy + int(36 * scale)), mag, fill=PAPER_RGB, font=font_d)
            draw.text((ox + int(250 * scale), oy + int(48 * scale)), "/", fill=STEEL_RGB, font=font_s)
            draw.text((ox + int(284 * scale), oy + int(48 * scale)), reserve, fill=STEEL_RGB, font=font_s)
        if mode == "switch" and name:
            draw.line(
                [(ox + int(168 * scale), oy + int(118 * scale)), (ox + width - int(24 * scale), oy + int(118 * scale))],
                fill=STEEL_RGB,
                width=2,
            )
            draw.text((ox + int(168 * scale), oy + int(86 * scale)), name, fill=PAPER_RGB, font=font_s)
    elif kind == "name":
        if sample:
            draw.text((ox + int(28 * scale), oy + int(12 * scale)), sample, fill=PAPER_RGB, font=font_s)


def _sheet(bg: tuple[int, int, int, int], small: bool) -> "object":
    from PIL import Image, ImageDraw

    w, h = (640, 360) if small else (1280, 720)
    scale = 0.42 if small else 0.85
    image = Image.new("RGBA", (w, h), bg)
    draw = ImageDraw.Draw(image)
    title = "Ma\u0142y rozmiar, dystans d\u0142oni" if small else "Rozmiar redakcyjny"
    draw.text((16, 8), title, fill=STEEL_RGB, font=_font(16 if not small else 13))
    y = 36 if small else 40
    _plate_pil(image, (16, y), "health", "full", scale, "10/10" if not small else "10/10")
    _plate_pil(image, (int(400 * scale) + 24, y), "ammo", "template", scale, "12/48")
    y2 = y + int(130 * scale) + (8 if small else 16)
    _plate_pil(image, (16, y2), "health", "low", scale, "2/10")
    _plate_pil(image, (int(400 * scale) + 24, y2), "ammo", "empty", scale, "0/20")
    y3 = y2 + int(160 * scale) + (8 if small else 12)
    if y3 + int(80 * scale) < h:
        _plate_pil(image, (16, y3), "ammo", "switch", scale, "12/48", name="Centurion")
    return image


def _hands_png(bg: tuple[int, int, int, int]) -> "object":
    from PIL import Image, ImageDraw

    image = Image.new("RGBA", (1280, 720), bg)
    draw = ImageDraw.Draw(image)
    ink = STEEL_RGB if bg[0] < 40 else (40, 48, 56, 255)
    draw.text((32, 16), "Siedzenie na pod\u0142odze. P\u0142yty przy d\u0142oniach, \u015brodek pusty.", fill=ink, font=_font(18))
    for cx, label in ((340, "Prawa r\u0119ka celuje"), (960, "Lewa r\u0119ka celuje")):
        draw.ellipse([cx - 230, 170, cx + 230, 630], outline=ink, width=2)
        draw.text((cx - 100, 186), label, fill=ink, font=_font(16))
    # Low placement: seated hands, not chest height.
    _plate_pil(image, (150, 500), "health", "full", 0.40, None)
    _plate_pil(image, (338, 500), "ammo", "template", 0.40, None)
    _plate_pil(image, (770, 500), "ammo", "template", 0.40, None, medial="right")
    _plate_pil(image, (958, 500), "health", "full", 0.40, None, medial="left")
    return image


def _prompts_png() -> "object":
    from PIL import Image, ImageDraw

    image = Image.new("RGBA", (980, 460), (18, 20, 22, 255))
    draw = ImageDraw.Draw(image)
    font = _font(22)
    draw.text((24, 16), "Token znika. Zostaje ikona wewn\u0105trz zdania.", fill=STEEL_RGB, font=_font(18))
    rows = [
        ("health", "sense-circle", "Naci\u015bnij ", " aby u\u017cy\u0107 apteczki."),
        ("use", "sense-grip", "", " Apteczka"),
        ("age", "sense-square", "Naci\u015bnij ", " aby odnowi\u0107 apteczki."),
        ("reload", "sense-grip", "Naci\u015bnij ", " aby prze\u0142adowa\u0107."),
        ("cancel", "sense-circle", "Naci\u015bnij ", " aby wr\u00f3ci\u0107. Tylko menu."),
    ]
    y = 56
    for token, icon, before, after in rows:
        draw.text((16, y + 18), "[" + token + "]", fill=STEEL_RGB, font=_font(15))
        draw.text((130, y + 16), before, fill=PAPER_RGB, font=font)
        icon_x = 130 + (draw.textlength(before, font=font) if before else 0)
        _draw_icon_pil(draw, icon, int(icon_x), y)
        draw.text((icon_x + 72, y + 16), after, fill=PAPER_RGB, font=font)
        y += 76
    return image


def _gray(image):
    from PIL import Image

    # Keep the plate silhouette. Amber must not be the only signal.
    rgb = image.convert("RGB").convert("L").convert("RGBA")
    return rgb


def _write_png(path: Path, image) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path, "PNG")


def _atlas_png():
    from PIL import Image, ImageDraw

    image = Image.new("RGBA", (ATLAS_W, ATLAS_H), CLEAR)
    draw = ImageDraw.Draw(image)
    for name, x, y in RECTS:
        _draw_icon_pil(draw, name, x, y)
    return image


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    digest.update(path.read_bytes())
    return digest.hexdigest()


def _obj() -> str:
    # 64 mm by 20 mm by 1.6 mm. Centered. No skeleton and no socket.
    w, h, d = 0.032, 0.010, 0.0008
    verts = [
        (-w, -h, d), (w, -h, d), (w, h, d), (-w, h, d),
        (-w, -h, -d), (w, -h, -d), (w, h, -d), (-w, h, -d),
    ]
    faces = [
        (1, 2, 3, 4),
        (5, 8, 7, 6),
        (1, 5, 6, 2),
        (2, 6, 7, 3),
        (3, 7, 8, 4),
        (4, 8, 5, 1),
    ]
    lines = [
        "# Katorga HUD plate frame. Original, MIT.",
        "# Units: meters. 64 mm x 20 mm x 1.6 mm.",
        "# Flat frame only. No skeleton. No weapon socket. No TMD socket.",
        "o katorga_plate_frame",
    ]
    for x, y, z in verts:
        lines.append(f"v {x:.4f} {y:.4f} {z:.4f}")
    for a, b, c, d in faces:
        lines.append(f"f {a} {b} {c} {d}")
    lines.append("")
    return "\n".join(lines)


def _docs(manifest: dict) -> tuple[str, str]:
    assets = f"""# Assety HUD-u Katorga

Grafika jest gotowa jako pliki. Podpięcie pod broń, nadgarstek i zdania gry nie jest zrobione i nie jest sprawdzone w goglach.

## Pochodzenie i licencja

Wszystkie płyty, ikony, podglądy i ramka OBJ są narysowane na nowo do tego moda. Objęte są licencją MIT repozytorium. Nie pochodzą z plików Singularity, z fontów gry, z atlasu Kenney ani z arkusza znaków konsoli. Żaden font nie jest osadzony. Napisy w SVG używają nazwy kroju tylko jako podpowiedzi dla podglądu. Raster podglądu powstał czcionką Segoe UI obecną w Windows w chwili generowania i ta czcionka nie wchodzi do gry.

Kolory są z projektu HUD-u: płyta `#101214`, stal `#5a6f8d`, papier `#e6e1d6`, bursztyn `#c4a46a` wyłącznie jako drugi kanał obok słowa albo ubytku kształtu.

## Pliki

- `assets/hud/katorga/plates/` — edytowalne płyty. Pola `health-value`, `ammo-mag`, `ammo-reserve`, `weapon-name-text` są puste.
- `assets/hud/katorga/states/` — spokój, mało zdrowia, pusty magazynek, zmiana broni. Nadal bez przykładowych liczb.
- `assets/hud/katorga/icons/` — pojedyncze komórki.
- `assets/hud/katorga/atlas/katorga-hud-atlas.svg`, `.png` i `atlas.json`.
- `assets/hud/katorga/previews/` — jasne i ciemne tło, mały rozmiar, obie ręce, zdania promptów, wersja bez koloru.
- `assets/hud/katorga/frame/plate-frame.obj` — płaska ramka 64×20×1,6 mm. Nie ma kości ani gniazda na broń albo TMD.
- `assets/hud/katorga/previews/weapon-wheel.png` — koło broni (atlas rząd 3, nie concept art).
- `assets/hud/katorga/3d/previews/weapon_wheel_generated.png` — ten sam renderer co wyżej, dla paczki MAIN.

Źródłem odtwarzalnym jest `tools/hud/katorga_kit.py`. Sylwetki broni w atlasie (`weapon-*`, `wheel-*`) są oryginalnym line-art moda, nie wycinkami z Singularity.

## Koło broni (R3)

Komórki atlasu y=128: `wheel-current`, `weapon-centurion`, `weapon-valkyrie`, `weapon-spikeshot`, `weapon-kasimov`, `wheel-slot-empty`. Mapowanie sektorów: `assets/hud/katorga/3d/weapon_wheel_catalog.json`. Podgląd: `python tools/hud/render_weapon_wheel_preview.py`.

## Stany

Spokój: dziesięć pełnych kresek, zamknięty prostokąt magazynka. Mało zdrowia: zostają dwie krótsze kreski i słowo MAŁO. Pusty magazynek: przerwany obrys komory i słowo PRZEŁADUJ. Zmiana broni: pusta linia nazwy nad nabojami. Bursztynowa ramka dochodzi przy MAŁO i przy pustym magazynku, ale po zdjęciu koloru zostają liczba kresek, przerwa w obrysie i słowo.

Liczby na podglądach PNG (`10/10`, `2/10`, `0/20`, `Centurion`) są tylko próbką czytelności. W płytach ich nie ma.

## Tokeny promptów

Sprawdzone w `spikes/view_matrix/d3d9.cpp` na `72c8e1c`, tabela `g_padButtons`, oraz `input_remap.h`. To nie jest mapa z `prompt_roles.h` (tam zdrowie jest krzyżakiem, a użycie R1) i nie jest tabelą „Pad N” z `prompt_label_rewrite.h`.

| Token | Ikona | Ścieżka, prawa ręka celuje | Co dostaje gra | Lewa ręka celuje |
| --- | --- | --- | --- | --- |
| `[health]` | `sense-circle` | `/user/hand/right/input/b/click` | maska D-pada w górę `0x0001` | `mirrorHand` przenosi na `left/y`. Ta sama ścieżka jest przyciskiem menu (trójkąt) i menu się nie odbija. Kolizja. Ikony lewej ręki nie wdrażać przed testem. |
| `[use]` | `sense-grip` | prawy `squeeze/value` | `XI_X` `0x4000` | lewy chwyt |
| `[reload]` | `sense-grip`, ten sam prostokąt | ten sam wiersz „Use / reload” | `XI_X` `0x4000` | lewy chwyt |
| `[age]` | `sense-square` | `/user/hand/left/input/x/click` | `XI_LSHOULDER` `0x0100` | `right/a`, czyli krzyżyk. Skok idzie w drugą stronę. |
| `[cancel]` | `sense-circle`, tylko menu | `g_actUiBack`, prawy `b/click`, bez odbicia | `XI_B` `0x2000`, gdy menu jest otwarte | zostaje prawe kółko |

Zdania, w które ikona ma wejść zamiast tokenu:

- `[health]` — „Naciśnij [health], aby użyć znalezionych w świecie apteczek.” Klucz `GameDialogs.HealthPackDesc`.
- `[use]` — „[use] Apteczka”. Klucz `Pickup.TakeHealthPack`.
- `[age]` — „Naciśnij [age], aby odnowić apteczki…”. Klucz `Tutorial.Medkit`.
- `[reload]` — „Naciśnij [reload], aby przeładować”. Klucz `Tutorial.Reload`.
- `[cancel]` — w tym przejściu nie ma polskiego zdania z tym tokenem. W kodzie anulowanie menu jest natywnym B, a kółko je wysyła tylko przy otwartym menu. W grze kółko wysyła zdrowie, a natywne B idzie z prawego drążka w dół (kucanie). Anulowanie koła broni to L3 w `weapon_wheel.h` i nie jest tym tokenem.

Podmiana dotyczy samego glifu w zdaniu. Nie dokłada się drugiego rzędu podpisów.

Siedzenie na podłodze: podgląd `hands-floor` stawia płyty w dolnej części koła. Nie ma tu wysokości stojącej postaci ani punktu na siatce.

## Budżet renderowania

Jedna tekstura atlasu {manifest["width"]}×{manifest["height"]} RGBA8, {manifest["bytes_rgba"]} bajtów nieskompresowanych, SHA-256 `{manifest["png_sha256"][:16]}…`. Komórka 64×64, początek w lewym górnym rogu. Prostokąty są w `atlas.json`.

Na oko: dwa quady (zdrowie i amunicja). Nazwa broni jest drugą linią quadu amunicji, nie trzecim rysowaniem. Ikona promptu nie jest quadem: zastępuje glif w już rysowanym zdaniu. Jedno mieszanie alfa, bez drugiego przejścia pełnoekranowego, bez cienia i bez poświaty. Puls poniżej 0,5 Hz tylko na kreskach niskiego zdrowia, bez skalowania płyty. Atlas nie przebudowuje się co klatkę. Jedna mipmapa wystarcza, bo płyta jest blisko dłoni.

Ramka OBJ nie wchodzi do tego budżetu, dopóki nie ma miejsca montażu.

## Czego ten pakiet nie robi

Nie czyta amunicji, nie kotwiczy płyty, nie zmienia `HudStereoPair`, kina, dłoni, cieni, dubbingu, logo, kursora, dźwięku, projektora ani instalacji Steam. Wynik jest offline. Nie jest przejściem na PSVR2.
"""
    handoff = """# Handoff MAIN — assety HUD-u Katorga

SIDE, gałąź `fix/psvr2-hud-design`. Bez merge i bez Steam. To jest paczka grafiki, nie renderer.

## Gotowe

- Płyty SVG z pustymi polami danych: `assets/hud/katorga/plates/`.
- Stany i podglądy, w tym mały rozmiar, jasne i ciemne tło, obie ręce, wersja bez koloru: `states/` i `previews/`.
- Atlas i `atlas.json`: `assets/hud/katorga/atlas/`.
- Opis tokenów i budżet: `docs/hud/KATORGA_ASSETS.md`.

## Do późniejszej integracji, niezweryfikowane

1. Etap 1 z `HUD_STAGE1.md` nadal jest pierwszy: cenzus, potem wcięcie istniejącego zdrowia i amunicji. Te płyty nie zastępują etapu 1.
2. Quad na kontrolerze jest osobno. OBJ `frame/plate-frame.obj` nie ma gniazda. Nie parentować go do siatki broni ani TMD.
3. Podmiana promptu: w zdaniu zostaje polski tekst, a w miejsce `[token]` wchodzi komórka atlasu. Nie dodawać drugiej legendy. Nie używać `prompt_roles.h` (krzyżak i R1) ani tabeli „Pad N”.
4. `[health]` to kółko, prawa ścieżka `b/click`, maska `kCircleHealthMask`. `[use]` i `[reload]` to jeden prawy chwyt. `[age]` to kwadrat na lewym `x/click`. `[cancel]` to kółko tylko w menu (`g_actUiBack` wysyła `XI_B`). W rozgrywce to samo kółko wysyła zdrowie.
5. Lewa ręka celująca odbija ścieżki przy restarcie. Zdrowie wpada wtedy na `left/y`, gdzie stoi menu. Tego nie rysować jako gotowej ikony.

Test gogli należy do MAIN i ma osobny skrypt w `HUD_STAGE1.md`. Ten pakiet go nie zalicza.
"""
    return assets, handoff


def write_all() -> dict:
    icons = _icons()
    plates = OUT / "plates"
    states = OUT / "states"
    icon_dir = OUT / "icons"
    atlas = OUT / "atlas"
    previews = OUT / "previews"
    frame = OUT / "frame"
    for folder in (plates, states, icon_dir, atlas, previews, frame):
        folder.mkdir(parents=True, exist_ok=True)

    (plates / "health.svg").write_text(health_svg("template"), encoding="utf-8")
    (plates / "ammo.svg").write_text(ammo_svg("template"), encoding="utf-8")
    (plates / "weapon-name.svg").write_text(weapon_name_svg(), encoding="utf-8")
    (states / "normal.svg").write_text(
        _pair("state-normal", "Spokoj", health_svg("full"), ammo_svg("template"), 180),
        encoding="utf-8",
    )
    (states / "low-health.svg").write_text(health_svg("low"), encoding="utf-8")
    (states / "empty-mag.svg").write_text(ammo_svg("empty"), encoding="utf-8")
    (states / "weapon-switch.svg").write_text(
        _pair("state-switch", "Zmiana broni", ammo_svg("switch"), weapon_name_svg(), 180),
        encoding="utf-8",
    )
    (states / "sheet.svg").write_text(_state_sheet_svg(), encoding="utf-8")
    (previews / "hands-floor.svg").write_text(hands_svg(), encoding="utf-8")
    (previews / "prompts.svg").write_text(prompt_sheet_svg(icons), encoding="utf-8")

    for name, body in icons.items():
        svg = _svg_open(CELL, CELL, name, name) + body + "\n</svg>\n"
        (icon_dir / f"{name}.svg").write_text(svg, encoding="utf-8")
    (atlas / "katorga-hud-atlas.svg").write_text(atlas_svg(icons), encoding="utf-8")

    atlas_image = _atlas_png()
    atlas_png = atlas / "katorga-hud-atlas.png"
    _write_png(atlas_png, atlas_image)

    dark = (18, 20, 22, 255)
    light = (217, 211, 199, 255)
    _write_png(previews / "states-dark.png", _sheet(dark, False))
    _write_png(previews / "states-light.png", _sheet(light, False))
    _write_png(previews / "states-gray.png", _gray(_sheet(dark, False)))
    _write_png(previews / "small-dark.png", _sheet(dark, True))
    _write_png(previews / "small-light.png", _sheet(light, True))
    _write_png(previews / "hands-floor.png", _hands_png(dark))
    _write_png(previews / "hands-floor-light.png", _hands_png(light))
    _write_png(previews / "prompts.png", _prompts_png())
    (frame / "plate-frame.obj").write_text(_obj(), encoding="utf-8")

    png_sha = _sha256(atlas_png)
    manifest = {
        "name": "katorga-hud",
        "width": ATLAS_W,
        "height": ATLAS_H,
        "cell": CELL,
        "origin": "top-left",
        "format": "RGBA8",
        "bytes_rgba": ATLAS_W * ATLAS_H * 4,
        "png_sha256": png_sha,
        "license": "MIT",
        "origin_note": "Original drawings for this mod. Not extracted from Singularity, Kenney, or a platform button sheet.",
        "rects": [
            {"id": name, "x": x, "y": y, "w": CELL, "h": CELL}
            for name, x, y in RECTS
        ],
        "reserved": [
            {"x": x, "y": 128, "w": CELL, "h": CELL} for x in (384, 448)
        ],
        "weapon_wheel_icons": [
            "wheel-current",
            "weapon-centurion",
            "weapon-valkyrie",
            "weapon-spikeshot",
            "weapon-kasimov",
            "wheel-slot-empty",
        ],
        "tokens": _token_table(),
    }
    (atlas / "atlas.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    assets_doc, handoff = _docs(manifest)
    (ROOT / "docs" / "hud" / "KATORGA_ASSETS.md").write_text(assets_doc, encoding="utf-8")
    (ROOT / "docs" / "handoffs" / "main-katorga-hud-assets.md").write_text(handoff, encoding="utf-8")
    return manifest


def main() -> None:
    manifest = write_all()
    print(f"atlas {manifest['width']}x{manifest['height']} sha256={manifest['png_sha256']}")


if __name__ == "__main__":
    main()
