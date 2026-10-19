# Minimal VR Settings and combined Valkyrie candidate

## Scope and mechanism

The 2026-10-19 candidate includes the existing Valkyrie reload/carry/ray
changes, observed weapon-wheel selection, authored Volk grip and the separate
60% NPC-name package candidate. It does not claim a new rifle VFX repair:
native idle/three-shot traces are still needed to identify those passes.
Accepted native pistol particle repair remains enabled, unchanged.

`[Render] VrSettingsMinimal=1` exposes seven main rows: controls, note size,
Polish text, optional UI narrator, optional reload instructions, the existing
PL/EN tutorial, and close. Controls contains turn mode/angle/speed, movement
direction, handedness, recenter and back. Engineering pages remain available
with the flag OFF, or via their existing INI keys; no calibration is erased.
Minimal-mode save walks only visible pages, never hidden shader/hand controls.

`[Input] ReloadHints=0` is the default. The menu toggle enables the existing
eight-second reload guide after Circle, for all four supported weapons; it
does not enable or bypass a weapon's reload implementation. Text uses the
current Polish/English setting. The full tutorial remains independently usable.

## Proven input/layout defects

`mkAction` creates fire/altfire with zero subaction paths. Gameplay already
queries their aggregate state, but the panel requested a hand subaction and
could receive `XR_ERROR_PATH_UNSUPPORTED`. The panel now uses the same aggregate
query, choosing the physical right trigger from fire/altfire according to
handedness. No extra OpenXR action or binding is introduced.

The settings plate is flat per-eye backbuffer geometry; the previous reticle
and beam endpoint were separately projected through eye poses. That introduced
IPD parallax relative to the plate/click box. Only mod-panel endpoints now use
the same logical-UV-to-inset-pixel transform as hit-testing. Native GFx/menu
world projection and the controller origin of the beam remain unchanged.
Drawing and row hit-testing share measured-backbuffer layout math, not source
resolution assumptions. Viewport is restored after drawing. Hover highlights
the target row; one R2 press activates it, with release required after opening
while R2 is held. Stick navigation remains a fallback outside laser hover.

## Safe test preparation

Prepare with `-Weapon Valkyrie -CarryRayFix -NativeFxCapture -VisualFollowup
-MinimalSettings` using `tools/reload/prepare_vr_settings_rifle_test.ps1`.
This stages DLL, assets, INI and change plan outside Steam. Only Valkyrie is
opted into the three-weapon transaction FSM in this session; Volk grip and wheel
visual changes are still present. Do not claim Volk/Kasimov manual reload was
physically retested by this session.

The candidate sets `[Diagnostic] VrSettingsSessionOnly=1`: settings change live
but PanelSave does not write the installed INI, preserving the exact-hash
restore lease while the user tests OFF/ON. In normal production (default 0),
settings persist on close. After the controlled test, accepted files return.

One headset route, no chat during gameplay: open VR Settings, aim at both near
and far rows, click R2, enable reload instructions, close; with Valkyrie idle
five seconds then fire three short bursts three seconds apart for native VFX
identification, perform Circle/left-shoulder/insert reload; check weapon wheel,
one-/two-hand Volk grip if available and an NPC name; exit normally. MAIN
collects bounded logs and restores verified backups. Deployment and launch
require a new user `start` after candidate preparation.

## Evidence boundaries

CPU tests cover aggregate action paths, inactive/error/NaN states, held-trigger
debounce, mono/SBS layouts, row/gap/outside hits and shared UV mapping. Python
guards check runtime wiring, minimal save scope and session-only protection.
L1_PASS: 36 C++ programs in the reload runner, including 5184 panel hit checks
and 7506 original fractional animation poses; 108 runner Python tests, five
candidate-config tests, three NPC patch tests, plus menu-polish and six frontend
regression programs passed. Initial optimized x86 compilation passed; the clean
build receipt staged with the candidate supplies final L2 provenance and SHA.
L1/L2 evidence is not Steam/OpenXR/PSVR2 visual acceptance.
