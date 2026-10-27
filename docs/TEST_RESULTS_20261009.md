# SingularityVR test results — 2026-10-09

```
HEAD=c421375+working-tree
DLL_SHA=3A4F88AE25A73EA61524DAFBD8D2E12E01C0293590FCEA5607D14425D64FBC7E
Frame=17/17 desktop regression tests + glove/finger + VFX ABI
Eye=shared palette path; stereo restoration covered by tests
WorldScale=80
CurrentPass=foreground arms/gloves/fingers seam
MenuState=not applicable during palette tests
```

## Deployment

- Branch: `fix/vr-menu-input-grip-performance`
- Injector: texture-upscale variant **B**, 80 DDS targets present in Steam and kept intact.
- Steam `d3d9.dll`: SHA-256 `3A4F88AE25A73EA61524DAFBD8D2E12E01C0293590FCEA5607D14425D64FBC7E`.
- PE identity: PE32 x86 (`Machine=0x014C`, `OptionalHeader=0x010B`).
- `SingularityVfx.dll`: installed in `Binaries`, SHA-256 `120E6C7534D2F3B29FE7DBD4D5464CDE179F14BD5A891608793D61A3ABCCF456`.
- VFX atlas: 4 DDS assets present.

The plugin is intentionally installed. Without it the host reports `plugin=absent, items=0`; the valid xrsim log now reports `plugin=loaded`, `drawer=ok`, and `atlas=vfx_atlas dir`.

## VR startup race fix

The log showed a startup race: `InitXR()` was previously attempted once, before SteamVR/PSVR2 had exposed the HMD, which permanently left the game in flat mode. The current DLL installs hooks once and retries OpenXR initialization every second when the failed attempt left no instance; an incomplete `xrGetSystem` instance is destroyed before retry. `StartInVr=1` remains enabled.

The new Steam deployment was verified with PE32 x86 and the hash above. The previous runtime log already demonstrated `OpenXR ready`, `session running`, and `FIRST FRAME SUBMITTED TO THE HEADSET`; the new retry path covers the case where those events occur later than the first D3D frame.

### Fresh SteamVR runtime check (2026-10-09 16:17-16:18)

The currently running Steam launch reached the complete VR path again: `OpenXR ready`, `BACKSPACE: VR mode ON`, `session running`, and `*** FIRST FRAME SUBMITTED TO THE HEADSET ***` are present in `C:\Users\Trilex\AppData\Local\SingularityVR\view_matrix.log`. SteamVR also logged PSVR2 HMD tracking during the same session. This confirms the installed DLL is entering VR; a headset-side black/flat view would therefore be a SteamVR/PSVR2 presentation or standby issue, rather than the game failing to initialize OpenXR.

### Runtime contamination found and cleared (2026-10-09 16:23-16:24)

The flat two-eye window was caused by the long-lived Steam process inheriting `XR_RUNTIME_JSON` from the desktop XRSIM test. `xrsim-scratch-run/state.json` identified the running game as `runtime=dvr-xrsim`, `system=Meta Quest 3`, with 921 simulated frames. Steam and SteamVR were stopped and restarted from a clean environment. The next Steam launch was registered by SteamVR as `VRApplication_OpenXRInstance` and then `VRApplication_OpenXRScene` for `steam.app.42670`; SteamVR loaded and remapped the PSVR2 Sense bindings. The stale XRSIM state did not change. This is the required clean launch path for the physical PSVR2 test.

## Regression and simulation tests

- Production desktop suite: **17/17 PASS**.
- VFX ABI round-trip: **PASS**.
- VFX muzzle/smoke lifetime and stereo world-draw tests: **PASS**.
- Canonical replay: **PASS**, 727 batches, 80.641 s, 8 fire events, 2 melee events, 1 recenter, 6 UI clicks.
- Replay final state: both hands valid and independent of HMD (`handL valid=true followsHead=false`, `handR valid=true followsHead=false`).
- The legacy `vfx_host_test.exe` that expects the plugin to be absent now returns FAIL by design; it is an obsolete negative test, not a deployment failure.

## Shadows

The previous live run had no classified shadow-layout draws (`shadow=1` was absent) while `ShadowMode=3` was selected. I changed the installed and candidate INI to `ShadowMode=1`, which keeps the shadow receiver mono and prevents per-eye remapping of the light-space sample. The run still records shadow resources with zero allocation failures, but a fresh complete mode-1 render log was not produced by the headless launcher. Therefore the optical result is **not yet confirmed**: the next headset check must verify that the left eye has a normal shadow and the right eye has no rectangle/seam.

The current Steam INI is `ShadowMode=1`; the latest startup log records `ShadowMode=1` and `plugin=loaded` at `C:\Users\Trilex\AppData\Local\SingularityVR\view_matrix.log`. It did not reach a complete replay/render window, so the optical verdict remains headset-pending.

Evidence log for the complete plugin/VFX replay: `C:\Users\Trilex\AppData\Local\SingularityVR\view_matrix_2026-10-09_16-01-33.log` (complete mode-3 run before the A/B switch).

## Muzzle VFX

The old universal 100 mm offset was removed. `FireAmmunition` now selects an asset-derived socket distance (pistol 300 mm, shotgun/rifle 350 mm, sniper 950 mm, rail/energy class 750 mm) and accepts `Render/VfxMuzzleOffsetMm` as an explicit override. Smoke remains disabled and the first-person emitter remains suppressed; the plugin overlay is driven from the gun pivot.

The current live log confirms:

```
VFX host init: plugin=loaded drawer=ok atlas=vfx_atlas dir
VFX muzzle offset: 300 mm (asset socket table; gun verts=5799)
VFX host: fires=... items=... plugin=loaded
```

The event/overlay path is fixed and retested. Exact optical placement at the physical muzzle still requires a headset image; the simulator has no reliable view of that point.

## Hands and holster

- Glove-only rendering is enabled (`ShowControllerArms=0`), the independent-hand path is enabled (`IndependentHandsPrototype=1`), and verified `Collapse` slots are now active (`GloveIsolate=2`) after the census pass.
- Finger pose, glove isolation, arm palette, support-grip, holster, and transition tests all pass in the desktop suite.
- The draw path explicitly keeps arm/glove transforms independent while applying the hip transform to the weapon only (`weapon holster: arm/glove draw kept independent; weapon-only hip transform`).
- The canonical replay did not exercise a positive holster latch, so the visual condition “only the weapon disappears and the glove remains tracked” is still headset-pending.

The short post-change startup log confirms the active runtime configuration:
`C:\Users\Trilex\AppData\Local\SingularityVR\view_matrix_2026-10-09_16-13-21.log` contains `glove isolation=2`, `finger pose enabled on verified glove sections`, `OpenXR ready`, and the trigger/touch grace state.

## Current status

The tested build and VFX plugin are installed in Steam. The injector remains variant B. Menu/input, VFX event routing, muzzle socket selection, hand math, and regression coverage are ready for the physical pass. The remaining acceptance items are visual: confirm shadows in both eyes, confirm the flash originates at the barrel, and confirm weapon-only holstering while moving both hands independently.
