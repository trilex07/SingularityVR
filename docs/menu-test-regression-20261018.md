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

## PID15380 physical acceptance and note/prompt followup

User confirms the smaller health plate, native VFX, Steam screenshots and
desktop framing. Runtime confirms particleStereo=1. All131072 sampled XR source
pixels now have A=255; Steam JPEG `20261018185351_1.jpg` contains the complete
world, without stale loading/HUD accumulation. Screenshot fix has physical and
actual saved-pixel evidence on this SteamVR setup. No blanket runtime claim.
Watchdog restored the exact historical backup; independent DLL/INI hashes match.

New failure: `mirror-probe-15380-1-xr-source.bmp` shows left-page content at
the far left and right-page content at the far right. SPNotepad reflection
identifies class4, null RTT/mode0; submit remains SBS half-rect (no crop).
The shared panel path used inset.4. ProjectCanvasMatrix left the homogeneous
scale in metres; ApplyEyeRemap therefore wrote +/-0.5*panelDepth into clip X.
For an affine GFx consumer with implicit W=1 this becomes +/-1.1 at2.2m,
consistent with the clipped opposite edges. Dividing all output coefficients
by panel depth is homogeneous-equivalent for full XYZW consumers and restores
W=1 for parallel affine canvases. Existing projective tests still pass; new
tests emulate affine XY output and SBS placement at.6/.95/2.2/3m. The shader's
implicit-W behavior is a hypothesis supported by these pixels, not a completed
runtime shader disassembly. Next physical note check is the falsifier.

The old prompt overlay unconditionally painted R1 based only on draw geometry;
it cannot identify reload versus use. Default-OFF `Render/VrPromptTokens=1`
instead rewrites exact `[reload]`/`[use]`/`[health]`/`[jump]` tokens in the existing
audited Localize FString result, before native pad alias substitution. Active
right-handed, unswapped ReloadControlLayout maps reload to `(O)`, health to
`(X)`, jump to `PRAWA GALKA W GORE`; use is `PRAWY CHWYT`. Normal reload/use
fallback follows the weapon-hand grip. Unknown tokens, markup and sentence
bytes are preserved; overflow keeps original text. Bounded logs identify which
templates are actually consumed. No blanket `Pad 11` substring replacement.
The unconditional icon overlay is bypassed while these native labels are enabled.

Token rewriting has deterministic tests for layout/fallback/unknown/overflow;
the existing engine FString allocator handles expanded labels. Runtime token
consumption remains unproven until a pickup/tutorial/reload hint is encountered.
If it still shows PadN, inspect the actual post-localization alias producer.
