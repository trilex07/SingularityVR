# Pause menu: native-backed VR overlay feasibility (2026-10-12)

**Context:** Native GFx movie-level hook is **NO-GO** (`research/pause-gfx-native-ownership` @ `8ae0502`). This pass evaluates replacing **visual presentation** only while keeping Raven’s pause **logic** alive.

**Steam baseline (unchanged):** DLL SHA-256 `2FE5011C20DFEAAEDFFFFE58CD39E92A9570867F344649B00C0038AD97B39A47` @ `726e1a7`. No deploy of experimental builds for this audit.

**Scope boundary:** No further GFx RE, Ghidra, D3D9 batch capture (Gate B), or `HudStereoPair` / c5 / scissor experiments. Code references are from the accepted tree’s `spikes/view_matrix/` sources.

---

## Executive decision

### **NO-GO** — full native-backed pause overlay that preserves Objectives, in-game Options, and navigation

**Single blocking capability:** There is **no implemented, verified way to read native GFx pause state** (focused row/tab, Objectives text/selection, or submenu contents). The mod only observes **which movie class is focused** (`NativeVrUi::UpdateMenu` → `g_vrMenuClass`, `g_vrMenuStateToken`). A mod-drawn panel cannot stay synchronized with the live game menu without either (a) **shadow UI state** (explicitly disallowed) or (b) **GFx `GetVariable` / `Invoke` readback** on the live `RvUIGFxMovie_Pause` instance (not present in the codebase; Scaleform spike stopped at chunked SWF access — `HISTORY.md`).

A **partial** overlay (Resume-only, fixed rows, blind UV forwarding) is technically conceivable but **violates the safety bar** in this task: *“Do not implement … an incomplete replacement that breaks Objectives, settings, or game navigation.”*

---

## Investigation answers (code-backed)

### 1. Can native pause logic stay active while we replace visuals?

| Aspect | Verdict | Evidence |
|--------|---------|----------|
| Keep `RvUIGFxMovie_Pause` running | **Yes** | Focus chain and input already use live movie; no need to disable native tick. |
| Hide native **pixels** only | **Partial** | **Capture + suppress** path (`CaptureVrMenuTarget`, `SuppressCapturedVrMenuDraw`) works for **small offscreen** GFx RTs (≈1280×720, class 2 main menu). `SuppressCapturedVrMenuDraw` requires **scene** RT (`fail \|= 16` when `!g_rtIsScene`). |
| Class 3 in-game pause (broken path) | **Poor fit** | Gate A / `HudStereoPair`: live pause GFx often draws on **5264×2684 backbuffer** with per-eye canvas remap — outside capture size gate (`640–2048` in `CaptureVrMenuTarget`). Head-quad overlay **cannot** reuse captured texture for class 3 today. |
| Cover-up without capture | **Possible in principle** | `DrawVrMenuPointer` already composites laser, synthetic VR Settings row, and optional captured quad **after** scene submission to the swapchain. Full-screen **ClearRects** + mod panel there could mask **final** image without editing `HudStereoPair`. Native GFx would still **run** (CPU/logic + wasted GPU). |

**Conclusion:** Logic can stay native; **reliable visual replacement for class 3** is not available via the proven capture/suppress pipeline. End-of-frame overlay masking is a different architecture (see §6).

---

### 2. Which actions can be safely forwarded to the original game?

| Action | Forwarding mechanism today | Overlay replacement |
|--------|---------------------------|---------------------|
| **Resume / navigate / select / back** | **Yes** — `NativeVrUi::Drain` synthesizes `WM_MOUSEMOVE`, mouse deltas, and left-button edges on the bound `FViewport` via engine vtable (`MoveFn` / `AxisFn` / `KeyFn`). | **Only if** mod UI maps each control to **fixed normalized UV** on the logical GFx canvas (`ui_pointer::MapCanvas`, same as cursor hook). Layout drift or localization breaks blind maps. |
| **Open in-game Options** | **Native click** at correct UV, or user uses game flow. Focus change to `RvUIGFxMovie_Options*` surfaces as **`g_vrMenuClass == 4`** (prefix match in `MenuClass()`). | Mod must **hand off** to native Options movie or duplicate entire Options UX — no readback of Options tree. |
| **Open Objectives tab** | Same as navigate — **click** at tab UV inside **same** `RvUIGFxMovie_Pause`. | No focus change when switching tabs → **cannot detect** Objectives vs main tab from `g_vrMenuClass` alone. |
| **VR Settings (mod)** | **Yes** — `PauseVrSettingsHitNormalized` + `OpenVrSettingsFromPause()`; consumes click before native GFx (`native_ui_input.h`). | Already shipped pattern; independent of pause layout fix. |
| **Gamepad while mod panel open** | Panel path **clears pad** to game (`g_panelOpen`); VR Settings auto-closes if focus leaves class 2/3. | Reusable pattern for a future pause overlay panel. |

---

### 3. Can we read the currently selected native menu item?

**No.** Nothing in `d3d9.cpp` / `native_ui_input.h` calls GFx **`GetVariable`**, **`Invoke`**, or parses ActionScript state. Selection feedback exists only in **rendered pixels** (broken in VR). Any mod-side `g_selectedRow` would be **shadow state** — unreliable and forbidden for this design.

---

### 4. Can submenu transitions be detected from existing focus/state?

**Partial — movie-level only.**

| Signal | Detects | Does not detect |
|--------|---------|-----------------|
| `g_vrMenuStateToken` bump | Any focus movie change | Tab within `RvUIGFxMovie_Pause` |
| `g_vrMenuClass` 1/2/3/4 | IIS / Main / Pause / other GFx (`Dialog`, `Options*`, stations, …) | Objectives vs Resume list on pause |
| `Log("VR UI focus: …")` | Object + class name on change | Current widget / index |
| `g_gamePaused` (`SetPause` hook) | World paused | Not identical to “pause menu visible” (`HISTORY.md`) |

---

### 5. Can Objectives text/selection come from native UE3/GFx state?

**No** in the current mod. Objectives live inside **`RvUIGFxMovie_Pause`** (one movie, two draw bands — Gate A / ownership audit). Without SWF/AS decompile or runtime **`Invoke`/`GetVariable`** on the live movie object, the mod cannot populate an overlay. Rebuilding from save/game data would duplicate Raven’s model — out of scope and high risk.

---

### 6. OpenXR: opaque black + VR panel without `HudStereoPair` / c5 / scissor changes?

| Approach | Touches | Masks broken pause? | Masks world? |
|----------|---------|---------------------|--------------|
| Extend **`DrawVrMenuPointer`** (existing overlay pass) | Post-scene D3D compositing (same class as VR Settings panel) | **Can** cover swapchain after native draws if ordered last | Full-screen black rects per eye — **yes** if drawn each frame |
| **`SuppressCapturedVrMenuDraw`** | SetTexture hook | Only registered **menu-source** textures on **scene** RT; not class-3 backbuffer path | No |
| OpenXR **`XrCompositionLayerProjection`** only (`d3d9.cpp` ~10936) | Submit path | Reprojects whatever is already in the swapchain — **does not remove** broken GFx unless swapchain cleared earlier | Optional if upload buffer cleared before copy |
| New OpenXR **quad layer** | OpenXR session | Theoretically yes | Yes — but needs **content** (texture + layout) |

**Strict reading of “no D3D9 draw hooks”:** Do not add pause experiments to **`HudStereoPair`**, capture mirrors, or c5/scissor remaps. Using the **existing** overlay draw path (`DrawVrMenuPointer` / `DrawVrPanel`) is the same infrastructure already used for VR Settings — acceptable for a **future** PoC, not authorized by this NO-GO.

**Rollback:** Feature flag off → current behavior; no Steam deploy required for this document.

---

### 7. Laser / UV input reuse vs native fighting?

**Yes, with known rules.**

- **Shared ray:** `UpdateVrUiPointer` → `g_vrUiU/V` → `NativeVrUi::Coordinates` → synthetic mouse (`native_ui_input.h`).
- **VR Settings row:** Mod hit-test **before** native click (`PauseVrSettingsHitNormalized`).
- **Panel open:** `g_panelOpen` blocks native mouse leak and gamepad (`Drain` early out; pad cleared in XR input).
- **Conflict risk:** A mod-drawn pause list must either **replace** native hit regions entirely (needs correct UV map) or **show native** capture texture — class 3 capture unavailable.

---

## Feasibility matrix

| Area | Status | Supported today | Missing / blocked |
|------|--------|---------------|-------------------|
| **Basic pause menu (visual)** | **NO-GO** | Focus detect; laser → native mouse | No capture for class-3 backbuffer; no selection readback |
| **Basic pause (input/logic)** | **GO** | Full native routing if user sees UI | N/A when UI invisible |
| **Options / submenus** | **NO-GO** | Detect **movie** change to Options* (class 4) | Cannot render/sync Options tree; must keep native GFx visible or reimplement |
| **Objectives** | **NO-GO** | Forward clicks if user guesses UV | No tab detect; no text/state export |
| **Input / navigation** | **Conditional GO** | Native mouse synthesis; panel pad isolation | Mod overlay needs 1:1 layout map or shadow state |
| **Rendering isolation** | **Conditional** | End-frame overlay black + mod quads (unbuilt) | Cannot use proven capture/suppress for class 3; world masking needs full-screen overlay each frame |
| **Rollback** | **GO** | INI flag pattern (`MenuOverlay`, `HideFpInMenu`, panel) | — |

---

## Smallest implementation (if scope were cut — not approved)

Not recommended given the NO-GO above. For documentation only:

```ini
[Render]
PauseNativeOverlay=0   ; default OFF
```

- Gate on `g_vrMenuClass == 3 && g_vrUiMenu`.
- In **`DrawVrMenuPointer` only:** per-eye full-screen dark clear + minimal mod labels; forward confirm via existing `Drain` after mapping row → **fixed UV table** (fragile).
- **Explicitly out of scope:** Objectives, in-game Options pages, Load/Save confirmation flows.
- **Do not ship** without headset proof and without resolving Objectives/settings — conflicts with task safety rules.

---

## References (in-repo)

| Mechanism | Location |
|-----------|----------|
| Pause focus / class | `native_ui_input.h` — `UpdateMenu`, `g_vrMenuClass` |
| Synthetic mouse | `native_ui_input.h` — `Drain`, `Coordinates` |
| VR Settings row | `pause_vr_settings.h`, `OpenVrSettingsFromPause` in `d3d9.cpp` |
| Head-plane overlay / laser | `vr_menu_overlay.h` — `UpdateVrUiPointer`, `DrawVrMenuPointer` |
| Menu capture / suppress | `vr_menu_overlay.h`, `SuppressCapturedVrMenuDraw` in `d3d9.cpp` |
| Class 3 HUD path | `d3d9.cpp` — `HudStereoPair` pause backbuffer canvas route |
| Native ownership NO-GO | `docs/audits/pause-gfx-native-ownership-20261012.md` |

---

## Next step (if product direction changes)

The only path that avoids shadow UI **and** supports Objectives is **bounded runtime readback** on the live pause movie (`GFxMovie` **`GetVariable` / `Invoke`** — names exist in decompressed `GFxUI.u`, no caller in mod). That is a **small, explicit** engineering spike (read-only logs first), not open-ended exe/Ghidra work. Until that exists, remain on **visual fix attempts within native rendering** or accept **flatscreen-style pause outside VR** — both outside this overlay NO-GO.
