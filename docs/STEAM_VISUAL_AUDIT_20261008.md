# Steam/XR desktop audit — 2026-10-08

This is the desktop/XR-simulator audit of the screenshots in
`C:\Users\Trilex\vr-mods\review-screens`. It does not replace the final PSVR2 visual check.

## Evidence from the screenshots

- `capture_2026-10-08_17-35-07.jpg` contains the old stretched white right-eye quad.
- `capture_2026-10-08_17-36-34.jpg` shows both eyes rendering the scene and the left hand/arm mesh independently; no reliable dynamic shadow is visible.
- `capture_2026-10-08_17-37-03.jpg` shows the native menu low in the view and the cursor/menu presentation that still needs a headset comfort check.
- The current eye-scale regression test passes, so the old white quad is covered by the immutable per-eye VFX conversion fix.

## Runtime log evidence

The full simulator log (`%LOCALAPPDATA%\SingularityVR\view_matrix_2026-10-08_20-15-33.log`) reached D3D9 creation and recorded:

```
VFX host init: plugin=loaded drawer=ok atlas=vfx_atlas dir (VfxHost=1)
shadow sampling: ShadowMode=2
VR render: ... shadows=0
```

The production Steam profile is therefore intentionally left at the known safe state:

```
ShadowMode=2
VrShadows=0
VfxHost=1
VfxDraw=2
VfxSmoke=0
VfxScreenFlash=1
HideMuzzleFx=1
```

Temporary `VrShadows=1` and `ShadowMode=3` runs did not produce a complete D3D9 runtime log in the simulator, so they were not deployed. No shadow register or muzzle-socket guess was promoted from those runs.

## Tooling change

`tools/steam-xrsim-singularity.ps1` now waits for the D3D9 device marker and chooses the largest complete run log when Steam creates a short attach log followed by the real game log. This prevents a false “startup stopped before D3D9” result.

The current Steam directory was restored after the A/B runs. The deployed `d3d9.dll` is the VFX candidate SHA-256 `86FA0B0EFC1D62DA7E1303CF145DB985CE31EE3D43C954D7457CE141B4CCF930`.
