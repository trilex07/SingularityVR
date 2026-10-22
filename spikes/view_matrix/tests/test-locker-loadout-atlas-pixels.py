"""Offline content check of the actual installed HUD atlas (never writes to Steam)."""
import json
import hashlib
import sys
from pathlib import Path

from PIL import Image


def main() -> None:
    atlas_json, atlas_png = map(Path, sys.argv[1:3])
    meta = json.loads(atlas_json.read_text(encoding="utf-8-sig"))
    assert hashlib.sha256(atlas_png.read_bytes()).hexdigest().upper() == meta["png_sha256"].upper()
    im = Image.open(atlas_png).convert("RGBA")
    assert im.size == (meta["width"], meta["height"])
    rects = {r["id"]: r for r in meta["rects"]}
    sprites = ("gun-centurion", "gun-valkyrie", "gun-spikeshot", "gun-kasimov")
    content = set()
    for name in sprites:
        r = rects[name]
        assert r["x"] >= 0 and r["y"] >= 0
        assert r["x"] + r["w"] <= im.width and r["y"] + r["h"] <= im.height
        tile = im.crop((r["x"], r["y"], r["x"] + r["w"], r["y"] + r["h"]))
        alpha = tile.getchannel("A")
        visible = sum(v > 0 for v in alpha.get_flattened_data())
        assert visible > 100, (name, visible)
        content.add(tile.tobytes())
        print(f"{name}: visible_pixels={visible}, rect=({r['x']},{r['y']},{r['w']},{r['h']})")
    assert len(content) == 4
    print("four distinct nontransparent atlas sprites PASS")


if __name__ == "__main__":
    main()
