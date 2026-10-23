# Singularity VR — UI & Utility Graphics Design System (v2.1 Production)

## 1. Aesthetic Vision: Abandoned Soviet Research Facility

The visual language of Singularity VR bridges 1950s Soviet scientific-industrial architecture on Katorga-12 with decaying, crude Element-99 (E99) temporal technology. The interface rejects generic modern cyberpunk neon, floating holographic panels, and flat digital dashboard conventions in favor of **tactile laboratory brutalism, vacuum tube luminescence, weathered dark steel, graphite, ash, aged ivory enamel, and worn brass**.

The core design metaphor represents:
> *A heavy laboratory control console and military terminal manufactured in a Soviet research facility in 1955, abandoned for decades inside Katorga-12, then modified with crude experimental E99 instrumentation.*

---

## 2. Material Hierarchy & Color Calibration (v2.1 Proportions)

The visual space is strictly calibrated to the **70 / 15 / 8 / 5 / 2** material balance:

| Component Role | Target Share | Hex Code | RGB | Material Characteristic |
|---|---|---|---|---|
| **Deep Graphite Chassis** | **70%** | `#1C2024` | `(28, 32, 36)` | Base chassis metal, switch wells, dark faceplates |
| **Weathered Gray Steel** | **70%** | `#24292E` | `(36, 41, 46)` | Main console faceplates, rotary wheel sectors, plates |
| **Cold-Rolled Steel Plate**| **70%** | `#343B42` | `(52, 59, 66)` | Elevated panels, bezels, mechanical push-buttons |
| **Bare Steel Chipped Edge**| **70%** | `#6C7882` | `(108, 120, 130)`| Scratched highlights, worn corners revealing bare metal |
| **Concrete & Ash Undertone**| **70%** | `#202428` | `(32, 36, 40)` | Dark background environment, soot, dirty crevices |
| **Aged Ivory Typography** | **15%** | `#DDD6C0` | `(222, 214, 192)`| High-contrast stenciled lettering (Polish & English UI) |
| **Bright Ivory Enamel** | **15%** | `#F4EEDE` | `(244, 238, 222)`| Active headers, selected row text, calibration marks |
| **Secondary Olive Accent** | **8%** | `#3C4234` | `(60, 66, 52)` | Faded inspection tags, clipboard headers, backing bars |
| **Muted Olive Tag** | **8%** | `#2E3428` | `(46, 52, 40)` | Secondary equipment borders and dark accents |
| **Weathered Brass Hardware**| **5%** | `#8C8166` | `(140, 129, 102)`| Milled knobs, screws, slider handles, dial needles |
| **Oxidized Crevice Rust** | **5%** | `#884E26` | `(136, 78, 38)` | Realistic seam oxidation, patina around screw heads |
| **Selector Amber Light** | **5%** | `#D97E26` | `(217, 126, 38)` | Vacuum-tube amber backlight for selected rows & sectors |
| **Bright Amber Bulb** | **5%** | `#F8A83E` | `(248, 168, 62)` | Active switch indicator lamps, high-priority warnings |
| **Restrained E99 Cyan** | **2%** | `#41AFAA` | `(65, 175, 170)` | Active Element-99 core filaments only (strictly isolated) |
| **E99 Core Filament** | **2%** | `#82F0EB` | `(130, 240, 235)`| Focused energy core luminescence (strictly restrained) |

---

## 3. Bilingual Language & Typography Policy

- **Full Bilingual Support: Polish (`pl`) & English (`en`).**
  - Every interactive button, header, option row, status pill, and controller prompt is mapped across both languages in [`ui_localization_pl_en.json`](file:///C:/Users/Trilex/vr-mods/SingularityVR-ui-utility-remaster/ui_localization_pl_en.json).
  - Language-neutral visual plates are combined with dynamic font rendering or localized texture atlases (`ui_atlas_localized_pl.png` vs `ui_atlas_localized_en.png`).
  - Examples (PL / EN):
    - `WZNOWIENIE GRY` / `RESUME GAME`
    - `METODA CELOWANIA` / `AIM METHOD`
    - `WYBÓR BRONI` / `WEAPON SELECT`
    - `AR9 WALKIRIA` / `AR9 VALKYRIE`
    - `[ZWOLNIJ R3] WYBIERZ` / `[RELEASE R3] SELECT`
- **Authentic Cyrillic Markings:**
  - Cyrillic is reserved strictly for environmental and manufacturing markings on equipment nameplates and serial stamps.
  - Examples: `ГОСТ 1955 // КАТОРГА-12`, `ЗАВ. № 1955 // СЕРИЯ 4-С`.
- **Typeface:** `DIN 1451` / `Bahnschrift` (Mittelschrift & Engschrift).
  - Authentic Cold War European/Soviet engineering standard.
  - High x-height, sturdy uniform stems (never <3px at 1080p equivalent) to prevent optical blur or distortion across VR headset lenses.
  - Full support for Polish diacritics: `ą`, `ć`, `ę`, `ł`, `ń`, `ó`, `ś`, `ź`, `ż`, `Ą`, `Ć`, `Ę`, `Ł`, `Ń`, `Ó`, `Ś`, `Ź`, `Ż`.

---

## 4. Mechanical Surface & Weathering Rules

1. **Weathered Steel Dominance:** Panels and dial faces are cold-rolled steel, oxidized iron, and graphite. Military olive green is strictly a secondary accent (stamped tags, clipboard headers, inspection bands).
2. **Physical Credibility:** All panels feature mechanical mounting hardware (hex bolts, slotted brass screws, flathead chassis fasteners).
3. **Edge Wear & Bare Metal Highlights:** Edges show subtle chipping and scratches where the surface finish has worn away to reveal bare steel underneath (`#6C7882`).
4. **Corner Rust:** Rust stains and dirt accumulate realistically in corners, recessed grooves, and around screw heads without obscuring legible UI text.
5. **No Digital Neon Borders:** Borders are physical milled bevels or engraved metal seams, never electric neon glow strokes.
6. **Restrained E99 Phosphor:** Cyan is treated as an exotic, dangerous element—present only as a controlled core filament or active status indicator (occupying ≤ 2% of visual surface).
