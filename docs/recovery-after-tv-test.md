# Recovery after the 2026-10-14 TV test

User rejects the latest trial: stretched single TV view, broken menus, odd free
hands/FPS and lost Polish language/notes. Do not retain earlier acceptance claims
for this combined installation.

## Observed installation

Current renderer A8F38EF6, UI69CABCCF (keep the new ring/title), font3F7BF0BE
(stock, not accepted Polish60E72871), injectorAAA07005 (not accepted F5137234).
All eight first-area native speech delta packages and all three language
manifests match their original identities. NativePolishSpeech, NoTmdPaletteFix,
NoTmdWristAnchorFix, CutsceneMenuHandoffFix, CutsceneMenuPairFix,
CutsceneWorldScreen and TextureBridgeOwned are absent. MenuOverlay=0,
DesktopMirrorEye=2, UiElementDump=8, IndependentHandsAudit=1.
This is not the accepted PID37660 payload (renderer2763A984/source34b180e).
The earlier guard correctly restored test files; later DLL-only deployment did
not re-stage the complete accepted configuration and assets.

## Mechanisms and falsifiers

- Missing Polish banks/font/manifests are a proved installed-file mismatch.
  Re-use accepted sources; no new TTS or bank fabrication. Full-game audio QA
  remains outside this first-area acceptance.
- Missing wrist/palette and cinema/menu transition flags explain a different
  render path, not yet a visual proof of every symptom. Restore these switches,
  preserve user calibration/recenter/narrator OFF and retest.
- UI source pixels were captured by the other agent. Keeping an in-scene copy
  with MenuOverlay OFF is not the accepted native RTT panel presentation.
  Recovery selects MenuOverlay ON so the same capture is presented with its
  matching pointer plane. Falsifier: correctly submitted RTT still malformed.
- TV code expands a half-width eye to the entire backbuffer. Aspect-fit must
  also account for Present scaling to a different window shape. Unit fixtures
  cover that mapping; physical TV proof is pending. The scratch is now half
  width, format changes invalidate it, and subrectangle Present is bypassed.
  HMD captures use a separate copied surface before this mirror operation;
  this ordering is source evidence, not a claim of independent GPU validation.
- Existing log records ~78 FPS and missed frames; inclusive 'our work' residual
  is not proof of a specific CPU bottleneck. Do not invent an input fix or sum
  nested timings. First remove normal-trial diagnostic flags and restore the
  complete known configuration. Mirror OFF is the same-DLL performance control.

## Preparation and test boundary

`tools/prepare-recovery-test.ps1` pins both accepted manifests and the observed
current core/UI identities. Unknown changes abort before staging. It only writes
inside the recovery checkout and generates exact core4/asset38 manifests for the
existing backup/lease/restore tools. The title/ring UI is frozen unchanged.
No stock game assets are committed or redistributed.

Install/launch only after a fresh user `start`, with full process visibility,
backup hashes, core/asset restore guards and the accepted quiet-WAV lease.
No overwrite of a running game. Test without tab switching: menu, new game,
helicopter/pause, free hands, pistol, Polish NPC/recording/note, return to menu,
normal exit. Record TV proportions too. Stop for discomfort/crash. Re-hash after
exit; do not promote this combined rebuild merely from earlier L4 results.
