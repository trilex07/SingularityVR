"""Offline raster of the patched TypekaMix notes font.

The note field measured in SPMenus is character 279: font id 278 (WFontNote),
400 twips, word wrap, multiline, fixed bounds of 3960 by 3960 twips. Glyph
units are DefineFont3 units (em square 20480). Previews are enlarged so the
same wrap width can be inspected without the game.
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "title"))
sys.path.insert(0, str(Path(__file__).resolve().parent))
from PIL import Image, ImageDraw  # noqa: E402
from swf_shape import contour_bbox, decode_shape, flatten_points, split_contours  # noqa: E402
from swf_title_patch import split_tags  # noqa: E402
from typeka_polish import EM_SQUARE, SPECS, parse_define_font3  # noqa: E402
from ue3_layout import inspect  # noqa: E402

FIELD_TWIPS = 3960
FONT_HEIGHT = 400
FIELD_LEADING = 40
POLISH = "ĄĆĘŁŃÓŚŹŻąćęłńóśźż"
PANGRAM = "Zażółć gęślą jaźń"
INK = (236, 232, 220)
PAPER = (24, 26, 28)
MARGIN = (150, 48, 48)


def movie_from_package(package: bytes) -> bytes:
    info = inspect("preview", package, include_all=True)
    export = info["exports"][0]
    raw = next(prop for prop in export["properties"] if prop["name"] == "RawData")
    value = raw["tag_offset"] + 24
    size = raw["value"]["bytes"]
    return package[value + 4 : value + size]


def typeka_from_package(package: bytes) -> dict:
    movie = movie_from_package(package)
    _, tags = split_tags(movie)
    for tag in tags:
        payload = movie[tag["pay_off"] : tag["pay_off"] + tag["pay_len"]]
        if tag["code"] == 75 and b"TypekaMix" in payload[:40]:
            return parse_define_font3(payload)
    raise ValueError("patched movie has no TypekaMix")


def advance_twips(advance: int) -> float:
    return advance * FONT_HEIGHT / EM_SQUARE


def wrap_note(text: str, font: dict, width: float = FIELD_TWIPS) -> list[list[str]]:
    """Wrap on the note field's width. Newlines start paragraphs. Tokens stay intact."""
    by_code = {code: advance for code, advance in zip(font["codes"], font["advances"])}
    lines = []
    for paragraph in text.split("\n"):
        if paragraph == "":
            lines.append([])
            continue
        current = ""
        current_width = 0.0
        for word in paragraph.split(" "):
            token = word if current == "" else " " + word
            extra = sum(advance_twips(by_code[ord(char)]) for char in token)
            if current and current_width + extra > width:
                lines.append(list(current))
                current = word
                current_width = sum(advance_twips(by_code[ord(char)]) for char in word)
            else:
                current += token
                current_width += extra
        lines.append(list(current))
    return lines


def line_width(chars: list[str], font: dict) -> float:
    by_code = {code: advance for code, advance in zip(font["codes"], font["advances"])}
    return sum(advance_twips(by_code[ord(char)]) for char in chars)


def _contours(font: dict, char: str):
    slot = font["codes"].index(ord(char))
    return split_contours(decode_shape(font["shapes"][slot]))


def _draw_contours(draw: ImageDraw.ImageDraw, contours, origin, scale: float, paper) -> None:
    prepared = []
    for contour in contours:
        points = flatten_points(contour, 6)
        if len(points) < 3:
            continue
        box = contour_bbox(contour)
        prepared.append((points, box, (box[2] - box[0]) * (box[3] - box[1])))

    def inside(inner, outer) -> bool:
        return (
            inner[0] > outer[0]
            and inner[1] > outer[1]
            and inner[2] < outer[2]
            and inner[3] < outer[3]
        )

    holes = []
    outers = []
    for index, item in enumerate(prepared):
        contained = any(
            other_index != index and inside(item[1], other[1]) and other[2] > item[2]
            for other_index, other in enumerate(prepared)
        )
        (holes if contained else outers).append(item[0])

    def pixels(points):
        return [(origin[0] + x * scale, origin[1] + y * scale) for x, y in points]

    for points in outers:
        draw.polygon(pixels(points), fill=INK)
    for points in holes:
        draw.polygon(pixels(points), fill=paper)


def write_glyph_sheet(font: dict, path: Path) -> None:
    chars = list(POLISH)
    cell = 150
    columns = 9
    rows = 2
    image = Image.new("RGB", (columns * cell, rows * cell + 28), PAPER)
    draw = ImageDraw.Draw(image)
    draw.text((8, 4), "TypekaMix Polish slots after the user-package patch", fill=(180, 170, 140))
    for index, char in enumerate(chars):
        left = (index % columns) * cell
        top = 28 + (index // columns) * cell
        draw.text((left + 8, top + 4), char, fill=(190, 170, 90))
        contours = _contours(font, char)
        points = [point for contour in contours for point in flatten_points(contour, 4)]
        if not points:
            continue
        minx = min(point[0] for point in points)
        maxx = max(point[0] for point in points)
        miny = min(point[1] for point in points)
        maxy = max(point[1] for point in points)
        scale = (cell - 46) / max(maxx - minx, maxy - miny, 1)
        _draw_contours(draw, contours, (left + 16 - minx * scale, top + 30 - miny * scale), scale, PAPER)
    image.save(path)


def _paint_text(font: dict, lines: list[list[str]], path: Path, title: str) -> None:
    zoom = 2.5
    scale = (FONT_HEIGHT / EM_SQUARE) * zoom
    width = int(FIELD_TWIPS * zoom) + 40
    line_units = font["ascent"] + font["descent"] + font["leading"] + FIELD_LEADING * EM_SQUARE / FONT_HEIGHT
    line_px = line_units * scale
    height = int(36 + max(1, len(lines)) * line_px + 20)
    image = Image.new("RGB", (width, height), PAPER)
    draw = ImageDraw.Draw(image)
    draw.text((12, 8), title, fill=(180, 170, 140))
    margin = 20
    limit = margin + FIELD_TWIPS * zoom
    draw.line((limit, 32, limit, height - 8), fill=MARGIN)
    by_code = {code: index for index, code in enumerate(font["codes"])}
    for row, chars in enumerate(lines):
        baseline = 36 + row * line_px + font["ascent"] * scale
        cursor = float(margin)
        for char in chars:
            slot = by_code[ord(char)]
            _draw_contours(
                draw,
                split_contours(decode_shape(font["shapes"][slot])),
                (cursor, baseline),
                scale,
                PAPER,
            )
            cursor += font["advances"][slot] * scale
    image.save(path)


def sample_note() -> str:
    return (
        "Zażółć gęślą jaźń\n"
        "\n"
        "[use] Notatka\n"
        "\n"
        "Śmierć doktora Demiczewa w pożarze miała znacznie gorsze skutki, "
        "niż ktokolwiek się spodziewał. Liczyliśmy, że doktor Barisow będzie "
        "mógł kontynuować nasze badania... ale właśnie otrzymaliśmy wiadomość, "
        "że Moskwa uznała naszą pracę za zbyt niebezpieczną.\n"
        "\n"
        "Dalsze badania nad E-99 zostają wstrzymane, a cały personel ma opuścić wyspę."
    )


def write_previews(package: bytes, dest: Path) -> list[str]:
    font = typeka_from_package(package)
    dest.parent.mkdir(parents=True, exist_ok=True)
    stem = dest.with_suffix("")
    glyphs = stem.parent / f"{stem.name}-glyphs.png"
    pangram = stem.parent / f"{stem.name}-pangram.png"
    note = stem.parent / f"{stem.name}-note.png"
    write_glyph_sheet(font, glyphs)
    _paint_text(font, wrap_note(PANGRAM + " / ĄĆĘŁŃÓŚŹŻ ąćęłńóśźż", font), pangram, "pangram at the note field width")
    _paint_text(font, wrap_note(sample_note(), font), note, "long note, paragraphs, [use] token")
    return [str(glyphs), str(pangram), str(note)]
