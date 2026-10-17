# Centurion physical feedback — 2026-10-17, SIDE

Base: MAIN integration `9c7db48` (production code `65a3866`), not PR45's
three-other-weapon branch. ZERO Steam/install access; separate worktree.

Observed by user: cylinder travels too far; spare differs in colour and draws
over hand/gun; inserted cylinder already appears shut; closing gesture starts
the complete original reload again.

Measured from the verified private SVCA (SHA85ABCDDE...A945EA6): clip local
translation from idle is3.844UU at frame12,42.340UU at frame18,29.956UU at19,
3.589UU at26, and0 at32/33. Previous NeedAmmo12..19 walks the ENTIRE native
withdrawal, not a compact opening. Previous Loaded26..33 closes automatically
in280ms. The final native tap then runs the whole original animation; the old
palette override leaves the parent/root untouched and ends at ammo ACK.
The spare overlay binds its own albedo-only shader path and clears PRIVATE
depth, so neither native material matching nor hand/gun occlusion was implemented.

Hypothesis/controlled experiment: default-OFF CenturionVisualFollowup uses the
original compact frame12 while open, keeps frame26 until the actual close,
and runs original26..32 only on closing. Hold observed native root through the
native reload tail (without suppressing native execution/notifies/ammo). Draw
the spare as a second native gun draw with ONLY its rigid clip bone alive and
retargeted to the left hand/shoulder: same original VB/IB/material/shaders/depth,
no overlay depth clear or tint fudge. Other bones collapse to one common point;
original rigid clip extraction proves no crossing triangles.

Falsifiers: source-frame displacement exceeds frame12; Loaded closes without
input; another gun/paused/cinematic/shadow draw is modified; replay changes
material/RT/depth or leaks palette; tracking->native roundtrip fails; the real
spare still draws over nearer geometry or differs with identical material.
Native replay does NOT establish real lighting/shadow/PSVR2 acceptance offline.

## Implementation and remaining boundary

`centurion_visual_followup.h` is used by the real production DIP path. The
second draw runs INSIDE the existing per-eye StereoPair callback, with the
same VB/IB, UV/normals, bound shaders/textures/material constants and depth
target/states. Only15 CTAB-resolved bone registers change, then restore even
on partial upload/draw failure. All non-clip bones collapse to a common point.
No custom tint, private depth clear, new material/IB, GPU allocation or file I/O
in that route. The old spare overlay is disabled only with the new opt-in.
Full gun replay costs one extra native DIP per eligible pass/eye while the
spare is visible; performance must be measured by MAIN, not assumed free.

The observed native root is held per pawn/weapon/transaction through closing
and2600ms of native tail. This is NOT disabling native Reload/notifies: the
existing native backend still owns ammo; only gun-root/clip visual motion is
held/retimed. Other animations, including native arm effects, are NOT claimed
suppressed; physical regression feedback is still required. Colour/light/depth
matching depends on the REAL pass exposing LocalToWorld and the audited clip
layout; an unsupported pass fails closed rather than guessing registers.

Control: keep MAIN's current Centurion flags/assets/calibration and add ONLY
`[Input] CenturionVisualFollowup=1` (default0, requires CylinderAnimation=1).
No newly generated asset or audio pack is needed. Do not use the older DLL on
newer MAIN code: integrate this source onto CURRENT accepted dependencies.

MAIN test, one continuous headset route: fire a few shots; Circle/open/dump;
take ammo behind the LEFT shoulder, pass it behind/in front of the gun and
hand, insert and WAIT2seconds without a closing flick (must stay visibly
unseated); then close and verify ONE refill without a second gun sweep.
Pause/cancel once, reload again and exit normally. MAIN checks IB witness,
`nativeSpareDraws` increasing, no restore fault, conserved native ammo and
actual frame timings; user checks size, colour and occlusion BOTH eyes.
No mid-test chat; deployment/backup/lease/restore remain MAIN-only.

## Offline receipt

Implementation f7a50e7; clean integrated production source
`547f8e0de7f1d5992282e17f75bee60e8e5da6a2`, including MAIN's newer
HUD/scripts/holster work through2d04892 (not a rollback to the initial9c7 base).
L1_PASS:17 CPU +2 GPU fixtures,40 Python tests; additional3 MAIN compatibility
fixtures (ammo layout/scripts readiness/holster cue) PASS. All22 executable
fixtures checked x86 PE32. Includes original SVCA measurement,324 full affine
roundtrips, root hold/phase/cancellation policy and replay failure restoration.
OWN D3D9Ex shader/geometry:10609/10609 pixels, ZERO wrong-material/occlusion/body
pixels, two resource rounds with ResetEx; not actual game pixel proof.

L2_PASS: clean x86 PE32 `/O2`, performance instrumentation OFF, DLL SHA-256
`A2BFE0F978B4272F35E85AC7E18AFDA94DBD53D779AF3CDBC7F713C21A2A815F`.
Private local candidate:
`C:/Users/Trilex/vr-mods/SingularityVR-centurion-visual-feedback/build/centurion-reload/visual-candidate-547f8e0/`.
DLL/build receipt and manifest only; existing SVCA dependency retained, no INI
or asset pack deployed. Logs: `build/centurion-reload/visual-tests.log` and
`visual-build.log`. Documentation-only receipt commits do not change the DLL.
Installed/accepted/restore identities NOT inspected. L3/L4 remain UNKNOWN.
