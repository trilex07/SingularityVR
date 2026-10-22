"""Count actual 5x7 glyph runs needed by the WeaponLocker title."""
import re
from pathlib import Path


source = (Path(__file__).parents[1] / "d3d9.cpp").read_text(encoding="utf-8")
font = source.split("static const unsigned char kFont", 1)[1].split("static int GlyphIndex", 1)[0]
glyphs = {
    match.group(2): [int(x, 16) for x in re.findall(r"0x[0-9A-Fa-f]+", match.group(1))]
    for match in re.finditer(r"\{([^{}]+)\},\s*//\s*([A-Z])\b", font)
}


def runs(rows):
    return sum(
        bool(row & (16 >> i)) and not (i and row & (16 >> (i - 1)))
        for row in rows for i in range(5)
    )


title = "VR LOADOUT"
required = sum(runs(glyphs[ch]) for ch in title if ch != " ")
loadout_draw = source.split("static void DrawLockerVrLoadout(IDirect3DDevice9* dev) {", 1)[1]
capacity = int(re.search(r"D3DRECT title\[(\d+)\]", loadout_draw).group(1))
assert required == 100, required
assert capacity >= required, (capacity, required)
print(f"title={title!r} glyph_runs={required} capacity={capacity} PASS")
