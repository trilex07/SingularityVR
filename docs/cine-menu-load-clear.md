# CINE latch stuck after menu -> Load (flat game)

Reference session: 2026-10-15 main-lineage build, log `%LOCALAPPDATA%\SingularityVR\view_matrix.log`.

## OBSERVE

- Exit to menu keeps stereo. Click Load: image goes flat, menu doubled (mono
  image in both SBS halves), loaded game stays flat till exit.
- Earlier loads in the SAME session recovered to GameplayStereo; the last one
  stuck in CutsceneFlat (last SceneMode transition, never returns, pawn
  PlayerWalking, `view drift -114 deg ... cine ON` to end of log).
- `SetCinematicMode` fired 6x, all `arg raw 1 -> CINE ON`; zero exits, whole
  session. The exit event remains invisible (known since run 95/211).
- Decisive cut: `camera 1A80B600 -> 5064CB00, CINE stays ON
  reason=video-world-handoff pawn=1B577800 oldPawn=1B577800 menuClass=4
  mode=VideoFlat`. Same player, menu was showing, loading movie running.
- Tail state: `TRUE STEREO ON but NOTHING was split (scene target does not
  match backbuffer)` + `scan complete: 0 candidate window(s)` repeating:
  mono fallback, no matrix lock, no stereo.
- Wheel/plate code exonerated: no wheel OPEN and no plate draw in the stuck
  tail (both early-return when closed/hidden); neither touches
  g_inCinematic, SceneMode, or the camera policy. Latch predates (runs 95-272).

## HYPOTHESIS

`CameraChangeClearsCine` keeps CINE on any cut while `videoHandoff` holds
(VideoFlat + stale cineArgRaw=1). On menu->Load the cut fires during the
loading movie with the menu still focused and the SAME pawn, so videoHandoff
short-circuits the menuFocused branch that would otherwise clear. All proven
Matinee-internal cuts in the log have menuClass=0; the failing cut has
menuClass=4. Discriminator: menu-focused + same-pawn.

## EXPERIMENT (default OFF)

`[Render] CineMenuLoadClear=0`: `menuLoadClear && menuFocused && pawnSame`
clears before the videoHandoff short-circuit, with a distinct
`menu-load cut: ... clearing CINE despite video handoff` log line.
Pause-during-real-cinema risk: a same-pawn cut with menu focused mid-cinema
would clear early; camera cuts on pause/resume are not observed, and the
flag is OFF unless leased. Falsifier: L3 menu->Load must log the menu-load
line, return to GameplayStereo with `cine off` drift lines and split draws;
if still stuck (or any real cinematic loses cinema), the discriminator is
wrong — report, do not expand the rule.

## ROUND 2 (same session, second load): menu closes before the cut

The leased flag did NOT fire: the failing cut read `menuClass=0` — the menu
had already closed when the camera cut landed. Instantaneous menuClass
cannot catch it. Added `MenuLoadLatch` (pure, L1-tested): frontend menu
visited after gameplay arms it, GameplayStereo clears it, boot/pause menus
never touch it. Policy now clears on `menuLoadClear &&
((menuFocused && pawnSame) || menuSeenSinceGameplay)`, logged with
`focused/latch/samePawn` evidence. Replayed the whole session trace: the
latch is set exactly once (frontend quit-to-menu) and only the failing cut
sees it set — zero false positives in-session. Pause-menu Load stays
uncovered (no frontend phase); that is the next experiment if reported.

## GATES

- L1: `test-cutscene-mono` gate cases (menu-load clears iff flag; Matinee
  and changed-pawn verdicts unchanged) PASS.
- L2: x86 PE32 /O2, static analysis clean, provenance recorded.
- L3 (MAIN only): leased `CineMenuLoadClear=1`, menu -> Load replay, mode
  recovery + stereo evidence, then INI restore + DLL rollback verified.
