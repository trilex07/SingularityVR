# Right-eye window and Steam screenshots

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
