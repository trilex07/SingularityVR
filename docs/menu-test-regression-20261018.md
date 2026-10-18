# Menu test regression, 2026-10-18

Session PID 20864 used DLL `7A17E3ED79EE7A96A4C4EAFDCD0FF0B11EF697DD9D34F8EEE6B441C68255BF5C`, INI `60586169E5FD9FF16D9F06701DEF58341AD4A5A0BCCE6CF0089DCDC1C9211B37`.
User accepts desktop framing, reports missing VFX, right hand lowering during left-knife swings, failed Steam screenshots. Health size/opacity/gesture was not tested. Do not promote this combined build.

## Configuration regression

The test copied a historical restore INI from the startup/Centurion baseline.
The merged `NativeMuzzleParticleStereo` implementation is default OFF; the test
INI omitted its required `Render/NativeMuzzleParticleStereo=1`. Thus the tested
DLL contained the accepted fix but did not execute it. The cursor INI generator
now includes this required flag, preserving every unrelated input value.
Do not use the historical restore profile as a complete feature specification.

Game exited; both watchdog and independent hashes confirmed restoration of
DLL `789749E5D4DB8FC6C603DF599A38A4FB1EC00DE374E4E2FDD469B09B9C5F361B`
and INI `8B0B0B4CF2DB78514A44B72CF3E79CC1C48C74E5632CDA34A468CA61702CCFAF`.
These are the historical restore files, NOT a claim that the current installation
retains every subsequently accepted feature. No new deployment in this followup.

## Steam VR screenshots

Local evidence: `mirror-probe-20864-0-{xr-source,pre-present,post-mirror}.bmp`
and Steam `42670/screenshots/20261018183715_1.jpg`. The Steam VDF marks this
as type 2, imported VR capture, with a companion `_vr.jpg`. RGB in the XR source
contains the full stereo world. Steam's image contains accumulated old loading
and HUD pixels. Of the 512x256 source pixels, 130311 have alpha zero; only 88
have alpha 255. This supports, but does not prove, the hypothesis that the VR
screenshot compositor consumes scene alpha even though the XR projection layer
is submitted as opaque.

Default-OFF `Render/XrCaptureOpaqueAlpha=1` writes only A=255 on the completed
shared XR output, after RGB copy and before the existing GPU fence. Original
engine targets/depth remain untouched. Original draw/setter functions avoid
hook recursion. Reusable state block, no per-frame allocations; released by
the existing Reset cleanup. Currently applies only to the active zero-copy
bridge. Failure disables the experiment for the session. No screenshot API,
Steam files or screenshot hotkeys are modified.

L2 own-process D3D9Ex fixture checks exact RGB and opaque alpha on both sides
and the eye seam, restores RT0/MRT/depth/viewport/blend/write-mask, and releases
resources before Reset. Steam acceptance remains UNKNOWN. Falsifier: source
alpha becomes opaque but the next Steam screenshot still accumulates old pixels.

## Knife/right hand

Five recorded left-hand swings invoke native RB damage and log `melee display
stays controller-tracked holder=0`. This does not prove the right gun's native
skeletal animation is cancelled. The code corrects the held knife wrist; the
weapon-hand glove retains its native grip palette and the gun root remains live.
Hypothesis: native melee lowers that gun/aim-hand palette while controller poses
stay stable. No guessed bone suppression was added.

Default-OFF `Diagnostic/KnifeMeleePoseProbe=1` records at most four idle and 24
active gun-palette samples, spaced at least 100 ms, using reflected shader
layout and the existing gun draw classifier. It records aim-hand position,
orientation and arm-window state alongside at most five native bone matrices.
No game memory writes or full-resolution captures. A moving controller or
unchanged native palette would falsify the skeletal-animation explanation.

## Next candidate

`tools/prepare-menu-regression-test.ps1` builds an offline bundle from a reviewed
prior menu plan and freshly built DLL, preserving scoped cursor/mirror values,
adding the accepted VFX flag and these two opt-in investigations. It checks the
restore profile hash, PE32/O2 build manifest and refuses existing output.
MAIN must preflight the complete plan before any new user-authorized launch.

Offline receipt: source `fe7e15a`, optimized x86 PE32 build PASS, DLL SHA-256
`A9E2B49B2FD71502295B8A57ED7415F931641CB84093948A643E6C80A09AE7D2`.
Bundle `build/menu-regression-alpha-20261018`, INI SHA-256
`B1C13D497AABC62311EBC622CC68C9407B60B70B4BFF377C63A5503D7D66B65D`.
Read-only complete accepted-profile/change-plan preflight PASS. Steam untouched.
Next physical route: gameplay, a few shots, several left-knife swings while
holding the right controller still, one Steam screenshot, normal exit. No chat
switch while in PSVR2. Knife remains diagnostic; screenshots remain experimental.
