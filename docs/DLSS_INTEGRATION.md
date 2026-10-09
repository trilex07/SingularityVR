# DLSS Integration — Feasibility & Production Instructions

**Date:** 2026-10-09
**Worktree:** `C:\Users\Trilex\vr-mods\SingularityVR-dlss-experiment`
**Branch:** `feature/dlss-xr-postprocess` (based on production `7275802`)
**Status:** **GO — GENERIC POSTPROCESS STAGE READY, DLSS BLOCKED**

## FEASIBILITY

**NO-GO for an embedded x86 DLSS-NR backend.** Four independent blockers, each fatal on its own:

### Blocker 1 — NGX is x64-only (hard blocker)
NVIDIA NGX Programming Guide: *"Both files are located in the `\lib\x64` folder (NGX is provided as a 64bit only library)."*
The import libraries (`nvsdk_ngx_s.lib` / `nvsdk_ngx_d.lib`) are x64-only and cannot be linked from a 32-bit process. SingularityVR is an x86 PE32 DLL (`cl /LD ... x86`, see `build.ps1`).
**Proof:** reference `nvidia/build.ps1` links `(Join-Path $msvc 'lib\x64')` and `(Join-Path $sdkLib 'um\x64')` — the reference layer itself is built x64.

### Blocker 2 — DLSS-NR is D3D12-only
The reference layer drives DLSS-NR exclusively through `NVSDK_NGX_D3D12_Init_Ext` / `_CreateFeature` / `_EvaluateFeature` on `ID3D12Device` / `ID3D12GraphicsCommandList` / `ID3D12Resource` (`nvidia/NgxNr.cpp`). There is no D3D11 or D3D9 path. Singularity bridges D3D9 → D3D11 for XR submission; no D3D12 device exists anywhere in the pipeline.

### Blocker 3 — Caller-validating runtime
`nvngx_dlssnr.dll` inspects the return address of calls into it and refuses unrecognized callers with `0xBAD00002`. The reference works around this with a separate `caller\nvngx.dll` shim built `/Od /MT` so every forwarder keeps a real CALL. Any embedded use would need the same second DLL.

### Blocker 4 — Required inputs do not exist in the pipeline
DLSS-NR requires: color, depth, motion vectors, normals, albedo, roughness, exposure, current/previous matrices, jitter. The UE3 D3D9 pipeline produces **color only**. Motion vectors, normals, albedo, roughness and exposure are not generated anywhere in the engine. Depth exists in D3D9 but is not exposed to D3D11 in a consumable form. Per the task rules ("do not fake unavailable inputs"), the backend must not be marked ready.

**Capability probe evidence (this machine, x86):**
`XrPostprocessDlss_Probe()` → `dllsFound=0, reason=NotPresent` (no NGX runtime DLLs installed). The probe reads PE headers from disk and never loads the DLLs, so it is safe from a 32-bit process. If an x64 NGX were present it would report `X64Only`; an x86 NGX would still report `D3D12Only` (blocker 2/4 remain).

### What IS feasible
A **generic, modular postprocess stage** with pluggable backends:
- NIS is already integrated and working (`NisBridge_*` in `d3d9.cpp`, `nis_bridge.h`).
- The new `xr_postprocess` interface is the seam where a future backend (DLSS, FSR, AMDNR) would plug in **if** NVIDIA ever ships an x86 NGX with D3D11 entry points.
- Default OFF, no behavior change, no dependency initialization when off.

## REFERENCE

- **Repo:** https://github.com/lz1264739703-spec/dlss-nr-xr-layer (cloned at `references/dlss-nr-xr-layer`)
- **Branch:** `main`
- **HEAD:** `c0ded95a30d304153d8b7b16fb1507c9a706158d`
- **License:** MIT, Copyright (c) 2026 3zwr1 (AMDNR)
- **Dependencies:** OpenXR-SDK `include`, FidelityFX-FSR headers (`ffx_a.h`, `ffx_fsr1.h`), DLSS SDK `include` (headers only — no import lib needed by the layer itself), MSVC x64 Build Tools, Windows SDK
- **Build:** plain `cl.exe` DLL, `/W4 /O2 /EHsc /std:c++20 /utf-8 /LD /MD` (x64), plus a separate caller shim DLL (`/std:c++17 /Od /MT`)
- **Architecture:** OpenXR **implicit layer** that intercepts `xrEndFrame` — NOT an embedded postprocess. Its AMD variant does FSR-style spatial NR from the swapchain image; its NVIDIA variant drives NGX D3D12. Both require a 64-bit layer process.

## SINGULARITY INSERTION POINT

**File:** `spikes/view_matrix/d3d9.cpp`
**Function:** `Hook_Present` — the XR submission block

Current flow (verified against `7275802`):

```
~10669  frame = CopyBackbufferToD3D11(gameDev)      // D39 backbuffer -> D3D11 staging
~10736  nisDone = NisBridge_ProcessStereo(...)       // NIS: 1 stage copy + 2 per-eye dispatches
~10740      g_ctx11->CopyResource(g_scImages[0][idx], g_nis.rt.outTex[swap ? 1 : 0]);
~10749  if (!nisDone && g_frameSrc11) {              // native path
~10754      g_ctx11->CopySubresourceRegion(...) or CopyResource(g_scImages[filled][idx], g_frameSrc11);
        }
~10757  xrReleaseSwapchainImage(...)
...     right eye: same pattern for g_swapchain[1]
...     xrEndFrame(...)
```

**The single seam for a future backend** is between the NIS dispatch and the swapchain copies: the texture that is about to be copied into `g_scImages[eye][idx]` is the postprocess input; the backend would write its output and the copy would take the output instead. For the NIS path the input is `g_nis.rt.outTex[eye]`; for the native path it is `g_frameSrc11` (either half via `CopySubresourceRegion`).

## IMPLEMENTATION CREATED

### Files (all NEW — no production file modified)

| File | Purpose |
|---|---|
| `spikes/view_matrix/xr_postprocess.h` | Generic interface: `XrPostprocessMode` (None/Nis/DlssNr), `XrPostprocessInput/Output`, `ProcessEye`, `Shutdown`, `InvalidateDevice`, `SetMode/GetMode`, `GetDeviceGeneration` |
| `spikes/view_matrix/xr_postprocess.cpp` | Mode dispatch. `None`/`Nis` = passthrough (NIS already runs earlier in the frame). `DlssNr` = refuses, caller falls back to native. Device-generation counter for future temporal backends. |
| `spikes/view_matrix/xr_postprocess_dlss.h` | DLSS capability layer: `XrPostprocessDlssStatus` (reason codes), probe + honest stub Init/Shutdown |
| `spikes/view_matrix/xr_postprocess_dlss.cpp` | PE-header probe of `_nvngx.dll` / `nvngx_dlssnr.dll` (exe dir, System32, sysnative). Reads headers only — never `LoadLibrary`, so a 32-bit process can safely inspect an x64 runtime. |
| `spikes/view_matrix/test_xr_postprocess.cpp` | Standalone x86 test harness (builds to an EXE, not part of `d3d9.dll`) |
| `docs/DLSS_INTEGRATION.md` | This document |

### Build safety (requirement 18)
`build.ps1` compiles **only `d3d9.cpp`** (`cl ... /Fe:d3d9.dll d3d9.cpp $mhSrc`). The new files are **not** in the build, so production compiles and links exactly as before — no NVIDIA dependency, no NGX DLL, no behavior change. The test EXE is built manually:

```
cl /nologo /W4 /O2 /EHsc /std:c++17 /Fe:test_xr_postprocess.exe ^
    test_xr_postprocess.cpp xr_postprocess.cpp xr_postprocess_dlss.cpp
```

### Test results (x86, /O2)
**29 passed, 0 failed** — covers: None passthrough, NIS passthrough, DlssNr refusal + fallback, left/right eye isolation, device-generation increment, dimension change, shutdown reset, invalid format, unsupported input combination, DLSS probe never-available, probe does not load the runtime, DLSS Init refusal, reason-text stability.

## GENERIC POSTPROCESS

- **Mode None:** input texture returned unchanged (passthrough), no resources created
- **NIS-ready:** yes — NIS already runs earlier in `Hook_Present`; the stage is a passthrough for it by design (no double processing)
- **DLSS-ready:** no — `DlssNr` mode refuses; `XrPostprocess_SetMode(DlssNr)` returns false

## DLSS BACKEND (capability layer only)

- **Initialization:** `XrPostprocessDlss_Init()` — always returns false (no x86 NGX exists to initialize)
- **Required inputs:** color (available via D3D11 bridge), depth (D3D9-only, not exposed), motion vectors / normals / albedo / roughness / exposure (not generated by UE3) — all marked unavailable
- **Per-eye resources:** none created (stub)
- **History:** none (stub)
- **Reset:** `XrPostprocess_InvalidateDevice()` bumps the device-generation counter; backends would release resources on change
- **Shutdown:** no-op
- **Probe:** `XrPostprocessDlss_Probe()` — one-time, side-effect-free PE inspection with reason codes `NotPresent / X64Only / D3D12Only / MissingInputs`

## AVAILABLE INPUTS (mapped to real Singularity data)

- **Color:** YES — D3D11 staging texture from `CopyBackbufferToD3D11` (`g_frameSrc11`), or NIS output `g_nis.rt.outTex[eye]`
- **Depth:** NO — D3D9 depth surface exists but is never copied/shared into D3D11; exposing it would need a `ID3D11Texture2D` shared handle or a GPU copy from a D3D9 depth texture (not implemented; production depth behavior untouched)
- **Motion vectors:** NO — UE3 D3D9 pipeline generates none; camera-only reprojection from depth+VP would be an experiment, not production-ready (animated enemies/hands/weapon/particles would smear)
- **Normals / albedo / roughness:** NO — not generated
- **Exposure:** NO — not exposed
- **Matrices:** camera matrices tracked in `g_camMats` (per-eye in `DrawItems`), but without motion/depth they are insufficient for temporal DLSS

## BUILD

- **Worktree:** `C:\Users\Trilex\vr-mods\SingularityVR-dlss-experiment`
- **Branch:** `feature/dlss-xr-postprocess`
- **Base HEAD:** `7275802b3a4cd235ef763e080e9e90f541e7c105` (= production HEAD)
- **PE32 / x86 / /O2:** verified for the test harness and both implementation TUs (`cl /W4 /O2 /EHsc /std:c++17`, x86 toolset)
- **OpenXR SDK:** `C:\Users\Trilex\vr-mods\dependencies\openxr-local` (not needed by the new files — they only use d3d11.h + windows.h)

## TESTS

See **Test results** above. Harness: `test_xr_postprocess.cpp` (standalone EXE; run it directly).

## COMMITS (cherry-pick order)

1. `feat(xr): add optional postprocess backend interface` — `xr_postprocess.h`, `xr_postprocess.cpp`
2. `feat(dlss): add NVIDIA backend capability probe` — `xr_postprocess_dlss.h`, `xr_postprocess_dlss.cpp`
3. `test(xr): add postprocess lifecycle tests` — `test_xr_postprocess.cpp`
4. `docs(dlss): add production integration instructions` — `docs/DLSS_INTEGRATION.md`

## PUSH

`origin/feature/dlss-xr-postprocess` (origin = https://github.com/trilex07/SingularityVR.git). Production branch NOT merged, NOT cherry-picked.

## MAIN AGENT INTEGRATION

**Nothing is required today.** The stage is dormant and not compiled into `d3d9.dll`. If the main agent later wants the seam wired in, the complete patch is:

### 1. Add include (top of `d3d9.cpp`, next to `#include "nis_upscale.h"`)
```cpp
#include "xr_postprocess.h"   // research/nvidia-vr-upscaling: optional postprocess seam
```

### 2. One call at the seam (~line 10749, replacing the two copy sites)
For the NIS path (after `NisBridge_ProcessStereo` succeeds):
```cpp
if (nisDone) {
    const bool swap = InterlockedCompareExchange(&g_swapEyes, 0, 0) != 0;
    XrPostprocessInput in{};
    in.color = g_nis.rt.outTex[swap ? 1 : 0];
    in.eye = swap ? 1 : 0;
    in.frameGeneration = (uint64_t)InterlockedCompareExchange(&g_frameDrawIdx, 0, 0);
    in.width = nisOutW; in.height = nisOutH;
    in.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    XrPostprocessOutput out{};
    ID3D11Texture2D* src = (XrPostprocess_ProcessEye(in, out) && out.color) ? out.color : in.color;
    g_ctx11->CopyResource(g_scImages[0][idx], src);
}
```
and identically for the native path (`in.color = g_frameSrc11`, `in.width = g_srcW`, `in.height = g_srcH`) and for the right eye (~line 10790).

**That is the entire integration** — ~15 lines in one function. All backend logic stays in the new files.

### 3. INI key (optional; default OFF)
`[Render] XrPostprocess=0` — `0=None` (passthrough), `1=NIS` (passthrough, NIS already runs), `2=DLSS` (refused → fallback). Wire with the existing `GetPrivateProfileInt` pattern used by other hub rows.

### 4. Fallback behavior
`ProcessEye` returns the input texture unchanged for `None`/`Nis`/any refusal, so the swapchain copy is byte-identical to today. A missing or x64-only NGX can never break the game: the probe never loads it, and `SetMode(DlssNr)` returns false.

### 5. Build requirements for the main agent
None beyond the local OpenXR SDK already used. The new TUs are **not** in `build.ps1`; to compile them into the DLL later, add them to the `cl ... /Fe:d3d9.dll d3d9.cpp $mhSrc` source list (they are dependency-free). Until then they build only via the manual test command above.

## D3D9.CPP PATCH

- **Number of changed lines:** 0 today (seam documented, not wired); ~15 when the main agent chooses to wire it
- **Reason:** the generic stage is dormant-by-design; production must not change until a real backend exists

## DEPENDENCIES

None for the main agent. Future real DLSS backend would need: x86 NGX import libraries (do not exist), an x86/D3D11 NGX runtime (does not exist), the caller-shim pattern from `references/dlss-nr-xr-layer/nvidia/NvngxCaller.cpp`, and engine-side motion-vector/normal/albedo/exposure export (does not exist).

## NOT IMPLEMENTED

- Real DLSS-NR processing (blocked: x64-only NGX, D3D12-only entry points, missing inputs)
- D3D9→D3D11 depth exposure (would require shared-handle staging; left as documented requirement)
- Camera-only reprojection experiment (possible future experiment, clearly not production-ready)
- NIS migration into the `xr_postprocess` interface (NIS intentionally left in place; the stage is a passthrough for it)

## PRODUCTION SAFETY

- **Production modified:** NO
- **Game deployed:** NO
- **Production branch merged:** NO
- **Production build affected:** NO (new files are outside `build.ps1`'s source list)

## NEXT MAIN AGENT TASK

The seam is ready but nothing needs doing now. Revisit only if: (a) NVIDIA ships an x86 NGX with D3D11 entry points, or (b) the project wants a non-temporal postprocess (e.g. FSR-style spatial NR à la the reference's AMD variant, which needs only color + optional depth). In case (b), the AMD variant of the reference layer (`references/dlss-nr-xr-layer/amd/`) is the better starting point than NGX, and `xr_postprocess.h` is the seam to implement it behind.
