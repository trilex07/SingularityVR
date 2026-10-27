# Menu and first-person gloves — 2026-10-10 (updated)

## Current candidate (in progress)

Build after this note: run `spikes/view_matrix/build.ps1` and record SHA-256 here.
Previous agent build: `EB8771035F2746C753E02B40F1D2B4C42AEC392A90310C64DC9CED6894F7FAF1`.

Intro path unchanged (user confirmed).

## Menu

**Pause** was already correct via OpenXR bridge `menuFlat` (classes 2–3).

**Title / IIS boot (class 1)** now uses the same policy:

- `StampCaptureState`: `menuFlat` when `g_vrUiMenu` and `g_vrMenuClass` in **1..3** (was 2–3 only).
- Retained GFx quad copy (`DrawNativeMainMenuIndexedPrimPerEye`) and UP menu quad path gated on **class 1..3** (was token/menuKind 2–3 only).

Log line to expect: `menu: XR bridge submits one identical source crop to both eyes`.

**Gate:** fresh SBS capture on title + pause after deploy; compare `bridge-pause-*` and a title-screen shot.

## Hands / gloves

1. **Face filter (2,922 triangles)** runs whenever `IndependentHandsPrototype=1` and the noTMD section palette succeeds — no longer requires `GloveIsolate>=2` for the face list itself.
2. **Default `GloveIsolate=2`** (collapse arm flesh + hide stray 390 section + gloves-only arm hide).
3. **`FgRidesGun` no longer rides** audited glove vertex counts (3314/2924/7444/2626) when independent hands are on — avoids **double transform** (palette + camera matrix) that caused dark slabs and diagonal streaks.
4. **Depth-prime copies** of those meshes skip ride when independent hands are active.

Shadow path (same DLL): `ShadowMode=3` default, `RtNeverSplitForStereo`, `g_shadowRegsCarry` — see shadow patch in `d3d9.cpp`.

## INI (recommended)

```ini
[Render]
ShadowMode=3
GloveIsolate=2
IndependentHandsPrototype=1
```

## Checks

- `glove_isolate_test.exe` / `test-vr-interaction.exe` after build.
- Log: `GloveIsolation: drawing 2,922 glove-only faces`, `IndependentHandsAudit ... success>0`, `ShadowMode3: ... carry=0x...`.
- Visual: no large sleeve slab; no dark diagonal through hands; title menu same in both eyes.

## Pending

- PSVR2 acceptance.
- Update SHA and screenshot paths after successful Steam/xrsim run (`bridge-pause-1657` or newer).
