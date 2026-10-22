# Singularity VR — UI Utility Graphics Implementation Guide for MAIN

This guide details how the MAIN engineering agent can integrate, load, slice, and scale the remastered UI assets created in `side/ui-utility-graphics-remaster`.

---

## 1. Asset Classification: What Is What

The assets produced in this package fall into three distinct operational tiers:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ 1. Direct Drop-In Replacements                                              │
│    - vr_prompt_atlas_remastered.bmp / .png (replaces vr_prompt_atlas.bmp)   │
│    - Laser pointer reticle dots & feedback sprites                         │
│    - Synthetic Pause / Main Menu "VR SETTINGS" button                       │
├─────────────────────────────────────────────────────────────────────────────┤
│ 2. Modular & Sliced 9-Slice Assets                                          │
│    - Panel corners (panel_corner_tl, tr, bl, br) & borders (border_h, v)    │
│    - Option row backgrounds (row_bg_default, hover, selected)               │
│    - Toggle switch states & Slider track/knob sprites                       │
│    - Scroll track, thumb, and chevron arrows                                │
├─────────────────────────────────────────────────────────────────────────────┤
│ 3. Presentation Mockups & Design Concepts                                   │
│    - Full screen 1920x1080 concept mockups (reference targets only)         │
│    - Not loaded as single textures at runtime                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Drop-In Replacements

### 2.1 Remastered VR Prompt Atlas
- **Current Runtime Path:** `vr-assets\vr_prompt_atlas.bmp` (referenced in `spikes/view_matrix/vr_menu_overlay.h:22`).
- **Remastered Asset:** `assets/processed/ui_remaster/interaction_icons/vr_prompt_atlas_remastered.bmp`
  - Dimensions: `512x128` (32bpp ARGB format, exactly matching `LoadVrPromptAtlas` bitmap requirements).
  - UV Mapping Descriptor: `assets/processed/ui_remaster/interaction_icons/vr_prompt_atlas_remastered.json`
- **Mapping Comparison:**
  - *Vanilla Kenney Atlas:* 5 icons at 320x64 (R2, Circle, Cross, R1, Options).
  - *Remastered Atlas:* 16 icons at 512x128 (8 columns x 2 rows, 64x64 each).
  - Row 0: `R2`, `R1`, `Cross`, `Circle`, `L2`, `L1`, `Square`, `Triangle`.
  - Row 1: `Interact`, `Pickup`, `TMD`, `Reload`, `Melee`, `Locked`, `R3`, `L3`.
- **Integration Steps for MAIN:**
  1. Copy `vr_prompt_atlas_remastered.bmp` to `vr-assets/vr_prompt_atlas.bmp`.
  2. In `spikes/view_matrix/prompt_atlas_generated.h` (or corresponding header), update:
     - `kVrPromptAtlasWidth = 512;`
     - `kVrPromptAtlasHeight = 128;`
     - `kVrPromptIconPx = 64;`
     - `kVrPromptIconCount = 16;`
  3. Load UV coordinates directly from `vr_prompt_atlas_remastered.json`.

### 2.2 Synthetic "VR SETTINGS" Button in Pause / Main Menu
- **Current Runtime Behavior:** `vr_menu_overlay.h:247-252` draws a raw D3D clear box using:
  ```cpp
  ClearRects(dev, &box, 1, hovered ? 0xD0507040 : 0xB0081218);
  ClearRects(dev, text, n, hovered ? 0xFFFFFFFF : 0xFF50FF90);
  ```
- **Remastered Replacement:**
  - `btn_vr_settings_default.png` (Default state)
  - `btn_vr_settings_hover.png` (Hover state)
  - `btn_vr_settings_pressed.png` (Pressed state)
- **Integration Recommendation:**
  Instead of primitive `ClearRects` bitmap font draws, load these textures during menu initialization and render via two textured quads (left and right eyes) using `DrawVrPromptIcon`-style `D3DPT_TRIANGLESTRIP` UP.

### 2.3 Laser Pointer Feedback Kit
- **Current Runtime Behavior:** `vr_menu_overlay.h:186` draws a single 1-pixel flat line from hand to hit point.
- **Remastered Replacement:**
  - `laser_dot_idle_64px.png` and `laser_dot_active_64px.png`: Render as a textured billboard sprite at `uipointer::Hit hit.world`.
  - `laser_hover_ring.png`: Draw when `valid && isHoveredInteractive`.
  - `laser_click_burst.png`: Flash for 6 to 8 frames upon `XI_A` or right trigger pull confirmation.

---

## 3. Modular Assets: Slicing & Scaling Rules

### 3.1 9-Slice Panel Framing
- **Assets:**
  - `panel_corner_tl.png`, `tr`, `bl`, `br` (128x128 px)
  - `panel_border_h.png` (128x32 px)
  - `panel_border_v.png` (32x128 px)
  - Central plate fill: Flat color `rgba(12, 17, 24, 0.94)`
- **Slicing Geometry:**
  - Corners remain fixed at 1:1 scale (or scaled with eye resolution).
  - Horizontal borders tile or stretch along X.
  - Vertical borders tile or stretch along Y.

### 3.2 Option Row Backgrounds
- **Assets:**
  - `row_bg_default.png` (800x56)
  - `row_bg_hover.png` (800x56)
  - `row_bg_selected.png` (800x56)
  - `row_bg_disabled.png` (800x56)
- **Scaling Method:** 3-slice horizontal (left cap 16px, center 1px stretch, right cap 16px).

### 3.3 Interactive Widgets
- **Toggle Switches:** `toggle_switch_off.png` / `toggle_switch_on.png` (120x48 px, fixed aspect ratio).
- **Sliders:**
  - Base: `slider_track_empty.png` (320x32 px).
  - Fill: `slider_track_fill.png` (UV-clipped along X according to `(value - min) / (max - min)`).
  - Thumb: `slider_knob_normal.png` / `slider_knob_hover.png` (32x48 px, centered at thumb position).

---

## 4. D3D9 Render State & Blending Guidance

When rendering these UI utility textures in D3D9 via `DrawPrimitiveUP`:

```cpp
// Set Alpha Blending for clean anti-aliased edges
dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

// Bilinear texture filtering to avoid shimmering under head motion
dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

// Clamp addressing to prevent wrap bleeding on 9-slice borders
dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
```

---

## 5. Scope Boundaries Reminder

This package provides visual designs, source vectors, and production raster assets.
- **Do not** modify weapon wheel logic (`SectorForStick`, etc.).
- **Do not** change native GFx hooking or scaleform bytecode.
- **Do not** attempt to fix the native pause render-target bug with these graphics; pause graphics are designed for the head-relative projected overlay.
