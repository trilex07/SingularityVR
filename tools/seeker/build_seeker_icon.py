"""Render an original-geometry Seeker icon and patch only its private atlas cell.

The 1024 source illustration and 64px atlas cell are generated from the
verified PSK and original diffuse UVs. No synthetic gun silhouette is used.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageEnhance, ImageFilter

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'reload'))
from prepare_weapon_animation import read_psk

CELL = (256, 128, 64, 64)
SOURCE_ATLAS_SHA = 'e77799b6c52182d3330e5dbd726b1c762349d54e25a1213e0708ce7450214e33'
APPROVED_ART_SHA = 'c27f32c3022d284288b7a14695738f5ce22c9b2eedaf61d9bff5bb0961cf6efe'
WEAPON_BONES = ('cog_e99_steerable', 'Trigger', 'clip_release', 'clip',
                'bullet_slide', 'bullet_1', 'bullet_2', 'bullet_3',
                'clip_cover', 'bullet_loader', 'hood')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def render(psk_path, diffuse_path):
    names, _, _, _, points, weights, wedges, faces = read_psk(psk_path)
    if len(wedges) != 12444 or len(faces) != 14996 or names[53] != 'cog_e99_steerable':
        raise ValueError('unreviewed Seeker mesh')
    selected = {names.index(n) for n in WEAPON_BONES}
    diffuse = Image.open(diffuse_path).convert('RGB')
    if diffuse.size not in ((2048, 1024), (4096, 2048)):
        raise ValueError('unreviewed Seeker diffuse')
    texture = np.asarray(diffuse)
    xyz = np.asarray(points, dtype=np.float64)*[1, -1, 1]
    chosen = [f for f in faces if all(weights[wedges[k][0]][0][1] in selected for k in f[:3])]
    if len(chosen) != 14996:
        raise ValueError('gun triangles missing')
    used = np.array([xyz[wedges[k][0]] for f in chosen for k in f[:3]])
    lo, hi = used.min(0), used.max(0)
    # Native muzzle is +Z; rear magazine projects below the butt at low Z.
    scale = min(880/(hi[2]-lo[2]), 470/(hi[1]-lo[1]))
    def project(v):
        return (int(512+(v[2]-(hi[2]+lo[2])/2)*scale),
                int(500-(v[1]-(hi[1]+lo[1])/2)*scale))
    canvas = Image.new('RGBA', (1024, 1024), (0, 0, 0, 0))
    draw = ImageDraw.Draw(canvas)
    # Painter depth: native +X seen by viewer. Texture sample is source UV,
    # adjusted by simple normal lighting for legible 64px downsampling.
    ordered = sorted(chosen, key=lambda f: sum(xyz[wedges[k][0]][0] for k in f[:3])/3)
    for face in ordered:
        rows = [wedges[k] for k in face[:3]]
        vertices = [xyz[r[0]] for r in rows]
        uv = np.array([[r[1], r[2]] for r in rows]).mean(0)
        tx = int(np.clip(uv[0], 0, 1)*(diffuse.width-1))
        ty = int(np.clip(uv[1], 0, 1)*(diffuse.height-1))
        rgb = texture[ty, tx].astype(float)
        normal = np.cross(vertices[1]-vertices[0], vertices[2]-vertices[0])
        length = np.linalg.norm(normal)
        if length < 1e-8:
            continue
        normal /= length
        light = .58+.28*abs(normal[0])+.14*max(0, normal[1])
        rgb = np.clip(rgb*light*1.7+10, 0, 220).astype(int)
        draw.polygon([project(v) for v in vertices], fill=tuple(rgb)+(255,))
    alpha = canvas.getchannel('A')
    outline = alpha.filter(ImageFilter.MaxFilter(7))
    edge = Image.new('RGBA', canvas.size, (164, 152, 119, 0))
    edge.putalpha(Image.fromarray(np.clip(np.asarray(outline, dtype=np.int16)-
                                          np.asarray(alpha, dtype=np.int16), 0, 180).astype('uint8')))
    edge.alpha_composite(canvas)
    bounds = edge.getbbox()
    if not bounds or bounds[0] < 24 or bounds[2] > 1000:
        raise ValueError('icon clipped')
    return edge


def package(psk, diffuse, atlas_png, atlas_json, output, art_master=None):
    output.mkdir(parents=True, exist_ok=False)
    if art_master:
        if sha(art_master.read_bytes()) != APPROVED_ART_SHA:
            raise ValueError('unreviewed Seeker illustration')
        source_art = Image.open(art_master).convert('RGBA')
        bounds = source_art.getchannel('A').point(lambda v: 255 if v > 32 else 0).getbbox()
        if not bounds:
            raise ValueError('empty illustration')
        crop = source_art.crop(bounds)
        size = (900, round(crop.height*900/crop.width))
        master = Image.new('RGBA', (1024,1024), (0,0,0,0))
        master.alpha_composite(crop.resize(size, Image.Resampling.LANCZOS),
                               ((1024-size[0])//2,(1024-size[1])//2))
    else:
        master = render(psk, diffuse)
    master.save(output/'weapon_icon_seeker_master.png')
    # The wheel samples this at 64px. Lift shaded metal and retain a single
    # cool, high-contrast contour; no heavy outer glow or background disc.
    lifted = ImageEnhance.Brightness(master).enhance(1.55)
    mask = master.getchannel('A')
    ring = mask.filter(ImageFilter.MaxFilter(25))
    outline_alpha = Image.fromarray(np.clip(np.asarray(ring, dtype=np.int16)-
                                            np.asarray(mask, dtype=np.int16), 0, 145).astype('uint8'))
    outlined = Image.new('RGBA', master.size, (161, 198, 202, 0))
    outlined.putalpha(outline_alpha)
    outlined.alpha_composite(lifted)
    icon = outlined.resize((64, 64), Image.Resampling.LANCZOS)
    icon.save(output/'weapon_icon_seeker.png')
    source = atlas_png.read_bytes()
    if sha(source) != SOURCE_ATLAS_SHA:
        raise ValueError('unreviewed atlas')
    atlas = Image.open(atlas_png).convert('RGBA')
    data = json.loads(atlas_json.read_text(encoding='utf-8-sig'))
    rows = [r for r in data['rects'] if r['id'] == 'dev-seeker']
    if atlas.size != (512, 256) or len(rows) != 1 or tuple(rows[0][k] for k in ('x','y','w','h')) != CELL:
        raise ValueError('atlas mapping drift')
    before = np.asarray(atlas).copy()
    atlas.paste(icon, CELL[:2])
    after = np.asarray(atlas)
    mask = np.ones((256, 512), bool)
    x,y,w,h = CELL
    mask[y:y+h,x:x+w] = False
    if not np.array_equal(before[mask], after[mask]):
        raise ValueError('atlas cells outside Seeker changed')
    target = output/'katorga-hud-atlas.png'
    atlas.save(target)
    data['png_sha256'] = sha(target.read_bytes()).upper()
    (output/'atlas.json').write_text(json.dumps(data, indent=2)+'\n', encoding='utf-8')
    report = {'psk_sha256': sha(psk.read_bytes()), 'diffuse_sha256': sha(diffuse.read_bytes()),
              'source_atlas_sha256': sha(source), 'atlas_sha256': data['png_sha256'],
              'icon_master_sha256': sha((output/'weapon_icon_seeker_master.png').read_bytes()),
              'cell': CELL, 'other_pixels_unchanged': True, 'source_geometry': 'original Seeker PSK',
              'art_source_sha256': sha(art_master.read_bytes()) if art_master else None}
    (output/'manifest.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    return report


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    for k in ('psk','diffuse','atlas-png','atlas-json','output'):
        ap.add_argument('--'+k, required=True, type=Path)
    ap.add_argument('--art-master', type=Path)
    a = ap.parse_args()
    print(json.dumps(package(a.psk,a.diffuse,a.atlas_png,a.atlas_json,a.output,a.art_master),indent=2))
