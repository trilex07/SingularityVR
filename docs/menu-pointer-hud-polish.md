# Front menu, pointer, modal confirmation, and hand HUD polish

This candidate contains five presentation changes, scoped to the VR front-menu
and duplicated gameplay paths:

- The main menu uses `FrontMenuScalePct=72`, 10% smaller than the accepted 80%
  setting; boot, pause, and the mod-owned settings panel retain their scale.
- Controller UV is published as one aligned 64-bit atomic pair. The native
  viewport cursor, hit testing, laser endpoint, hover state, and reticle consume
  that pair. A ray miss keeps the last VR position but clears the click edge;
  focus changes reset both UV and pressed state.
- The replacement reticle is a per-eye projected, pulsing ring with a center
  point. Its geometry is computed from the same normalized panel UV used by the
  input path. It is a presentation candidate; physical alignment still needs
  PSVR2 confirmation.
- Save-slot confirmation suppression is fail-closed: only an exact reflected
  `Dialog.RenderTexture` surface, validated against the known UE3 RHI wrapper,
  can be presented as the modal. The prior Slots source is retained by COM
  identity and suppressed only when distinct from the dialog source. If that
  exact identity is unavailable, the candidate leaves the game UI unsuppressed.
- Both wrist-health renderers and the ammo plate are limited to active gameplay
  (not cinema, pause, or menus), scaled to 88%, and use alpha modulation for
  quieter plates while retaining readable digits.

Offline validation: `tools/tests/test-menu-polish.ps1`,
`tools/tests/test-frontend-menu-comfort.ps1`, and
`tools/tests/test-accepted-profile-guard.ps1`. These establish CPU policy,
projection, and deployment-guard behavior only; they do not prove live GFx
surface separation, rendered pixels, or physical PSVR2 appearance.
