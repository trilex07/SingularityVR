# Regression recovery — 2026-10-12

Hands candidate **frozen** at DLL SHA-256 `2FE5011C20DFEAAEDFFFFE58CD39E92A9570867F344649B00C0038AD97B39A47`
(`726e1a7` / branch `feature/regression-recovery`). No support-grip or glove palette changes in this pass.

## P1 — Polish localization

**Cause (confirmed):** Installed `SingularityVR.ini` had `PolishLanguage=0`. Runtime loads manifests but
`PolishLocalization::IsPolishEnabled()` gates ProcessEvent FString rewrite and voice redirect — log showed
`Polish localization initialized (PolishLanguage=0, Fallback=1)`.

**Not Steam language:** Game stays on English selection; mod toggle is `[Localization] PolishLanguage=1`.

**Installed assets (Steam `Binaries`, pre-fix):**

| Asset | SHA-256 (installed) | Notes |
|---|---|---|
| `vr-assets/localization/pl/polish_text.jsonl` | `D70BED5D0EE7D214CB133E2AB712F021401FA465FEA78EE4A73110303298FDBB` | Differs from `install_polish.ps1` catalog hash; still present |
| `polish_voice.jsonl` | (see deploy `-Action Status`) | Present |
| `vr-assets/audio/polish/*.wav` | — | **1886** files |
| `Coalesced_INT.bin` | — | Present under `RvGame/CookedPC` |
| `SingularityVfx.dll` | — | **Missing** (optional VFX plugin; not required for jsonl/WAV polish path) |

**Restoration:** `tools/deploy-singularityvr.ps1 -EnsurePolishLanguage` sets `PolishLanguage=1` via
`WritePrivateProfileString` with read-back verify; backs up INI when `-KeepIni`/`-DllOnly` would otherwise skip it.
Post-deploy report includes `PolishAssets` hashes.

**L3 evidence:** After restart, expect log lines `polish localize:` / `polish voice:` (bounded) and Polish UI strings in gameplay — **PSVR2 confirms dialogue/subtitles**.

## P2 — Black materials (hypothesis + A/B)

**Hypothesis:** Texture injector (`dinput8.dll` + `texture-override/*.dds`, variant **B** deployed earlier) causes bad overrides → black surfaces. Hands deploys used `-DllOnly`, leaving injector active.

**A/B (same DLL, same save):** `-TextureInjector Disable` renames `dinput8.dll` → `dinput8.dll.disabled-by-vr` (backup in deployment timestamp folder). Re-enable with `-TextureInjector Enable`.

`PauseMenuDiag` is logging-only; not used as a rendering variable for this A/B.

**Remaining:** Visual confirmation in Steam gameplay — **PSVR2** or MAIN windowed inspection (not stereo acceptance).

## P3 — Pause menu (evidence only — no new HUD fix)

From in-game pause (`PauseMenuDiag=1`, class 3):

- **Compose (offscreen diegetic):** `pause-evidence compose: class=3 target=offscreen … compose=0 c5 sx=1 ty=0` — single full-width GFx draw into scene RT (SBS columns in one texture).
- **Compose (backbuffer path):** `compose=1`, `canvasOrtho=1` per-eye canvas/scissor.
- **Submit:** `pause-evidence submit: class=3 dup=1 stereo=0 menuFlat=0 cropEyes=0 backbuffer=5264x2684` — `MenuSubmitForcesFlat(3)==false` (see `menu_submit_policy.h`); each eye keeps its **half** of the SBS backbuffer → matches “Cele right eye only / other text clipped left”.
- **`menu-mono trace`:** alternates offscreen `compose=0` and backbuffer `compose=1`.
- **`pause-evidence sbs-columns`:** one-shot sample may be absent if pause frame not hit; capture on next headset pause.

**Blocker for fix:** Confirm whether Objectives column is already SBS-encoded in the offscreen RT before stereo scene submit, vs lost at XR half-rect submit. No viewport/c5/scissor experiments until that stage is proven.

**Fix (implemented on `fix/pause-menu-stereo`):** `[Render] PauseHeadMenu=1` (default on) redirects class-3 orthographic GFx batches from the scene-sized diegetic target into a pooled **1280×720** RT, then reuses the existing captured-menu provider (`CaptureVrMenuTarget` + `DrawVrMenuPointer` head-plane quad + suppression). See `docs/audits/pause-menu-head-overlay.md`. L3 log markers: `PauseHeadMenu: pooled GFx target`, `VR menu sink: private GFx candidate 1280x720`, `pauseMenuProvided` counter — **PSVR2 confirms full pause UI in both eyes**.

## P4 — Hands

**No code changes.** PSVR2 accepted behavior preserved on same DLL SHA.
