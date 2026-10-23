# Singularity VR — UI Utility Graphics Implementation Guide for MAIN

**Package:** Singularity VR UI & Utility Graphics Remaster (V2.1 Production)  
**Branch:** `side/ui-utility-graphics-remaster`  
**Approved Art Direction:** Soviet Laboratory Brutalism / Weathered Gray Steel & Graphite (70/15/8/5/2)  
**Target Headset:** PSVR2 / SteamVR / OpenXR  
**Author:** Senior Game UI / UX Artist & Technical UI Prototype Designer  

---

## 1. Executive Architecture: Language-Neutral Graphics + Dynamic Text

To achieve maximum performance and prevent VRAM bloat on PSVR2, all interface geometry in V2.1 is strictly separated into:
1. **Language-Neutral Visual Plates (`ui_atlas_neutral.png`):** Heavy gray steel faceplates, recessed toggle wells, slider scales, button push-plates, outer rotary dials, and stenciled icons. These contain **zero baked language text**.
2. **Localization Map ([`ui_localization_pl_en.json`](file:///C:/Users/Trilex/vr-mods/SingularityVR-ui-utility-remaster/ui_localization_pl_en.json)):** Contains stable keys, official Polish and English text, intended UI elements, and recommended bounding boxes with alignment and font sizes.
3. **Baked Localized Fallback Atlases (`ui_atlas_localized_pl.png` & `ui_atlas_localized_en.png`):** Pre-rendered texture variants for headless or fixed-pipeline D3D9 overlays that cannot render dynamic glyphs at runtime.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       MAIN RUNTIME MEMORY PIPELINE                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  [ui_atlas_neutral.png] (4096x2048, 32 MiB)  <-- LOAD ONCE INTO VRAM        │
│  ├── Console plates, terminal slates, rotary sectors (deduplicated)         │
│  ├── Toggle wells/levers, calibration sliders, focus frames, reticles       │
│  ├── Zero-bleed 8px gutters (pad=4px) around every packed sprite            │
│  └── All action icons, status symbols, and PSVR2 button glyphs              │
│                                                                             │
│  [LANGUAGE SWITCHING & LOCALIZED TEXT PIPELINE: 50% VRAM SAVINGS]           │
│  ├── Method A (Recommended): D3D9 Dynamic Font / Canvas Rendering          │
│  │   MAIN renders neutral plate quad from ui_atlas_neutral,                 │
│  │   then draws font text using string from ui_localization_pl_en.json.     │
│  │   Active VRAM: 32 MiB total. 0 bytes extra VRAM on language switch.      │
│  │                                                                          │
│  └── Method B: Two-Quad Multi-Pass Texture Overlay                          │
│      Pass 1: Draw Neutral Plate Quad (UVs from ui_atlas_neutral.json)       │
│      Pass 2: Draw Localized Text Quad (UVs from ui_atlas_localized_{pl,en}) │
│      Active VRAM: 32 MiB (neutral) + 8 MiB (active language) = 40 MiB.      │
│      (Down from 80 MiB in unoptimized layout — 50% memory reduction).       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Directory Structure & Production Assets

All assets are exported under [`assets/processed/ui_remaster/`](file:///C:/Users/Trilex/vr-mods/SingularityVR-ui-utility-remaster/assets/processed/ui_remaster/):

```
assets/processed/ui_remaster/
├── atlas/
│   ├── ui_atlas_neutral.png        (4096x2048 Master language-neutral texture sheet, 32 MiB RGBA8)
│   ├── ui_atlas_neutral.json       (UV mapping coordinates [u0, v0, u1, v1], pixel rects, aliases)
│   ├── ui_atlas_localized_pl.png   (2048x1024 Polish baked text atlas, 8 MiB RGBA8)
│   ├── ui_atlas_localized_pl.json  (Polish UV mapping descriptors, 36 sprites)
│   ├── ui_atlas_localized_en.png   (2048x1024 English baked text atlas, 8 MiB RGBA8)
│   └── ui_atlas_localized_en.json  (English UV mapping descriptors, 36 sprites)
├── weapon_wheel/                   (Wheel chassis, 4 sectors x 5 states, needle, weapon icons)
├── vr_settings/                    (Console plate, tabs, toggle wells/levers, slider track/knob)
├── pause_menu/                     (Terminal slate, header plates, push buttons, modal dialogs)
├── interaction_icons/              (Action stencils, status markers, PSVR2 buttons, prompt pills)
├── laser_pointer/                  (Optical reticle dot, hover ring, click shockwave, focus brackets)
└── utility_hud/                    (Ammo counter bracket, vitals frame, E99 tube, compass, hazard)
```

Vector source files are located in [`assets/source/ui_remaster/`](file:///C:/Users/Trilex/vr-mods/SingularityVR-ui-utility-remaster/assets/source/ui_remaster/).

---

## 3. Localization Integration (`ui_localization_pl_en.json`)

MAIN should parse [`ui_localization_pl_en.json`](file:///C:/Users/Trilex/vr-mods/SingularityVR-ui-utility-remaster/ui_localization_pl_en.json) during engine initialization into a fast key-value lookup:

```cpp
struct LocalizationBounds {
    int maxWidthPx;
    int maxHeightPx;
    int fontSizePt;
    std::string textAlign; // "left", "center", "right"
    bool lineWrap;
};

struct LocalizationEntry {
    std::string pl;
    std::string en;
    std::string uiElement;
    LocalizationBounds bounds;
};

// Global lookup map in C++
std::unordered_map<std::string, LocalizationEntry> g_uiLocalization;

inline const char* LocText(const std::string& key, int languageId) {
    auto it = g_uiLocalization.find(key);
    if (it == g_uiLocalization.end()) return key.c_str();
    return (languageId == 1) ? it->second.pl.c_str() : it->second.en.c_str();
}
```

### Verified Polish Diacritics:
All Polish strings have been validated for native font rendering:
- `ą`, `ć`, `ę`, `ł`, `ń`, `ó`, `ś`, `ź`, `ż`
- `Ą`, `Ć`, `Ę`, `Ł`, `Ń`, `Ó`, `Ś`, `Ź`, `Ż`
- Use typeface `Bahnschrift` or `DIN 1451 Mittelschrift` with UTF-8 character encoding.

---

## 4. D3D9 Vertex Buffer & Quad Rendering Pipeline

When rendering sprites from `ui_atlas_neutral.png` or localized atlases:

### 4.1 Vertex Structure
```cpp
struct UiVertex {
    float x, y, z, rhw; // Transformed screen coordinates
    DWORD color;        // Diffuse color / tint (e.g. 0xFFFFFFFF)
    float u, v;         // Normalized texture coordinates
};

#define D3DFVF_UIVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)
```

### 4.2 Drawing an Atlas Quad
Given `uv_rect = [u0, v0, u1, v1]` from the atlas JSON:
```cpp
void DrawAtlasSprite(IDirect3DDevice9* dev, float x, float y, float w, float h,
                     float u0, float v0, float u1, float v1, DWORD color = 0xFFFFFFFF) {
    UiVertex verts[4] = {
        { x,     y,     0.0f, 1.0f, color, u0, v0 },
        { x + w, y,     0.0f, 1.0f, color, u1, v0 },
        { x,     y + h, 0.0f, 1.0f, color, u0, v1 },
        { x + w, y + h, 0.0f, 1.0f, color, u1, v1 }
    };
    dev->SetFVF(D3DFVF_UIVERTEX);
    dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(UiVertex));
}
```

### 4.3 D3D9 Render States
```cpp
// Transparency and blending
dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
dev->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

// Bilinear filtering for smooth VR projection
dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
```

### 4.4 Two-Quad Multi-Pass Rendering Architecture (For Baked Localized Sprites)
> [!IMPORTANT]
> **Independent UV Spaces Between Atlases:**
> Because `ui_atlas_neutral` (plates/frames) and `ui_atlas_localized_{lang}` (text labels) have independent UV coordinates, **do not attempt to bind both textures to a single quad using multi-texturing unless your vertex stream provides distinct `TEXCOORD0` and `TEXCOORD1` channels.**
> 
> The standard and robust rendering pattern in D3D9 / UE3 is **Two-Pass (Two-Quad) Compositing**:
> 1. **Pass 1 (Background Plate):** Bind `ui_atlas_neutral.png` to Stage 0. Draw the button/panel quad using UVs from `ui_atlas_neutral.json`.
> 2. **Pass 2 (Localized Label):** Bind `ui_atlas_localized_{lang}.png` to Stage 0. Draw the text overlay quad centered over the plate using UVs from `ui_atlas_localized_{lang}.json` with `D3DBLEND_SRCALPHA / D3DBLEND_INVSRCALPHA`.
> 
> Alternatively, **Method A (Dynamic Canvas/Font)** only draws Pass 1 from `ui_atlas_neutral`, then renders the localized string directly via UE3 Canvas / Font engine using `ui_localization_pl_en.json`.

### 4.5 Bilinear Gutters & Zero Edge Bleeding (QA Verified)
- In D3D9, bilinear texture filtering interpolates neighboring texels. In tight atlas packing, this causes neighboring sprites to bleed into transparent edges when projected onto stereoscopic VR quads.
- All atlases in V2.1 enforce a **strict 8px transparent gutter** (`pad = 4px` margin around every packed sprite).
- Bit-exact roundtrip extraction tests verify that every sprite extracts with `max_diff == 0` and zero adjacent texel contamination.

---

## 5. Screen Layout & Component Mapping

### 5.1 Weapon Wheel (Center: Screen 960, 560)
- **Outer Housing:** `wheel_chassis_outer.png` (800x800).
- **Four Sectors:**
  - `wheel_sector_top_*` (Sector 1: **Centurion .44**)
  - `wheel_sector_right_*` (Sector 2: **AR9 Walkiria / Valkyrie**)
  - `wheel_sector_bottom_*` (Sector 3: **Spikeshot**)
  - `wheel_sector_left_*` (Sector 4: **Kasimov SNV-E99**)
- **States:** `idle`, `hover`, `selected` (warm amber glow), `held` (bright steel), `disabled`.
- **Center Selector Knob:** `wheel_center_hub_neutral.png` (260x260) with `wheel_pointer_needle.png` rotated by sector angle.
- **Telemetry Card:** Positioned at (1420, 450), 420x220px. Displays caliber, magazine status, and fire mode.

### 5.2 VR Settings Console (1200x840 at Screen 360, 120)
- **Chassis Plate:** `settings_console_plate_neutral.png`.
- **Tabs (Top Row):** `tab_frame_idle_neutral.png` / `tab_frame_active_neutral.png` (140x36px).
- **Row Focus Frame:** `row_focus_amber_frame.png` (1120x48px). Rendered when a row is hovered or laser-pointed.
- **Toggle Controls:** `toggle_well_neutral.png` (110x44) + `toggle_lever_off.png` / `toggle_lever_on.png` + `indicator_lamp_on.png`.
- **Slider Controls:** `slider_track_recessed.png` (360x32) + `slider_knob_brass.png` (36x44) positioned proportionally to the setting value.

### 5.3 Pause Menu Operations Terminal (680x840 at Screen 620, 120)
- **Terminal Slate:** `terminal_slate_neutral.png`.
- **Header Plate:** `header_plate_neutral.png` (560x90). Displays authentic Katorga-12 seal and localized title.
- **Buttons (480x54px):** `btn_plate_idle_neutral.png`, `btn_plate_hover_neutral.png`, `btn_plate_pressed_neutral.png`.
- **Dedicated VR Settings Row:** `btn_plate_special_amber_idle_neutral.png` / `btn_plate_special_amber_hover_neutral.png` (accented brass frame).

### 5.4 Confirmation Modal Dialogs (640x380 at Screen 640, 320)
- **Chassis Plate:** `modal_dialog_plate_neutral.png`.
- **Action Buttons:** `btn_confirm_pl.png` (R2) and `btn_cancel_pl.png` (Circle).

### 5.5 Interaction Prompts & Utility Icons
- **Prompt Pills:** 320x52px rounded steel plate with stenciled icon and key prompt.
- **PSVR2 Buttons:** 64x64px high-contrast icons for Cross, Circle, Triangle, Square, L1, L2, R1, R2, Left Stick, Right Stick, and Options.

### 5.6 Collimated Optical Laser Pointer
- **Reticle Dot:** 32x32px collimated red-amber focal dot (`pointer_reticle_dot.png`).
- **Hover Ring:** 64x64px micrometer ring (`pointer_hover_ring.png`) active over interactive controls.
- **Click Feedback Ping:** 96x96px expanding amber shockwave (`pointer_click_ping.png`) triggered on R2 click for 6–8 frames.

---

## 6. Language Switching Runtime Instructions

To switch language between Polish and English without restarting the game or stalling the GPU:

```cpp
void SetInterfaceLanguage(int langId) { // 0 = EN, 1 = PL
    g_currentLanguage = langId;
    
    // If using Method A (Dynamic font rendering over neutral plates):
    // No texture reload is needed! Next frame DrawVrPanel() simply queries:
    // LocText("SETTINGS_PAGE_ROOT", g_currentLanguage);
    
    // If using Method B (Secondary baked atlas texture swap):
    const char* atlasFile = (langId == 1) 
        ? "vr-assets/ui_atlas_localized_pl.png" 
        : "vr-assets/ui_atlas_localized_en.png";
    dev->SetTexture(1, g_pLocalizedAtlasTextures[langId]);
}
```

This guarantees **zero allocation stalls** during active VR gameplay.
