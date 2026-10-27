# Review of VR Mod Vibecoding Wizard — 2026-10-09

Reference checked:
`https://github.com/bigbossafman/VR-Mod-Vibecoding-Wizard`

This repository is a workflow skill, not a runtime DLL or an Unreal/DirectX
implementation that can be copied into SingularityVR. Its useful material is the
test and diagnostics discipline around a VR conversion.

## Practices worth carrying into SingularityVR

1. **Feature flags and safe fallbacks.** Every risky renderer/input feature should
   have an INI switch and a native fallback. SingularityVR already follows this for
   NIS, VFX, shadows, menu overlay, arm IK, and recorder. Keep the current default
   path safe when a resource or runtime is unavailable.
2. **Four-layer verification.** Keep pure-math tests, desktop/XR simulator tests,
   in-game assertions/logging, and one batched headset test. The current project has
   the first three for menu projection, transitions, recorder, NIS, IK and weapon
   state; the physical menu-transition FPS sample remains the missing layer.
3. **Action-level bindings.** The skill recommends naming actions (`fire`, `grab`,
   `select`, `back`) and mapping them per controller profile rather than spreading
   button numbers through rendering/input code. This matches the current R2 menu
   select versus gameplay-fire split and should be retained for prompt icons.
4. **Fixtures and repeatable playtests.** A fixed save/route is more valuable than
   an open-ended headset session. For the current performance issue, the required
   fixture is: gameplay → pause → return to main menu → wait five seconds → quit.
5. **Structured bug capture.** The L3+R3 screenshot chord, `.vrrec` recorder and
   timestamped log already provide the core of the proposed in-VR bug trigger. Keep
   recorder start/stop separate from the screenshot/recenter chord to avoid input
   leakage.

## What was not imported

The setup wizard's staged project scaffold, generic manifest, controller tables and
framework research flow would restart the Singularity audit and duplicate the current
production docs. They are reference process only; no runtime code from this repo was
copied.

## Direct relevance to the menu hitch

The skill's performance guidance says to separate frame rate from stutter and use
runtime/render-scale measurements before disabling features. That agrees with the
captured Singularity log: the hitch is a Present/render transition (47.82 ms), while
XR copy was 0.01 ms and the mod gameplay hooks were 0.90 ms. The current fix therefore
quiesces gameplay object scans during menu/loading, while the remaining duplicated
scene/menu render cost stays an explicit headset A/B item in
`PERFORMANCE_MENU_TRANSITION_20261009.md`.
