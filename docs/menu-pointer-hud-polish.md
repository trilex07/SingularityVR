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
- Save-slot confirmation prefers an exact reflected `Dialog.RenderTexture`
  surface, validated against the known UE3 RHI wrapper. Runtime evidence showed
  that this game's `RvUIGFxMovie_Dialog` instead has a reflected null
  `RenderTexture` and `RenderTextureMode=0`, so it draws directly into the
  scene. The render-thread menu token could change before the explicit release
  callback, clearing the parent Slots source too early. The follow-up retains
  the exact Slots texture before invalidation and suppresses only that retained
  source when the direct-render path is proven and the source is bound; the
  native Dialog remains visible. Unknown property layouts remain fail-closed.
- Both wrist-health renderers and the ammo plate are limited to active gameplay
  (not cinema, pause, or menus), scaled to 88%, and use alpha modulation for
  quieter plates while retaining readable digits.

Offline validation: `tools/tests/test-menu-polish.ps1`,
`tools/tests/test-frontend-menu-comfort.ps1`, and
`tools/tests/test-accepted-profile-guard.ps1`. These establish CPU policy,
projection, and deployment-guard behavior only; they do not prove live GFx
surface separation, rendered pixels, or physical PSVR2 appearance.

2026-10-18 physical result (DLL `5032B353AB00D961908E78DE40FB903C04F9497AFB9CD43FF23ED7E69C2E08C1`,
Steam PID 36352): user accepted the save confirmation separation. The session
log proves the null-target/mode-0 Dialog path, retention of one exact Slots
texture, and suppression of that bound parent. Cursor appearance/alignment in
Dialogs, notes, and pickup messages remains BROKEN; this acceptance does not
cover those contexts. After normal exit, guarded rollback and independent hashes
restored the declared pre-test DLL `789749E5...F361B` and INI `8B0B0B4C...CCFAF`.
