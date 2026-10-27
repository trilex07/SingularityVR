# Real mesh preview vs target look

## What is trustworthy here

| Asset | Source | Use |
| --- | --- | --- |
| `glove_left.obj` / `glove_right.obj` | `ch_inview_noTMD.psk`, mat0, arm verts removed | Bind-space glove-only geometry for MAIN |
| `inview_glove_d.png` | Game export | Diffuse UV layout for gloves |
| `pistol.obj` | `ch_inview_pistol_mesh.psk` gun tris | In-view weapon mesh (needs socket align in runtime) |
| `handgun_d.png` | Game export | Pistol diffuse |

## What is NOT a product mockup

The `0*-*.jpg` renders from `tools/hud/render_real_glove_hud.py` are a **debug orthographic-ish rasterizer**. They prove mesh + texture paths; they do **not** match in-game lighting, socket attach, or VR perspective.

**Approved visual target** for discussions and PSVR2 intent:

- `C:\Users\Trilex\.cursor\projects\c-Users-Trilex-vr-mods\assets\katorga-target-*.jpg` (generated from the walka reference + glove texture)
- Earlier `katorga-hud-walka.jpg` / `katorga-hud-zmiana-broni.jpg` (concept; add glove-only in integration notes)

## Następny krok SIDE (3D pack)

Pełny brief: `docs/handoffs/side-hud-3d-attach-pack.md`  
Szkielet output: `assets/hud/katorga/3d/` (glTF + `attach_schema.json` + Blender previews).

## Integration (MAIN)

1. Gloves: existing noTMD draw + `GloveIsolate` (no arms).
2. Weapon: parent in-view mesh to `cog_pistol` / controller, not the debug transform in the Python preview.
3. HUD plates: camera-facing quads on controller/wrist sockets, Katorga atlas.
4. Weapon wheel: separate transparent ring mesh, full circle in view space, not a cropped annulus.
