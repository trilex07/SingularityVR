# Right-eye window and Steam screenshots

## MAIN 2026-10-18: physical failure; separate SteamVR screenshots

Candidate `5478D2A3...` failed user TV/screenshot acceptance. Mirror reported
`bb=5904x3012 win=5904x3012 dst=(429,131,1487,1211)`, 456 successful
compositions in one 600-frame interval. This is not proof of visible TV pixels.
RT0 was already the same COM surface as the backbuffer; stamping did nothing.
The original 789749E5 DLL / 8B0B0B4C INI were restored and independently hashed.

Steam's `screenshots.vdf` identifies `20261018171748_1.jpg` as `type=2`,
`imported=1`, with `_VR.png`/`_vr.jpg` companion files, width2004 height2005.
These are VR screenshot imports, not evidence of the D3D9 F12 producer. The
actual JPEG contains the startup curtain and HUD. The RT0 patch does not prove
anything about this separate capture route. Do not add another RT0 patch.

Next falsifiable observation: default-OFF `[Diagnostic] MirrorPixelProbe=1`
captures the shared D3D9 surface consumed by the XR bridge, pre-Present
backbuffer, and post-mirror backbuffer in one frame. Two groups maximum
(gameplay, contextual UI), after 180 eligible focused frames, no active curtain.
Each image is GPU-downsampled to512x256 before readback (about512KiB), no target
bindings or source pixels changed. Compare these pixels with the VR-imported
screenshot before choosing a new presentation/capture implementation. If the
post-mirror image is fitted but TV is not, investigate actual Present consumer;
if XR source is good but SteamVR output is not, investigate VR capture separately.

## Continuation 2026-10-17 — SIDE side/mirror-screenshot-fix: MAIN root-cause in, window resize OUT

MAIN's evidence (rolled back to accepted `F5E925…`, test build `620FE76D…`
= pre-fuse `3ad9975`, INI Eye=2/Fill=0/Window=1/Capture=1):

- `reset-guard:` fired exactly ONCE
  (`1914x1003 client → enforced SBS 5904x3012`). No Reset war: S1 (no XR)
  and S2 (Reset loop) are both falsified — `xr-heartbeat` shows
  `submit=1005, validCapture=90, missingLayer=0`.
- `mirror-rect: src=(0,0,1914,1003) dst=none bb=5904x3012` → the engine
  Presented a window-sized subrect, so the mirror correctly bypassed
  (`skipRect=600, ok=0`); monitor showed native content.
- Dumped-frame analysis: non-zero bbox `(0,0,3640,1883)` on a `5904x3012`
  backbuffer; everything at `x>=2952` pure black. UE3 binds its render
  viewport AND its Present region to the window size: shrinking the window
  leaves the right-eye half black in both the backbuffer and the XR copy.
  Forcing the backbuffer to stay big cannot fix that — the engine only
  rasterizes a window-sized corner.

Architectural correction: the `SetWindowPos` window-fit path is REMOVED
(`FitWindow`/`WindowBox` deleted, contract `test_window_never_resized`).
`DesktopMirrorWindow=1` now means: right eye, aspect-fit, into the VISIBLE
client region of the untouched SBS-sized window (`VisibleEyeDst` +
`VisibleClientBox`, L1-verified for arbitrary monitor shapes) — zero engine
interaction, zero Resets, VR keeps the SteamVR SBS size by construction.
The Reset guard stays as a fail-open tripwire (any future `reset-guard:`
line proves an external shrink) plus the anti-war fuse.

Falsifier for MAIN (updated): with flags
`Eye=2 Fill=0 Window=1 Capture=1`, the log must show `ok>0` rising,
`skipRect` quiet, `bb=<SBS>` stable, NO `reset-guard:` lines, and
`mirror-place:` with client == SBS size. F12/Share then re-tests the stamp
on a render with two live halves (the earlier F12-black falsification of
the RT0 binding predates this fix and may have been a black-right-half
render, not the overlay path).

Remaining risk (stated, not solved): if the game window is minimized or
fully covered when Share is pressed, any window/DWM-sampling overlay path
yields black regardless of RT0 pixels — that would be T3, inspect the
overlay producer next. The stamp is harmless in all cases.

## Continuation 2026-10-16 — SIDE side/mirror-screenshot-fix (base 9576f5e)

Two user-reported failures, one branch:

1. Monitor should show the right-eye mirror fitted inside the monitor while
   VR keeps the SteamVR resolution.
2. Screenshots (Share button on the left PSVR2 Sense) come out black with
   only mod-rendered pixels (weapon HUD etc.) visible.

### 1. Window fit vs SteamVR resolution — Root cause and fix

MAIN's centurion-branch INI already records the mechanism:
`DesktopMirrorWindow=0` with the comment "fit window (resizes UE3 backbuffer
and drops VR resolution)". The engine follows the fitted window with a
`WM_SIZE` Reset sized to the window, so fitting the window shrank the
backbuffer and the whole XR chain (copy size, swapchains) followed it down.

Fix: the fit stays, the shrink does not. Both `Hook_ResetDevice` and
`Hook_ResetDeviceEx` now pass windowed Reset requests through
`desktop_mirror::DecideResetSize` (pure, L1-tested in
`test-desktop-mirror.cpp`): a request that merely follows the fitted client
area (explicit size equal to the live client rect, or `0` = "use client
area") is redirected to the measured SBS size `g_srcW/H` while VR
duplication is active. Everything else passes through untouched — identical
sizes, explicit larger sizes (user raised the resolution; the copy path
follows), explicit smaller non-window sizes (user's own choice is kept, not
fought), and fail-open whenever no trusted size or client rect is known.

AutoResolution (cached recommended SBS injected as `-ResX/-ResY -windowed`)
is unchanged and remains the resolution source; the guard keys off the
MEASURED copy size, never the cache, so a refused `-ResX` can never be
enforced. A burst fuse (`ResetFuse`, L1-tested: 4 enforcements inside
10 s) stops enforcing for the session and passes everything through, so a
Reset-warring engine degrades to the old window-sized render instead of a
black HMD + native monitor. Falsifier for MAIN: `reset-guard:` log lines
(max 6/session) show requested/client/enforced SBS sizes; `FUSE BLOWN`
means the engine Resets in a loop and the guard stood down.
requested/client/enforced sizes. Zero lines + stable SBS = the engine did not
Reset after the fit (guard a harmless no-op). Lines present + SBS stable =
the fix held. Lines present + size still dropped = the engine resizes
outside `Reset` (viewport/scissor path) — do not add another patch, inspect
that producer instead.

### 2. Black screenshots — RT0 binding falsified, pixels stamped instead

Status inherited: the transient `PresentBackbufferScope` (bind the finished
mirror backbuffer to RT0 DURING `Present`) was tested by MAIN and F12 stayed
black — the overlay does not sample the binding. What survives a pre-hook or
post-restore capture is PIXELS.

Fix: after every successful mirror composition, `StampCaptureRt` also copies
the finished backbuffer into the engine's current RT0 surface when it is a
same-size/format alias (`ShouldStampRt`, L1-tested). No bindings change
(`StretchRect` never rebinds), no CPU readback, and a failed stamp never
touches the committed mirror. The transient scope is kept underneath for
samplers that read the binding rather than pixels. Both are gated on the
existing `SteamCaptureBackbuffer=1` INI flag (shipped ON); `OFF` restores the
exact previous behavior.

Bounded diagnostics (`steam-capture:` one-shot lines + counters) prove which
case MAIN is in: `stamped ... into RT0` (pixels delivered), `RT0 ... vs
backbuffer ... skipped` (size/format mismatch — binding scope still covers
`Present`), or RT0 already the backbuffer (both paths no-ops). Falsifier for
MAIN: with flags on, take F12 in menu and in gameplay and inspect the saved
images plus these lines. If images are still black while `stamped` fired,
the overlay reads neither RT0 pixels nor the RT0 binding (front
buffer/DWM/window path — check window placement/occlusion next, and whether
the window is minimized when Share is pressed). Do not stack another blind
RT0 patch; inspect the overlay's actual capture producer.

### Incidental finding (fixture-proven, production-safe by construction)

This stack resets the viewport on EVERY `SetRenderTarget` — even an
identical rebind (isolated in the own-process D3D9Ex fixture). The stamp was
designed around it (no rebinding, so no viewport side effect); the capture
scope already saves/restores the viewport explicitly. Present-time overlays
rebind while the game viewport is already full-frame, so they are unaffected
in practice. No overlay code was changed for this.

### Evidence (SIDE, no Steam)

- L1: `tools/tests/test-right-eye-mirror.ps1` — `test-desktop-mirror`
  (incl. 9 ResetDecision + 3 ShouldStampRt cases), own-process D3D9Ex
  `test-desktop-mirror-pixels` (right/left/cinema pixels, window-fit
  override, intentional black-RT0 control, stamp/no-op/mismatch pixel
  checks, nested-scope RT/MRT/depth/viewport restoration, subrect bypass,
  Reset release/recreate), 7 Python source contracts (incl. new
  stamp-before-scope order + Reset-guard presence). Six menu/cinema CPU
  regression suites re-run green on this tree.
- L2: clean `spikes/view_matrix` build, x86 PE32 (`014C`), `/O2`,
  DLL SHA-256 `594D11ABFCA21985FC6159A2A0E2B66628FC892B12F56F3619D751E7A47BCB49`
  (manifest `spikes/view_matrix/d3d9.build.json`, local). Provenance:
  branch `side/mirror-screenshot-fix`, base `9576f5e`.
- L3/L4: UNKNOWN. No Steam launch, deploy, or installed-file change.
  ZERO headset claims.

### MAIN L3 script (short, one variable at a time)

1. Deploy candidate via `tools/deploy-singularityvr.ps1` (verify installed
   SHA), INI `DesktopMirrorEye=2 DesktopMirrorFill=0 DesktopMirrorWindow=1
   SteamCaptureBackbuffer=1` (scoped edits only).
2. Start via Steam, one gameplay scene: confirm one full right-eye mirror
   fitted inside the monitor work area; read log for `mirror-window:`,
   `reset-guard:` (if any), `command line:` + `per eye ... against the
   headset's` (resolution verdict), and scene/backbuffer/XR sizes.
3. F12 in gameplay and in pause menu: inspect the two saved Steam images.
   Pass = scene visible in both. Read `steam-capture:` lines to name the
   path taken.
4. Resize/move the window, pause/resume, return to menu, normal exit.
   Stay in headset until exit. Roll back per the deploy tooling on any
   critical failure and re-verify installed hashes.

---

Scope: SIDE, no Steam launches/deployment or installed-file changes. Base
`origin/main` 67b82c1. Preserve MAIN's newer integration when cherry-picking.

## Evidence and experiment

The current mirror runs after `UpdateFromHeadset`/the XR copy and before the
original device Present. `DesktopMirrorFill=1` stretches; 2 discards eye pixels.
`VisibleEyeDst` incorrectly multiplies the source aspect by the backbuffer to
window scale (then applies that scale again). Its old tests used equal window
and backbuffer dimensions, hiding that error. No code fits the window itself
inside the monitor work area. The old mirror also clears the real backbuffer
before its last fallible copy: a failed copy leaves a black presentation.

F12 is still assigned to alternate-eye debug stereo. L3+R3 requests a synchronous
diagnostic BMP. Neither should be a production screenshot binding.

Steam black screenshots have NOT been independently reproduced by SIDE. The
controlled hypothesis is that the overlay observes restored offscreen RT0 rather
than the finished backbuffer. The opt-in Present scope binds the completed mirror
backbuffer to RT0, detaches depth/MRTs, then restores all those bindings and the
viewport after Present. A synthetic capture consumer will test the distinction;
only MAIN's Steam F12 result can confirm this hypothesis. Do not claim Steam PASS
from synthetic pixels, a success counter or a build.

The mirror composition will be transactional: build offscreen, commit only after
all preparatory operations succeed. Thus failure before commit preserves SBS
instead of deliberately presenting an empty buffer. No CPU readback per frame.

## MAIN test settings (scoped edits, NEVER replace the whole INI)

```ini
[Render]
DesktopMirrorEye=2
DesktopMirrorFill=0
DesktopMirrorWindow=1
SteamCaptureBackbuffer=1
```

New flags default OFF in code. `DesktopMirrorWindow=1` forces right-eye aspect-fit,
and asynchronously fits the existing *windowed* game window inside its current
monitor's work area (including borders/title bar), client cap 1920x1080. It does
not change resolution commands, render scale, XR viewports or D3D present params.
Exclusive fullscreen is deliberately not converted by this feature.

Important falsifier: UE3 can react to window-size messages. MAIN must compare
scene/backbuffer/XR dimensions before/after fitting and check stereo/hands; the
standalone D3D test cannot establish UE3's WM_SIZE behavior. Disable the new
window flag if engine resolution changes; do not force a 1920-wide stereo target.

L3 script: start via Steam, inspect one complete right-eye mirror in the window,
F12 in menu and gameplay, inspect actual saved Steam images, resize/move window,
pause/resume and return to menu. L4: stereo/hands unchanged; L3+R3 makes no BMP
but still honors opt-in recenter; recorder and R3 wheel remain unchanged. Stay in
headset until normal exit. MAIN owns backups, deployment identity and restore.

API references: [D3D9 Present](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-present)
and [Steam screenshots](https://partner.steamgames.com/doc/api/ISteamScreenshots).
This experiment does not replace/initialize Steam API, hook its callbacks, or
write/import screenshots into Steam's library directly. If F12 is still black,
the RT0 hypothesis is falsified; inspect the overlay's actual capture producer
before adding another patch.

## Offline receipt — 2026-10-16

`tools/tests/test-right-eye-mirror.ps1`: optimized x86 `/W4 /WX` geometry,
transaction fault injection and own-process D3D9Ex fixture PASS; six Python
source/config contracts PASS. GPU fixture verifies actual right/left/cinema
pixels, window-fit override, physical window cap, an intentionally black RT0
control, nested capture scopes, RT0/MRT/depth/viewport identity restoration,
subrect bypass and resource release/recreation across ResetEx. No visible
window, game process, Steam session or XR runtime was launched. These pixels are
synthetic and are NOT proof of Steam screenshot output or UE3 resize behavior.
All six frontend/menu/cinema CPU regression executables also PASS.

Clean production build source `4513db60d7ce59aaa4cd7c3d86a40088fbe0a385`, x86
PE32 (`014C/010B`), `/O2`, performance instrumentation OFF, SHA-256:
`B8E9AE47DB0C9780B0FB684A766DAA167F404D09CC52CAA0152EC582DCA9EF73`.
Later handoff/test-only commit does not change production sources. Build warnings
include the pre-existing interlocked diagnostic counter warning; no compile
errors. Manifest: `spikes/view_matrix/d3d9.build.json` (local, not distributed).

Candidate and exact manifest are kept under `build/right-eye-mirror/candidate/`,
with NO full INI staged. MAIN should integrate the source into its CURRENT
accepted branch and rebuild; do not replace a newer MAIN DLL with this standalone
`origin/main` build and accidentally lose unmerged reload/localization changes.
Cost: one additional full-backbuffer GPU commit plus one reusable DEFAULT-pool
target versus the old mirror. No per-frame CPU readback; performance on the
real game remains L3/L4 UNKNOWN. The original SBS XR copy precedes all mirror
operations. Only MAIN may stage/deploy/launch and verify installed hashes.
