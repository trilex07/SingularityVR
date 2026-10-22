# Singularity VR — UI & Utility Graphics Design System

## 1. Aesthetic Vision & Direction: Dark Soviet Retrofuturism

The visual language of Singularity VR bridges 1950s Soviet scientific-industrial architecture on Katorga-12 with advanced, decaying Element-99 (E99) temporal technology. The interface eschews generic, overly sterile modern sci-fi neon aesthetics and flat smartphone conventions in favor of **tactile industrial brutalism, vacuum tube luminescence, stamped cold-rolled steel, and high-contrast cathode phosphor**.

In Virtual Reality (specifically on the PlayStation VR2 headset and high-resolution SteamVR headsets), the visual system prioritizes **optical legibility, depth perception, edge stability, and anti-aliasing resilience** at 2684× SBS / native render targets.

---

## 2. Color Palette Specification

The palette is engineered with distinct luminance tiers to maintain high contrast against both bright outdoor environments (snowstorms, water explosions) and dark underground facility interiors.

| Color Role | Hex Code | RGB | Luminance | Usage Description |
|---|---|---|---|---|
| **Primary Iron/Slate (Base)** | `#0B0F15` | `(11, 15, 21)` | 1.8% | Deepest backdrop plate, modal scrims, recessed bezels |
| **Stamped Steel (Surface)** | `#10161E` | `(16, 22, 30)` | 2.5% | Panel faces, unhovered cards, structural frames |
| **Milled Chamfer (Edge)** | `#415566` | `(65, 85, 102)` | 10.5% | Mechanical outlines, hex bolts, border dividers |
| **E99 Phosphor Cyan (Active)** | `#2CE8E4` | `(44, 232, 228)` | 63.8% | Primary interactive highlights, active sectors, laser dot |
| **Supercharged Cyan (Core)** | `#78FFFA` | `(120, 255, 250)` | 85.2% | Reticle center, high-luminance text, selected state |
| **Ambient Cyan Phosphor** | `#146E78` | `(20, 110, 120)` | 15.3% | Inner card glows, soft hover wash, subtle backlights |
| **Vacuum Tube Amber (Gauge)** | `#FFA927` | `(255, 169, 39)` | 51.2% | Dials, secondary warnings, waypoints, locked buttons |
| **Industrial Yellow (Hazard)**| `#F5C323` | `(245, 195, 35)` | 60.1% | 45-degree caution stripes, classified headers |
| **Security Red (Alarm)** | `#EB2D37` | `(235, 45, 55)` | 22.4% | System lock, restricted sectors, disabled marker |
| **Readable Off-White (Text)** | `#EBF4FA` | `(235, 244, 250)` | 90.5% | Primary readable typography, labels, numerical values |
| **Technical Grey (Dim)** | `#7D919E` | `(125, 145, 158)` | 28.2% | Sub-labels, serial numbers, metadata annotations |

---

## 3. In-Headset Typography Recommendations

### Recommended Typefaces
1. **Primary Technical & Stencil Typeface:** `DIN 1451` / `Bahnschrift` (Mittelschrift & Engschrift).
   - Authentic Cold War European/Soviet engineering standard.
   - Uniform stroke width avoids thin stem dropouts across Fresnel and Pancake lenses.
   - High x-height ensures immediate recognition at distances between 0.8m and 2.5m in virtual space.
2. **Monospace & Serial Markings:** `Consolas` / `Liberation Mono`.
   - Used for calibration indexes, memory offsets, and Cyrillic/Latin serial stamps (`ГОСТ 1955 // ОБЪЕКТ-99`).

### Headset Legibility Rules
- **Minimum Stroke Thickness:** No line or glyph stem thinner than **3.0px** at 1080p backbuffer equivalent, or **6.0px** on high-res UI textures.
- **Letter Spacing (Tracking):** Generous positive tracking (+8% to +15% letter-spacing) prevents optical character fusion in VR peripheral zones.
- **Angular Resolution Budget:** UI text must subtend at least **32 to 45 arcminutes** for comfortable, strain-free reading without leaning forward.
- **Bilingual Stencils:** All primary headers present dual Russian/English designations (`ПАРАМЕТРЫ VR // VR SETTINGS`), honoring the setting while ensuring instant clarity.

---

## 4. Panel Spacing & 9-Slice Geometry

All rectangular UI frames and dialog boxes are designed with modular 9-slice construction:
- **Corner Insets:** 32px to 64px fixed-radius corners featuring reinforced chamfers and hex bolts.
- **Border Slices:** 16px to 32px vertical and horizontal repeat slices with subtle linear grooves.
- **Central Backing:** Scalable semi-transparent dark steel plate (`rgba(16, 22, 30, 0.92)`).
- **Safe Area Padding:** Minimum **24px internal padding** between outer metallic borders and text/widget elements to prevent optical clipping at headset peripheral bounds.

---

## 5. Interactive State Matrix

| State | Background Fill | Border Treatment | Indicator & Glyphs | Laser Response |
|---|---|---|---|---|
| **Default / Idle** | `rgba(16, 22, 30, 0.85)` | `2px #415566` (Milled steel) | `#EBF4FA` (Off-white) | Dormant / Subtle |
| **Hover / Focused** | `rgba(20, 50, 68, 0.92)` | `3px #2CE8E4` + Corner Pips | `#78FFFA` (Glow Cyan) | Targeting ring expands + dot bloom |
| **Selected / Active**| `rgba(18, 70, 90, 0.98)` | `4px #78FFFA` + Amber accent | `#FFFFFF` + Amber notch | Confirmation wavefront burst |
| **Disabled / Locked**| `rgba(12, 15, 20, 0.55)` | `1px #283440` (Dark iron) | `#556978` + Diagonal hatch | Slashed red marker |

---

## 6. Icon Stroke & Fill Architecture

- **Geometry Construction:** Heavy geometric silhouettes with minimum **12% to 16% stroke-to-radius ratio**.
- **Perimeter Backings:** All prompt glyphs include optional circular dark iron backing pads (`rgba(14, 20, 28, 0.86)`) to ensure 100% contrast against bright backgrounds (snow, sky, electrical arcs).
- **Tactile Details:** Recessed notches, gear cogs, and knurled grips provide physical mechanical credibility.
