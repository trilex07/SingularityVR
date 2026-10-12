# Pause GFx native ownership investigation (2026-10-12)

**Steam baseline (unchanged):** DLL SHA-256 `2FE5011C20DFEAAEDFFFFE58CD39E92A9570867F344649B00C0038AD97B39A47` @ commit `726e1a7`.

**Scope:** Where `RvUIGFxMovie_Pause` gets its layout before D3D9 `HudStereoPair` remaps it. No draw-level capture, no Gate B redeploy.

External references (architecture only — no offsets copied):

| Project | Relevant idea |
|---------|----------------|
| [Dishonored-VR](https://github.com/VR-Stereo-Hub/Dishonored-VR) `ui_state.cpp`, `hud_owner.cpp`, `hud_capture.cpp` | Read-only UI state + **pre-draw** GFx display/publish hooks; private sink + quad delivery |
| [Dishonored-VR](https://github.com/VR-Stereo-Hub/Dishonored-VR) `mono_screen.cpp` | Full-frame capture only works when the **game already produced one mono image** |
| Starfield2VR (mutars) | Per-movie Scaleform `SetViewPort` — **not mapped** on Singularity yet |
| `research/cutscene-detection` @ `ad08eb4` | `CutsceneFlat` draws scene once on SBS RT, but OpenXR still submits **per-eye halves** unless `menuFlat`-style crop runs (`CUTSCENE_READINESS_20261012.md` §5) |

Supplied report `singularity_vr_ue3_gfx_stereo_menu_research_2026-10-12.md` was **not present in the repo** at investigation time; findings below are from this tree, Gate A log evidence, and `native_ui_input.h`.

---

## 1. Pause movie object and lifecycle (verified in mod)

| Item | Evidence |
|------|----------|
| Focus movie | Replicated **GetFocusMovie** body @ `Singularity.exe+0x00D7C2C0` (image base `0x00400000`): `engine = *(0x01CF8778)` → `record = *(engine+0x148)` → `movie = *(record+0x50)` |
| Class ID | `NameOf(movie)` + `NameOf(*(movie+0x34))`; pause menu class string **`RvUIGFxMovie_Pause`** → `g_vrMenuClass = 3` |
| Token | `g_vrMenuStateToken` bumps on every focus change (`generation<<32 \| kind`) |
| Input thread | `NativeVrUi::Poll` @ `0x011D31C0` (MinHook); viewport cursor vtable `0x011CFC10/40/70` |
| Pause detection | **`SetPause` is not used for menu class**; GFx focus is authoritative (`HISTORY.md`, `native_ui_input.h`) |

**Not verified:** native C++ **render** entry (`Display` / `Advance` / `RenderUI`) for `RvUIGFxMovie_Pause` — no `Singularity.exe` hook or signature scan completed in this pass.

---

## 2. Logical viewport, stage, and backbuffer (partial)

| Signal | Source | Observed / inferred |
|--------|--------|---------------------|
| **FViewport SizeX/Y** | `native_ui_input.h` `Coordinates()` reads `viewport+0x4C`, `+0x50` | Documented live **`5264×2684`** during VR; used for **cursor → GFx** mapping |
| **D3D backbuffer** | `g_srcW` / `g_srcH` from `EnsureCopyResources` | Same SBS dimensions as headset render target |
| **GFx ortho `c5`** | `pause_menu_space.h`, Gate A log | Canvas ortho: small diagonal scale (~`7.8e-5` band A, ~`1.3e-5` band B); **`tx` spans ~[-0.92,+0.43]** on **full-width** clip before per-eye remap |
| **Cursor UV** | `ui_pointer.h` | Addresses **logical GFx canvas** before inset/stereo remap |

**Conclusion:** Scaleform layout and input are driven against the **full SBS logical width** (5264), not a separate 16:9 mono viewport, unless an **unlocated** movie-player field overrides stage size internally.

Changing only D3D9 backbuffer dimensions does **not** change UE3 render resolution (`d3d9.cpp` comment @ run 14; ini ignores `[Render] ResX/ResY`).

---

## 3. Main pause UI vs Objectives (same producer)

Gate A (`docs/audits/pause-canvas-gate-a-20261012.md`, log 2026-10-12 13:09):

- One focused movie class: **`RvUIGFxMovie_Pause`** (`token …0003`).
- **One RT** per settled frame: `5264×2684` backbuffer `surf=0AB75A00`.
- **Band A** (~main pause text) and **Band B** (~Objectives) appear in the **same** closed frame transaction (`bandA>0 && bandB>0`), different `c5` scale bands, **not** separate focus movies.

`RvUIGFxMovie_Notepad` exists in the menu table but is **not** the Objectives tab at focus layer.

---

## 4. Render path before `HudStereoPair` (class 3)

Two native targets (from `d3d9.cpp` + `menu_mono_trace.h`):

| Target | `HudStereoPair` behaviour | Class 3 |
|--------|---------------------------|---------|
| **Offscreen** scene-sized RT | Single draw (`compose=0`); no per-eye canvas split | Used when `!g_rtIsBackbuffer` |
| **Backbuffer** | `PauseBackbufferCanvasRoute`: full VP, **per-eye** scissor + `c5` remap (`compose=1`) | **Gate A proved live pause draws here** |

Mod intercepts are **`d3d9.dll` user-pointer draws** (`HudStereoPair`), not `Singularity.exe` GFx dispatch.

**Stale doc note:** `docs/audits/menu-intro-rootcause.md` still describes class 2/3 as “diegetic only”; Gate A + `menu-mono trace` supersede that for **pause on backbuffer**.

---

## 5. Comparison to reference architectures

| Approach | Dishonored-VR | Singularity today |
|----------|---------------|-------------------|
| Ownership | Engine GFx **display/publish** hooks; element table | **None** — only D3D draw remap |
| Layout boundary | Before redirect to private RT | **After** GFx has built full SBS canvas |
| Presentation | OpenXR **quad** per element | Stereo SBS + optional head-plane provider |
| Input | Ray → logical coords | Same (`ui_pointer` + viewport SizeX/Y) |

A Dishonored-style fix requires finding an **equivalent pre-draw boundary** on UE3 `GFxMoviePlayer` / `RvUIGFxMovie_*`, not extending `HudStereoPair`.

Starfield2VR-style **per-movie viewport** is the closest match to “16:9 logical menu while world stays SBS”, but Singularity’s bridge is the **2010 thin GFxMovie API** (`HISTORY.md` Scaleform spike) — viewport hook target is **unknown**.

---

## 6. Bounded instrumentation (this branch)

Optional read-only probe (no engine writes):

```ini
[Diagnostic]
PauseGfxProbe=1
```

On first focus of `RvUIGFxMovie_Pause`, logs movie pointer, FViewport SizeX/Y, and `g_srcW×g_srcH`. Existing `menu-mono trace` (class 3) logs first `(target, WxH, compose)` tuple.

**No Steam deploy required** for the investigation commit; MAIN runs probe on a candidate build when pursuing RE.

---

## 7. Decision gate

### **NO-GO** — native viewport fix without further reverse engineering

**Reason:** There is **no verified, safe hook point** on `RvUIGFxMovie_Pause` (or `UGFxMoviePlayer`) that sets logical viewport / stage size **before** GFx emits the SBS canvas. All proven layout happens on a **5264-wide** coordinate system; post-draw fixes are exhausted.

**Missing evidence (blocks GO):**

1. **Function:** `Singularity.exe` symbol or signature for pause movie **render/display** (not GetFocusMovie).
2. **Fields:** Offsets for GFx **ViewWidth/ViewHeight** (or UE3 `FGFxViewPort`) on live `RvUIGFxMovie_Pause` instances — compare pause vs main menu vs IIS.
3. **Coupling:** Prove whether stage size is **independent** of `FViewport::SizeX` or always derived from it.
4. **Thread/lifetime:** Confirm render runs on game thread with same movie pointer as focus chain.
5. **Feasibility:** One logged frame showing **16:9 logical layout** (both main + Objectives) **without** `-ResX` relaunch — must come from engine-side viewport, not D3D scissors.

### Smallest **technically justified** next step (still investigation, not shipping)

1. **RE pass (Ghidra, GOG `Singularity.exe` + `GFxUI.u`):**  
   - Xref `RvUIGFxMovie_Pause`, `UGFxMoviePlayer`, `SetViewport`, `GetViewportSize`.  
   - Build **signature candidates** (no hardcoded addresses in committed code).

2. **One Steam log session** with `PauseGfxProbe=1` + existing `menu-mono trace` on **in-game pause** (not main menu only): confirm `target=backbuffer 5264x2684 compose=1` on accepted DLL.

3. **If (1) finds a viewport setter tied to movie player:** minimal **read-only** hook that logs `(oldW, oldH, newW, newH)` on pause open/close only — still **no writes** until dimensions decouple from SBS in logs.

4. **Only after (3) shows decoupling:** isolated PoC branch — set pause movie logical viewport to **`height × 16/9`** at open, restore on close, **LEGACY fallback** unchanged, **no** `HudStereoPair` / Gate B changes.

---

## 8. Explicit non-goals (this investigation)

- No Gate B / `MirrorBatch` / `D3DSBT_ALL` capture.
- No OpenXR-only cinema without mono producer.
- No glove, weapon, Polish, or texture changes.
- No merge; no PSVR2 PASS without headset run after any future PoC.
